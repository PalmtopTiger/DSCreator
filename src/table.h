#ifndef TABLE_H
#define TABLE_H

#include <QList>
#include <QString>

namespace Table
{
const QChar SEP_CSV = ';', SEP_TSV = '\t';

struct Row
{
    uint start;
    uint end;
    QString style;
    QString text;
};

class Table
{
public:
    ~Table();
    void clear();
    bool isEmpty() const;
    void append(Row* ptr);
    void mergeSiblings();
    QStringList styles() const;
    QString toCSV(const QStringList& styles, const double fps) const;
    QString toTSV(const QStringList& styles, const double fps) const;

private:
    QList<Row*> _rows;

    QString _timeToPT(const uint time, const double fps) const;
    QString _generate(const QStringList& styles, const double fps, const QChar separator) const;
};
}

#endif // TABLE_H
