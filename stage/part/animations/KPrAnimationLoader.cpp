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

#include "KPrAnimationLoader.h"

#include <KoShape.h>
#include <KoShapeLoadingContext.h>
#include <KoTextBlockData.h>
#include <KoXmlNS.h>
#include <KoXmlReader.h>

#include <QDomDocument>
#include <QDomNode>
#include <QVariant>

#include "KPrAnimationBase.h"
#include "KPrAnimationFactory.h"
#include "KPrAnimationStep.h"
#include "KPrAnimationSubStep.h"
#include "KPrShapeAnimation.h"
#include "KPrShapeApplicationData.h"
#include "StageDebug.h"

KPrAnimationLoader::KPrAnimationLoader() = default;

KPrAnimationLoader::~KPrAnimationLoader() = default;

void debugXml(const QString &pos, const KoXmlElement &element)
{
    QByteArray array;
    QDomDocument doc;
    QTextStream st(&array);
    KoXml::asQDomElement(doc, element);
    st << doc.documentElement();
    debugStageAnimation << pos << array;
}

bool KPrAnimationLoader::loadOdf(const KoXmlElement &element, KoShapeLoadingContext &context)
{
    // have an overall structure for animations each step needs to be in its own QSequentialAnimationGroup subclass
    // use KPrAnimationStep for that
    KoXmlElement stepElement;
    forEachElement(stepElement, element)
    {
        if (stepElement.tagName() == "par" && stepElement.namespaceURI() == KoXmlNS::anim) {
            // this creates a new step
            KPrAnimationStep *animationStep = new KPrAnimationStep();

            KoXmlElement parElement;
            forEachElement(parElement, stepElement)
            {
                KoXmlElement innerParElement;
                forEachElement(innerParElement, parElement)
                {
                    if (innerParElement.tagName() == "par" && innerParElement.namespaceURI() == KoXmlNS::anim) {
                        loadOdfAnimation(&animationStep, innerParElement, context);
                    }
                }
            }
            m_animations.append(animationStep);
        } else {
            Q_ASSERT(0);
            // according to spec there should be only par elements
        }
    }

    debug();

    return true;
}

void KPrAnimationLoader::debug()
{
    foreach (KPrAnimationStep *step, m_animations) {
        debugStageAnimation << "step";
        debug(step, 1);
    }
}

void KPrAnimationLoader::debug(QAbstractAnimation *animation, int level)
{
    QString indent;
    for (int i = 0; i < level; ++i) {
        indent += ' ';
    }
    if (KPrAnimationStep *a = dynamic_cast<KPrAnimationStep *>(animation)) {
        Q_UNUSED(a);
        debugStageAnimation << indent + "animation step";
    } else if (KPrAnimationSubStep *a = dynamic_cast<KPrAnimationSubStep *>(animation)) {
        Q_UNUSED(a);
        debugStageAnimation << indent + "animation sub step";
    } else if (KPrShapeAnimation *a = dynamic_cast<KPrShapeAnimation *>(animation)) {
        Q_UNUSED(a);
        debugStageAnimation << indent + "shape animation";
    } else if (KPrAnimationBase *a = dynamic_cast<KPrAnimationBase *>(animation)) {
        Q_UNUSED(a);
        debugStageAnimation << indent + "animation base";
    }

    if (QAnimationGroup *group = dynamic_cast<QAnimationGroup *>(animation)) {
        for (int i = 0; i < group->animationCount(); ++i) {
            debug(group->animationAt(i), level + 1);
        }
    }
}

bool KPrAnimationLoader::loadOdfAnimation(KPrAnimationStep **animationStep, const KoXmlElement &element, KoShapeLoadingContext &context)
{
    QString nodeType = element.attributeNS(KoXmlNS::presentation, "node-type", "with-previous");
    debugStageAnimation << "nodeType:" << nodeType;
    KPrAnimationSubStep *subStep = nullptr;
    if (nodeType == "on-click") {
        // if there is already an animation create a new step
        if ((*animationStep)->animationCount() != 0 || m_animations.isEmpty()) {
            m_animations.append(*animationStep);
            *animationStep = new KPrAnimationStep();
        }
        subStep = new KPrAnimationSubStep();
        (*animationStep)->addAnimation(subStep);
        // add par animation
    } else if (nodeType == "after-previous") {
        // add to sequence
        // add par
        subStep = new KPrAnimationSubStep();
        (*animationStep)->addAnimation(subStep);
        // add par animation
    } else {
        if (nodeType != "with-previous") {
            warnStageAnimation << "unsupported node-type" << nodeType << "found. Using with-previous";
        }
        // use the current substep
        if ((*animationStep)->animationCount()) {
            subStep = static_cast<KPrAnimationSubStep *>((*animationStep)->animationAt((*animationStep)->animationCount() - 1));
        } else {
            subStep = new KPrAnimationSubStep();
            (*animationStep)->addAnimation(subStep);
        }
        // add par to current par
    }

    // load preset and id
    // TODO: ole-action, media-call are not supported

    KPrShapeAnimation *shapeAnimation = nullptr;
    // The shape info and create a KPrShapeAnimation. If there is
    KoXmlElement e;
    forEachElement(e, element)
    {
        // TODO add a check that the shape animation is still the correct one
        if (shapeAnimation == nullptr) {
            QString targetElement(e.attributeNS(KoXmlNS::smil, "targetElement", QString()));
            if (!targetElement.isEmpty()) {
                KoShape *shape = nullptr;
                QTextBlockUserData *quData = nullptr;

                if (e.attributeNS(KoXmlNS::anim, "sub-item", "whole") == "text") {
                    QPair<KoShape *, QVariant> pair = context.shapeSubItemById(targetElement);
                    shape = pair.first;
                    quData = pair.second.value<QTextBlockUserData *>();
                } else {
                    shape = context.shapeById(targetElement);
                }

                if (shape) {
                    shapeAnimation = new KPrShapeAnimation(shape, quData);
                } else {
                    // shape animation not created
                    // TODO logging
                    continue;
                }
            }
        }
        if (shapeAnimation) {
            KPrAnimationBase *animation(KPrAnimationFactory::createAnimationFromOdf(e, context, shapeAnimation));
            if (animation) {
                shapeAnimation->addAnimation(animation);
            }
        }
    }

    if (shapeAnimation) {
        QString presetClass = element.attributeNS(KoXmlNS::presentation, "preset-class");
        QString animationId = element.attributeNS(KoXmlNS::presentation, "preset-id");
        QString presetSubType = element.attributeNS(KoXmlNS::presentation, "preset-sub-type");
        subStep->addAnimation(shapeAnimation);
        shapeAnimation->setSubStep(subStep);
        shapeAnimation->setStep((*animationStep));
        if (presetClass == "custom") {
            shapeAnimation->setPresetClass(KPrShapeAnimation::Custom);
        }
        if (presetClass == "entrance") {
            shapeAnimation->setPresetClass(KPrShapeAnimation::Entrance);
        } else if (presetClass == "exit") {
            shapeAnimation->setPresetClass(KPrShapeAnimation::Exit);
        } else if (presetClass == "emphasis") {
            shapeAnimation->setPresetClass(KPrShapeAnimation::Emphasis);
        } else if (presetClass == "motion-path") {
            shapeAnimation->setPresetClass(KPrShapeAnimation::MotionPath);
        } else if (presetClass == "ole-action") {
            shapeAnimation->setPresetClass(KPrShapeAnimation::OleAction);
        } else if (presetClass == "media-call") {
            shapeAnimation->setPresetClass(KPrShapeAnimation::MediaCall);
        } else {
            shapeAnimation->setPresetClass(KPrShapeAnimation::None);
        }
        if (!animationId.isEmpty()) {
            shapeAnimation->setId(animationId);
        }
        if (!presetSubType.isEmpty()) {
            shapeAnimation->setPresetSubType(presetSubType);
        }
        // Register animation in shape
        if (shapeAnimation->shape()) {
            KPrShapeApplicationData *applicationData = dynamic_cast<KPrShapeApplicationData *>(shapeAnimation->shape()->applicationData());
            if (applicationData == nullptr) {
                applicationData = new KPrShapeApplicationData();
                shapeAnimation->shape()->setApplicationData(applicationData);
            }
            applicationData->animations().insert(shapeAnimation);
        }
    }
    return true;
}

QList<KPrAnimationStep *> KPrAnimationLoader::animations()
{
    return m_animations;
}
