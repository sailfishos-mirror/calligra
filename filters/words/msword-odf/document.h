/* This file is part of the Calligra project
   SPDX-FileCopyrightText: 2002 Werner Trobin <trobin@kde.org>
   SPDX-FileCopyrightText: 2002 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2008 Benjamin Cail <cricketc@gmail.com>
   SPDX-FileCopyrightText: 2009 Inge Wallin <inge@lysator.liu.se>
   SPDX-FileCopyrightText: 2010, 2011 Matus Uzak <matus.uzak@ixonos.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the Library GNU General Public
   version 2 of the License, or (at your option) version 3 or,
   at the discretion of KDE e.V (which shall act as a proxy as in
   section 14 of the GPLv3), any later version..

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "generated/leinputstream.h"
#include "pole.h"
#include "tablehandler.h"

#include <wv2/src/functor.h>
#include <wv2/src/functordata.h>
#include <wv2/src/handlers.h>

#include <QBuffer>
#include <QDomDocument>
#include <QDomElement>
#include <QList>
#include <QObject>
#include <QRectF>
#include <QStack>
#include <QString>
#include <QStringList>
#include <queue>
#include <string>

#include <KoGenStyles.h>
#include <KoXmlWriter.h>

class KoStore;

namespace wvWare
{
class Parser;
namespace Word97
{
struct BRC;
}
}
class MSWordOdfImport;
/* class KoFilterChain; */
class WordsReplacementHandler;
class WordsTableHandler;
class WordsTextHandler;
class WordsGraphicsHandler;

class Document : public QObject, public wvWare::SubDocumentHandler
{
    Q_OBJECT
public:
    Document(const std::string &fileName,
             MSWordOdfImport *filter,
             /*              KoFilterChain* chain, */
             KoXmlWriter *bodyWriter,
             KoXmlWriter *metaWriter,
             KoXmlWriter *manifestWriter,
             KoStore *store,
             KoGenStyles *mainStyles,
             LEInputStream &wordDocument,
             POLE::Stream &table,
             LEInputStream *data,
             LEInputStream *si);
    ~Document() override;

    void setProgress(const int percent) override;

    void bodyStart() override;
    void bodyEnd() override;

    void headerStart(wvWare::HeaderData::Type type) override;
    void headerEnd() override;
    void headersMask(QList<bool> mask) override;

    void footnoteStart() override;
    void footnoteEnd() override;

    void annotationStart() override;
    void annotationEnd() override;

    /**
     * Call the wv2 parser and check if our handlers are fine after the parsing
     * process is finished.
     * @return 0 - Ok, 1 - parser error, 2 - handler error
     */
    quint8 parse();

    void processSubDocQueue();

    void finishDocument();

    typedef const wvWare::FunctorBase *FunctorPtr;
    struct SubDocument {
        SubDocument(FunctorPtr ptr, int d, const QString &n, const QString &extra)
            : functorPtr(ptr)
            , data(d)
            , name(n)
            , extraName(extra)
        {
        }
        ~SubDocument() = default;
        FunctorPtr functorPtr;
        int data;
        QString name;
        QString extraName;
    };

    // Provide access to private attributes for other handlers
    QString masterPageName(void) const
    {
        return m_masterPageName_list.size() ? m_masterPageName_list.first() : m_lastMasterPageName;
    }
    void set_writeMasterPageName(bool val)
    {
        m_writeMasterPageName = val;
    }
    bool writeMasterPageName(void) const
    {
        return m_writeMasterPageName;
    }
    bool omittMasterPage(void) const
    {
        return m_omittMasterPage;
    }
    bool useLastMasterPage(void) const
    {
        return m_useLastMasterPage;
    }
    bool writingHeader(void) const
    {
        return m_writingHeader;
    }
    KoXmlWriter *headerWriter(void) const
    {
        return m_headerWriter;
    }
    WordsTextHandler *textHandler(void) const
    {
        return m_textHandler;
    }
    bool hasParser(void) const
    {
        return m_parser != nullptr;
    }
    bool bodyFound(void) const
    {
        return m_bodyFound;
    }

    /**
     * Add a color item to the background-color stack.
     * @param color in the format "#RRGGBB"
     */
    void addBgColor(const QString &val)
    {
        m_bgColors.push(val);
    }

    /**
     * Remove the last item from the background-color stack.
     */
    void rmBgColor(void)
    {
        m_bgColors.pop();
    }

    /**
     * Update the last item of the background-color stack.
     * @param color in the format "#RRGGBB"
     */
    void updateBgColor(const QString &val)
    {
        m_bgColors.pop();
        m_bgColors.push(val);
    }

    /**
     * @return the current background-color in the format "#RRGGBB".
     */
    QString currentBgColor(void)
    {
        return m_bgColors.isEmpty() ? QString() : m_bgColors.top();
    }

    /**
     * @return the list of names of TOC related styles.
     */
    QList<QString> tocStyleNames(void)
    {
        return m_tocStyleNames;
    }

    /**
     * Checks if the header/footer content of the current section differs from
     * the previous section's header/footer.
     *
     * @return TRUE - different content; FALSE - no difference or the Header
     * document doesn't exist.
     */
    bool headersChanged(void) const;

    // Provide access to POLE/LEInput streams to other handlers.
    POLE::Stream &poleTableStream(void) const
    {
        return m_tblstm_pole;
    }
    LEInputStream &wdocumentStream(void) const
    {
        return m_wdstm;
    }
    LEInputStream *tableStream(void) const
    {
        return m_tblstm;
    }
    LEInputStream *dataStream(void) const
    {
        return m_datastm;
    }

    // get the style name used for line numbers
    QString lineNumbersStyleName() const
    {
        return m_lineNumbersStyleName;
    }

public Q_SLOTS:
    // Connected to the WordsTextHandler only when parsing the body
    void slotSectionFound(wvWare::SharedPtr<const wvWare::Word97::SEP>);

    void slotSectionEnd(wvWare::SharedPtr<const wvWare::Word97::SEP>);

    // Add to our parsing queue, for headers, footers, footnotes, annotations, text boxes etc.
    // Note that a header functor will parse ALL the header/footers (of the section)
    void slotSubDocFound(const wvWare::FunctorBase *functor, int data);

    void slotFootnoteFound(const wvWare::FunctorBase *functor, int data);

    void slotAnnotationFound(const wvWare::FunctorBase *functor, int data);

    void slotHeadersFound(const wvWare::FunctorBase *functor, int data);

    void slotTableFound(Words::Table *table);

    void slotInlineObjectFound(const wvWare::PictureData &data, KoXmlWriter *writer);

    void slotFloatingObjectFound(unsigned int globalCP, KoXmlWriter *writer);

    void slotTextBoxFound(unsigned int index, bool stylesxml);

    // Similar to footnoteStart/footnoteEnd but cells, connected to WordsTableHandler
    /*     void slotTableCellStart(int row, int column, int rowSize, int columnSize, const QRectF& cellRect, */
    /*                             const QString& tableName, */
    /*                             const wvWare::Word97::BRC& brcTop, const wvWare::Word97::BRC& brcBottom, */
    /*                             const wvWare::Word97::BRC& brcLeft, const wvWare::Word97::BRC& brcRight, */
    /*                             const wvWare::Word97::SHD& shd ); */
    /*     void slotTableCellEnd(); */

private:
    void processStyles();
    void processAssociatedStrings();

    void setPageLayoutStyle(KoGenStyle *pageLayoutStyle, wvWare::SharedPtr<const wvWare::Word97::SEP> sep, bool firstPage);

    // Handlers for different data types in the document.
    WordsTextHandler *m_textHandler;
    WordsTableHandler *m_tableHandler;
    WordsReplacementHandler *m_replacementHandler;
    WordsGraphicsHandler *m_graphicsHandler;

    MSWordOdfImport *m_filter;
    /*     KoFilterChain* m_chain; */

    wvWare::SharedPtr<wvWare::Parser> m_parser;

    std::queue<SubDocument> m_subdocQueue;
    /*     std::queue<Words::Table> m_tableQueue; */

    bool m_bodyFound;

    int m_footNoteNumber; // number of footnote _framesets_ written out
    int m_endNoteNumber; // number of endnote _framesets_ written out

    // Helpers to generate the various parts of an ODF file.
    KoXmlWriter *m_bodyWriter; // for writing to the body of content.xml
    KoGenStyles *m_mainStyles; // for collecting styles
    KoXmlWriter *m_metaWriter; // for writing to meta.xml
    KoXmlWriter *m_headerWriter; // for header/footer writing in styles.xml

    int m_headerCount; // to have a unique name for element we're putting into an masterPageStyle
    bool m_writingHeader; // flag for headers/footers, where we write the actual text to styles.xml
    bool m_evenOpen; // processing an even header/footer
    bool m_firstOpen; // processing a first page header/footer
    QBuffer *m_buffer; // for odd and first page header/footer tags
    QBuffer *m_bufferEven; // for even header/footer tags

    KoGenStyle m_pageLayoutStyle_last; // the latest page-layout style
    QList<KoGenStyle *> m_masterPageStyle_list; // master-page styles
    QList<KoGenStyle *> m_pageLayoutStyle_list; // page-layout styles
    QStringList m_masterPageName_list; // master-page names

    QList<bool> m_headersMask; // mask informing of section's empty/nonempty header/footer stories
    QList<bool> m_hasHeader_list; // does master-page/page-layout require a header element
    QList<bool> m_hasFooter_list; // does master-page/page-layout require a footer element

    bool m_writeMasterPageName; // whether to write the master-page name into a paragraph/table
    bool m_omittMasterPage; // whether master-page style for current section has been omitted
    bool m_useLastMasterPage; // whether to use the last define master-page style for current section

    QString m_lineNumbersStyleName;
    QString m_lastMasterPageName;

    // pointers to streams
    LEInputStream &m_wdstm;
    LEInputStream *m_tblstm;
    LEInputStream *m_datastm;
    LEInputStream *m_sistm;
    POLE::Stream &m_tblstm_pole;

    // A stack for background-colors, which represets a background color context
    // for automatic colors.
    QStack<QString> m_bgColors;

    // A list storing names of TOC related styles required in TextHandler to
    // process the TOC field.
    QList<QString> m_tocStyleNames;
};

#endif // DOCUMENT_H
