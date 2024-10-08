/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2008 Girish Ramakrishnan <girish@forwardbias.in>
 * SPDX-FileCopyrightText: 2009 Elvis Stansvik <elvstone@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KOTEXTDEBUG_H
#define KOTEXTDEBUG_H

#include "kotext_export.h"

class QTextDocument;
class QTextFrame;
class QTextBlock;
class QTextTable;
class QTextTableCell;
class QTextFragment;
class QTextCharFormat;
class QTextListFormat;
class QTextTableFormat;
class QTextTableCellFormat;
class QTextFrameFormat;
class QTextBlockFormat;
class QTextStream;
class KoParagraphStyle;
class KoCharacterStyle;
class KoTableStyle;
class KoTableCellStyle;

#include <QMap>
#include <QVariant>

/**
 * @brief KoText debugging class.
 *
 * This class provides a set of public static functions for debugging the structure of
 * QTextDocument text documents. The functions will dump the structure of the document
 * along with any formats in a human-friendly pseudo-XML format.
 *
 * To most top level function is dumpDocument(), which can be used to dump an entire
 * document. In addition to that, there's a set of functions for dumping certain
 * parts of a document , such as dumpFrame(), dumpBlock() et.c.
 *
 * For example, the following code
 *
 * @code
 * QTextDocument doc;
 * QTextCursor cursor(&doc);
 * cursor.insertText("Hello!\n");
 * cursor.insertHtml("<b>World!</b>");
 *
 * QTextStream out(stdout);
 * KoTextDebug::dumpDocument(&doc, out);
 * @endcode
 *
 * will result in this output:
 *
 * @verbatim
 * <pre>
 * <document defaultfont="Sans Serif,9,-1,5,50,0,0,0,0,0">
 *   <frame margin="4" top-margin="4" bottom-margin="4" left-margin="4" right-margin="4" border-style="Outset">
 *     <block type="char">
 *       <fragment type="char">
 *         |Hello!|
 *       </fragment>
 *     </block>
 *
 *     <block type="char">
 *       <fragment type="char" font="weight 75">
 *         |World!|
 *       </fragment>
 *     </block>
 *   </frame>
 * </document>
 * </pre>
 * @endverbatim
 *
 * @sa dumpDocument(), dumpFrame(), dumpBlock()
 */
class KOTEXT_EXPORT KoTextDebug
{
public:
    /**
     * Dump the structure of the specified document.
     *
     * @param document a pointer to the document that should be dumped.
     * @param out output stream to dump to.
     */
    static void dumpDocument(const QTextDocument *document, QTextStream &out);

    /**
     * Dump the structure of the specified frame.
     *
     * @sa frameAttributes()
     *
     * @param frame a pointer to the frame that should be dumped.
     * @param out output stream to dump to.
     */
    static void dumpFrame(const QTextFrame *frame, QTextStream &out);

    /**
     * Dump the structure of the specified block.
     *
     * @param block the block that should be dumped.
     * @param out output stream to dump to.
     */
    static void dumpBlock(const QTextBlock &block, QTextStream &out);

    /**
     * Dump the structure of the specified table.
     *
     * @sa tableAttributes()
     *
     * @param table a pointer to the table that should be dumped.
     * @param out output stream to dump to.
     */
    static void dumpTable(const QTextTable *table, QTextStream &out);

    /**
     * Dump the structure of the specified table cell.
     *
     * @sa tableCellAttributes()
     *
     * @param cell the cell that should be dumped.
     * @param out output stream to dump to.
     */
    static void dumpTableCell(const QTextTableCell &cell, QTextStream &out);

    /**
     * Dump the contents of the specified text fragment.
     *
     * @note { The fragment content will be enclosed in '|' characters. }
     *
     * @param fragment the fragment which's content should be dumped.
     * @param out output stream to dump to.
     */
    static void dumpFragment(const QTextFragment &fragment, QTextStream &out);

    /**
     * Get the properties of the given text character format.
     *
     * The returned string will be formatted in XML-like attribute list format:
     *
     * <pre>"key=value key2=value2 ..."</pre>
     *
     * @param format the text character format from which properties should be fetched.
     * @return the formatted attribute string.
     */
    static QString textAttributes(const QTextCharFormat &format);

    /**
     * Get the properties of the given character style.
     *
     * The returned string will be formatted in XML-like attribute list format:
     *
     * <pre>"key=value key2=value2 ..."</pre>
     *
     * @param style the character style from which properties should be fetched.
     * @return the formatted attribute string.
     */
    static QString textAttributes(const KoCharacterStyle &style);

    /**
     * Get the properties of the given text block format.
     *
     * The returned string will be formatted in XML-like attribute list format:
     *
     * <pre>"key=value key2=value2 ..."</pre>
     *
     * @param format the text block format from which properties should be fetched.
     * @return the formatted attribute string.
     */
    static QString paraAttributes(const QTextBlockFormat &format);

    /**
     * Get the properties of the given paragraph style.
     *
     * The returned string will be formatted in XML-like attribute list format:
     *
     * <pre>"key=value key2=value2 ..."</pre>
     *
     * @param style the paragraph style from which properties should be fetched.
     * @return the formatted attribute string.
     */
    static QString paraAttributes(const KoParagraphStyle &style);

    /**
     * Get the properties of the given list format.
     *
     * The returned string will be formatted in XML-like attribute list format:
     *
     * <pre>"key=value key2=value2 ..."</pre>
     *
     * @param format the list format from which properties should be fetched.
     * @return the formatted attribute string.
     */
    static QString listAttributes(const QTextListFormat &format);

    /**
     * Get the properties of the given table style.
     *
     * The returned string will be formatted in XML-like attribute list format:
     *
     * <pre>"key=value key2=value2 ..."</pre>
     *
     * @param tableStyle the table style from which properties should be fetched.
     * @return the formatted attribute string.
     */
    static QString tableAttributes(const KoTableStyle &tableStyle);

    /**
     * Get the properties of the given table format.
     *
     * The returned string will be formatted in XML-like attribute list format:
     *
     * <pre>"key=value key2=value2 ..."</pre>
     *
     * @param tableFormat the table format from which properties should be fetched.
     * @return the formatted attribute string.
     */
    static QString tableAttributes(const QTextTableFormat &tableFormat);

    /**
     * Get the properties of the given table cell style.
     *
     * The returned string will be formatted in XML-like attribute list format:
     *
     * <pre>"key=value key2=value2 ..."</pre>
     *
     * @param tableCellStyle the table cell style from which properties should be fetched.
     * @return the formatted attribute string.
     */
    static QString tableCellAttributes(const KoTableCellStyle &tableCellStyle);

    /**
     * Get the properties of the given table cell format.
     *
     * The returned string will be formatted in XML-like attribute list format:
     *
     * <pre>"key=value key2=value2 ..."</pre>
     *
     * @param tableCellFormat the table cell format from which properties should be fetched.
     * @return the formatted attribute string.
     */
    static QString tableCellAttributes(const QTextTableCellFormat &tableCellFormat);

    /**
     * Get the properties of the given text frame format.
     *
     * The returned string will be formatted in XML-like attribute list format:
     *
     * <pre>"key=value key2=value2 ..."</pre>
     *
     * @param frameFormat the text frame format from which properties should be fetched.
     * @return the formatted attribute string.
     */
    static QString frameAttributes(const QTextFrameFormat &frameFormat);

    /**
     * Get the inline object properties of the object with the given text character format.
     *
     * The returned string will be formatted in XML-like attribute list format:
     *
     * <pre>"key=value key2=value2 ..."</pre>
     *
     * @param textFormat the character format of the object from which properties should be fetched.
     * @return the formatted attribute string.
     */
    static QString inlineObjectAttributes(const QTextCharFormat &textFormat);

private:
    KoTextDebug() = delete;
    KoTextDebug(const KoTextDebug &) = delete;
    KoTextDebug operator=(const KoTextDebug &) = delete;

    static const QTextDocument *document; /**< Pointer to the debugged document. */
    static int depth; /**< Current indentation depth. */
    static const int INDENT; /**< Indentation value. */
};

#endif /* KOTEXTDEBUG_H */
