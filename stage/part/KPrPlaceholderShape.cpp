/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2008 Thorsten Zachmann <zachmann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (  at your option ) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "KPrPlaceholderShape.h"

#include <KoOdfWorkaround.h>
#include <KoShapeSavingContext.h>
#include <KoViewConverter.h>
#include <KoXmlWriter.h>
#include <QPainter>
#include <QTextOption>

#include "KPrPlaceholderStrategy.h"

KPrPlaceholderShape::KPrPlaceholderShape()
    : m_strategy(nullptr)
{
}

KPrPlaceholderShape::KPrPlaceholderShape(const QString &presentationClass)
    : m_strategy(nullptr)
{
    m_strategy = KPrPlaceholderStrategy::create(presentationClass);
}

KPrPlaceholderShape::~KPrPlaceholderShape()
{
    delete m_strategy;
}

void KPrPlaceholderShape::paint(QPainter &painter, const KoViewConverter &converter, KoShapePaintingContext &paintcontext)
{
    QRectF rect(QPointF(0, 0), size());
    if (m_strategy) {
        m_strategy->paint(painter, converter, rect, paintcontext);
    } else {
        applyConversion(painter, converter);
        QPen pen(Qt::gray, 0);
        pen.setStyle(Qt::DashLine);
        painter.setPen(pen);
        painter.drawRect(rect);
    }
}

bool KPrPlaceholderShape::loadOdf(const KoXmlElement &element, KoShapeLoadingContext &context)
{
    loadOdfAttributes(element, context, OdfAdditionalAttributes);

#ifndef NWORKAROUND_ODF_BUGS
    KoOdfWorkaround::fixPresentationPlaceholder(this);
#endif
    delete m_strategy;

    m_strategy = KPrPlaceholderStrategy::create(additionalAttribute("presentation:class"));
    if (m_strategy == nullptr) {
        return false;
    }

    // first check if we can create a placeholder before we load the attributes
    loadOdfAttributes(element, context, OdfMandatories | OdfTransformation | OdfGeometry | OdfCommonChildElements);
    m_strategy->loadOdf(element, context);

    return true;
}

void KPrPlaceholderShape::saveOdf(KoShapeSavingContext &context) const
{
    KoXmlWriter &writer = context.xmlWriter();
    writer.startElement("draw:frame");
    saveOdfAttributes(context, OdfAllAttributes);
    if (m_strategy) {
        m_strategy->saveOdf(context);
    }
    saveOdfCommonChildElements(context);
    writer.endElement(); // draw:frame
}

KoShape *KPrPlaceholderShape::createShape(KoDocumentResourceManager *documentResources)
{
    Q_ASSERT(m_strategy);
    KoShape *shape = nullptr;
    if (m_strategy) {
        shape = m_strategy->createShape(documentResources);
    }
    return shape;
}

void KPrPlaceholderShape::initStrategy(KoDocumentResourceManager *documentResources)
{
    Q_ASSERT(m_strategy);
    if (m_strategy) {
        m_strategy->init(documentResources);
    }
}

KoShapeUserData *KPrPlaceholderShape::userData() const
{
    Q_ASSERT(m_strategy);
    return m_strategy ? m_strategy->userData() : nullptr;
}
