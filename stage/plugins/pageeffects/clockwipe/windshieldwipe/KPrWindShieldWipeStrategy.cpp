/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2008 Sven Langkamp <sven.langkamp@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KPrWindShieldWipeStrategy.h"
#include "KPrWindShieldWipeEffectFactory.h"

#include <QPainter>
#include <QPainterPath>
#include <QWidget>
#include <math.h>

#include "KPrClockWipeSubpathHelper.h"

KPrWindShieldWipeStrategy::KPrWindShieldWipeStrategy(int subType, const char *smilType, const char *smilSubType, bool reverse)
    : KPrSweepWipeStrategy(subType, smilType, smilSubType, reverse)
{
}

KPrWindShieldWipeStrategy::~KPrWindShieldWipeStrategy() = default;

void KPrWindShieldWipeStrategy::setup(const KPrPageEffect::Data &data, QTimeLine &timeLine)
{
    Q_UNUSED(data);
    timeLine.setFrameRange(0, 360);
}

void KPrWindShieldWipeStrategy::paintStep(QPainter &p, int currPos, const KPrPageEffect::Data &data)
{
    int width = data.m_widget->width();
    int height = data.m_widget->height();
    QRect rect(0, 0, width, height);
    p.drawPixmap(QPoint(0, 0), data.m_oldPage, rect);

    double startAngle1;
    QRect boundingRect1;

    double startAngle2;
    QRect boundingRect2;

    double rotationRange1 = -0.5 * M_PI;
    double rotationRange2 = -0.5 * M_PI;

    if (subType() == KPrWindShieldWipeEffectFactory::Right || subType() == KPrWindShieldWipeEffectFactory::Up
        || subType() == KPrWindShieldWipeEffectFactory::RightReverse || subType() == KPrWindShieldWipeEffectFactory::UpReverse) {
        switch (subType()) {
        case KPrWindShieldWipeEffectFactory::Right:
        case KPrWindShieldWipeEffectFactory::RightReverse:
            startAngle1 = 1.5 * M_PI;
            boundingRect1 = QRect(0, 0, width, height / 2);

            startAngle2 = 0.5 * M_PI;
            boundingRect2 = QRect(0, height / 2, width, height / 2);

            rotationRange1 = 2 * M_PI;
            rotationRange2 = -rotationRange1;
            break;
        case KPrWindShieldWipeEffectFactory::Up:
        case KPrWindShieldWipeEffectFactory::UpReverse:
            startAngle1 = 0;
            boundingRect1 = QRect(0, 0, width / 2, height);

            startAngle2 = M_PI;
            boundingRect2 = QRect(width / 2, 0, width / 2, height);

            rotationRange1 = 2 * M_PI;
            rotationRange2 = -rotationRange1;
            break;
        default:
            return;
        }

        if (reverse()) {
            startAngle1 = startAngle1 + rotationRange1;
            rotationRange1 *= -1;
            startAngle2 = startAngle2 + rotationRange2;
            rotationRange2 *= -1;
        }

        drawSweep(p, startAngle1, rotationRange1 * currPos / 360, boundingRect1, data);
        drawSweep(p, startAngle2, rotationRange2 * currPos / 360, boundingRect2, data);
    } else {
        switch (subType()) {
        case KPrWindShieldWipeEffectFactory::Vertical:
        case KPrWindShieldWipeEffectFactory::VerticalReverse:

            startAngle1 = 0.5 * M_PI;
            boundingRect1 = QRect(0, 0, width, height / 2);

            startAngle2 = 1.5 * M_PI;
            boundingRect2 = QRect(0, height / 2, width, height / 2);

            rotationRange1 = 2 * M_PI;
            rotationRange2 = rotationRange1;
            break;
        case KPrWindShieldWipeEffectFactory::Horizontal:
        case KPrWindShieldWipeEffectFactory::HorizontalReverse:

            startAngle1 = M_PI;
            boundingRect1 = QRect(0, 0, width / 2, height);

            startAngle2 = 0;
            boundingRect2 = QRect(width / 2, 0, width / 2, height);

            rotationRange1 = 2 * M_PI;
            rotationRange2 = rotationRange1;
            break;
        default:
            return;
        }

        if (reverse()) {
            startAngle1 += M_PI;
            startAngle2 += M_PI;
        }

        double angle = static_cast<double>(currPos) / 360 * M_PI;
        drawSweep(p, startAngle1 - angle, 2 * angle, boundingRect1, data);
        drawSweep(p, startAngle2 - angle, 2 * angle, boundingRect2, data);
    }
}
