/*******************************************************************************
 * This file is part of Re_Sync Next.
 *
 * Copyright (C) 2011-2019  Andrey Efremov <duxus@yandex.ru>
 *
 * Re_Sync is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Re_Sync is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Re_Sync.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include "script.h"
#include <QHash>
#include <QRegularExpression>


namespace Script
{
namespace Line
{
uint StrToTime(const QString& str, const ScriptType type)
{
    // В этой функции мы пытаемся получить хоть какое-то время из строки.
    // Считаем, что чисел может недоставать только с конца (миллисекунды и далее).
    uint hour = 0,
         min  = 0,
         sec  = 0,
         msec = 0;
    QStringList list = str.split(':');

    // Часы
    if (!list.isEmpty())
    {
        hour = list.first().trimmed().toUInt();
        list.removeFirst();
    }

    // Минуты
    if (!list.isEmpty())
    {
        min = list.first().trimmed().toUInt();
        list.removeFirst();
    }

    if (!list.isEmpty())
    {
        if (SCR_ASS == type || SCR_SSA == type) list = list.first().split('.');
        else list = list.first().split(',');

        // Секунды
        if (!list.isEmpty())
        {
            sec = list.first().trimmed().toUInt();
            list.removeFirst();
        }

        // Миллисекунды
        if (!list.isEmpty())
        {
            msec = list.first().trimmed().toUInt();
            if (SCR_ASS == type || SCR_SSA == type) msec *= 10u;
        }
    }

    return ((hour * 60u + min) * 60u + sec) * 1000u + msec;
}

QString TimeToStr(const uint time, const ScriptType type)
{
    const uint hour = time / 3600000u,
               min  = time / 60000u % 60u,
               sec  = time / 1000u  % 60u,
               msec = time % 1000u;

    QString ret;
    if (SCR_ASS == type || SCR_SSA == type)
    {
        ret = QString("%1:%2:%3.%4").arg(hour).arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')).arg(msec / 10u, 2, 10, QChar('0'));
    }
    else
    {
        ret = QString("%1:%2:%3,%4").arg(hour, 2, 10, QChar('0')).arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')).arg(msec, 3, 10, QChar('0'));
    }

    return ret;
}

// Базовая строка
Base::Base()
{}

Base::Base(const QString& value) :
    _value(value)
{}

QString Base::generate(const ScriptType type) const
{
    Q_UNUSED(type);
    return _value;
}

// Простейшая строка с двоеточием
Named::Named(const QString& name) :
    _name(name)
{}

Named::Named(const QString& name, const QStringList& before) :
    _name(name),
    _before(before)
{}

void Named::clearBefore()
{
    _before.clear();
}

QString Named::name() const
{
    return _name;
}

QString Named::generate(const ScriptType type, const QString& value) const
{
    QString result;

    if (SCR_ASS == type || SCR_SSA == type)
    {
        if (_before.length())
        {
            result.append( _before.join("\n") );
            result.append("\n");
        }
        result.append( QString("%1: %2").arg(_name).arg(value) );
    }

    return result;
}

QString Named::generate(const ScriptType type) const
{
    QString result;

    if (SCR_ASS == type || SCR_SSA == type) result = this->generate(type, text);

    return result;
}

// Строка стиля
Style::Style() :
    Named("Style")
{
    this->init();
}

Style::Style(const QStringList& before) :
    Named("Style", before)
{
    this->init();
}

void Style::init()
{
    styleName       = defaultStyle;
    fontName        = defaultFont;
    fontSize        = 20.0;
    primaryColour   = 0xFFFFFF;
    secondaryColour = 0xFF;
    outlineColour   = 0;
    backColour      = 0;
    bold            = false;
    italic          = false;
    underline       = false;
    strikeOut       = false;
    scaleX          = 100.0;
    scaleY          = 100.0;
    spacing         = 0.0;
    angle           = 0.0;
    borderStyle     = 1;
    outline         = 2.0;
    shadow          = 2.0;
    alignment       = 2;
    marginL         = 10;
    marginR         = 10;
    marginV         = 10;
    encoding        = 1;
}

QString Style::generate(const ScriptType type) const
{
    QString result;

    if (SCR_ASS == type || SCR_SSA == type)
    {
        QStringList list;

        list.append(styleName);
        list.append(fontName);
        list.append( QString::number(fontSize, 'g', 10) );

        if (SCR_ASS == type)
        {
            list.append( QString("&H%1").arg(primaryColour,   8, 16, QChar('0')).toUpper() );
            list.append( QString("&H%1").arg(secondaryColour, 8, 16, QChar('0')).toUpper() );
            list.append( QString("&H%1").arg(outlineColour,   8, 16, QChar('0')).toUpper() );
            list.append( QString("&H%1").arg(backColour,      8, 16, QChar('0')).toUpper() );
        }
        else
        {
            list.append( QString::number( static_cast<qint32>(primaryColour) ) );
            list.append( QString::number( static_cast<qint32>(secondaryColour) ) );
            list.append( QString::number( static_cast<qint32>(outlineColour) ) );
            list.append( QString::number( static_cast<qint32>(backColour) ) );
        }

        list.append( bold   ? "-1" : "0" );
        list.append( italic ? "-1" : "0" );

        if (SCR_ASS == type)
        {
            list.append( underline ? "-1" : "0" );
            list.append( strikeOut ? "-1" : "0" );
            list.append( QString::number(scaleX,  'g', 10) );
            list.append( QString::number(scaleY,  'g', 10) );
            list.append( QString::number(spacing, 'g', 10) );
            list.append( QString::number(angle,   'g', 10) );
        }

        list.append( QString::number(borderStyle) );
        list.append( QString::number(outline, 'g', 10) );
        list.append( QString::number(shadow,  'g', 10) );

        if (SCR_SSA == type && alignment > 0 && alignment < AlignmentASS.length())
        {
            list.append( QString::number(AlignmentASS.at(alignment)) );
        }
        else
        {
            list.append( QString::number(alignment) );
        }

        list.append( QString::number(marginL) );
        list.append( QString::number(marginR) );
        list.append( QString::number(marginV) );

        if (SCR_SSA == type)
        {
            list.append("0");
        }

        list.append( QString::number(encoding) );

        result = Named::generate(type, list.join(','));
    }

    return result;
}

// Строка события
Event::Event() :
    Named("Dialogue")
{
    this->init();
}

Event::Event(const QStringList& before) :
    Named("Dialogue", before)
{
    this->init();
}

void Event::init()
{
    layer   = 0;
    start   = 0;
    end     = 0;
    style   = defaultStyle;
    marginL = 0;
    marginR = 0;
    marginV = 0;
}

QString Event::generate(const ScriptType type) const
{
    QString result;

    if (SCR_ASS == type || SCR_SSA == type)
    {
        QStringList list;

        if (SCR_SSA == type)
        {
            list.append( QString("Marked=%1").arg(layer) );
        }
        else
        {
            list.append( QString::number(layer) );
        }

        list.append( TimeToStr(start, type) );
        list.append( TimeToStr(end, type) );
        list.append(style);
        list.append(actorName);
        list.append( QString::number(marginL) );
        list.append( QString::number(marginR) );
        list.append( QString::number(marginV) );
        list.append(effect);
        list.append(text);

        result = Named::generate(type, list.join(','));
    }
    else if (SCR_SRT == type)
    {
        result.append( QString("%1 --> %2\n").arg(TimeToStr(start, type)).arg(TimeToStr(end, type)) );
        result.append( QString(text).replace("\\N", "\n", Qt::CaseInsensitive) );
    }

    return result;
}
}

// Скрипт
Script::Script() :
    header(SEC_HEADER),
    styles(SEC_STYLES),
    events(SEC_EVENTS),
    fonts(SEC_FONTS),
    graphics(SEC_GRAPHICS)
{}

void Script::clearBefore()
{
    _before.clear();
}

void Script::clearAfter()
{
    _after.clear();
}

void Script::clear()
{
    header.clear();
    styles.clear();
    events.clear();
    fonts.clear();
    graphics.clear();
    clearBefore();
    clearAfter();
}

void Script::appendBefore(const QStringList& before)
{
    _before.append(before);
}

void Script::appendAfter(const QStringList& after)
{
    _after.append(after);
}

QString Script::generate(const ScriptType type) const
{
    QString result;

    if (SCR_ASS == type || SCR_SSA == type)
    {
        if (_before.length())
        {
            result.append( _before.join("\n") );
            result.append("\n");
        }

        result.append( header.generate(type) );
        result.append("\n");
        result.append( styles.generate(type) );
        result.append("\n");
        result.append( events.generate(type) );

        if (!fonts.isEmpty())
        {
            result.append("\n");
            result.append( fonts.generate(type) );
        }

        if (!graphics.isEmpty())
        {
            result.append("\n");
            result.append( graphics.generate(type) );
        }

        if (_after.length())
        {
            result.append("\n");
            result.append( _after.join("\n") );
            result.append("\n");
        }
    }
    else if (SCR_SRT == type)
    {
        result = events.generate(type);
    }

    return result;
}

//
// Определение формата
//
ScriptType DetectFormat(QTextStream &in)
{
    in.seek(0);

    QString str = in.read(5120);
    if (str.contains(QRegularExpression("ScriptType *: *v4\\.00\\+")))
    {
        return SCR_ASS;
    }
    else if (str.contains(QRegularExpression("ScriptType *: *v4\\.00")))
    {
        return SCR_SSA;
    }
    else if (str.contains(QRegularExpression("\\d{2}:\\d{2}:\\d{2},\\d{3} *--> *\\d{2}:\\d{2}:\\d{2},\\d{3}")))
    {
        return SCR_SRT;
    }

    return SCR_UNKNOWN;
}

//
// Парсер SSA
//
bool ParseSSA(QTextStream& in, Script& script)
{
    in.seek(0);

    // Регулярка для определения секции
    QRegularExpression reSection("^\\[([^\\]]+?)\\]$");
    QRegularExpressionMatch match;

    // Таблица состояний
    QHash<QString, SectionType> sectionTable = {
        {Sections::header.toLower(),    SEC_HEADER},
        {Sections::stylesSSA.toLower(), SEC_STYLES},
        {Sections::stylesASS.toLower(), SEC_STYLES},
        {Sections::events.toLower(),    SEC_EVENTS},
        {Sections::fonts.toLower(),     SEC_FONTS},
        {Sections::graphics.toLower(),  SEC_GRAPHICS}
    };

    // Таблица типов файлов
    QHash<QString, ScriptType> typeTable = {
        {"v4.00",  SCR_SSA},
        {"v4.00+", SCR_ASS},
        {Sections::stylesSSA.toLower(), SCR_SSA},
        {Sections::stylesASS.toLower(), SCR_ASS}
    };

    // Имена строк
    const QString ltStyle      = "style";
    const QString ltEvent      = "dialogue";
    const QString ltFormat     = "format";
    const QString ltScriptType = "scripttype";

    QString line, name, text, tempStr;
    SectionType state = SEC_UNKNOWN;
    QStringList tempStrList, tempList;
    bool readNext = true, atBegin = true;
    ScriptType type = SCR_SSA;
    int pos;
    while ( !in.atEnd() )
    {
        // Если вернулись из секции, имя новой секции надо сохранить
        if (readNext)
        {
            line = in.readLine().trimmed();
        }
        else
        {
            readNext = true;
        }

        // Пропускаем пустые строки, чтобы не делать проверки дальше
        if (!atBegin && line.isEmpty()) continue;

        switch (state)
        {
        // Вне секций
        case SEC_UNKNOWN:
            // Нашли заголовок
            if ((match = reSection.match(line)).hasMatch())
            {
                tempStr = match.captured(1).trimmed().toLower();

                // Есть ли такой заголовок в таблице?
                if (sectionTable.contains(tempStr))
                {
                    state = sectionTable[tempStr];

                    // Заголовок с версией файла?
                    if (typeTable.contains(tempStr))
                    {
                        type = typeTable[tempStr];
                    }

                    // В начале файла
                    if (atBegin)
                    {
                        atBegin = false;
                        script.appendBefore(tempStrList);
                        tempStrList.clear();
                    }
                }
            }

            // Спасаем неизвестное
            //! @todo: временно отключено - мусор ломает формат
            /*if (SEC_UNKNOWN == state)
            {
                tempStrList.append(line);
            }*/
            break;

        case SEC_HEADER:
            // Такой комментарий может быть только в заголовке
            if ( line.startsWith(';') )
            {
                tempStrList.append(line);
            }
            // Началась другая секция
            else if (reSection.match(line).hasMatch())
            {
                script.header.appendAfter(tempStrList);
                tempStrList.clear();

                readNext = false;
                state = SEC_UNKNOWN;
            }
            // Нормальная строка
            else if ( -1 != ( pos = line.indexOf(':') ) )
            {
                name = line.left(pos).trimmed();
                text = line.mid(pos + 1).trimmed();

                // Версия файла (шо, опять?)
                if (ltScriptType == name.toLower())
                {
                    tempStr = text.toLower();
                    if (typeTable.contains(tempStr))
                    {
                        type = typeTable[tempStr];
                    }
                }
                else
                {
                    Line::Named* ptr = new Line::Named(name, tempStrList);
                    tempStrList.clear();

                    ptr->text = line.mid(pos + 1).trimmed();
                    script.header.append(ptr);
                }
            }
            // Мусор
            else
            {
                tempStrList.append(line);
            }
            break;

        case SEC_STYLES:
            // Началась другая секция
            if (reSection.match(line).hasMatch())
            {
                script.styles.appendAfter(tempStrList);
                tempStrList.clear();

                readNext = false;
                state = SEC_UNKNOWN;
            }
            // Нормальная строка
            else if ( -1 != ( pos = line.indexOf(':') ) )
            {
                name = line.left(pos).trimmed();
                text = line.mid(pos + 1).trimmed();

                tempStr = name.toLower();
                // Строка стиля
                if (ltStyle == tempStr)
                {
                    Line::Style* ptr = new Line::Style(tempStrList);
                    tempStrList.clear();

                    tempList = text.split(',');

                    // Пытаемся спасти большую часть строки
                    // Name
                    if (!tempList.isEmpty())
                    {
                        ptr->styleName = tempList.first().trimmed();
                        tempList.removeFirst();
                    }

                    // Fontname
                    if (!tempList.isEmpty())
                    {
                        ptr->fontName = tempList.first().trimmed();
                        tempList.removeFirst();
                    }

                    // Fontsize
                    if (!tempList.isEmpty())
                    {
                        ptr->fontSize = tempList.first().trimmed().toDouble();
                        tempList.removeFirst();
                    }

                    // PrimaryColour
                    if (!tempList.isEmpty())
                    {
                        tempStr = tempList.first().trimmed();
                        if ( tempStr.startsWith("&H") )
                        {
                            ptr->primaryColour = tempStr.mid(2).toUInt(nullptr, 16);
                        }
                        else
                        {
                            ptr->primaryColour = static_cast<quint32>( tempStr.toInt() );
                        }
                        tempList.removeFirst();
                    }

                    // SecondaryColour
                    if (!tempList.isEmpty())
                    {
                        tempStr = tempList.first().trimmed();
                        if ( tempStr.startsWith("&H") )
                        {
                            ptr->secondaryColour = tempStr.mid(2).toUInt(nullptr, 16);
                        }
                        else
                        {
                            ptr->secondaryColour = static_cast<uint>( tempStr.toInt() );
                        }
                        tempList.removeFirst();
                    }

                    // OutlineColour
                    if (!tempList.isEmpty())
                    {
                        tempStr = tempList.first().trimmed();
                        if ( tempStr.startsWith("&H") )
                        {
                            ptr->outlineColour = tempStr.mid(2).toUInt(nullptr, 16);
                        }
                        else
                        {
                            ptr->outlineColour = static_cast<uint>( tempStr.toInt() );
                        }
                        tempList.removeFirst();
                    }

                    // BackColour
                    if (!tempList.isEmpty())
                    {
                        tempStr = tempList.first().trimmed();
                        if ( tempStr.startsWith("&H") )
                        {
                            ptr->backColour = tempStr.mid(2).toUInt(nullptr, 16);
                        }
                        else
                        {
                            ptr->backColour = static_cast<uint>( tempStr.toInt() );
                        }
                        tempList.removeFirst();
                    }

                    // Bold
                    if (!tempList.isEmpty())
                    {
                        ptr->bold = tempList.first().trimmed().toInt() != 0;
                        tempList.removeFirst();
                    }

                    // Italic
                    if (!tempList.isEmpty())
                    {
                        ptr->italic = tempList.first().trimmed().toInt() != 0;
                        tempList.removeFirst();
                    }

                    if (SCR_ASS == type)
                    {
                        // Underline
                        if (!tempList.isEmpty())
                        {
                            ptr->underline = tempList.first().trimmed().toInt() != 0;
                            tempList.removeFirst();
                        }

                        // StrikeOut
                        if (!tempList.isEmpty())
                        {
                            ptr->strikeOut = tempList.first().trimmed().toInt() != 0;
                            tempList.removeFirst();
                        }

                        // ScaleX
                        if (!tempList.isEmpty())
                        {
                            ptr->scaleX = tempList.first().trimmed().toDouble();
                            tempList.removeFirst();
                        }

                        // ScaleY
                        if (!tempList.isEmpty())
                        {
                            ptr->scaleY = tempList.first().trimmed().toDouble();
                            tempList.removeFirst();
                        }

                        // Spacing
                        if (!tempList.isEmpty())
                        {
                            ptr->spacing = tempList.first().trimmed().toDouble();
                            tempList.removeFirst();
                        }

                        // Angle
                        if (!tempList.isEmpty())
                        {
                            ptr->angle = tempList.first().trimmed().toDouble();
                            tempList.removeFirst();
                        }
                    }

                    // BorderStyle
                    if (!tempList.isEmpty())
                    {
                        ptr->borderStyle = tempList.first().trimmed().toUShort();
                        tempList.removeFirst();
                    }

                    // Outline
                    if (!tempList.isEmpty())
                    {
                        ptr->outline = tempList.first().trimmed().toDouble();
                        tempList.removeFirst();
                    }

                    // Shadow
                    if (!tempList.isEmpty())
                    {
                        ptr->shadow = tempList.first().trimmed().toDouble();
                        tempList.removeFirst();
                    }

                    // Alignment
                    if (!tempList.isEmpty())
                    {
                        ptr->alignment = tempList.first().trimmed().toUShort();
                        if (SCR_SSA == type && ptr->alignment > 0 && ptr->alignment < Line::AlignmentSSA.length())
                        {
                            ptr->alignment = Line::AlignmentSSA.at(ptr->alignment);
                        }

                        if (ptr->alignment < 1 || ptr->alignment > 9)
                        {
                            ptr->alignment = 2;
                        }

                        tempList.removeFirst();
                    }

                    // MarginL
                    if (!tempList.isEmpty())
                    {
                        ptr->marginL = tempList.first().trimmed().toUShort();
                        tempList.removeFirst();
                    }

                    // MarginR
                    if (!tempList.isEmpty())
                    {
                        ptr->marginR = tempList.first().trimmed().toUShort();
                        tempList.removeFirst();
                    }

                    // MarginV
                    if (!tempList.isEmpty())
                    {
                        ptr->marginV = tempList.first().trimmed().toUShort();
                        tempList.removeFirst();
                    }

                    if (SCR_SSA == type)
                    {
                        // AlphaLevel
                        tempList.removeFirst();
                    }

                    // Encoding
                    if (!tempList.isEmpty())
                    {
                        ptr->encoding = tempList.first().trimmed().toUShort();
                    }

                    script.styles.append(ptr);
                }
                // Строка формата - пропускаем
                else if (ltFormat == tempStr) {}
                // Мусор
                else
                {
                    tempStrList.append(line);
                }
            }
            // Мусор
            else
            {
                tempStrList.append(line);
            }
            break;

        case SEC_EVENTS:
            // Началась другая секция
            if (reSection.match(line).hasMatch())
            {
                script.events.appendAfter(tempStrList);
                tempStrList.clear();

                readNext = false;
                state = SEC_UNKNOWN;
            }
            // Нормальная строка
            else if ( -1 != ( pos = line.indexOf(':') ) )
            {
                name = line.left(pos).trimmed();
                text = line.mid(pos + 1).trimmed();

                tempStr = name.toLower();
                // Строка события
                if (ltEvent == tempStr)
                {
                    Line::Event* ptr = new Line::Event(tempStrList);
                    tempStrList.clear();

                    tempList = text.split(',');

                    // Пытаемся спасти большую часть строки
                    // Layer
                    if (!tempList.isEmpty())
                    {
                        ptr->layer = tempList.first().remove(QRegularExpression("\\D")).toUInt();
                        tempList.removeFirst();
                    }

                    // Start
                    if (!tempList.isEmpty())
                    {
                        ptr->start = Line::StrToTime(tempList.first(), type);
                        tempList.removeFirst();
                    }

                    // End
                    if (!tempList.isEmpty())
                    {
                        ptr->end = Line::StrToTime(tempList.first(), type);
                        tempList.removeFirst();
                    }

                    // Style
                    if (!tempList.isEmpty())
                    {
                        ptr->style = tempList.first().trimmed();
                        tempList.removeFirst();
                    }

                    // Name
                    if (!tempList.isEmpty())
                    {
                        ptr->actorName = tempList.first().trimmed();
                        tempList.removeFirst();
                    }

                    // MarginL
                    if (!tempList.isEmpty())
                    {
                        ptr->marginL = tempList.first().trimmed().toUShort();
                        tempList.removeFirst();
                    }

                    // MarginR
                    if (!tempList.isEmpty())
                    {
                        ptr->marginR = tempList.first().trimmed().toUShort();
                        tempList.removeFirst();
                    }

                    // MarginV
                    if (!tempList.isEmpty())
                    {
                        ptr->marginV = tempList.first().trimmed().toUShort();
                        tempList.removeFirst();
                    }

                    // Effect
                    if (!tempList.isEmpty())
                    {
                        ptr->effect = tempList.first().trimmed();
                        tempList.removeFirst();
                    }

                    // Text
                    if (!tempList.isEmpty())
                    {
                        ptr->text = tempList.join(',');
                    }

                    script.events.append(ptr);
                }
                // Строка формата - пропускаем
                else if (ltFormat == tempStr) {}
                // Мусор
                else
                {
                    tempStrList.append(line);
                }
            }
            // Мусор
            else
            {
                tempStrList.append(line);
            }
            break;

        case SEC_FONTS:
            // Началась другая секция
            if (reSection.match(line).hasMatch())
            {
                readNext = false;
                state = SEC_UNKNOWN;
            }
            // Контент
            else
            {
                script.fonts.append(new Line::Base(line));
            }
            break;

        case SEC_GRAPHICS:
            // Началась другая секция
            if (reSection.match(line).hasMatch())
            {
                readNext = false;
                state = SEC_UNKNOWN;
            }
            // Контент
            else
            {
                script.graphics.append(new Line::Base(line));
            }
            break;
        }
    }

    // Спасаем мусор в конце файла
    if (tempStrList.length())
    {
        script.appendAfter(tempStrList);
    }

    return true;
}

//
// Парсер SRT
//
enum SRTState {SRTST_EMPTY, SRTST_NEW, SRTST_TEXT};

bool ParseSRT(QTextStream& in, Script& script)
{
    in.seek(0);

    // Идея: можно просто разбить файл, используя два перевода строки (и номер, и время) - медленно

    // Регулярка для определения секции
    QRegularExpression reTime("^(\\d{2}:\\d{2}:\\d{2},\\d{3}) *--> *(\\d{2}:\\d{2}:\\d{2},\\d{3})$");
    QRegularExpressionMatch match;

    QString line;
    QStringList tempList;
    SRTState state = SRTST_EMPTY;
    bool isNumber;
    uint start = 0, end = 0;
    while ( !in.atEnd() )
    {
        line = in.readLine().trimmed();

        switch (state)
        {
        case SRTST_EMPTY:
            // Пустая строка - пропускаем
            if (line.isEmpty()) {}
            else
            {
                line.toInt(&isNumber);
                if (isNumber)
                {
                    state = SRTST_NEW;
                }
                else
                {
                    return false;
                }
            }
            break;

        case SRTST_NEW:
            // Пустая строка - ошибка
            if (line.isEmpty())
            {
                return false;
            }
            else if ((match = reTime.match(line)).hasMatch())
            {
                state = SRTST_TEXT;

                start = Line::StrToTime(match.captured(1), SCR_SRT);
                end   = Line::StrToTime(match.captured(2), SCR_SRT);
            }
            else
            {
                return false;
            }
            break;

        case SRTST_TEXT:
            // Пустая строка - фраза закончилась
            if (line.isEmpty())
            {
                state = SRTST_EMPTY;

                if (!tempList.isEmpty())
                {
                    Line::Event* ptr = new Line::Event();
                    ptr->start = start;
                    ptr->end = end;
                    ptr->text = tempList.join("\\N");
                    script.events.append(ptr);
                    tempList.clear();
                }
            }
            else
            {
                tempList.append(line);
            }
            break;
        }
    }
    if (!tempList.isEmpty())
    {
        Line::Event* ptr = new Line::Event();
        ptr->start = start;
        ptr->end = end;
        ptr->text = tempList.join("\\N");
        script.events.append(ptr);
        tempList.clear();
    }

    // Важные заголовки
    Line::Named* ptr = new Line::Named("WrapStyle", QStringList("; Script generated by Re_Sync 2"));
    ptr->text = "0";
    script.header.append(ptr);

    ptr = new Line::Named("ScaledBorderAndShadow");
    ptr->text = "yes";
    script.header.append(ptr);

    ptr = new Line::Named("Collisions");
    ptr->text = "Normal";
    script.header.append(ptr);

    // Стиль по умолчанию
    script.styles.append(new Line::Style());

    return true;
}

void GenerateSSA(QTextStream& out, const Script& script)
{
    out << script.generate(SCR_SSA);
}

void GenerateASS(QTextStream& out, const Script& script)
{
    out << script.generate(SCR_ASS);
}

void GenerateSRT(QTextStream& out, const Script& script)
{
    out << script.generate(SCR_SRT);
}
}
