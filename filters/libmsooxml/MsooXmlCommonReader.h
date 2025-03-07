/*
 * This file is part of Office 2007 Filters for Calligra
 *
 * SPDX-FileCopyrightText: 2009 Nokia Corporation and /or its subsidiary(-ies).
 *
 * Contact: Suresh Chande suresh.chande@nokia.com
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#ifndef MSOOXMLCOMMONREADER_H
#define MSOOXMLCOMMONREADER_H

#include <KoGenStyle.h>
#include <styles/KoCharacterStyle.h>
#include <styles/KoListLevelProperties.h>

#include "MsooXmlReader.h"
#include "MsooXmlUtils.h"

namespace MSOOXML
{

//! A class reading generic parts of MSOOXML main document's markup.
class KOMSOOXML_EXPORT MsooXmlCommonReader : public MsooXmlReader
{
protected:
    explicit MsooXmlCommonReader(KoOdfWriters *writers);

    MsooXmlCommonReader(QIODevice *io, KoOdfWriters *writers);

    ~MsooXmlCommonReader() override;

    // -- for read_p()
    enum read_p_arg {
        read_p_Skip
    };
    Q_DECLARE_FLAGS(read_p_args, read_p_arg)
    read_p_args m_read_p_args;

    KoGenStyle *m_currentDrawStyle; //! used by all classes that need a graphics style.
    QList<KoGenStyle *> m_drawStyleStack;
    KoGenStyle m_currentGradientStyle;
    void pushCurrentDrawStyle(KoGenStyle *newStyle);
    void popCurrentDrawStyle();

    //! Used for creating style in w:pPr (style:style/\@style:name attr)
    KoGenStyle m_currentParagraphStyle;

    void setupParagraphStyle();

    KoGenStyle m_currentTextStyle;
    KoCharacterStyle *m_currentTextStyleProperties;

    KoGenStyle m_currentListStyle;
    MSOOXML::Utils::ParagraphBulletProperties m_currentBulletProperties;
    // Properties for potentially all 9 lvls which are possible
    QMap<int, MSOOXML::Utils::ParagraphBulletProperties> m_currentCombinedBulletProperties;
    QMap<int, KoGenStyle> m_currentCombinedParagraphStyles;
    QMap<int, KoGenStyle> m_currentCombinedTextStyles;

    //! Style (from styles.xml) to apply to the current paragraph or similar element, set by read_pStyle()
    QString m_currentStyleName;

    bool isDefaultTocStyle(const QString &name) const;

    //! Adds reference to a file in the ODF document to manifest.xml
    //! The media-type attribute is based on the extension of @p path.
    void addManifestEntryForFile(const QString &path);

    //! Adds manifest entry for "Pictures/"
    void addManifestEntryForPicturesDir();

    //! value of recent pPr_lvl attribute; set by read_pPr()
    uint m_pPr_lvl;

    bool m_paragraphStyleNameWritten; //!< set by setupParagraphStyle()

    bool m_addManifestEntryForPicturesDirExecuted;

    bool m_moveToStylesXml;

    QSize m_imageSize;

    QSet<QString> m_copiedFiles; //!< collects source names to avoid multiple copying of media files

    //    //! Used for creating style names (style:style/@style:name attr)
    //    //! To achieve this, in XSLT it generate-id(.) for w:p is used.
    //    ////! Starts with 1. Updated in read_p();
    //    uint m_currentParagraphStyleNumber;
    //    QString currentParagraphStyleName() const;
private:
    Q_DISABLE_COPY(MsooXmlCommonReader)

    void init();
};

}

#endif
