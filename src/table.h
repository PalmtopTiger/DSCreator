#ifndef TABLE_H
#define TABLE_H

#include "script.h"
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
    Table() {};
    Table(const Script::Script& script);
    ~Table();
    Table& operator=(const Script::Script& script);
    void clear();
    bool isEmpty() const;
    void append(Row* ptr);
    void import(const Script::Script& script);
    void mergeSiblings();
    QStringList styles() const;
    QString toCSV(const QStringList& styles, const double fps, const int timeStart) const;
    QString toTSV(const QStringList& styles, const double fps, const int timeStart) const;
    void toPDF(const QString& fileName, const QStringList& styles, const double fps, const int timeStart) const;

private:
    QList<Row*> _rows;

    QString _timeToPT(const uint time, const double fps, const int timeStart) const;
    QString _generate(const QStringList& styles, const double fps, const int timeStart, const QChar separator) const;
};
}

#endif // TABLE_H
