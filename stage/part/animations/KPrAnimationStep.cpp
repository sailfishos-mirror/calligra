/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2010 Thorsten Zachmann <zachmann@kde.org>
 * SPDX-FileCopyrightText: 2010 Benjamin Port <port.benjamin@gmail.com>
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

#include "KPrAnimationStep.h"
#include "KPrAnimationSubStep.h"
#include "KoXmlWriter.h"
#include <KoPASavingContext.h>

KPrAnimationStep::KPrAnimationStep() = default;

KPrAnimationStep::~KPrAnimationStep() = default;

void KPrAnimationStep::init(KPrAnimationCache *animationCache, int step)
{
    for (int i = 0; i < this->animationCount(); i++) {
        QAbstractAnimation *animation = this->animationAt(i);
        if (KPrAnimationSubStep *a = dynamic_cast<KPrAnimationSubStep *>(animation)) {
            a->init(animationCache, step);
        }
    }
}

bool KPrAnimationStep::saveOdf(KoPASavingContext &paContext) const
{
    KoXmlWriter &writer = paContext.xmlWriter();
    writer.startElement("anim:par");
    for (int i = 0; i < this->animationCount(); i++) {
        bool startStep = !i;
        QAbstractAnimation *animation = this->animationAt(i);
        if (KPrAnimationSubStep *a = dynamic_cast<KPrAnimationSubStep *>(animation)) {
            a->saveOdf(paContext, startStep);
        }
    }
    writer.endElement();
    return true;
}

void KPrAnimationStep::deactivate()
{
    for (int i = 0; i < this->animationCount(); i++) {
        QAbstractAnimation *animation = this->animationAt(i);
        if (KPrAnimationSubStep *a = dynamic_cast<KPrAnimationSubStep *>(animation)) {
            a->deactivate();
        }
    }
}
