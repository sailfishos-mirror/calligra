/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2008 Jan Hambrecht <jaham@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "BarnDoorWipeStrategy.h"
#include "BarnDoorWipeEffectFactory.h"
#include <QPainter>
#include <QPainterPath>
#include <QWidget>

const int StepCount = 250;

BarnDoorWipeStrategy::BarnDoorWipeStrategy(int subtype, const char *smilSubType, bool reverse)
    : KPrPageEffectStrategy(subtype, "barnDoorWipe", smilSubType, reverse)
{
}

BarnDoorWipeStrategy::~BarnDoorWipeStrategy() = default;

void BarnDoorWipeStrategy::setup(const KPrPageEffect::Data &data, QTimeLine &timeLine)
{
    Q_UNUSED(data);
    timeLine.setFrameRange(0, StepCount);
}

void BarnDoorWipeStrategy::paintStep(QPainter &p, int currPos, const KPrPageEffect::Data &data)
{
    p.drawPixmap(QPoint(0, 0), data.m_oldPage, data.m_widget->rect());
    p.setClipPath(clipPath(currPos, data.m_widget->rect()));
    p.drawPixmap(QPoint(0, 0), data.m_newPage, data.m_widget->rect());
}

void BarnDoorWipeStrategy::next(const KPrPageEffect::Data &data)
{
    data.m_widget->update();
}

QPainterPath BarnDoorWipeStrategy::clipPath(int step, const QRect &area)
{
    qreal percent = static_cast<qreal>(step) / static_cast<qreal>(StepCount);
    if (reverse())
        percent = static_cast<qreal>(StepCount - step) / static_cast<qreal>(StepCount);

    int stepx = static_cast<int>(area.width() * percent);
    int stepy = static_cast<int>(area.height() * percent);

    int width_2 = area.width() >> 1;
    int height_2 = area.height() >> 1;

    QPainterPath path;

    switch (subType()) {
    case BarnDoorWipeEffectFactory::Vertical:
    case BarnDoorWipeEffectFactory::VerticalReverse: {
        stepx = static_cast<int>(width_2 * percent);
        QPoint top(width_2, 0);
        QPoint bottom(width_2, area.height());
        path.moveTo(top - QPoint(stepx, 0));
        path.lineTo(top + QPoint(stepx, 0));
        path.lineTo(bottom + QPoint(stepx, 0));
        path.lineTo(bottom - QPoint(stepx, 0));
        break;
    }
    case BarnDoorWipeEffectFactory::Horizontal:
    case BarnDoorWipeEffectFactory::HorizontalReverse: {
        stepy = static_cast<int>(height_2 * percent);
        QPoint left(0, height_2);
        QPoint right(area.width(), height_2);
        path.moveTo(left + QPoint(0, stepy));
        path.lineTo(left - QPoint(0, stepy));
        path.lineTo(right - QPoint(0, stepy));
        path.lineTo(right + QPoint(0, stepy));
        break;
    }
    case BarnDoorWipeEffectFactory::DiagonalBottomLeft:
    case BarnDoorWipeEffectFactory::DiagonalBottomLeftReverse: {
        path.moveTo(area.bottomLeft() + QPoint(stepx, 0));
        path.lineTo(area.bottomLeft());
        path.lineTo(area.bottomLeft() - QPoint(0, stepy));
        path.lineTo(area.topRight() - QPoint(stepx, 0));
        path.lineTo(area.topRight());
        path.lineTo(area.topRight() + QPoint(0, stepy));
        break;
    }
    case BarnDoorWipeEffectFactory::DiagonalTopLeft:
    case BarnDoorWipeEffectFactory::DiagonalTopLeftReverse: {
        path.moveTo(area.topLeft() + QPoint(0, stepy));
        path.lineTo(area.topLeft());
        path.lineTo(area.topLeft() + QPoint(stepx, 0));
        path.lineTo(area.bottomRight() - QPoint(0, stepy));
        path.lineTo(area.bottomRight());
        path.lineTo(area.bottomRight() - QPoint(stepx, 0));
        break;
    }
    default:
        return QPainterPath();
    }

    path.closeSubpath();

    if (reverse()) {
        QPainterPath areaPath;
        areaPath.addRect(area);
        path = areaPath.subtracted(path);
    }

    return path;
}
