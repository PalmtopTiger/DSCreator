#include "writer.h"
#include <QtMath>
#include <QSet>
#include <QMap>
#include <QTextDocument>
#include <QTextCursor>
#include <QPrinter>

namespace Writer
{
Table::Table(const Script::Script& script)
{
    this->import(script);
}

Table::~Table()
{
    qDeleteAll(_rows);
}

Table& Table::operator=(const Script::Script& script)
{
    if (!_rows.isEmpty())
    {
        qDeleteAll(_rows);
        _rows.clear();
    }
    this->import(script);
    return *this;
}

void Table::import(const Script::Script& script)
{
    const QString emptyActor = "[не размечено]";
    const QRegExp endLineTag("\\\\n", Qt::CaseInsensitive), assTags("\\{[^\\}]*\\}", Qt::CaseInsensitive);
    QString actor;
    Row* row;
    foreach (const Script::Line::Event* event, script.events.content)
    {
        actor = event->actorName.trimmed();
        row   = new Row;

        row->start = event->start;
        row->end   = event->end;
        row->actor = actor.isEmpty() ? emptyActor : actor;
        row->text  = event->text.trimmed().replace(endLineTag, " ").replace(assTags, QString::null);

        _rows.append(row);
    }
}

// Возвращает объединённые фразы
RowList Table::_joinedRows(const int joinInterval) const
{
    RowList result;
    Row* joined = nullptr;
    foreach (const Row* row, _rows)
    {
        // Если фраза не первая, актёр совпадает и расстояние между фразами не более 5 сек.
        if ( joined && joinInterval > 0 && row->actor == joined->actor &&
             row->start >= joined->end && row->start - joined->end <= static_cast<uint>(joinInterval) )
        {
            joined->end  = row->end;
            joined->text = joined->text + " " + row->text;
        }
        else
        {
            if (joined) result.append(joined);

            joined = new Row;
            (*joined) = (*row);
        }
    }
    if (joined) result.append(joined);

    return result;
}

QString Table::_timeToPT(const uint time, const double fps, const int timeStart) const
{
    const uint newTime = time + timeStart;
    uint hour = newTime / 3600000u,
         min  = newTime % 3600000u / 60000u,
         sec  = newTime % 60000u   / 1000u,
         msec = newTime % 1000u;

    return QString("%1:%2:%3:%4")
            .arg(hour, 2, 10, QChar('0'))
            .arg(min,  2, 10, QChar('0'))
            .arg(sec,  2, 10, QChar('0'))
            .arg(qFloor(msec * fps / 1000.0), 2, 10, QChar('0'));
}

QString Table::_generate(const QStringList& actors, const double fps, const int timeStart, const int joinInterval, const QChar separator) const
{
    RowList joinedRows = _joinedRows(joinInterval);

    // const int width = QString::number(rows.size()).size();
    // QMap<QString, uint> counters;
    // uint counter;
    // QString id;
    QString prevActor;
    QStringList line;
    QString result;
    foreach (const Row* row, joinedRows)
    {
        if ( actors.isEmpty() || actors.contains(row->actor, Qt::CaseInsensitive) )
        {
            // counter = counters.value(row->actor, 0) + 1;
            // counters[row->actor] = counter;
            // id = QString("%1%2").arg(row->actor).arg(counter, width, 10, QChar('0'));

            if (separator == SEP_CSV)
            {
                line.append( _timeToPT(row->start, fps, timeStart) );
                line.append( _timeToPT(row->end, fps, timeStart) );
                line.append( row->actor != prevActor ? row->actor : QString::null );
                line.append( row->text );
            }
            else if (separator == SEP_TSV)
            {
                line.append( row->actor );
                line.append( _timeToPT(row->start, fps, timeStart) );
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

            prevActor = row->actor;
        }
    }

    qDeleteAll(joinedRows);

    return result;
}

QString Table::toCSV(const QStringList& actors, const double fps, const int timeStart, const int joinInterval) const
{
    return _generate(actors, fps, timeStart, joinInterval, SEP_CSV);
}

QString Table::toTSV(const QStringList& actors, const double fps, const int timeStart, const int joinInterval) const
{
    return _generate(actors, fps, timeStart, joinInterval, SEP_TSV);
}

void Table::toPDF(const QString& fileName, const QStringList& actors, const double fps, const int timeStart, const int joinInterval) const
{
    RowList joinedRows = _joinedRows(joinInterval);

    QTextDocument document;
    document.setDefaultFont(QFont("Helvetica", 14));
    QTextCursor cursor(&document);

    // const int width = QString::number(_rows.size()).size();
    // QMap<QString, uint> counters;
    // uint counter;
    foreach (const Row* row, joinedRows)
    {
        if ( actors.isEmpty() || actors.contains(row->actor, Qt::CaseInsensitive) )
        {
            // counter = counters.value(row->actor, 0) + 1;
            // counters[row->actor] = counter;
            // .arg(counter, width, 10, QChar('0'))

            cursor.insertHtml( QString("<b>%1</b> %2<br/>%3")
                               .arg(row->actor)
                               .arg(_timeToPT(row->start, fps, timeStart))
                               .arg(row->text) );

            if (row != joinedRows.last())
            {
                cursor.insertBlock();
                cursor.insertBlock();
            }
        }
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPaperSize(QPrinter::A4);
    printer.setOutputFileName(fileName);
    document.print(&printer);
}
}
