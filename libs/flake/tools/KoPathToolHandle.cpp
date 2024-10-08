/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006, 2008 Jan Hambrecht <jaham@gmx.net>
 * SPDX-FileCopyrightText: 2006, 2007 Thorsten Zachmann <zachmann@kde.org>
 * SPDX-FileCopyrightText: 2007, 2010 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2007 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoPathToolHandle.h"
#include "KoCanvasBase.h"
#include "KoConnectionShape.h"
#include "KoDocumentResourceManager.h"
#include "KoParameterChangeStrategy.h"
#include "KoParameterShape.h"
#include "KoPathConnectionPointStrategy.h"
#include "KoPathControlPointMoveStrategy.h"
#include "KoPathPointMoveStrategy.h"
#include "KoPathTool.h"
#include "KoPointerEvent.h"
#include "KoSelection.h"
#include "KoShapeController.h"
#include "KoViewConverter.h"
#include "commands/KoPathPointTypeCommand.h"
#include <QPainter>

KoPathToolHandle::KoPathToolHandle(KoPathTool *tool)
    : m_tool(tool)
{
}

KoPathToolHandle::~KoPathToolHandle() = default;

uint KoPathToolHandle::handleRadius() const
{
    return m_tool->canvas()->shapeController()->resourceManager()->handleRadius();
}

PointHandle::PointHandle(KoPathTool *tool, KoPathPoint *activePoint, KoPathPoint::PointType activePointType)
    : KoPathToolHandle(tool)
    , m_activePoint(activePoint)
    , m_activePointType(activePointType)
{
}

void PointHandle::paint(QPainter &painter, const KoViewConverter &converter)
{
    painter.save();
    painter.setTransform(m_activePoint->parent()->absoluteTransformation(&converter) * painter.transform());
    KoShape::applyConversion(painter, converter);

    KoPathToolSelection *selection = dynamic_cast<KoPathToolSelection *>(m_tool->selection());

    KoPathPoint::PointType type = KoPathPoint::Node;
    if (selection && selection->contains(m_activePoint))
        type = KoPathPoint::All;
    m_activePoint->paint(painter, handleRadius(), type);
    painter.restore();
}

void PointHandle::repaint() const
{
    m_tool->repaint(m_oldRepaintedRect);
    bool active = false;
    KoPathToolSelection *selection = dynamic_cast<KoPathToolSelection *>(m_tool->selection());
    if (selection && selection->contains(m_activePoint))
        active = true;
    m_oldRepaintedRect = m_activePoint->boundingRect(!active);
    m_tool->repaint(m_oldRepaintedRect);
}

KoInteractionStrategy *PointHandle::handleMousePress(KoPointerEvent *event)
{
    if ((event->button() & Qt::LeftButton) == 0)
        return nullptr;
    if ((event->modifiers() & Qt::ShiftModifier) == 0) { // no shift pressed.
        KoPathToolSelection *selection = dynamic_cast<KoPathToolSelection *>(m_tool->selection());

        // control select adds/removes points to/from the selection
        if (event->modifiers() & Qt::ControlModifier) {
            if (selection->contains(m_activePoint)) {
                selection->remove(m_activePoint);
            } else {
                selection->add(m_activePoint, false);
            }
            m_tool->repaint(m_activePoint->boundingRect(false));
        } else {
            // no control modifier, so clear selection and select active point
            if (!selection->contains(m_activePoint)) {
                selection->add(m_activePoint, true);
                m_tool->repaint(m_activePoint->boundingRect(false));
            }
        }
        // TODO remove canvas from call ?
        if (m_activePointType == KoPathPoint::Node) {
            QPointF startPoint = m_activePoint->parent()->shapeToDocument(m_activePoint->point());
            return new KoPathPointMoveStrategy(m_tool, startPoint);
        } else {
            KoPathShape *pathShape = m_activePoint->parent();
            KoPathPointData pd(pathShape, pathShape->pathPointIndex(m_activePoint));
            return new KoPathControlPointMoveStrategy(m_tool, pd, m_activePointType, event->point);
        }
    } else {
        KoPathPoint::PointProperties props = m_activePoint->properties();
        if (!m_activePoint->activeControlPoint1() || !m_activePoint->activeControlPoint2())
            return nullptr;

        KoPathPointTypeCommand::PointType pointType = KoPathPointTypeCommand::Smooth;
        // cycle the smooth->symmetric->unsmooth state of the path point
        if (props & KoPathPoint::IsSmooth)
            pointType = KoPathPointTypeCommand::Symmetric;
        else if (props & KoPathPoint::IsSymmetric)
            pointType = KoPathPointTypeCommand::Corner;

        QList<KoPathPointData> pointData;
        pointData.append(KoPathPointData(m_activePoint->parent(), m_activePoint->parent()->pathPointIndex(m_activePoint)));
        m_tool->canvas()->addCommand(new KoPathPointTypeCommand(pointData, pointType));
    }
    return nullptr;
}

bool PointHandle::check(const QList<KoPathShape *> &selectedShapes)
{
    if (selectedShapes.contains(m_activePoint->parent())) {
        return m_activePoint->parent()->pathPointIndex(m_activePoint) != KoPathPointIndex(-1, -1);
    }
    return false;
}

KoPathPoint *PointHandle::activePoint() const
{
    return m_activePoint;
}

KoPathPoint::PointType PointHandle::activePointType() const
{
    return m_activePointType;
}

ParameterHandle::ParameterHandle(KoPathTool *tool, KoParameterShape *parameterShape, int handleId)
    : KoPathToolHandle(tool)
    , m_parameterShape(parameterShape)
    , m_handleId(handleId)
{
}

void ParameterHandle::paint(QPainter &painter, const KoViewConverter &converter)
{
    painter.save();
    painter.setTransform(m_parameterShape->absoluteTransformation(&converter) * painter.transform());

    m_parameterShape->paintHandle(painter, converter, m_handleId, handleRadius());
    painter.restore();
}

void ParameterHandle::repaint() const
{
    m_tool->repaint(m_parameterShape->shapeToDocument(QRectF(m_parameterShape->handlePosition(m_handleId), QSize(1, 1))));
}

KoInteractionStrategy *ParameterHandle::handleMousePress(KoPointerEvent *event)
{
    if (event->button() & Qt::LeftButton) {
        KoPathToolSelection *selection = dynamic_cast<KoPathToolSelection *>(m_tool->selection());
        if (selection)
            selection->clear();
        return new KoParameterChangeStrategy(m_tool, m_parameterShape, m_handleId);
    }
    return nullptr;
}

bool ParameterHandle::check(const QList<KoPathShape *> &selectedShapes)
{
    return selectedShapes.contains(m_parameterShape);
}

ConnectionHandle::ConnectionHandle(KoPathTool *tool, KoParameterShape *parameterShape, int handleId)
    : ParameterHandle(tool, parameterShape, handleId)
{
}

KoInteractionStrategy *ConnectionHandle::handleMousePress(KoPointerEvent *event)
{
    if (event->button() & Qt::LeftButton) {
        KoPathToolSelection *selection = dynamic_cast<KoPathToolSelection *>(m_tool->selection());
        if (selection)
            selection->clear();
        KoConnectionShape *shape = dynamic_cast<KoConnectionShape *>(m_parameterShape);
        if (!shape)
            return nullptr;
        return new KoPathConnectionPointStrategy(m_tool, shape, m_handleId);
    }
    return nullptr;
}
