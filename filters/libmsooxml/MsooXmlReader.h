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

#ifndef MSOOXMLREADER_H
#define MSOOXMLREADER_H

#include "komsooxml_export.h"

#include <QStack>
#include <QUrl>
#include <QXmlStreamReader>

#include "MsooXmlDebug.h"
#include <KoFilter.h>
#include <KoOdfExporter.h>
#include <KoXmlReader.h>

namespace MSOOXML
{

class MsooXmlRelationships;

//! Context for MsooXmlReader::read()
class KOMSOOXML_EXPORT MsooXmlReaderContext
{
protected:
    MsooXmlReaderContext(MsooXmlRelationships *_relationships = nullptr);

public:
    virtual ~MsooXmlReaderContext();
    MSOOXML::MsooXmlRelationships *relationships;
    QMap<QString, QString> colorMap;

    // element:graphic - A frame might contain a graphic object that was
    // generated by an external source and needs a container in which to be
    // displayed.  A graphic object might represent a group of objects.
    bool graphicObjectIsGroup;

private:
    Q_DISABLE_COPY(MsooXmlReaderContext)
};

//! A base class reading MSOOXML parts like document.xml or styles.xml.
class KOMSOOXML_EXPORT MsooXmlReader : public QXmlStreamReader, public KoOdfWriters
{
public:
    explicit MsooXmlReader(KoOdfWriters *writers);

    MsooXmlReader(QIODevice *io, KoOdfWriters *writers);

    virtual ~MsooXmlReader();

    //! Reads/parses the file
    virtual KoFilter::ConversionStatus read(MsooXmlReaderContext *context = nullptr) = 0;

    //! Sets filename for the document being read.
    //! Only for error reporting purposes, used in raiseError().
    void setFileName(const QString &fileName)
    {
        m_fileName = fileName;
    }

    //! @return filename for the document being read.
    //! Only for error reporting purposes, used in raiseError().
    QString fileName() const
    {
        return m_fileName;
    }

    //! Reimplemented after QXmlStreamReader: adds line, column and filename information
    void raiseError(const QString &message = QString());

    // Uncomment if debugging is needed
    //! Reimplemented after QXmlStreamReader for supporting undo read and for debugging purposes
    // TokenType readNext();

    //! Undoes recent readNext(); only one recent readNext() can be undoed
    // void undoReadNext();

    // const strings (for optimization)
    static const char constOn[];
    static const char constOff[];
    static const char constTrue[];
    static const char constFalse[];
    static const char constNone[];
    static const char const1[];
    static const char const0[];
    static const char constAuto[];
    static const char constFloat[];
    static const char constPercentage[];
    static const char constCurrency[];
    static const char constDate[];
    static const char constTime[];
    static const char constBoolean[];
    static const char constString[];

protected:
    // -- general
    bool expectElName(const char *elementName);
    bool expectElNameEnd(const char *elementName);
    bool expectEl(const char *qualifiedElementName);
    bool expectEl(const QString &qualifiedElementName);
    bool expectEl(const QList<QByteArray> &qualifiedElementNames);
    bool expectElEnd(const QString &qualifiedElementName);
    bool expectElEnd(const char *qualifiedElementName);
    bool expectNS(const char *nsName);
    void raiseElNotFoundError(const char *elementName);
    void raiseAttributeNotFoundError(const char *attrName);
    void raiseNSNotFoundError(const char *nsName);
    void raiseUnexpectedAttributeValueError(const QString &value, const char *attrName);
    void raiseUnexpectedSecondOccurenceOfElError(const char *elementName);

    //! Decodes boolean attribute. Used by read_b(), read_i(), etc.
    bool readBooleanAttr(const char *attrName, bool defaultValue = false) const;

    QString m_defaultNamespace; //!< stores namespace (for optimization)

    QStack<QByteArray> m_callsNames;
#ifndef NDEBUG
    QStack<QByteArray> m_callsNamesDebug;
#endif

private:
    Q_DISABLE_COPY(MsooXmlReader)

    QString m_fileName;
    bool m_readUndoed;
    // QXmlStreamReader::TokenType m_recentType;

    void init();
};

} // namespace MSOOXML

//! debugMsooXml stream operator. Writes this reader to the debug output in a nicely formatted way.
//! @todo add the same for QXmlStreamWriter
KOMSOOXML_EXPORT QDebug operator<<(QDebug dbg, const QXmlStreamReader &reader);

#endif // MSOOXMLREADER_H
