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

#include "MsooXmlRelationshipsReader.h"
#include <MsooXmlSchemas.h>
#include <MsooXmlUtils.h>

// #define MSOOXML_CURRENT_NS
#define MSOOXML_CURRENT_CLASS MsooXmlRelationshipsReader
#define BIND_READ_CLASS MSOOXML_CURRENT_CLASS

#include <MsooXmlReader_p.h>

using namespace MSOOXML;

MsooXmlRelationshipsReaderContext::MsooXmlRelationshipsReaderContext(const QString &_path,
                                                                     const QString &_file,
                                                                     QMap<QString, QString> &_rels,
                                                                     QMap<QString, QString> &_targetsForTypes)
    : path(_path)
    , file(_file)
    , rels(&_rels)
    , targetsForTypes(&_targetsForTypes)
{
}

class MsooXmlRelationshipsReader::Private
{
public:
    Private() = default;
    ~Private() = default;
    QString pathAndFile;
};

MsooXmlRelationshipsReader::MsooXmlRelationshipsReader(KoOdfWriters *writers)
    : MSOOXML::MsooXmlReader(writers)
    , m_context(nullptr)
    , d(new Private)
{
    init();
}

MsooXmlRelationshipsReader::~MsooXmlRelationshipsReader()
{
    delete d;
}

void MsooXmlRelationshipsReader::init()
{
}

KoFilter::ConversionStatus MsooXmlRelationshipsReader::read(MSOOXML::MsooXmlReaderContext *context)
{
    m_context = dynamic_cast<MsooXmlRelationshipsReaderContext *>(context);
    const KoFilter::ConversionStatus result = readInternal();
    m_context = nullptr;
    if (result == KoFilter::OK)
        return KoFilter::OK;
    return result;
}

KoFilter::ConversionStatus MsooXmlRelationshipsReader::readInternal()
{
    debugMsooXml << "=============================";

    d->pathAndFile = MsooXmlRelationshipsReader::relKey(m_context->path, m_context->file, QString());

    readNext();
    if (!isStartDocument()) {
        return KoFilter::WrongFormat;
    }

    // Relationships
    readNext();
    debugMsooXml << *this << namespaceUri();

    if (!expectEl("Relationships")) {
        return KoFilter::WrongFormat;
    }
    if (!expectNS(MSOOXML::Schemas::relationships)) {
        return KoFilter::WrongFormat;
    }
    /*
        const QXmlStreamAttributes attrs( attributes() );
        for (int i=0; i<attrs.count(); i++) {
            debugMsooXml << "1 NS prefix:" << attrs[i].name() << "uri:" << attrs[i].namespaceUri();
        }*/

    QXmlStreamNamespaceDeclarations namespaces(namespaceDeclarations());
    for (int i = 0; i < namespaces.count(); i++) {
        debugMsooXml << "NS prefix:" << namespaces[i].prefix() << "uri:" << namespaces[i].namespaceUri();
    }

    TRY_READ(Relationships)
    debugMsooXml << "===========finished============";
    return KoFilter::OK;
}

#undef CURRENT_EL
#define CURRENT_EL Relationships
//! Relationships handler
/*!
 No parent elements.

 Child elements:
    - [done] Relationship
*/
KoFilter::ConversionStatus MsooXmlRelationshipsReader::read_Relationships()
{
    READ_PROLOGUE
    while (!atEnd()) {
        readNext();
        debugMsooXml << *this;
        BREAK_IF_END_OF(CURRENT_EL)
        if (isStartElement()) {
            TRY_READ_IF(Relationship)
            ELSE_WRONG_FORMAT
        }
    }
    READ_EPILOGUE
}

#undef CURRENT_EL
#define CURRENT_EL Relationship
//! Relationship handler
/*!
 Parent elements:
    - [done] Relationships

 No child elements.
*/
KoFilter::ConversionStatus MsooXmlRelationshipsReader::read_Relationship()
{
    READ_PROLOGUE
    const QXmlStreamAttributes attrs(attributes());
    READ_ATTR_WITHOUT_NS(Id)
    READ_ATTR_WITHOUT_NS(Type)
    READ_ATTR_WITHOUT_NS(Target)
    QString fixedPath(m_context->path);
    while (Target.startsWith(QLatin1String("../"))) {
        // debugMsooXml << "- Target:" << Target << "fixedPath:" << fixedPath;
        Target.remove(0, 3);
        fixedPath.truncate(fixedPath.lastIndexOf('/'));
        // debugMsooXml << "= Target:" << Target << "fixedPath:" << fixedPath;
    }
    // debugMsooXml << "adding rel:";
    // debugMsooXml << d->pathAndFile + Id;
    // debugMsooXml << fixedPath + '/' + Target;

    m_context->rels->insert(d->pathAndFile + Id, fixedPath + '/' + Target);
    // debugMsooXml << "added target" << Target << "for type" << Type << "path=" << fixedPath << "key=" << targetKey(m_context->path + '/' + m_context->file,
    // Type);
    m_context->targetsForTypes->insert(targetKey(m_context->path + '/' + m_context->file, Type), fixedPath + '/' + Target);

    readNext();
    READ_EPILOGUE
}
