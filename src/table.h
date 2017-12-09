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
    QString actor;
    QString text;
};
typedef QList<Row*> RowList;

class Table
{
public:
    Table() {};
    Table(const Script::Script& script);
    ~Table();
    Table& operator=(const Script::Script& script);
    void import(const Script::Script& script);
    QStringList actors() const;
    QString toCSV(const QStringList& actors, const double fps, const int timeStart, const int joinInterval) const;
    QString toTSV(const QStringList& actors, const double fps, const int timeStart, const int joinInterval) const;
    void toPDF(const QString& fileName, const QStringList& actors, const double fps, const int timeStart, const int joinInterval) const;

private:
    RowList _rows;

    RowList _joinedRows(const int joinInterval) const;
    QString _timeToPT(const uint time, const double fps, const int timeStart) const;
    QString _generate(const QStringList& actors, const double fps, const int timeStart, const int joinInterval, const QChar separator) const;
};
}

#endif // TABLE_H
