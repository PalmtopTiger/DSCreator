#include "writer.h"
#include <QtMath>
//#include <QMap>
#include <QTextCodec>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTable>
//#include <QPrinter>
#include <QRegularExpression>

namespace Writer
{
QString TimeToPT(const uint time, const double fps, const int timeStart)
{
    // Отделяем кадры от времени
    const int frames = timeStart % 1000;
    int newTime = static_cast<int>(time) + (timeStart - frames);

    // Пересчитываем кадры в миллисекунды
    const double tmpMsec = static_cast<double>(frames) * 1000.0 / fps;
    newTime += tmpMsec < 0 ? qFloor(tmpMsec) : qCeil(tmpMsec);

    // Запоминаем знак
    const bool negative = newTime < 0;
    newTime = abs(newTime);

    // Делим на компоненты
    const int hour = newTime / 3600000,
              min  = newTime / 60000 % 60,
              sec  = newTime / 1000  % 60,
              msec = newTime % 1000;

    // Собираем строку (последний компонент - кадры)
    const QChar fillChar = QChar('0');
    return QString("%1%2:%3:%4:%5")
            .arg(negative ? QString("−") : QString::null)
            .arg(hour, 2, 10, fillChar)
            .arg(min,  2, 10, fillChar)
            .arg(sec,  2, 10, fillChar)
            .arg(qFloor(static_cast<double>(msec) * fps / 1000.0), 2, 10, fillChar);
}

// Удаляет теги из текста фраз, объединяет соседние и фильтрует по актёрам
PhraseList PreparePhrases(const Script::Script& script, const QStringList& actors, const int joinInterval)
{
    const QRegularExpression assTags("\\{[^\\}]*?\\}");

    PhraseList result;
    Phrase phrase;
    QString actor, text;
    bool first = true;
    for (const Script::Line::Event* const event : qAsConst(script.events.content))
    {
        actor = event->actorName.isEmpty() ? ACTOR_EMPTY : event->actorName; // Already trimmed
        text  = event->text.trimmed().replace("\\N", " ", Qt::CaseInsensitive).replace(assTags, QString::null);

        // Если интервал указан, фраза не первая, актёр совпадает и расстояние между фразами не более 5 сек.
        if (!first &&
            joinInterval > 0 &&
            actor == phrase.actor &&
            event->start >= phrase.end &&
            event->start - phrase.end <= static_cast<uint>(joinInterval))
        {
            phrase.end  = event->end;
            phrase.text += " ";
            phrase.text += text;
        }
        else
        {
            if (!first) result.append(phrase);

            phrase.start = event->start;
            phrase.end   = event->end;
            phrase.actor = actor;
            phrase.text  = text;

            first = false;
        }
    }
    if (!first) result.append(phrase);

    if (!actors.isEmpty())
    {
        auto toRemove = [actors](const Phrase& phrase) {
            return !actors.contains(phrase.actor, Qt::CaseInsensitive);
        };
        result.erase(std::remove_if(result.begin(), result.end(), toRemove),
                     result.end());
    }

    return result;
}

bool SaveSV(const Script::Script& script, const QString& fileName, const QStringList& actors, const double fps, const int timeStart, const int joinInterval, const QChar separator)
{
    const PhraseList phrases = PreparePhrases(script, actors, joinInterval);

    // const int width = QString::number(rows.size()).size();
    // QMap<QString, uint> counters;
    // uint counter;
    // QString id;
    QString prevActor;
    QStringList line;
    QString result;
    for (const Phrase& phrase : phrases)
    {
        // counter = counters.value(row->actor, 0) + 1;
        // counters[row->actor] = counter;
        // id = QString("%1%2").arg(row->actor).arg(counter, width, 10, QChar('0'));

        if (separator == SEP_CSV)
        {
            line.append( TimeToPT(phrase.start, fps, timeStart) );
            line.append( TimeToPT(phrase.end, fps, timeStart) );
            line.append( phrase.actor != prevActor ? phrase.actor : QString::null );
            line.append( phrase.text );
        }
        else if (separator == SEP_TSV)
        {
            line.append( phrase.actor );
            line.append( TimeToPT(phrase.start, fps, timeStart) );
        }

        for (QStringList::iterator str = line.begin(); str != line.end(); ++str)
        {
            if ( str->contains(separator) )
            {
                (*str) = QString("\"%1\"").arg( str->replace(QChar('"'), "\"\"") );
            }
        }

        result.append( line.join(separator) );
        result.append( "\n" );
        line.clear();

        prevActor = phrase.actor;
    }

    QFile fout(fileName);
    if (!fout.open(QFile::WriteOnly | QFile::Text)) return false;

    QTextStream out(&fout);
    out.setCodec( QTextCodec::codecForName("UTF-8") );
    out.setGenerateByteOrderMark(true);
    out << result;

    fout.close();
    return true;
}

/*void SavePDF(const Script::Script& script, const QString& fileName, const QStringList& actors, const double fps, const int timeStart, const int joinInterval)
{
    const PhraseList phrases = PreparePhrases(script, actors, joinInterval);

    QTextDocument document;
    QFont font = document.defaultFont();
    font.setStyleHint(QFont::SansSerif);
    font.setFamily("Helvetica");
    font.setPointSize(14);
    document.setDefaultFont(font);
    QTextCursor cursor(&document);

    // const int width = QString::number(_rows.size()).size();
    // QMap<QString, uint> counters;
    // uint counter;
    bool first = true;
    for (const Phrase& phrase : phrases)
    {
        // counter = counters.value(row->actor, 0) + 1;
        // counters[row->actor] = counter;
        // .arg(counter, width, 10, QChar('0'))

        if (!first)
        {
            cursor.insertBlock();
            cursor.insertBlock();
        }

        cursor.insertHtml( QString("<b>%1</b> %2<br/>%3")
                           .arg(phrase.actor)
                           .arg(TimeToPT(phrase.start, fps, timeStart))
                           .arg(phrase.text) );

        first = false;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPaperSize(QPrinter::A4);
    printer.setOutputFileName(fileName);
    document.print(&printer);
}*/

bool SaveHTML(const Script::Script& script, const QString& fileName, const QStringList& actors, const double fps, const int timeStart, const int joinInterval)
{
    const PhraseList phrases = PreparePhrases(script, actors, joinInterval);

    QTextDocument document;
    QFont font = document.defaultFont();
    font.setStyleHint(QFont::SansSerif);
    font.setFamily("Arial");
    font.setPointSize(10);
    document.setDefaultFont(font);
    QTextCursor cursor(&document);

    QTextTableFormat tableFormat;
    tableFormat.setCellSpacing(0);
    tableFormat.setCellPadding(5.0);

    QTextTable* table = cursor.insertTable(phrases.size() + 1, 3, tableFormat);
    table->cellAt(0, 0).firstCursorPosition().insertText("Актёры");
    table->mergeCells(0, 0, 1, 3);

    for (int i = 0; i < phrases.size(); ++i)
    {
        const Phrase& phrase = phrases.at(i);
        const int row = i + 1;

        table->cellAt(row, 0).firstCursorPosition().insertText(TimeToPT(phrase.start, fps, timeStart));
        table->cellAt(row, 1).firstCursorPosition().insertText(phrase.actor);
        table->cellAt(row, 2).firstCursorPosition().insertText(phrase.text);
    }

    QString html = document.toHtml();
    html.insert(html.indexOf("</style>"),
                "table { font: 10pt Arial, Helvetica, sans-serif; border-collapse: collapse; }\n"
                "table, td { border: 1px solid black; }\n"
                "td { vertical-align: bottom; }\n");

    QFile fout(fileName);
    if (!fout.open(QFile::WriteOnly | QFile::Text)) return false;

    QTextStream out(&fout);
    out.setCodec( QTextCodec::codecForName("UTF-8") );
    out.setGenerateByteOrderMark(true);
    out << html;

    fout.close();
    return true;
}
}
