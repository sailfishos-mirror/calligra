/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2008 Girish Ramakrishnan <girish@forwardbias.in>
 * SPDX-FileCopyrightText: 2009 Elvis Stansvik <elvstone@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoTextDebug.h"

#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextFragment>
#include <QTextFrame>
#include <QTextList>
#include <QTextStream>
#include <QTextTable>

#include "KoTextBlockData.h"
#include "KoTextDocument.h"
#include "styles/KoCharacterStyle.h"
#include "styles/KoListStyle.h"
#include "styles/KoParagraphStyle.h"
#include "styles/KoStyleManager.h"
#include "styles/KoTableCellStyle.h"
#include "styles/KoTableStyle.h"
#include <KoImageData.h>
#include <KoInlineNote.h>
#include <KoInlineTextObjectManager.h>

#define PARAGRAPH_BORDER_DEBUG

int KoTextDebug::depth = 0;
const int KoTextDebug::INDENT = 2;
const QTextDocument *KoTextDebug::document = nullptr;

#define dumpIndent(T)                                                                                                                                          \
    {                                                                                                                                                          \
        for (int i = 0; i < T; ++i)                                                                                                                            \
            out << ' ';                                                                                                                                        \
    }
#define dumpList(T)                                                                                                                                            \
    {                                                                                                                                                          \
        foreach (const QString &x, T)                                                                                                                          \
            out << x << ' ';                                                                                                                                   \
    }

Q_DECLARE_METATYPE(QVector<KoText::Tab>)

static QString fontProperties(const QTextCharFormat &textFormat)
{
    QMap<int, QVariant> properties = textFormat.properties();
    QStringList fontProps;
    // add only font properties here
    foreach (int id, properties.keys()) {
        switch (id) {
        case QTextFormat::FontFamily:
            fontProps.append(properties[id].toString());
            break;
        case QTextFormat::FontPointSize:
            fontProps.append(QString("%1pt").arg(properties[id].toDouble()));
            break;
        case QTextFormat::FontSizeAdjustment:
            fontProps.append(QString("%1adj").arg(properties[id].toDouble()));
            break;
        case QTextFormat::FontWeight:
            fontProps.append(QString("weight %1").arg(properties[id].toInt()));
            break;
        case QTextFormat::FontItalic:
            fontProps.append(properties[id].toBool() ? "italic" : "non-italic");
            break;
        case QTextFormat::FontPixelSize:
            fontProps.append(QString("%1px").arg(properties[id].toDouble()));
            break;
        case QTextFormat::FontFixedPitch:
            fontProps.append(properties[id].toBool() ? "fixedpitch" : "varpitch");
            break;
        case QTextFormat::FontCapitalization:
            fontProps.append(QString("caps %1").arg(properties[id].toInt()));
            break;
        case KoCharacterStyle::FontCharset:
            fontProps.append(properties[id].toString());
            break;
        case QTextFormat::FontStyleHint:
            fontProps.append(QString::number(properties[id].toInt()));
            break;
        case QTextFormat::FontKerning:
            fontProps.append(QString("kerning %1").arg(properties[id].toInt()));
            break;
        default:
            break;
        }
    }
    return fontProps.join(",");
}

void KoTextDebug::dumpDocument(const QTextDocument *doc, QTextStream &out)
{
    Q_ASSERT(doc);
    document = doc;
    out << QString("<document defaultfont=\"%1\">").arg(doc->defaultFont().toString());
    dumpFrame(document->rootFrame(), out);
    out << "</document>";
    document = nullptr;
}

QString KoTextDebug::textAttributes(const KoCharacterStyle &style)
{
    QTextCharFormat format;
    style.applyStyle(format);
    return textAttributes(format);
}

QString KoTextDebug::inlineObjectAttributes(const QTextCharFormat &textFormat)
{
    QString attrs;

    if (textFormat.objectType() == QTextFormat::UserObject + 1) {
        KoInlineTextObjectManager *inlineObjectManager = KoTextDocument(document).inlineTextObjectManager();
        KoInlineObject *inlineObject = inlineObjectManager->inlineTextObject(textFormat);
        if (KoInlineNote *note = dynamic_cast<KoInlineNote *>(inlineObject)) {
            attrs.append(QString(" id=\"%1\"").arg(note->id()));
            if (note->type() == KoInlineNote::Footnote) {
                attrs.append(" type=\"footnote\"");
            } else if (note->type() == KoInlineNote::Endnote) {
                attrs.append(" type=\"endnote\"");
            }
            attrs.append(QString(" label=\"%1\"").arg(note->label()));
        } else {
            attrs.append(" type=\"inlineobject\">");
        }
    }

    return attrs;
}

QString KoTextDebug::textAttributes(const QTextCharFormat &textFormat)
{
    QString attrs;

    QTextImageFormat imageFormat = textFormat.toImageFormat();

    if (imageFormat.isValid()) {
        attrs.append(" type=\"image\">");
        return attrs;
    }

    KoStyleManager *styleManager = document ? KoTextDocument(document).styleManager() : nullptr;
    if (styleManager && textFormat.hasProperty(KoCharacterStyle::StyleId)) {
        int id = textFormat.intProperty(KoCharacterStyle::StyleId);
        KoCharacterStyle *characterStyle = styleManager->characterStyle(id);
        attrs.append(" characterStyle=\"id:").append(QString::number(id));
        if (characterStyle)
            attrs.append(" name:").append(characterStyle->name());
        attrs.append("\"");
    }

    QMap<int, QVariant> properties = textFormat.properties();
    attrs.append(" type=\"char\"");
    QString fontProps = fontProperties(textFormat);
    if (!fontProps.isEmpty())
        attrs.append(QString(" font=\"%1\"").arg(fontProps));

    if (textFormat.isAnchor()) {
        attrs.append(QString(" anchorHref=\"%1\"").arg(textFormat.anchorHref()));
        attrs.append(QString(" anchorName=\"%1\"").arg(textFormat.anchorNames().join(',')));
    }

    const auto keys = properties.keys();
    for (int id : keys) {
        QString key, value;
        switch (id) {
        case QTextFormat::TextOutline: {
            key = "outline";
            QPen pen = qvariant_cast<QPen>(properties[id]);
            if (pen.style() == Qt::NoPen)
                value = "false";
            else
                value = pen.color().name();
            break;
        }
        case KoCharacterStyle::UnderlineStyle:
            key = "underlinestyle";
            value = QString::number(properties[id].toInt());
            break;
        case QTextFormat::TextUnderlineColor:
            key = "underlinecolor";
            value = qvariant_cast<QColor>(properties[id]).name();
            break;
        case KoCharacterStyle::UnderlineType:
            key = "underlinetype";
            value = QString::number(properties[id].toInt());
            break;
        case KoCharacterStyle::UnderlineMode:
            key = "underlinemode";
            value = QString::number(properties[id].toInt());
            break;
        case KoCharacterStyle::UnderlineWeight:
            key = "underlineweight";
            value = QString::number(properties[id].toInt());
            break;
        case KoCharacterStyle::UnderlineWidth:
            key = "underlinewidth";
            value = QString::number(properties[id].toDouble());
            break;
        case KoCharacterStyle::StrikeOutStyle:
            key = "strikeoutstyle";
            value = QString::number(properties[id].toInt());
            break;
        case KoCharacterStyle::StrikeOutColor:
            key = "strikeoutcolor";
            value = qvariant_cast<QColor>(properties[id]).name();
            break;
        case KoCharacterStyle::StrikeOutType:
            key = "strikeouttype";
            value = QString::number(properties[id].toInt());
            break;
        case KoCharacterStyle::StrikeOutMode:
            key = "strikeoutmode";
            value = QString::number(properties[id].toInt());
            break;
        case KoCharacterStyle::StrikeOutWeight:
            key = "strikeoutweight";
            value = QString::number(properties[id].toInt());
            break;
        case KoCharacterStyle::StrikeOutWidth:
            key = "strikeoutwidth";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextFormat::ForegroundBrush:
            key = "foreground";
            value = qvariant_cast<QBrush>(properties[id]).color().name(); // beware!
            break;
        case QTextFormat::BackgroundBrush:
            key = "background";
            value = qvariant_cast<QBrush>(properties[id]).color().name(); // beware!
            break;
        case QTextFormat::BlockAlignment:
            key = "align";
            value = QString::number(properties[id].toInt());
            break;
        case QTextFormat::TextIndent:
            key = "textindent";
            value = QString::number(properties[id].toInt());
            break;
        case QTextFormat::BlockIndent:
            key = "indent";
            value = QString::number(properties[id].toInt());
            break;
        case KoCharacterStyle::Country:
            key = "country";
            value = properties[id].toString();
            break;
        case KoCharacterStyle::Language:
            key = "language";
            value = properties[id].toString();
            break;
        case KoCharacterStyle::HasHyphenation:
            key = "hyphenation";
            value = properties[id].toBool() ? "true" : "false";
            break;
        case KoCharacterStyle::StrikeOutText:
            key = "strikeout-text";
            value = properties[id].toString();
            break;
        case KoCharacterStyle::FontCharset:
            key = "font-charset";
            value = properties[id].toString();
            break;
        case KoCharacterStyle::TextRotationAngle:
            key = "rotation-angle";
            value = QString::number(properties[id].toInt());
            break;
        case KoCharacterStyle::TextRotationScale:
            key = "text-rotation-scale";
            value = properties[id].toInt() == KoCharacterStyle::Fixed ? "Fixed" : "LineHeight";
            break;
        case KoCharacterStyle::TextScale:
            key = "text-scale";
            value = QString::number(properties[id].toInt());
            break;
        case KoCharacterStyle::InlineRdf:
            key = "inline-rdf";
            value = QString::number(properties[id].toInt());
            break;
        default:
            key = "unknown" + QString::number(id);
            value = QString::number(properties[id].toInt());
            break;
        }
        if (!key.isEmpty())
            attrs.append(u' ').append(key).append("=\"").append(value).append("\"");
    }
    return attrs;
}

QString KoTextDebug::paraAttributes(const KoParagraphStyle &style)
{
    QTextBlockFormat format;
    style.applyStyle(format);
    return paraAttributes(format);
}

QString KoTextDebug::paraAttributes(const QTextBlockFormat &blockFormat)
{
    QString attrs;
    KoStyleManager *styleManager = document ? KoTextDocument(document).styleManager() : nullptr;
    if (styleManager && blockFormat.hasProperty(KoParagraphStyle::StyleId)) {
        int id = blockFormat.intProperty(KoParagraphStyle::StyleId);
        KoParagraphStyle *paragraphStyle = styleManager->paragraphStyle(id);
        attrs.append(" paragraphStyle=\"id:").append(QString::number(id));
        if (paragraphStyle)
            attrs.append(" name:").append(paragraphStyle->name());
        attrs.append("\"");
    }

    QMap<int, QVariant> properties = blockFormat.properties();
    foreach (int id, properties.keys()) {
        QString key, value;
        switch (id) {
        // the following are 'todo'
        case KoParagraphStyle::PercentLineHeight:
        case KoParagraphStyle::FixedLineHeight:
        case KoParagraphStyle::MinimumLineHeight:
        case KoParagraphStyle::LineSpacing:
        case KoParagraphStyle::LineSpacingFromFont:
        case KoParagraphStyle::AlignLastLine:
        case KoParagraphStyle::WidowThreshold:
        case KoParagraphStyle::OrphanThreshold:
        case KoParagraphStyle::DropCapsTextStyle:
        case KoParagraphStyle::FollowDocBaseline:
        case KoParagraphStyle::HasLeftBorder:
        case KoParagraphStyle::HasTopBorder:
        case KoParagraphStyle::HasRightBorder:
        case KoParagraphStyle::HasBottomBorder:
        case KoParagraphStyle::BorderLineWidth:
        case KoParagraphStyle::SecondBorderLineWidth:
        case KoParagraphStyle::DistanceToSecondBorder:
        case KoParagraphStyle::LeftPadding:
        case KoParagraphStyle::TopPadding:
        case KoParagraphStyle::RightPadding:
        case KoParagraphStyle::BottomPadding:
        case KoParagraphStyle::LeftBorderColor:
        case KoParagraphStyle::TopInnerBorderWidth:
        case KoParagraphStyle::TopBorderSpacing:
        case KoParagraphStyle::TopBorderStyle:
        case KoParagraphStyle::TopBorderColor:
        case KoParagraphStyle::RightInnerBorderWidth:
        case KoParagraphStyle::RightBorderSpacing:
        case KoParagraphStyle::RightBorderStyle:
        case KoParagraphStyle::RightBorderColor:
        case KoParagraphStyle::BottomInnerBorderWidth:
        case KoParagraphStyle::BottomBorderSpacing:
        case KoParagraphStyle::BottomBorderStyle:
        case KoParagraphStyle::BottomBorderColor:
        case KoParagraphStyle::ListStyleId:
        case KoParagraphStyle::ListStartValue:
        case KoParagraphStyle::RestartListNumbering:
        case KoParagraphStyle::TextProgressionDirection:
        case KoParagraphStyle::MasterPageName:
        case KoParagraphStyle::OutlineLevel:
            break;
        case KoParagraphStyle::AutoTextIndent:
            key = "autotextindent";
            value = properties[id].toBool() ? "true" : "false";
            break;
#ifdef PARAGRAPH_BORDER_DEBUG // because it tends to get annoyingly long :)
        case KoParagraphStyle::LeftBorderWidth:
            key = "border-width-left";
            value = QString::number(properties[id].toDouble());
            break;
        case KoParagraphStyle::TopBorderWidth:
            key = "border-width-top";
            value = QString::number(properties[id].toDouble());
            break;
        case KoParagraphStyle::RightBorderWidth:
            key = "border-width-right";
            value = QString::number(properties[id].toDouble());
            break;
        case KoParagraphStyle::BottomBorderWidth:
            key = "border-width-bottom";
            value = QString::number(properties[id].toDouble());
            break;
        case KoParagraphStyle::LeftBorderStyle:
            key = "border-style-left";
            value = QString::number(properties[id].toDouble());
            break;
        case KoParagraphStyle::LeftBorderSpacing:
            key = "inner-border-spacing-left";
            value = QString::number(properties[id].toDouble());
            break;
        case KoParagraphStyle::LeftInnerBorderWidth:
            key = "inner-border-width-left";
            value = QString::number(properties[id].toDouble());
            break;
#endif
        case KoParagraphStyle::TabStopDistance:
            key = "tab-stop-distance";
            value = QString::number(properties[id].toDouble());
            break;
        case KoParagraphStyle::TabPositions:
            key = "tab-stops";
            value.clear();
            foreach (const QVariant &qvtab, qvariant_cast<QList<QVariant>>(properties[id])) {
                KoText::Tab tab = qvtab.value<KoText::Tab>();
                value.append("{");
                value.append(" pos:").append(QString::number(tab.position));
                value.append(" type:").append(QString::number(tab.type));
                if (!tab.delimiter.isNull())
                    value.append(" delim:").append(QString(tab.delimiter));
                value.append(" leadertype:").append(QString::number(tab.leaderType));
                value.append(" leaderstyle:").append(QString::number(tab.leaderStyle));
                value.append(" leaderweight:").append(QString::number(tab.leaderWeight));
                value.append(" leaderwidth:").append(QString().setNum(tab.leaderWidth));
                value.append(" leadercolor:").append(tab.leaderColor.name());
                if (!tab.leaderText.isEmpty())
                    value.append(" leadertext:").append(QString(tab.leaderText));
                value.append("}, ");
            }
            break;
        case KoParagraphStyle::DropCaps:
            key = "drop-caps";
            value = QString::number(properties[id].toBool());
            break;
        case KoParagraphStyle::DropCapsLines:
            key = "drop-caps-lines";
            value = QString::number(properties[id].toInt());
            break;
        case KoParagraphStyle::DropCapsLength:
            key = "drop-caps-length";
            value = QString::number(properties[id].toInt());
            break;
        case KoParagraphStyle::DropCapsDistance:
            key = "drop-caps-distance";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextFormat::BlockBottomMargin:
            value = QString::number(properties[id].toDouble());
            if (value != "0")
                key = "block-bottom-margin";
            break;
        case QTextFormat::BlockTopMargin:
            value = QString::number(properties[id].toDouble());
            if (value != "0")
                key = "block-top-margin";
            break;
        case QTextFormat::BlockLeftMargin:
            value = QString::number(properties[id].toDouble());
            if (value != "0")
                key = "block-left-margin";
            break;
        case QTextFormat::BlockRightMargin:
            value = QString::number(properties[id].toDouble());
            if (value != "0")
                key = "block-right-margin";
            break;
        case KoParagraphStyle::UnnumberedListItem:
            key = "unnumbered-list-item";
            value = QString::number(properties[id].toBool());
            break;
        case KoParagraphStyle::IsListHeader:
            key = "list-header";
            value = '1';
            break;
        case KoParagraphStyle::ListLevel:
            key = "list-level";
            value = QString::number(properties[id].toInt());
            break;
        default:
            break;
        }
        if (!key.isEmpty())
            attrs.append(" ").append(key).append("=\"").append(value).append("\"");
    }
    return attrs;
}

QString KoTextDebug::listAttributes(const QTextListFormat &listFormat)
{
    QString attrs;
    KoStyleManager *styleManager = document ? KoTextDocument(document).styleManager() : nullptr;
    if (styleManager && listFormat.hasProperty(KoListStyle::StyleId)) {
        int id = listFormat.intProperty(KoListStyle::StyleId);
        KoListStyle *listStyle = styleManager->listStyle(id);
        attrs.append(" listStyle=\"id:").append(QString::number(id));
        if (listStyle)
            attrs.append(" name:").append(listStyle->name());
        attrs.append("\"");
    }

    QMap<int, QVariant> properties = listFormat.properties();
    foreach (int id, properties.keys()) {
        QString key, value;
        switch (id) {
        case QTextListFormat::ListStyle:
            key = "type";
            value = QString::number(properties[id].toInt());
            break;
        case QTextListFormat::ListIndent:
            key = "indent";
            value = QString::number(properties[id].toDouble());
            break;
        case KoListStyle::ListItemPrefix:
            key = "prefix";
            value = properties[id].toString();
            break;
        case KoListStyle::ListItemSuffix:
            key = "suffix";
            value = properties[id].toString();
            break;
        case KoListStyle::StartValue:
            key = "start-value";
            value = QString::number(properties[id].toInt());
            break;
        case KoListStyle::Level:
            key = "level";
            value = QString::number(properties[id].toInt());
            break;
        case KoListStyle::DisplayLevel:
            key = "display-level";
            value = QString::number(properties[id].toInt());
            break;
        case KoListStyle::Alignment:
            key = "alignment";
            value = QString::number(properties[id].toInt());
            break;
        case KoListStyle::RelativeBulletSize:
            key = "bullet-size";
            value = QString::number(properties[id].toInt());
            break;
        case KoListStyle::BulletCharacter:
            key = "bullet-char";
            value = properties[id].toString();
            break;
        case KoListStyle::LetterSynchronization:
            key = "letter-sync";
            value = QString::number(properties[id].toInt());
            break;
        case KoListStyle::StyleId:
            key = "styleid";
            value = QString::number(properties[id].toInt());
            break;
        case KoListStyle::MinimumWidth:
            key = "minimum-width";
            value = QString::number(properties[id].toDouble());
            break;
        case KoListStyle::ListId:
            key = "list-id";
            value = QString::number(properties[id].toInt());
            break;
        case KoListStyle::IsOutline:
            key = "is-outline";
            value = QString::number(properties[id].toBool());
            break;
        case KoListStyle::Indent:
            key = "indent";
            value = QString::number(properties[id].toInt());
            break;
        case KoListStyle::MinimumDistance:
            key = "minimum-distance";
            value = QString::number(properties[id].toDouble());
            break;
        case KoListStyle::Width:
            key = "width";
            value = QString::number(properties[id].toDouble());
            break;
        case KoListStyle::Height:
            key = "height";
            value = QString::number(properties[id].toDouble());
            break;
        case KoListStyle::BulletImage:
            key = "bullet-image";
            value = QString::number((quintptr)(properties[id].value<KoImageData *>()));
            break;
        case KoListStyle::Margin:
            key = "margin-left";
            value = QString::number(properties[id].toInt());
            break;
        case KoListStyle::TextIndent:
            key = "text-indent";
            value = QString::number(properties[id].toInt());
            break;
        case KoListStyle::AlignmentMode:
            key = "label-alignment";
            value = QString(properties[id].toBool() ? "true" : "false");
            break;
        case KoListStyle::LabelFollowedBy:
            key = "label-followed-by";
            value = QString::number(properties[id].toInt());
            break;
        case KoListStyle::TabStopPosition:
            key = "tab-stop-position";
            value = QString::number(properties[id].toInt());
            break;
        default:
            break;
        }
        if (!key.isEmpty())
            attrs.append(" ").append(key).append("=\"").append(value).append("\"");
    }
    return attrs;
}

QString KoTextDebug::tableAttributes(const KoTableStyle &tableStyle)
{
    QTextTableFormat format;
    tableStyle.applyStyle(format);
    return tableAttributes(format);
}

QString KoTextDebug::tableAttributes(const QTextTableFormat &tableFormat)
{
    QString attrs;
    KoStyleManager *styleManager = document ? KoTextDocument(document).styleManager() : nullptr;
    if (styleManager) {
        int id = tableFormat.intProperty(KoTableStyle::StyleId);
        KoTableStyle *tableStyle = styleManager->tableStyle(id);
        attrs.append(" tableStyle=\"id:").append(QString::number(id));
        if (tableStyle)
            attrs.append(" name:").append(tableStyle->name());
        attrs.append("\"");
    }

    QMap<int, QVariant> properties = tableFormat.properties();
    foreach (int id, properties.keys()) {
        QString key, value;
        switch (id) {
        case QTextTableFormat::TableColumnWidthConstraints:
        case QTextFormat::BackgroundBrush:
            key = "background";
            value = qvariant_cast<QBrush>(properties[id]).color().name(); // beware!
            break;
        case QTextFormat::BlockAlignment:
            key = "alignment";
            switch (properties[id].toInt()) {
            case Qt::AlignLeft:
                value = "left";
                break;
            case Qt::AlignRight:
                value = "right";
                break;
            case Qt::AlignHCenter:
                value = "center";
                break;
            case Qt::AlignJustify:
                value = "justify";
                break;
            default:
                value.clear();
                break;
            }
            break;
        case KoTableStyle::KeepWithNext:
            key = "keep-with-next";
            value = properties[id].toBool() ? "true" : "false";
            break;
        case KoTableStyle::BreakBefore:
            key = "break-before";
            value = properties[id].toBool() ? "true" : "false";
            break;
        case KoTableStyle::BreakAfter:
            key = "break-after";
            value = properties[id].toBool() ? "true" : "false";
            break;
        case KoTableStyle::MayBreakBetweenRows:
            key = "may-break-between-rows";
            value = properties[id].toBool() ? "true" : "false";
            break;
        case KoTableStyle::MasterPageName:
            key = "master-page-name";
            value = properties[id].toString();
            break;
        case QTextTableFormat::TableColumns:
            key = "columns";
            value = QString::number(properties[id].toInt());
            break;
        case QTextTableFormat::TableCellSpacing:
            key = "cell-spacing";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextTableFormat::TableHeaderRowCount:
            key = "header-row-count";
            value = QString::number(properties[id].toInt());
            break;
        default:
            break;
        }
        if (!key.isEmpty())
            attrs.append(" ").append(key).append("=\"").append(value).append("\"");
    }
    return attrs;
}

QString KoTextDebug::frameAttributes(const QTextFrameFormat &frameFormat)
{
    QString attrs;

    QMap<int, QVariant> properties = frameFormat.properties();
    foreach (int id, properties.keys()) {
        QString key, value;
        switch (id) {
        case QTextFrameFormat::FrameBorderBrush:
            break;
        case QTextFrameFormat::FrameBorderStyle:
            key = "border-style";
            // determine border style.
            switch (properties[id].toInt()) {
            case QTextFrameFormat::BorderStyle_None:
                value = "None";
                break;
            case QTextFrameFormat::BorderStyle_Dotted:
                value = "Dotted";
                break;
            case QTextFrameFormat::BorderStyle_Dashed:
                value = "Dashed";
                break;
            case QTextFrameFormat::BorderStyle_Solid:
                value = "Solid";
                break;
            case QTextFrameFormat::BorderStyle_Double:
                value = "Double";
                break;
            case QTextFrameFormat::BorderStyle_DotDash:
                value = "DotDash";
                break;
            case QTextFrameFormat::BorderStyle_DotDotDash:
                value = "DotDotDash";
                break;
            case QTextFrameFormat::BorderStyle_Groove:
                value = "Groove";
                break;
            case QTextFrameFormat::BorderStyle_Ridge:
                value = "Ridge";
                break;
            case QTextFrameFormat::BorderStyle_Inset:
                value = "Inset";
                break;
            case QTextFrameFormat::BorderStyle_Outset:
                value = "Outset";
                break;
            default:
                value = "Unknown";
                break;
            }
            break;
        case QTextFrameFormat::FrameBorder:
            key = "border";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextFrameFormat::FrameMargin:
            key = "margin";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextFrameFormat::FramePadding:
            key = "padding";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextFrameFormat::FrameWidth:
            key = "width";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextFrameFormat::FrameHeight:
            key = "height";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextFrameFormat::FrameTopMargin:
            key = "top-margin";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextFrameFormat::FrameBottomMargin:
            key = "bottom-margin";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextFrameFormat::FrameLeftMargin:
            key = "left-margin";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextFrameFormat::FrameRightMargin:
            key = "right-margin";
            value = QString::number(properties[id].toDouble());
            break;
        default:
            break;
        }
        if (!key.isEmpty())
            attrs.append(" ").append(key).append("=\"").append(value).append("\"");
    }
    return attrs;
}

QString KoTextDebug::tableCellAttributes(const KoTableCellStyle &tableCellStyle)
{
    QTextTableCellFormat format;
    tableCellStyle.applyStyle(format);
    return tableCellAttributes(format);
}

QString KoTextDebug::tableCellAttributes(const QTextTableCellFormat &tableCellFormat)
{
    QString attrs;
    KoStyleManager *styleManager = document ? KoTextDocument(document).styleManager() : nullptr;
    if (styleManager) {
        int id = tableCellFormat.intProperty(KoTableCellStyle::StyleId);
        KoTableCellStyle *tableCellStyle = styleManager->tableCellStyle(id);
        attrs.append(" tableCellStyle=\"id:").append(QString::number(id));
        if (tableCellStyle)
            attrs.append(" name:").append(tableCellStyle->name());
        attrs.append("\"");
    }

    QMap<int, QVariant> properties = tableCellFormat.properties();
    foreach (int id, properties.keys()) {
        QString key, value;
        switch (id) {
        case QTextTableCellFormat::TableCellRowSpan:
            key = "row-span";
            value = QString::number(properties[id].toInt());
            break;
        case QTextTableCellFormat::TableCellColumnSpan:
            key = "column-span";
            value = QString::number(properties[id].toInt());
            break;
        case QTextFormat::TableCellTopPadding:
            key = "top-padding";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextFormat::TableCellBottomPadding:
            key = "bottom-padding";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextFormat::TableCellLeftPadding:
            key = "left-padding";
            value = QString::number(properties[id].toDouble());
            break;
        case QTextFormat::TableCellRightPadding:
            key = "right-padding";
            value = QString::number(properties[id].toDouble());
            break;
        case KoTableCellStyle::MasterPageName:
            key = "master-page-name";
            value = properties[id].toString();
            break;
        default:
            break;
        }
        if (!key.isEmpty())
            attrs.append(" ").append(key).append("=\"").append(value).append("\"");
    }
    return attrs;
}

void KoTextDebug::dumpFrame(const QTextFrame *frame, QTextStream &out)
{
    depth += INDENT;

    dumpIndent(depth);
    out << "<frame" << frameAttributes(frame->frameFormat()) << '>' << Qt::endl;

    QTextFrame::iterator iterator = frame->begin();

    for (; !iterator.atEnd() && !iterator.atEnd(); ++iterator) {
        QTextFrame *childFrame = iterator.currentFrame();
        QTextBlock textBlock = iterator.currentBlock();

        if (childFrame) {
            QTextTable *table = qobject_cast<QTextTable *>(childFrame);
            if (table) {
                dumpTable(table, out);
            } else {
                dumpFrame(frame, out);
            }
        } else if (textBlock.isValid()) {
            dumpBlock(textBlock, out);
        }
    }

    dumpIndent(depth);
    out << "</frame>" << Qt::endl;
    depth -= INDENT;
}

void KoTextDebug::dumpBlock(const QTextBlock &block, QTextStream &out)
{
    depth += INDENT;

    QString attrs;
    attrs.append(paraAttributes(block.blockFormat()));
    // attrs.append(" blockcharformat=\"").append(textAttributes(QTextCursor(block).blockCharFormat())).append('\"');
    attrs.append(textAttributes(QTextCursor(block).blockCharFormat()));

    QTextList *list = block.textList();
    if (list) {
        attrs.append(" list=\"item:").append(QString::number(list->itemNumber(block) + 1)).append('/').append(QString::number(list->count()));
        attrs.append('"');
        attrs.append(listAttributes(list->format()));
    }

    dumpIndent(depth);
    out << "<block" << attrs << '>' << Qt::endl;

    QTextBlock::Iterator iterator = block.begin();
    for (; !iterator.atEnd() && !iterator.atEnd(); ++iterator) {
        QTextFragment fragment = iterator.fragment();
        if (fragment.isValid()) {
            dumpFragment(fragment, out);
        }
    }
    dumpIndent(depth);
    out << "</block>" << Qt::endl;
    depth -= INDENT;
    if (block.next().isValid())
        out << ' ';
}

void KoTextDebug::dumpTable(const QTextTable *table, QTextStream &out)
{
    depth += INDENT;

    QString attrs;
    attrs.append(tableAttributes(table->format()));
    attrs.append(frameAttributes(table->frameFormat())); // include frame attributes too.

    dumpIndent(depth);
    out << "<table" << attrs << '>' << Qt::endl;

    // loop through all the cells in the table and dump the cells.
    for (int row = 0; row < table->rows(); ++row) {
        for (int column = 0; column < table->columns(); ++column) {
            dumpTableCell(table->cellAt(row, column), out);
        }
    }

    dumpIndent(depth);
    out << "</table>" << Qt::endl;
    depth -= INDENT;
}

void KoTextDebug::dumpTableCell(const QTextTableCell &cell, QTextStream &out)
{
    depth += INDENT;

    QString attrs;
    attrs.append(textAttributes(cell.format()));
    attrs.append(tableCellAttributes(cell.format().toTableCellFormat()));

    dumpIndent(depth);
    out << "<cell" << attrs << '>' << Qt::endl;

    // iterate through the cell content.
    QTextFrame::iterator cellIter = cell.begin();
    while (!cellIter.atEnd()) {
        if (cellIter.currentFrame() != nullptr) {
            // content is a frame or table.
            dumpFrame(cellIter.currentFrame(), out);
        } else {
            // content is a block.
            dumpBlock(cellIter.currentBlock(), out);
        }
        ++cellIter;
    }

    dumpIndent(depth);
    out << "</cell>\n";

    depth -= INDENT;
}

void KoTextDebug::dumpFragment(const QTextFragment &fragment, QTextStream &out)
{
    depth += INDENT;

    QTextCharFormat charFormat = fragment.charFormat();
    KoInlineObject *inlineObject = KoTextDocument(document).inlineTextObjectManager()->inlineTextObject(charFormat);
    if (inlineObject) {
        QString cf = inlineObjectAttributes(charFormat);

        dumpIndent(depth);
        out << "<fragment" << cf << ">\n";
    } else {
        QString cf = textAttributes(charFormat);

        dumpIndent(depth);
        out << "<fragment" << cf << ">\n";
        dumpIndent(depth + INDENT);
        out << '|' << fragment.text() << "|\n";
        dumpIndent(depth);
        out << "</fragment>\n";
    }

    depth -= INDENT;
}
