/*
 * This file is part of Office 2007 Filters for Calligra
 * SPDX-FileCopyrightText: 2002 Laurent Montel <lmontel@mandrakesoft.com>
 * SPDX-FileCopyrightText: 2003 Lukas Tinkl <lukas@kde.org>
 * SPDX-FileCopyrightText: 2003 David Faure <faure@kde.org>
 * SPDX-FileCopyrightText: 2009-2010 Nokia Corporation and /or its subsidiary(-ies).
 *
 * Contact: Suresh Chande suresh.chande@nokia.com
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#ifndef MSOOXML_UTILS_H
#define MSOOXML_UTILS_H

#include "komsooxml_export.h"

#include "MsooXmlDebug.h"
#include <KoFilterChain.h>
#include <KoGenStyle.h>
#include <KoXmlReader.h>
#include <QBuffer>
#include <QColor>
#include <QSize>

class KZip;
struct KoOdfWriters;
class KoCharacterStyle;
class KoXmlWriter;
class KoGenStyles;

//! Returns from the current block if the result of @a call is not equal to KoFilter::OK
#define RETURN_IF_ERROR(call)                                                                                                                                  \
    {                                                                                                                                                          \
        const KoFilter::ConversionStatus result = call;                                                                                                        \
        if (result != KoFilter::OK)                                                                                                                            \
            return result;                                                                                                                                     \
    }

//! Common utilities for handling MSOOXML formats
namespace MSOOXML
{

class MsooXmlReader;
class MsooXmlReaderContext;

namespace Utils
{

enum autoFitStatus {
    autoFitUnUsed,
    autoFitOn,
    autoFitOff
};

enum MSOOXMLFilter {
    DocxFilter,
    PptxFilter,
    XlsxFilter
};

class KOMSOOXML_EXPORT ParagraphBulletProperties
{
public:
    ParagraphBulletProperties();

    void clear();

    QString convertToListProperties(KoGenStyles &mainStyles, MSOOXMLFilter filter = XlsxFilter);

    bool isEmpty() const;

    void setBulletChar(const QString &bulletChar);

    void setPrefix(const QString &prefixChar);

    void setSuffix(const QString &suffixChar);

    void setAlign(const QString &align);

    void setNumFormat(const QString &numFormat);

    void setMargin(const qreal margin);

    void setIndent(const qreal indent);

    void setPicturePath(const QString &picturePath);

    void setBulletFont(const QString &font);

    void setBulletColor(const QString &bulletColor);

    void setStartValue(const QString &value);

    void setBulletRelativeSize(const int size);

    void setBulletSizePt(const qreal size);

    void setFollowingChar(const QString &value);

    void setTextStyle(const KoGenStyle &textStyle);

    void setStartOverride(const bool startOverride);

    QString startValue() const;

    QString bulletRelativeSize() const;

    QString bulletSizePt() const;

    QString bulletColor() const;

    QString bulletChar() const;

    QString bulletFont() const;

    QString indent() const;

    QString margin() const;

    QString followingChar() const;

    KoGenStyle textStyle() const;

    bool startOverride() const;

    void addInheritedValues(const ParagraphBulletProperties &properties);

    int m_level;

    enum ParagraphBulletType {
        BulletType,
        NumberType,
        PictureType,
        DefaultType
    };
    ParagraphBulletType m_type;

private:
    QString m_startValue;
    QString m_bulletFont;
    QString m_bulletChar;
    QString m_numFormat;
    QString m_prefix;
    QString m_suffix;
    QString m_align;
    QString m_indent;
    QString m_margin;
    QString m_picturePath;
    QString m_bulletColor;
    QString m_followingChar;
    QString m_bulletRelativeSize;
    QString m_bulletSize;

    KoGenStyle m_textStyle;

    // MSWord specific: Restart the numbering when this list style is
    // used for the 1st time.  Otherwise don't restart in case any of the
    // styles inheriting from the same abstract numbering definition was
    // already used.  Let's ignore presence of this attribute in
    // addInheritedValues.
    bool m_startOverride;
};

//! Container autodeleter. Works for QList, QHash and QMap.
//! @todo move to more generic place
template<typename T>
class ContainerDeleter
{
public:
    ContainerDeleter(T &container)
        : m_container(&container)
    {
    }
    ~ContainerDeleter()
    {
        qDeleteAll(*m_container);
        m_container->clear();
    }

private:
    T *const m_container;
};

//! Decodes boolean attribute @a value. If unspecified returns @a defaultValue.
//! @return true unless @a value is equal to "false", "off" or "0".
KOMSOOXML_EXPORT bool convertBooleanAttr(const QString &value, bool defaultValue = false);

//! Loads content types from "[Content_Types].xml"
/*! Based on information from ECMA-376, Part 1: "11.2 Package Structure".
 @return status: KoFilter::OK on success or KoFilter::WrongFormat when any unexpected and critical incompatibility occurs.
*/
//! @todo create whole class keeping the data
KOMSOOXML_EXPORT KoFilter::ConversionStatus loadContentTypes(const KoXmlDocument &contentTypesXML, QMultiHash<QByteArray, QByteArray> &contentTypes);

//! Loads content types from "docProps/app.xml"
KOMSOOXML_EXPORT KoFilter::ConversionStatus loadDocumentProperties(const KoXmlDocument &appXML, QMap<QString, QVariant> &properties);

//! @return device for file @a fileName of @a zip archive. Status @a status is written on error.
//! The device is already opened for reading and should be deleted after use.
KOMSOOXML_EXPORT QIODevice *openDeviceForFile(const KZip *zip, QString &errorMessage, const QString &fileName, KoFilter::ConversionStatus &status);

//! QXmlStreamReader-based generic loading/parsing into @a doc KoXmlDocument
KOMSOOXML_EXPORT KoFilter::ConversionStatus loadAndParse(QIODevice *io, KoXmlDocument &doc, QString &errorMessage, const QString &fileName);

//! @see KoOdfReadStore::loadAndParse(QIODevice* fileDevice, KoXmlDocument& doc, QString& errorMessage, const QString& fileName)
KOMSOOXML_EXPORT KoFilter::ConversionStatus loadAndParse(KoXmlDocument &doc, const KZip *zip, QString &errorMessage, const QString &fileName);

//! QXmlStreamReader-based loading/parsing for document.xml
KOMSOOXML_EXPORT KoFilter::ConversionStatus loadAndParseDocument(MsooXmlReader *reader,
                                                                 const KZip *zip,
                                                                 KoOdfWriters *writers,
                                                                 QString &errorMessage,
                                                                 const QString &fileName,
                                                                 MsooXmlReaderContext *context = nullptr);

/*! Copies file @a sourceName from zip archive @a zip to @a outputStore store
 under @a destinationName name. If @a size is not 0, *size is set to size of the image
 @return KoFilter::OK on success.
 On failure @a errorMessage is set. */
KoFilter::ConversionStatus
copyFile(const KZip *zip, QString &errorMessage, const QString &sourceName, KoStore *outputStore, const QString &destinationName, bool oleType = false);

/*! Creates a file */
KoFilter::ConversionStatus createImage(QString &errorMessage, const QImage &source, KoStore *outputStore, const QString &destinationName);

/*! @return size of image file @a sourceName read from zip archive @a zip.
 Size of the image is returned in @a size.
 @return KoFilter::OK on success.
 On failure @a errorMessage is set. */
KoFilter::ConversionStatus imageSize(const KZip *zip, QString &errorMessage, const QString &sourceName, QSize *size);

//! Loads a thumbnail.
/*! @return conversion status
    @todo Thumbnails are apparently used only by PowerPoint or templates for now.
          Implement it, for now this feature is not needed for docx. */
KOMSOOXML_EXPORT KoFilter::ConversionStatus loadThumbnail(QImage &thumbnail, KZip *zip);

// -- conversions ---

//! Handles ST_Lang value @a value (Language Reference) (SharedML, 22.9.2.6)
/*! The value specifies that its contents contains a language identifier as defined by RFC 4646/BCP 47.
    Sets up @a language and @a country based on @a value that is of format {language}-{country}
    @return true on success. */
KOMSOOXML_EXPORT bool ST_Lang_to_languageAndCountry(const QString &value, QString &language, QString &country);

//! @return QColor value for ST_HexColorRGB (Hexadecimal Color Value) (SharedML, 22.9.2.5)
//!         or invalid QColor if @a color is not in the expected format.
//! @par val color value in RRGGBB hexadecimal format
inline QColor ST_HexColorRGB_to_QColor(const QString &color)
{
    if (color.length() != 6)
        return QColor();
    bool ok;
    const uint rgb = color.toUInt(&ok, 16);
    return ok ? QColor(QRgb(rgb)) : QColor(); // alpha ignored
    //    return QColor(QRgb(0xff000000 | color.toInt(0, 16)));
}

//! @return QBrush value for ST_HighlightColor
//! The brush is built out of solid color.
//! If colorName is not supported by the standard, QBrush() is returned.
//! @par colorName named text highlight color like "black", "blue" (17.18.40)
KOMSOOXML_EXPORT QBrush ST_HighlightColor_to_QColor(const QString &colorName);

//! Converts value for 22.9.2.9 ST_Percentage (Percentage Value with Sign) from string
//! Sets @arg ok to true on success.
KOMSOOXML_EXPORT qreal ST_Percentage_to_double(const QString &val, bool &ok);

//! Converts value for 22.9.2.9 ST_Percentage (Percentage Value with Sign) from string
//! If "%" suffix is not present (MSOOXML violation of OOXML), the format is expected to be int({ST_Percentage}*1000).
//! Sets @arg ok to true on success.
KOMSOOXML_EXPORT qreal ST_Percentage_withMsooxmlFix_to_double(const QString &val, bool &ok);

struct KOMSOOXML_EXPORT DoubleModifier {
    DoubleModifier(qreal v)
        : value(v)
        , valid(true)
    {
    }
    DoubleModifier()
        : value(0.0)
        , valid(false)
    {
    }
    qreal value;
    bool valid;
};

KOMSOOXML_EXPORT QColor colorForLuminance(const QColor &color, const DoubleModifier &modulation, const DoubleModifier &offset);

KOMSOOXML_EXPORT void modifyColor(QColor &color, qreal tint, qreal shade, qreal satMod);

//! Converts shape types from ECMA-376 to ODF.
/*! @return "Common Presentation Shape Attribute" value (ODF 1.1., 9.6.1)
            for presentation shape converted from ECMA-376 19.7.10
            ST_PlaceholderType (Placeholder ID) value, p. 2987.
    @param ecmaType ECMA-376 shape type
    The conversion is useful e.g. for presentation:class attribute of draw:frame
    out of ECMA-376's ph\@type attribute.
    By default (and for empty argument), "outline" is returned.
*/
//! @todo or "object"?  ST_PlaceholderType docs day the default is "obj".
//! CASE \#P500
KOMSOOXML_EXPORT QString ST_PlaceholderType_to_ODF(const QString &ecmaType);

//! Sets up @p textStyleProperties with underline style matching MSOOXML name @p msooxmlName.
//! Based on 17.18.99 ST_Underline (Underline Patterns), WML ECMA-376 p.1681
//! and on 20.1.10.82 ST_TextUnderlineType (Text Underline Types), DrawingML ECMA-376 p.3450 (merged)
KOMSOOXML_EXPORT void setupUnderLineStyle(const QString &msooxmlName, KoCharacterStyle *textStyleProperties);

//! @return the symbolic name of column @p column (counted from 0)
//! This is similar to the notation of spreadsheet's column, e.g. 0th column is "A", 1st is "B", 26th is "AA".
KOMSOOXML_EXPORT QString columnName(uint column);

//! Splits @a pathAndFile into path and file parts. Path does not end with '/'.
KOMSOOXML_EXPORT void splitPathAndFile(const QString &pathAndFile, QString *path, QString *file);

//! Returns calculated angle and xDiff, yDiff, caller has to apply these to style
KOMSOOXML_EXPORT void rotateString(const qreal rotation, const qreal width, const qreal height, qreal &angle, qreal &xDiff, qreal &yDiff);

//! Marker related utils
KOMSOOXML_EXPORT QString defineMarkerStyle(KoGenStyles &mainStyles, const QString &markerType);

KOMSOOXML_EXPORT qreal defineMarkerWidth(const QString &markerWidth, const qreal lineWidth);

//! A helper allowing to buffer xml streams and writing them back later
/*! This class is useful when information that has to be written in advance is
    based on XML elements parsed later.  In such case the information cannot be
    saved in one pass.  Example of this is paragraphs style name: is should be
    written to style:name attribute but relevant XML elements (that we use for
    building the style) are appearing later.  So we first output created XML to
    a buffer, then save the parent element with the style name and use
    KoXmlWriter::addCompleteElement() to redirect the buffer contents as a
    subelement.

     Example use:
     @code
     KoXmlWriter *body = ...;
     XmlWriteBuffer buf;
     body = buf.setWriter(body);
     // ...
     // buf.originalWriter() can be used here ...
     // ...
     // Use the new buffered body writer here, e.g.:
     body->startElement("text:span", false);
     body->addAttribute("text:style-name", currentTextStyleName);
     body->addTextSpan(text);
     body->endElement();

     // We are done with the buffered body writer, now release it and restore
     // the original body writer.  This inserts all the XML buffered by buf
     // into the original body writer (using KoXmlWriter::addCompleteElement()).

     body = buf.releaseWriter();
     @endcode */
class KOMSOOXML_EXPORT XmlWriteBuffer
{
public:
    //! Constructor; no writer is set initially.
    XmlWriteBuffer();

    //! Destructor, releases writer if there is any set.
    ~XmlWriteBuffer();

    //! Assigns writer @a writer to this buffer.
    /*! From now any output directed to @a writer is written to a buffer instead.
     Use releaseWriter() to write the changes back through the original writer.
     @return the newly created writer, which usually should be assigned
             to the variable passed as @a writer. */
    KoXmlWriter *setWriter(KoXmlWriter *writer);

    //! Releases the original writer set before using setWriter(KoXmlWriter*&).
    /*! This inserts all the XML buffered by buffer into the original body writer passed in setWriter()
     (internally using KoXmlWriter::addCompleteElement()).
     @return the original writer set in setWriter();
             this writer usually should be assigned back to the variable
             altered by the recent use of setWriter(). */
    KoXmlWriter *releaseWriter();

    //! Releases the original writer set before using setWriter(KoXmlWriter*&).
    /*! This inserts all the XML buffered by buffer into @a bkpXmlSnippet
     @return the original writer set in setWriter();
             this writer usually should be assigned back to the variable
             altered by the recent use of setWriter(). */
    KoXmlWriter *releaseWriter(QString &bkpXmlSnippet);

    //! @return the original writer set in setWriter(). Does not change the state of the buffer.
    /*! Use this method when you need to access the remembered writer without releasing it. */
    KoXmlWriter *originalWriter() const
    {
        return m_origWriter;
    }

    //! Clears this buffer without performing any output to the writer.
    void clear();

    //! Returns true if the buffer is empty; otherwise returns false.
    bool isEmpty() const
    {
        return m_buffer.buffer().isEmpty();
    }

private:
    //! Internal, used in releaseWriter() and the destructor; Does not assert when there's nothing to release.
    KoXmlWriter *releaseWriterInternal();

    QBuffer m_buffer;
    KoXmlWriter *m_origWriter;
    KoXmlWriter *m_newWriter;
};

//! The purpose of this class is to make sure the this->body variable is proper
//! set back to what it was before even if one of the TRY_READ calls lead to
//! us skipping out of this method. In that case we need to make sure to restore
//! the body variable else things may later crash.
//!
//! FIXME refactor the XmlWriteBuffer and merge this hack in so we don't
//! need to work-around at any place where it's used.
template<typename T>
class AutoRestore
{
public:
    explicit AutoRestore(T **originalPtr)
        : m_originalPtr(originalPtr)
        , m_prevValue(*originalPtr)
    {
    }
    ~AutoRestore()
    {
        if (m_originalPtr) {
            *m_originalPtr = m_prevValue;
        }
    }

private:
    T **m_originalPtr;
    T *m_prevValue;
};

} // Utils namespace

} // MSOOXML namespace

#endif /* MSOOXML_UTILS_H */
