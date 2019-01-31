/*
 * This file is part of DSCreator.
 * Copyright (C) 2014-2019  Andrey Efremov <duxus@yandex.ru>
 *
 * DSCreator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DSCreator is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DSCreator.  If not, see <https://www.gnu.org/licenses/>.
 */

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
//void SavePDF(const Script::Script& script, const QString& fileName, const QStringList& actors, const double fps, const int timeStart, const int joinInterval);
bool SaveHTML(const Script::Script& script, const QString& fileName, const QStringList& actors, const double fps, const int timeStart, const int joinInterval, const QString& title);
}

#endif // WRITER_H
