#ifndef WRITER_H
#define WRITER_H

#include "script.h"
#include <QList>
#include <QString>

namespace Writer
{
const QChar SEP_CSV = ';', SEP_TSV = '\t';
const QString ACTOR_EMPTY = "[не размечено]";

struct Phrase
{
    uint start;
    uint end;
    QString actor;
    QString text;
};
typedef QList<Phrase> PhraseList;

bool SaveSV(const Script::Script& script, const QString& fileName, const QStringList& actors, const double fps, const int timeStart, const int joinInterval, const QChar separator);
void SavePDF(const Script::Script& script, const QString& fileName, const QStringList& actors, const double fps, const int timeStart, const int joinInterval);
}

#endif // WRITER_H
