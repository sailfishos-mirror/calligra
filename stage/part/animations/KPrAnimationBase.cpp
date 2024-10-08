/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2010 Thorsten Zachmann <zachmann@kde.org>
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

#include "KPrAnimationBase.h"

#include <KoElementReference.h>
#include <KoPASavingContext.h>
#include <KoShapeLoadingContext.h>
#include <KoTextBlockData.h>
#include <KoXmlNS.h>
#include <KoXmlReader.h>
#include <KoXmlWriter.h>

#include "KPrAnimationCache.h"
#include "KPrDurationParser.h"
#include "KPrShapeAnimation.h"

KPrAnimationBase::KPrAnimationBase(KPrShapeAnimation *shapeAnimation)
    : m_shapeAnimation(shapeAnimation)
    , m_begin(0)
    , m_duration(1)
    , m_fill(FillAuto)
{
}

KPrAnimationBase::~KPrAnimationBase() = default;

int KPrAnimationBase::duration() const
{
    return m_duration + m_begin;
}

int KPrAnimationBase::begin() const
{
    return m_begin;
}

void KPrAnimationBase::setBegin(int value)
{
    m_begin = value;
}

void KPrAnimationBase::setDuration(int value)
{
    m_duration = value;
}

bool KPrAnimationBase::loadOdf(const KoXmlElement &element, KoShapeLoadingContext &context)
{
    Q_UNUSED(context)
    m_begin = KPrDurationParser::durationMs(element.attributeNS(KoXmlNS::smil, "begin"));
    if (m_begin == -1) {
        m_begin = 0;
    }
    m_duration = KPrDurationParser::durationMs(element.attributeNS(KoXmlNS::smil, "dur"));
    if (m_duration == -1) {
        m_duration = 1;
    }
    m_duration += m_begin;

    QString fill = element.attributeNS(KoXmlNS::smil, "fill");
    if (!fill.isEmpty()) {
        if (fill == "remove") {
            m_fill = FillRemove;
        }
        if (fill == "freeze") {
            m_fill = FillFreeze;
        }
        if (fill == "hold") {
            m_fill = FillHold;
        }
        if (fill == "transition") {
            m_fill = FillTransition;
        }
        if (fill == "auto") {
            m_fill = FillAuto;
        }
        if (fill == "default") {
            m_fill = FillDefault;
        }
    }

    return true;
}

void KPrAnimationBase::updateCache(const QString &id, const QVariant &value)
{
    m_animationCache->update(m_shapeAnimation->shape(), m_shapeAnimation->textBlockUserData(), id, value);
}

void KPrAnimationBase::updateCurrentTime(int currentTime)
{
    if (currentTime >= m_begin) {
        next(currentTime - m_begin);
    }
}

int KPrAnimationBase::animationDuration() const
{
    return totalDuration() - m_begin;
}

bool KPrAnimationBase::saveAttribute(KoPASavingContext &paContext) const
{
    KoXmlWriter &writer = paContext.xmlWriter();
    writer.addAttribute("smil:begin", KPrDurationParser::msToString(m_begin));
    writer.addAttribute("smil:dur", KPrDurationParser::msToString(m_duration));
    if (m_shapeAnimation->textBlockUserData()) {
        writer.addAttribute("smil:targetElement", paContext.existingXmlid(m_shapeAnimation->textBlockUserData()).toString());
        writer.addAttribute("anim:sub-item", "text");
    } else {
        writer.addAttribute("smil:targetElement", paContext.existingXmlid(m_shapeAnimation->shape()).toString());
    }

    QString fill;
    switch (m_fill) {
    case FillRemove:
        fill = "remove";
        break;
    case FillFreeze:
        fill = "freeze";
        break;
    case FillHold:
        fill = "hold";
        break;
    case FillTransition:
        fill = "transition";
        break;
    case FillAuto:
        fill = "auto";
        break;
    case FillDefault:
        fill = "default";
        break;
    }

    if (!fill.isEmpty()) {
        writer.addAttribute("smil:fill", fill);
    }

    return true;
}
