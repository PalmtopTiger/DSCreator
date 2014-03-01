/*
Copyright (c) 2013, Ronie P. Martinez <ronmarti18@gmail.com>
All rights reserved.

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice
and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "csvreader.h"
#include <QFile>
#include <QTextStream>
#include <QTextCodec>


CSVReader::CSVReader(const QChar &separator) :
    separator(separator)
{}

void CSVReader::setSeparator(const QChar &separator)
{
    this->separator = separator;
}

bool CSVReader::read(const QString &fileName, QStandardItemModel * const model)
{
    if (model == NULL) return false;
    this->model = model;

    QFile fin(fileName);
    if (!fin.open(QFile::ReadOnly | QFile::Text)) return false;

    //! @todo: autodetect separator

    QString temp;
    QTextStream in(&fin);
    in.setCodec( QTextCodec::codecForName("UTF-8") );
    while (!in.atEnd())
    {
        foreach (const QChar &character, in.readLine())
        {
            if (character == this->separator)
            {
                checkString(temp, character);
            }
            else {
                temp.append(character);
            }
        }

        if (!temp.isEmpty())
        {
            checkString(temp);
        }
    }

    return true;
}

void CSVReader::checkString(QString &temp, const QChar &character)
{
    if (temp.count("\"") % 2 == 0)
    {
        if ( temp.startsWith(QChar('\"')) && temp.endsWith(QChar('\"')) )
        {
            temp.remove(QRegExp("^\"|\"$"));
        }

        // will possibly fail if there are 4 or more reapeating double quotes
        temp.replace("\"\"", "\"");

        standardItemList.append(new QStandardItem(temp));
        if (character != this->separator)
        {
            this->model->appendRow(standardItemList);
            this->standardItemList.clear();
        }
        temp.clear();
    }
    else
    {
        temp.append(character);
    }
}
