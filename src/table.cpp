#include "table.h"
#include <QtMath>
#include <QSet>
#include <QMap>

namespace Table
{
Table::~Table()
{
    qDeleteAll(_rows);
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

QString Table::_timeToPT(const uint time, const double fps) const
{
    uint hour = time / 3600000u,
         min  = time % 3600000u / 60000u,
         sec  = time % 60000u   / 1000u,
         msec = time % 1000u;

    return QString("%1:%2:%3:%4")
            .arg(hour, 2, 10, QChar('0'))
            .arg(min, 2, 10, QChar('0'))
            .arg(sec, 2, 10, QChar('0'))
            .arg(qFloor(msec * fps / 1000.0), 2, 10, QChar('0'));
}

QString Table::_generate(const QStringList& styles, const double fps, const QChar separator) const
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
                line.append( _timeToPT(row->start, fps) );
                line.append( _timeToPT(row->end, fps) );
                line.append( id );
                line.append( row->text );
            }
            else if (separator == SEP_TSV)
            {
                line.append( id );
                line.append( _timeToPT(row->start, fps) );
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

QString Table::toCSV(const QStringList& styles, const double fps) const
{
    return _generate(styles, fps, SEP_CSV);
}

QString Table::toTSV(const QStringList& styles, const double fps) const
{
    return _generate(styles, fps, SEP_TSV);
}
}
