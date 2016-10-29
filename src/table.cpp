#include "table.h"
#include <QtMath>
#include <QSet>
#include <QMap>
#include <QTextDocument>
#include <QTextCursor>
#include <QPrinter>

namespace Table
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
    if (!this->isEmpty())
    {
        this->clear();
    }
    this->import(script);
    return *this;
}

void Table::clear()
{
    qDeleteAll(_rows);
    _rows.clear();
}

bool Table::isEmpty() const
{
    return _rows.isEmpty();
}

void Table::append(Row* ptr)
{
    _rows.append(ptr);
}

void Table::import(const Script::Script& script)
{
    const QRegExp endLineTag("\\\\n", Qt::CaseInsensitive), assTags("\\{[^\\}]*\\}", Qt::CaseInsensitive);
    Row* row;
    foreach (const Script::Line::Event* event, script.events.content)
    {
        row = new Row;

        row->start = event->start;
        row->end   = event->end;
        row->style = event->style.trimmed();
        row->text  = event->text.trimmed().replace(endLineTag, " ").replace(assTags, QString::null);

        this->append(row);
    }
}

// Определение единых фраз
void Table::mergeSiblings()
{
    const QRegExp phraseNotBegin("^\\W*[a-zа-яё]"), phraseNotEnd("[^.?!…]$");
    QList<Row*>::iterator prev = _rows.begin(), cur = _rows.begin();
    while (cur != _rows.end())
    {
        // Если стиль совпадает, текст прошлой не оканчивается на точку и текущая начинается с маленькой буквы
        if ( cur != _rows.begin() && (*cur)->style == (*prev)->style && (*cur)->text.contains(phraseNotBegin) && (*prev)->text.contains(phraseNotEnd) )
        {
            (*prev)->end  = (*cur)->end;
            (*prev)->text = (*prev)->text + " " + (*cur)->text;

            delete *cur;
            cur = _rows.erase(cur);
        }
        else
        {
            prev = cur++;
        }
    }
}

QStringList Table::styles() const
{
    QSet<QString> styles;
    foreach (const Row* row, _rows)
    {
        styles.insert(row->style);
    }
    return styles.values();
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

QString Table::_generate(const QStringList& styles, const double fps, const int timeStart, const QChar separator) const
{
    QString result;

    const int width = QString::number(_rows.size()).size();
    QStringList line;
    QMap<QString, uint> counters;
    uint counter;
    QString id;
    foreach (const Row* row, _rows)
    {
        if ( styles.isEmpty() || styles.contains(row->style, Qt::CaseInsensitive) )
        {
            counter = counters.value(row->style, 0) + 1;
            counters[row->style] = counter;
            id = QString("%1%2").arg(row->style).arg(counter, width, 10, QChar('0'));

            if (separator == SEP_CSV)
            {
                line.append( _timeToPT(row->start, fps, timeStart) );
                line.append( _timeToPT(row->end, fps, timeStart) );
                line.append( id );
                line.append( row->text );
            }
            else if (separator == SEP_TSV)
            {
                line.append( id );
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
        }
    }

    return result;
}

QString Table::toCSV(const QStringList& styles, const double fps, const int timeStart) const
{
    return _generate(styles, fps, timeStart, SEP_CSV);
}

QString Table::toTSV(const QStringList& styles, const double fps, const int timeStart) const
{
    return _generate(styles, fps, timeStart, SEP_TSV);
}

void Table::toPDF(const QString& fileName, const QStringList& styles, const double fps, const int timeStart) const
{
    QTextDocument document;
    document.setDefaultFont(QFont("Helvetica", 14));
    QTextCursor cursor(&document);

    const int width = QString::number(_rows.size()).size();
    QMap<QString, uint> counters;
    uint counter;
    foreach (const Row* row, _rows)
    {
        if ( styles.isEmpty() || styles.contains(row->style, Qt::CaseInsensitive) )
        {
            counter = counters.value(row->style, 0) + 1;
            counters[row->style] = counter;

            cursor.insertHtml( QString("<b>%1%2</b> %3<br/>%4")
                               .arg(row->style)
                               .arg(counter, width, 10, QChar('0'))
                               .arg(_timeToPT(row->start, fps, timeStart))
                               .arg(row->text) );

            if (row != _rows.last())
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
