/*
 * This file is part of Office 2007 Filters for Calligra
 *
 * SPDX-FileCopyrightText: 2009-2010 Nokia Corporation and /or its subsidiary(-ies).
 *
 * Contact: Suresh Chande suresh.chande@nokia.com
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#ifndef PPTXXMLDOCUMENTREADER_H
#define PPTXXMLDOCUMENTREADER_H

#include "PptxXmlSlideReader.h"
#include <MsooXmlThemesReader.h>

class PptxImport;
class PptxSlideProperties;

namespace MSOOXML
{
class MsooXmlRelationships;
}

class KoTable;

class PptxXmlDocumentReaderContext : public MSOOXML::MsooXmlReaderContext
{
public:
    PptxXmlDocumentReaderContext(PptxImport &_import, const QString &_path, const QString &_file, MSOOXML::MsooXmlRelationships &_relationships);
    PptxImport *import;
    const QString path;
    const QString file;
    MSOOXML::MsooXmlRelationships *relationships;
    bool firstReadRound;
    MSOOXML::DrawingMLTheme *themes;
    unsigned numberOfItems;
};

//! A class reading MSOOXML PPTX markup - presentation.xml part.
class PptxXmlDocumentReader : public MSOOXML::MsooXmlCommonReader
{
public:
    explicit PptxXmlDocumentReader(KoOdfWriters *writers);

    ~PptxXmlDocumentReader() override;

    //! Reads/parses the file of format document.xml.
    //! The output goes mainly to KoXmlWriter* KoOdfWriters::body
    KoFilter::ConversionStatus read(MSOOXML::MsooXmlReaderContext *context = nullptr) override;

protected:
    KoFilter::ConversionStatus readInternal();
    KoFilter::ConversionStatus read_presentation();
    KoFilter::ConversionStatus read_sldMasterIdLst();
    KoFilter::ConversionStatus read_sldMasterId();
    KoFilter::ConversionStatus read_notesMasterIdLst();
    KoFilter::ConversionStatus read_notesMasterId();
    KoFilter::ConversionStatus read_sldIdLst();
    KoFilter::ConversionStatus read_sldId();
    KoFilter::ConversionStatus read_sldSz();
    KoFilter::ConversionStatus read_notesSz();
    KoFilter::ConversionStatus read_defaultTextStyle();

    // Locates slide layout information for given slide. Caches the result.
    PptxSlideProperties *slideLayoutProperties(const QString &slidePath, const QString &slideFile);

    KoOdfWriters *m_writers;
    PptxXmlDocumentReaderContext *m_context;

    // Default pptx styles
    QVector<KoGenStyle> defaultParagraphStyles;
    QVector<KoGenStyle> defaultTextStyles;
    QVector<MSOOXML::Utils::ParagraphBulletProperties> defaultListStyles;
    QVector<QString> defaultBulletColors;
    QVector<QString> defaultTextColors;
    QVector<QString> defaultLatinFonts;

    enum ColorReadingState {
        defRPrState,
        buClrState
    };
    ColorReadingState m_colorState;

private:
    void init();

    class Private;
    Private *const d;
#include <MsooXmlCommonReaderDrawingMLMethods.h>
#include <MsooXmlCommonReaderMethods.h>
#include <MsooXmlDrawingReaderTableMethods.h>
};

#endif // PPTXXMLDOCUMENTREADER_H
