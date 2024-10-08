/*
 * This file is part of Office 2007 Filters for Calligra
 *
 * SPDX-FileCopyrightText: 2010 Sebastian Sauer <sebsauer@kdab.com>
 * SPDX-FileCopyrightText: 2010 Carlos Licea <carlos@kdab.com>
 * SPDX-FileCopyrightText: 2010 Nokia Corporation and /or its subsidiary(-ies).
 *
 * Contact: Suresh Chande suresh.chande@nokia.com
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 *
 */

#include "MsooXmlDiagramReader.h"

#define MSOOXML_CURRENT_NS "dgm"
#define MSOOXML_CURRENT_CLASS MsooXmlDiagramReader
#define BIND_READ_CLASS MSOOXML_CURRENT_CLASS

#include <KoGenStyles.h>
#include <KoXmlWriter.h>
#include <MsooXmlReader_p.h>
#include <MsooXmlUtils.h>
#include <QExplicitlySharedDataPointer>
#include <typeinfo>

#include "MsooXmlDiagramReader_p.h"

using namespace MSOOXML;

MsooXmlDiagramReaderContext::MsooXmlDiagramReaderContext(KoGenStyles *styles)
    : MSOOXML::MsooXmlReaderContext()
    , m_styles(styles)
    , m_context(new Diagram::Context)
{
}

MsooXmlDiagramReaderContext::~MsooXmlDiagramReaderContext()
{
    delete m_context;
}

int MsooXmlDiagramReaderContext::shapeListSize() const
{
    return m_context->m_shapeList.size();
}

void MsooXmlDiagramReaderContext::saveIndex(KoXmlWriter *xmlWriter, const QRect &rect)
{
    // The root layout node always inherits the canvas dimensions by default.
    m_context->m_rootLayout->m_values["l"] = rect.x();
    m_context->m_rootLayout->m_values["t"] = rect.y();
    m_context->m_rootLayout->m_values["w"] = rect.width();
    m_context->m_rootLayout->m_values["h"] = rect.height();
    debugMsooXml << "drawingRect" << rect;

    // Do the (re-)layout.
    m_context->m_rootLayout->layoutAtom(m_context);
    // Write the content.
    m_context->m_rootLayout->writeAtom(m_context, xmlWriter, m_styles);
}

MsooXmlDiagramReader::MsooXmlDiagramReader(KoOdfWriters *writers)
    : MSOOXML::MsooXmlCommonReader(writers)
    , m_type(InvalidType)
{
}

MsooXmlDiagramReader::~MsooXmlDiagramReader() = default;

KoFilter::ConversionStatus MsooXmlDiagramReader::read(MSOOXML::MsooXmlReaderContext *context)
{
    m_context = dynamic_cast<MsooXmlDiagramReaderContext *>(context);
    Q_ASSERT(m_context);

    readNext();
    if (!isStartDocument()) {
        return KoFilter::WrongFormat;
    }

    readNext();
    if (qualifiedName() == QLatin1String("dgm:dataModel")) {
        m_type = DataModelType;

        Diagram::PointListNode rootList;
        while (!atEnd()) {
            QXmlStreamReader::TokenType tokenType = readNext();
            if (tokenType == QXmlStreamReader::Invalid || tokenType == QXmlStreamReader::EndDocument)
                break;
            if (isStartElement()) {
                if (qualifiedName() == QLatin1String("dgm:ptLst")) { // list of points
                    rootList.readAll(m_context->m_context, this);
                } else if (qualifiedName() == QLatin1String("dgm:cxnLst")) { // list of connections
                    m_context->m_context->m_connections->readAll(m_context->m_context, this);
                }
            }
        }

        QMap<QString, Diagram::PointNode *> pointMap;
        foreach (Diagram::AbstractNode *node, rootList.children()) {
            if (Diagram::PointNode *point = dynamic_cast<Diagram::PointNode *>(node))
                if (!point->m_modelId.isEmpty())
                    pointMap[point->m_modelId] = point;
        }

        foreach (Diagram::AbstractNode *node, m_context->m_context->m_connections->children()) {
            if (Diagram::ConnectionNode *connection = dynamic_cast<Diagram::ConnectionNode *>(node)) {
                Q_ASSERT(pointMap.contains(connection->m_srcId));
                Q_ASSERT(pointMap.contains(connection->m_destId));
                Q_ASSERT(connection->m_parTransId.isEmpty() || pointMap.contains(connection->m_parTransId));
                Q_ASSERT(connection->m_sibTransId.isEmpty() || pointMap.contains(connection->m_sibTransId));
                Diagram::PointNode *source = pointMap.value(connection->m_srcId);
                Diagram::PointNode *destination = pointMap.value(connection->m_destId);
                Diagram::PointNode *parent = connection->m_parTransId.isEmpty() ? 0 : pointMap.value(connection->m_parTransId);
                Diagram::PointNode *sibling = connection->m_sibTransId.isEmpty() ? 0 : pointMap.value(connection->m_sibTransId);

                const int srcOrd = connection->m_srcOrd;
                const int destOrd = connection->m_destOrd;
                Q_UNUSED(destOrd); // TODO how to apply that one?

                if (parent) {
                    // add a transition between parent and child
                    Q_ASSERT(parent->m_type == "parTrans");
                    Q_ASSERT(rootList.children().contains(parent));
                    rootList.removeChild(parent);
                    source->insertChild(srcOrd, parent);
                }

                if (sibling) {
                    // add a transition between siblings
                    Q_ASSERT(sibling->m_type == "sibTrans");
                    Q_ASSERT(rootList.children().contains(sibling));
                    rootList.removeChild(sibling);
                    source->insertChild(srcOrd, sibling);
                }

                if (connection->m_type == QLatin1String("parOf")) {
                    // This defines a parent-child relationship in the sense that node X is a parent of node Y.
                    Q_ASSERT(rootList.children().contains(destination));
                    // Q_ASSERT(rootList.children().at(destOrd) == destination);
                    rootList.removeChild(destination);
                    source->insertChild(srcOrd, destination);
                } else if (connection->m_type == QLatin1String("presOf")) {
                    // A presentation type relationship. This type of relationship exists to actually present data.
                    // TODO
                } else if (connection->m_type == QLatin1String("presParOf")) {
                    // A relationship defining a parent of a presentation node.
                    // TODO
                }
            }
        }

        delete m_context->m_context->m_rootPoint;
        foreach (Diagram::AbstractNode *node, rootList.children()) {
            if (Diagram::PointNode *pt = dynamic_cast<Diagram::PointNode *>(node)) {
                if (pt->m_type == QLatin1String("doc")) {
                    m_context->m_context->m_rootPoint = pt;
                    break;
                }
            }
        }
        if (!m_context->m_context->m_rootPoint) {
            warnMsooXml << "Data-definition doesn't specify a root-node";
            return KoFilter::WrongFormat;
        }
        Q_ASSERT(rootList.children().contains(m_context->m_context->m_rootPoint));
        rootList.removeChild(m_context->m_context->m_rootPoint);
        // Q_ASSERT(rootList.children().isEmpty());
        m_context->m_context->setCurrentNode(m_context->m_context->m_rootPoint);

#if 0
        QFile visGraphFile( "graphDump" );
        visGraphFile.open( QFile::WriteOnly | QFile::Truncate );
        QTextStream visGraph( &visGraphFile );
        visGraph << "digraph { \n";
        m_context->m_context->m_rootPoint->dump( visGraph );
        visGraph << "}\n";
        visGraphFile.close();
#endif
        // m_context->m_context->m_rootPoint->dump(m_context->m_context, 0);
        // Q_ASSERT( false );
    } else if (qualifiedName() == QLatin1String("dgm:layoutDef")) {
        m_type = LayoutDefType;

        while (!atEnd()) {
            QXmlStreamReader::TokenType tokenType = readNext();
            if (tokenType == QXmlStreamReader::Invalid || tokenType == QXmlStreamReader::EndDocument)
                break;
            if (isStartElement()) {
                if (qualifiedName() == QLatin1String("dgm:layoutNode")) {
                    m_context->m_context->m_rootLayout->readAll(m_context->m_context, this);
                }
                // ELSE_TRY_READ_IF(title)
                // ELSE_TRY_READ_IF(sampData)
                // ELSE_TRY_READ_IF(styleData)
            }
        }
        if (!m_context->m_context->m_rootPoint) {
            warnMsooXml << "Layout-definition doesn't specify a root-point";
            return KoFilter::WrongFormat;
        }

        // build the layout-tree. This solves ForEachAtom, ChooseAtom and other things that need to be executed.
        m_context->m_context->m_rootLayout->build(m_context->m_context);
        // once done we start a second build-process that does additional things once the layout-tree is fully available.
        m_context->m_context->m_rootLayout->finishBuild(m_context->m_context);

        // m_context->m_context->m_rootLayout->dump(m_context->m_context,0);
        // Q_ASSERT( false );
    } else if (qualifiedName() == QLatin1String("dgm:styleDef")) {
        m_type = StyleDefType;
        // TODO
    } else if (qualifiedName() == QLatin1String("dgm:colorsDef")) {
        m_type = ColorsDefType;
        // TODO
    } else {
        return KoFilter::WrongFormat;
    }

    m_context = nullptr;
    m_type = InvalidType;
    return KoFilter::OK;
}
