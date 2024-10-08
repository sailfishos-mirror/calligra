/*
 * This file is part of Office 2007 Filters for Calligra
 *
 * SPDX-FileCopyrightText: 2010 Nokia Corporation and /or its subsidiary(-ies).
 *
 * Contact: Christoph Schleifenbaum christoph@kdab.com
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */
#include "NumberFormatParser.h"

#include <KoGenStyles.h>
#include <KoXmlWriter.h>

#include <MsoUtils.h>

#include <QBuffer>
#include <QLocale>
#include <QString>
#include <QStringList>

#include <QColor>
#include <QPalette>

QColor NumberFormatParser::color(const QString &name)
{
    if (name.startsWith(QLatin1String("color"), Qt::CaseInsensitive)) {
        bool ok = false;
        const int index = QStringView{name}.mid(5).toInt(&ok) + 7;
        return MSO::defaultIndexedColor(index);
    } else {
        return QColor(name);
    }
}

QLocale NumberFormatParser::locale(int langid)
{
    return MSO::localeForLangId(langid);
}

#define SET_TYPE_OR_RETURN(TYPE)                                                                                                                               \
    {                                                                                                                                                          \
        if (type == KoGenStyle::NumericDateStyle && TYPE == KoGenStyle::NumericTimeStyle) {                                                                    \
        } else if (type == KoGenStyle::NumericDateStyle && TYPE == KoGenStyle::NumericNumberStyle) {                                                           \
        } else if (type == KoGenStyle::NumericTimeStyle && TYPE == KoGenStyle::NumericNumberStyle) {                                                           \
        } else if (type == KoGenStyle::NumericTimeStyle && TYPE == KoGenStyle::NumericDateStyle) {                                                             \
            type = TYPE;                                                                                                                                       \
        } else if (type == KoGenStyle::NumericPercentageStyle && TYPE == KoGenStyle::NumericNumberStyle) {                                                     \
        } else if (type == KoGenStyle::NumericNumberStyle && TYPE == KoGenStyle::NumericPercentageStyle) {                                                     \
            type = TYPE;                                                                                                                                       \
        } else if (type == KoGenStyle::NumericCurrencyStyle && TYPE == KoGenStyle::NumericNumberStyle) {                                                       \
        } else if (type == KoGenStyle::NumericNumberStyle && TYPE == KoGenStyle::NumericCurrencyStyle) {                                                       \
            type = TYPE;                                                                                                                                       \
        } else if (type == KoGenStyle::NumericFractionStyle && TYPE == KoGenStyle::NumericNumberStyle) {                                                       \
        } else if (type == KoGenStyle::NumericNumberStyle && TYPE == KoGenStyle::NumericFractionStyle) {                                                       \
            type = TYPE;                                                                                                                                       \
        } else if (type == KoGenStyle::NumericScientificStyle && TYPE == KoGenStyle::NumericNumberStyle) {                                                     \
        } else if (type == KoGenStyle::NumericNumberStyle && TYPE == KoGenStyle::NumericScientificStyle) {                                                     \
            type = TYPE;                                                                                                                                       \
        } else if (type != KoGenStyle::ParagraphAutoStyle && type != TYPE) {                                                                                   \
            return KoGenStyle(KoGenStyle::ParagraphAutoStyle);                                                                                                 \
        } else {                                                                                                                                               \
            type = TYPE;                                                                                                                                       \
        }                                                                                                                                                      \
    }

#define FINISH_PLAIN_TEXT_PART                                                                                                                                 \
    {                                                                                                                                                          \
        if (!plainText.isEmpty()) {                                                                                                                            \
            hadPlainText = true;                                                                                                                               \
            xmlWriter.startElement("number:text");                                                                                                             \
            xmlWriter.addTextNode(plainText);                                                                                                                  \
            xmlWriter.endElement();                                                                                                                            \
            plainText.clear();                                                                                                                                 \
        }                                                                                                                                                      \
    }

static KoGenStyle styleFromTypeAndBuffer(KoGenStyle::Type type, const QBuffer &buffer)
{
    KoGenStyle result(type);

    const QString elementContents = QString::fromUtf8(buffer.buffer(), buffer.buffer().size());
    result.addChildElement("number", elementContents);

    return result;
}

KoGenStyle NumberFormatParser::parse(const QString &origNumberFormat, KoGenStyles *styles, KoGenStyle::Type type)
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    KoXmlWriter xmlWriter(&buffer);

    QString plainText;
    QMultiMap<QString, QString> conditions;
    QString condition;

    // This is for the month vs. minutes-context.
    bool justHadHours = false;

    // to skip escaped plain-text
    bool hadPlainText = false;
    QString numberFormat = origNumberFormat;

    for (int i = 0; i < numberFormat.length(); ++i) {
        const char c = numberFormat[i].toLatin1();
        bool isSpecial = true;

        const bool isLong = (i < numberFormat.length() - 1 && numberFormat[i + 1] == c);
        const bool isLonger = (isLong && i < numberFormat.length() - 2 && numberFormat[i + 2] == c);
        const bool isLongest = (isLonger && i < numberFormat.length() - 3 && numberFormat[i + 3] == c);
        const bool isWayTooLong = (isLongest && i < numberFormat.length() - 4 && numberFormat[i + 4] == c);

        switch (c) {
            // condition or color or locale or elapsed format
        case '[': {
            const char ch = (i < numberFormat.length() - 1) ? numberFormat[++i].toLatin1() : ']';
            if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
                // color code
                QString colorName;
                while (i < numberFormat.length() && numberFormat[i] != QLatin1Char(']'))
                    colorName += numberFormat[i++];

                const QColor color = NumberFormatParser::color(colorName);
                if (color.isValid()) {
                    xmlWriter.startElement("style:text-properties");
                    xmlWriter.addAttribute("fo:color", color.name());
                    xmlWriter.endElement();
                } else if ((colorName == "hh") || (colorName == "h") || (colorName == "mm") || (colorName == "m") || (colorName == "ss")
                           || (colorName == "s")) {
                    // was actually time in 'elapsed format'
                    numberFormat.insert(i + 1, colorName);
                }
            } else if (ch == '$' && i < numberFormat.length() - 2 && numberFormat[i + 1].toLatin1() != '-') {
                SET_TYPE_OR_RETURN(KoGenStyle::NumericCurrencyStyle);
                ++i;

                // currency code
                QString currency;
                while (i < numberFormat.length() && numberFormat[i] != QLatin1Char(']') && numberFormat[i] != QLatin1Char('-'))
                    currency += numberFormat[i++];

                QString language;
                QString country;

                if (numberFormat[i] == QLatin1Char('-')) {
                    ++i;
                    QString localeId;
                    while (i < numberFormat.length() && numberFormat[i] != QLatin1Char(']'))
                        localeId += numberFormat[i++];
                    const QLocale locale = NumberFormatParser::locale(localeId.toInt(nullptr, 16));
                    language = locale.name();
                    language = language.left(language.indexOf(QLatin1String("_")));
                    country = locale.name();
                    country = country.mid(country.indexOf(QLatin1String("_")) + 1);
                }

                xmlWriter.startElement("number:currency-symbol");
                if (!language.isEmpty()) {
                    xmlWriter.addAttribute("number:language", language);
                }
                if (!country.isEmpty()) {
                    xmlWriter.addAttribute("number:country", country);
                }

                xmlWriter.addTextSpan(currency);
                xmlWriter.endElement();
            } else {
                // unknown - no idea, skip
                while (i < numberFormat.length() && numberFormat[i] != QLatin1Char(']'))
                    ++i;
            }
        } break;

        // underscore: ignore the next char
        case '_':
            plainText += QLatin1Char(' ');
            ++i;
            break;

            // asterisk: ignore
        case '*':
        case '(':
        case ')':
            break;

            // percentage
        case '%':
            SET_TYPE_OR_RETURN(KoGenStyle::NumericPercentageStyle);
            FINISH_PLAIN_TEXT_PART
            xmlWriter.startElement("number:text");
            xmlWriter.addTextNode("%");
            xmlWriter.endElement();
            break;

            // a number
        case '.':
        case ',':
        case '#':
        case '0':
        case '?':
            SET_TYPE_OR_RETURN(KoGenStyle::NumericNumberStyle)
            FINISH_PLAIN_TEXT_PART
            // do following only if we are really a number and not part of another KoGenStyle like a date or time formatting
            if (type == KoGenStyle::NumericNumberStyle || type == KoGenStyle::NumericFractionStyle || type == KoGenStyle::NumericScientificStyle
                || type == KoGenStyle::NumericCurrencyStyle) {
                bool grouping = false;
                bool gotDot = false;
                bool gotE = false;
                bool gotFraction = false;
                int decimalPlaces = 0;
                int integerDigits = 0;
                int exponentDigits = 0;
                int numeratorDigits = 0;
                int denominatorDigits = 0;

                char ch = numberFormat[i].toLatin1();
                do {
                    if (ch == '.') {
                        gotDot = true;
                    } else if (ch == ',') {
                        grouping = true;
                    } else if (ch == 'E' || ch == 'e') {
                        SET_TYPE_OR_RETURN(KoGenStyle::NumericScientificStyle);

                        if (i >= numberFormat.length() - 1)
                            break;
                        const char chN = numberFormat[i + 1].toLatin1();
                        if (chN == '-' || chN == '+') {
                            gotE = true;
                            ++i;
                        }
                    } else if (ch == '0' && gotE) {
                        ++exponentDigits;
                    } else if (ch == '0' && !gotDot && !gotFraction) {
                        ++integerDigits;
                    } else if (ch == '0' && gotDot && !gotFraction) {
                        ++decimalPlaces;
                    } else if (ch == '?' && !gotFraction) {
                        ++numeratorDigits;
                    } else if (ch == '?' && gotFraction) {
                        ++denominatorDigits;
                    } else if (ch == '/') {
                        SET_TYPE_OR_RETURN(KoGenStyle::NumericFractionStyle);
                        if (gotDot)
                            return KoGenStyle();

                        gotFraction = true;
                    }

                    if (i >= numberFormat.length() - 1)
                        break;
                    ch = numberFormat[++i].toLatin1();

                    if (ch == ' ') {
                        // Spaces are not allowed - but there is an exception:
                        // if this is a fraction. Let's check for '?' or '/'
                        const char c = numberFormat[i + 1].toLatin1();
                        if (c == '?' || c == '/')
                            ch = numberFormat[++i].toLatin1();
                    }
                } while (i < numberFormat.length() && (ch == '.' || ch == ',' || ch == '#' || ch == '0' || ch == 'E' || ch == 'e' || ch == '?' || ch == '/'));

                if (!(ch == '.' || ch == ',' || ch == '#' || ch == '0' || ch == 'E' || ch == 'e' || ch == '?' || ch == '/')) {
                    --i;
                }

                if (gotFraction) {
                    xmlWriter.startElement("number:fraction");
                } else if (exponentDigits > 0) {
                    xmlWriter.startElement("number:scientific-number");
                } else {
                    xmlWriter.startElement("number:number");
                }
                if (!gotFraction) {
                    xmlWriter.addAttribute("number:decimal-places", decimalPlaces);
                }
                xmlWriter.addAttribute("number:min-integer-digits", integerDigits);
                if (exponentDigits > 0) {
                    xmlWriter.addAttribute("number:min-exponent-digits", exponentDigits);
                }
                if (grouping) {
                    xmlWriter.addAttribute("number:grouping", grouping ? "true" : "false");
                }
                if (gotFraction) {
                    xmlWriter.addAttribute("number:min-numerator-digits", numeratorDigits);
                    xmlWriter.addAttribute("number:min-denominator-digits", denominatorDigits);
                }
                xmlWriter.endElement();
            }
            break;

            // Everything related to date/time
            // AM/PM
        case 'A':
        case 'a':
            if (numberFormat.mid(i, 5).toLower() == QLatin1String("am/pm") || numberFormat.mid(i, 3).toLower() == QLatin1String("a/p")) {
                SET_TYPE_OR_RETURN(KoGenStyle::NumericTimeStyle)
                FINISH_PLAIN_TEXT_PART;
                xmlWriter.startElement("number:am-pm");
                xmlWriter.endElement();
                if (numberFormat.mid(i, 5).toLower() == QLatin1String("am/pm"))
                    i += 2;
                i += 2;
            }
            break;

            // hours, long or short
        case 'H':
        case 'h':
            SET_TYPE_OR_RETURN(KoGenStyle::NumericTimeStyle)
            FINISH_PLAIN_TEXT_PART;
            xmlWriter.startElement("number:hours");
            if (isLong) {
                xmlWriter.addAttribute("number:style", "long");
                ++i;
            }
            xmlWriter.endElement();
            break;

            // minutes or months, depending on context
        case 'M':
        case 'm':
            // must be month, then, at least three M
            if (isLonger) {
                SET_TYPE_OR_RETURN(KoGenStyle::NumericDateStyle)
                FINISH_PLAIN_TEXT_PART;
                xmlWriter.startElement("number:month");
                const bool isReallyReallyLong = (isWayTooLong && i < numberFormat.length() - 4 && numberFormat[i + 4] == c);
                if (isLongest && !isReallyReallyLong)
                    xmlWriter.addAttribute("number:style", "long");
                if (isWayTooLong) {
                    // If the month format is "mmmmm" then it's the
                    // extra-short format of month.
                    xmlWriter.addAttribute("calligra:number-length", "extra-short");
                }
                xmlWriter.addAttribute("number:textual", "true");
                xmlWriter.endElement();
                i += isLongest ? (isWayTooLong ? 4 : 3) : 2;
                if (isReallyReallyLong) {
                    ++i;
                }
            }
            // This depends on the context. After hours and before seconds,
            // it's minutes otherwise it's the month
            else {
                if (justHadHours) {
                    SET_TYPE_OR_RETURN(KoGenStyle::NumericTimeStyle)
                    FINISH_PLAIN_TEXT_PART;
                    xmlWriter.startElement("number:minutes");
                    if (isLong)
                        xmlWriter.addAttribute("number:style", "long");
                    xmlWriter.endElement();
                } else {
                    // On the next iteration, we might see whether there're
                    // seconds or something else. Let's just default to
                    // minutes, if there's nothing more.
                    bool minutes = true;

                    // So, let's look ahead:
                    for (int j = i + 1; j < numberFormat.length(); ++j) {
                        const char ch = numberFormat[i].toLatin1();
                        if (ch == 's' || ch == 'S') { // minutes
                            break;
                        }
                        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))) { // months
                            continue;
                        }
                        minutes = false;
                        break;
                    }
                    if (minutes) {
                        SET_TYPE_OR_RETURN(KoGenStyle::NumericTimeStyle)
                    } else {
                        SET_TYPE_OR_RETURN(KoGenStyle::NumericDateStyle)
                        FINISH_PLAIN_TEXT_PART;
                    }
                    if (minutes) {
                        xmlWriter.startElement("number:minutes");
                    } else {
                        xmlWriter.startElement("number:month");
                    }
                    if (isLong) {
                        xmlWriter.addAttribute("number:style", "long");
                    }
                    xmlWriter.endElement();
                }
                if (isLong) {
                    ++i;
                }
            }
            break;

            // day (of week)
        case 'D':
        case 'd':
            SET_TYPE_OR_RETURN(KoGenStyle::NumericDateStyle)
            FINISH_PLAIN_TEXT_PART;
            if (!isLonger) {
                xmlWriter.startElement("number:day");
                if (isLong) {
                    xmlWriter.addAttribute("number:style", "long");
                }
                xmlWriter.endElement();
            } else {
                xmlWriter.startElement("number:day-of-week");
                if (isLongest) {
                    xmlWriter.addAttribute("number:style", "long");
                }
                xmlWriter.endElement();
            }
            if (isLong) {
                ++i;
            }
            if (isLonger) {
                ++i;
            }
            if (isLongest) {
                ++i;
            }
            break;

            // seconds, long or short
        case 'S':
        case 's':
            SET_TYPE_OR_RETURN(KoGenStyle::NumericTimeStyle)
            FINISH_PLAIN_TEXT_PART;
            xmlWriter.startElement("number:seconds");
            if (isLong) {
                xmlWriter.addAttribute("number:style", "long");
                ++i;
            }
            xmlWriter.endElement();
            break;

            // year, long or short
        case 'Y':
        case 'y':
            SET_TYPE_OR_RETURN(KoGenStyle::NumericDateStyle)
            FINISH_PLAIN_TEXT_PART;
            xmlWriter.startElement("number:year");
            if (isLongest) {
                xmlWriter.addAttribute("number:style", "long");
                i += 3;
            } else if (isLong) {
                ++i;
            }
            xmlWriter.endElement();
            break;

            // Now it's getting really scary: semi-colon:
        case ';': {
            FINISH_PLAIN_TEXT_PART;
            buffer.close();

            // conditional style with the current format
            if (styles) {
                KoGenStyle result = styleFromTypeAndBuffer(type, buffer);
                result.addAttribute("style:volatile", "true");
                const QString styleName = styles->insert(result, "N");
                conditions.insert(condition, styleName);
            }
            condition.clear();

            // Start a new style
            buffer.setData(QByteArray());
            buffer.open(QIODevice::WriteOnly);
            type = KoGenStyle::ParagraphAutoStyle;
            hadPlainText = false;
        } break;

        // text-content
        case '@':
            FINISH_PLAIN_TEXT_PART;
            hadPlainText = true;
            xmlWriter.startElement("number:text-content");
            xmlWriter.endElement();
            break;

            // quote - plain text block
        case '"':
            isSpecial = false;
            while (i < numberFormat.length() - 1 && numberFormat[++i] != QLatin1Char('"'))
                plainText += numberFormat[i];
            break;

            // backslash escapes the next char
        case '\\':
            isSpecial = false;
            if (i < numberFormat.length() - 1) {
                plainText += numberFormat[++i];
            }
            break;

            // every other char is just passed
        default:
            isSpecial = false;
            plainText += c;
            break;
        }

        // for the context-sensitive 'M' which can mean either minutes or months
        if (isSpecial) {
            justHadHours = (c == 'h' || c == 'H');
        }
    }

    FINISH_PLAIN_TEXT_PART;

    if (type == KoGenStyle::ParagraphAutoStyle && hadPlainText) {
        SET_TYPE_OR_RETURN(KoGenStyle::NumericTextStyle)
    }

    if (!condition.isEmpty()) {
        buffer.close();

        // conditional style with the current format
        if (styles) {
            KoGenStyle result = styleFromTypeAndBuffer(type, buffer);
            result.addAttribute("style:volatile", "true");
            const QString styleName = styles->insert(result, "N");
            conditions.insert(condition, styleName);
        }
        condition.clear();

        // start a new style
        buffer.setData(QByteArray());
        buffer.open(QIODevice::WriteOnly);
    }

    // If conditions w/o explicit expressions where added, we create the
    // expressions.
    QStringList autoConditions;
    if (conditions.count(QString()) == 1) {
        autoConditions.push_back(QLatin1String("value()>=0"));
    } else {
        autoConditions.push_back(QLatin1String("value()>0"));
        autoConditions.push_back(QLatin1String("value()<0"));
        autoConditions.push_back(QLatin1String("value()=0"));
    }

    // Add conditional styles.
    for (auto it = conditions.constBegin(); it != conditions.constEnd(); ++it) {
        // Conditional styles are always numbers.
        type = KoGenStyle::NumericNumberStyle;

        xmlWriter.startElement("style:map");
        xmlWriter.addAttribute("style:condition", it.key().isEmpty() ? autoConditions.takeLast() : it.key());
        xmlWriter.addAttribute("style:apply-style-name", it.value());
        xmlWriter.endElement();
    }

    buffer.close();

    // conditional style with the current format
    return styleFromTypeAndBuffer(type, buffer);
}

bool NumberFormatParser::isDateFormat(const QString &numberFormat)
{
    // this is for the month vs. minutes-context
    bool justHadHours = false;

    for (int i = 0; i < numberFormat.length(); ++i) {
        const char c = numberFormat[i].toLatin1();
        bool isSpecial = true;

        const bool isLong = i < numberFormat.length() - 1 && numberFormat[i + 1] == c;
        const bool isLonger = isLong && i < numberFormat.length() - 2 && numberFormat[i + 2] == c;
        const bool isLongest = isLonger && i < numberFormat.length() - 3 && numberFormat[i + 3] == c;
        Q_UNUSED(isLongest);
        switch (c) {
            // condition or color or locale...
        case '[': {
            // don't care, skip
            while (i < numberFormat.length() && numberFormat[i] != QLatin1Char(']'))
                ++i;
        } break;

        // underscore: ignore the next char
        case '_':
            ++i;
            break;

            // asterisk: ignore
        case '*':
        case '(':
        case ')':
            break;

            // percentage
        case '%':
            // SET_TYPE_OR_RETURN(KoGenStyle::NumericPercentageStyle);
            break;

            // a number
        case '.':
        case ',':
        case '#':
        case '0':
        case '?': {
            // SET_TYPE_OR_RETURN(KoGenStyle::NumericNumberStyle)
            char ch = numberFormat[i].toLatin1();
            do {
                if (i >= numberFormat.length() - 1)
                    break;
                ch = numberFormat[++i].toLatin1();

                if (ch == ' ') {
                    // spaces are not allowed - but there's an exception: if
                    // this is a fraction. Let's check for '?' or '/'
                    const char c = numberFormat[i + 1].toLatin1();
                    if (c == '?' || c == '/')
                        ch = numberFormat[++i].toLatin1();
                }
            } while (i < numberFormat.length() && (ch == '.' || ch == ',' || ch == '#' || ch == '0' || ch == 'E' || ch == 'e' || ch == '?' || ch == '/'));

            if (!(ch == '.' || ch == ',' || ch == '#' || ch == '0' || ch == 'E' || ch == 'e' || ch == '?' || ch == '/')) {
                --i;
            }
        } break;

            // Everything related to date/time
            // AM/PM
        case 'A':
        case 'a':
            if (numberFormat.mid(i, 5).toLower() == QLatin1String("am/pm") || numberFormat.mid(i, 3).toLower() == QLatin1String("a/p")) {
                // SET_TYPE_OR_RETURN(KoGenStyle::NumericTimeStyle)
                if (numberFormat.mid(i, 5).toLower() == QLatin1String("am/pm"))
                    i += 2;
                i += 2;
            }
            break;

            // hours, long or short
        case 'H':
        case 'h':
            // SET_TYPE_OR_RETURN(KoGenStyle::NumericTimeStyle)
            if (isLong) {
                ++i;
            }
            break;

            // minutes or months, depending on context
        case 'M':
        case 'm':
            // must be month, then, at least three M
            if (isLonger) {
                return true;
            }
            // depends on the context. After hours and before seconds, it's minutes
            // otherwise it's the month
            else {
                if (justHadHours) {
                    // SET_TYPE_OR_RETURN(KoGenStyle::NumericTimeStyle)
                } else {
                    // On the next iteration, we might see whether there're
                    // seconds or something else. Let's just default to
                    // minutes, if there's nothing more.
                    bool minutes = true;
                    // so let's look ahead:
                    for (int j = i + 1; j < numberFormat.length(); ++j) {
                        const char ch = numberFormat[i].toLatin1();
                        if (ch == 's' || ch == 'S') { // minutes
                            break;
                        }
                        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))) { // months
                            continue;
                        }
                        minutes = false;
                        break;
                    }
                    if (minutes) {
                        // SET_TYPE_OR_RETURN(KoGenStyle::NumericTimeStyle)
                    } else {
                        return true;
                    }
                }
                if (isLong) {
                    ++i;
                }
            }
            break;

            // day (of week)
        case 'D':
        case 'd':
            return true;

            // seconds, long or short
        case 'S':
        case 's':
            // SET_TYPE_OR_RETURN(KoGenStyle::NumericTimeStyle)
            if (isLong) {
                ++i;
            }
            break;

            // year, long or short
        case 'Y':
        case 'y':
            return true;

            // now it's getting really scarry: semi-colon:
        case ';':
            break;

        // text-content
        case '@':
            break;

            // quote - plain text block
        case '"':
            isSpecial = false;
            while (i < numberFormat.length() - 1 && numberFormat[++i] != QLatin1Char('"'))
                /* empty */;
            break;

            // backslash escapes the next char
        case '\\':
            isSpecial = false;
            if (i < numberFormat.length() - 1) {
                ++i;
            }
            break;

            // every other char is just passed
        default:
            isSpecial = false;
            break;
        }

        // for the context-sensitive 'M' which can mean either minutes or months
        if (isSpecial) {
            justHadHours = (c == 'h' || c == 'H');
        }
    }

    return false;
}
