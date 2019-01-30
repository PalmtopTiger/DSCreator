#ifndef SCRIPT_H
#define SCRIPT_H

#include <QVector>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTextStream>


namespace Script
{
enum ScriptType {SCR_UNKNOWN, SCR_ASS, SCR_SSA, SCR_SRT};
enum SectionType {SEC_UNKNOWN, SEC_HEADER, SEC_STYLES, SEC_EVENTS, SEC_FONTS, SEC_GRAPHICS};

namespace Sections
{
const QString header    = "Script Info";
const QString stylesSSA = "V4 Styles";
const QString stylesASS = "V4+ Styles";
const QString events    = "Events";
const QString fonts     = "Fonts";
const QString graphics  = "Graphics";
}

namespace Line
{
const QVector<ushort> AlignmentSSA = {0, 1, 2, 3, 0, 7, 8, 9, 0, 4, 5, 6};
const QVector<ushort> AlignmentASS = {0, 1, 2, 3, 9, 10, 11, 5, 6, 7};

const QString defaultStyle = "Default";
const QString defaultFont = "Arial";

uint StrToTime(const QString& str, const ScriptType type);
QString TimeToStr(const uint time, const ScriptType type);

// Базовая строка
class Base
{
public:
    Base();
    Base(const QString& value);

    QString generate(const ScriptType type) const;

private:
    QString _value;
};

// Простейшая строка с двоеточием
class Named : public Base
{
public:
    QString text;

    Named(const QString& name);
    Named(const QString& name, const QStringList& before);

    void clearBefore();
    QString name() const;
    QString generate(const ScriptType type) const;

protected:
    QString     _name;
    QStringList _before;

    QString generate(const ScriptType type, const QString& value) const;
};

// Строка стиля
class Style : public Named
{
public:
    QString   styleName;
    QString   fontName;
    double    fontSize;
    quint32   primaryColour;
    quint32   secondaryColour;
    quint32   outlineColour;
    quint32   backColour;
    bool      bold;
    bool      italic;
    bool      underline;
    bool      strikeOut;
    double    scaleX;
    double    scaleY;
    double    spacing;
    double    angle;
    ushort    borderStyle;
    double    outline;
    double    shadow;
    ushort    alignment;
    ushort    marginL;
    ushort    marginR;
    ushort    marginV;
    ushort    encoding;

    Style();
    Style(const QStringList& before);

    QString generate(const ScriptType type) const;

private:
    void init();
};

// Строка события
class Event : public Named
{
public:
    uint        layer;
    uint        start;
    uint        end;
    QString     style;
    QString     actorName;
    ushort      marginL;
    ushort      marginR;
    ushort      marginV;
    QString     effect;

    Event();
    Event(const QStringList& before);

    QString generate(const ScriptType type) const;

private:
    void init();
};
}

// Секция в файле
template <class T>
class Section
{
public:
    QList<T*> content;

    Section(const Section<T> &original) :
        _sectionType(original._sectionType),
        _after(original._after)
    {
        for (const T* const e : original.content) content.append(new T(*e));
    }
    Section(const SectionType sectionType) :
        _sectionType(sectionType)
    {}

    ~Section()
    {
        qDeleteAll(content);
    }

    void clearAfter()
    {
        _after.clear();
    }

    void clear()
    {
        qDeleteAll(content);
        content.clear();
        clearAfter();
    }

    bool isEmpty() const
    {
        return content.isEmpty() && _after.isEmpty();
    }

    void appendAfter(const QStringList& after)
    {
        _after.append(after);
    }

    void append(T* ptr)
    {
        content.append(ptr);
    }

    QString generate(const ScriptType type) const
    {
        QString result;

        if (SCR_ASS == type || SCR_SSA == type)
        {
            switch (_sectionType)
            {
            case SEC_HEADER:
                result.append( QString("[%1]\n").arg(Sections::header) );
                break;

            case SEC_STYLES:
                if (SCR_ASS == type)
                {
                    result.append( QString("[%1]\n").arg(Sections::stylesASS) );
                    result.append("Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n");
                }
                else
                {
                    result.append( QString("[%1]\n").arg(Sections::stylesSSA) );
                    result.append("Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, TertiaryColour, BackColour, Bold, Italic, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, AlphaLevel, Encoding\n");
                }
                break;

            case SEC_EVENTS:
                result.append( QString("[%1]\n").arg(Sections::events) );
                if (SCR_ASS == type)
                {
                    result.append("Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n");
                }
                else
                {
                    result.append("Format: Marked, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n");
                }
                break;

            case SEC_FONTS:
                result.append( QString("[%1]\n").arg(Sections::fonts) );
                break;

            case SEC_GRAPHICS:
                result.append( QString("[%1]\n").arg(Sections::graphics) );
                break;

            default:
                break;
            }

            for (const T* const e : qAsConst(content))
            {
                result.append( e->generate(type) );
                result.append("\n");
            }

            // Уродливый костыль
            if (SEC_HEADER == _sectionType)
            {
                if (SCR_ASS == type)
                {
                    result.append("ScriptType: v4.00+\n");
                }
                else
                {
                    result.append("ScriptType: v4.00\n");
                }
            }

            if (_after.length())
            {
                result.append( _after.join("\n") );
                result.append("\n");
            }
        }
        else if (SCR_SRT == type && SEC_EVENTS == _sectionType)
        {
            for (typename QList<T*>::size_type i = 0, len = content.length(); i < len; ++i)
            {
                result.append( QString("%1\n").arg(i + 1) );
                result.append( content.at(i)->generate(type) );
                result.append("\n\n");
            }
        }

        return result;
    }

private:
    SectionType _sectionType;
    QStringList _after;
};

// Скрипт
class Script
{
public:
    Script();

    Section<Line::Named>  header;
    Section<Line::Style>  styles;
    Section<Line::Event>  events;
    Section<Line::Base>   fonts;
    Section<Line::Base>   graphics;

    void clearBefore();
    void clearAfter();
    void clear();
    void appendBefore(const QStringList& before);
    void appendAfter(const QStringList& after);
    QString generate(const ScriptType type) const;

private:
    QStringList _before;
    QStringList _after;
};

ScriptType DetectFormat(QTextStream& in);
bool ParseSSA(QTextStream& in, Script& script);
bool ParseSRT(QTextStream& in, Script& script);
void GenerateSSA(QTextStream& out, const Script& script);
void GenerateASS(QTextStream& out, const Script& script);
void GenerateSRT(QTextStream& out, const Script& script);
}

#endif // SCRIPT_H
