/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006-2007 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2006 C. Boemann <cbo@boemann.dk>
 * SPDX-FileCopyrightText: 2008 Jan Hambrecht <jaham@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ShapeShearStrategy.h"
#include "SelectionDecorator.h"
#include "SelectionTransformCommand.h"

#include <KoCanvasBase.h>
#include <KoPointerEvent.h>
#include <KoShapeManager.h>
#include <KoToolBase.h>
#include <commands/KoShapeMoveCommand.h>
#include <commands/KoShapeShearCommand.h>
#include <commands/KoShapeTransformCommand.h>

#include <KoSelection.h>
#include <QPointF>

#include <KLocalizedString>
#include <QDebug>
#include <math.h>

ShapeShearStrategy::ShapeShearStrategy(KoToolBase *tool, const QPointF &clicked, KoFlake::SelectionHandle direction)
    : KoInteractionStrategy(tool)
    , m_start(clicked)
{
    // FIXME: This test is also done in DefaultTool, so it should also be responsible for setting the shapes to operate on
    KoSelection *sel = tool->canvas()->shapeManager()->selection();
    QList<KoShape *> selectedShapes = sel->selectedShapes(KoFlake::StrippedSelection);
    foreach (KoShape *shape, selectedShapes) {
        if (shape->allowedInteraction(KoShape::ShearingAllowed, false)) {
            m_selectedShapes << shape;
            m_oldTransforms << shape->transformation();
        }
    }

    m_initialSelectionMatrix = sel->transformation();

    // Eventhoug we aren't currently activated by the corner handles we might as well code like it
    switch (direction) {
    case KoFlake::TopMiddleHandle:
        m_top = true;
        m_bottom = false;
        m_left = false;
        m_right = false;
        break;
    case KoFlake::TopRightHandle:
        m_top = true;
        m_bottom = false;
        m_left = false;
        m_right = true;
        break;
    case KoFlake::RightMiddleHandle:
        m_top = false;
        m_bottom = false;
        m_left = false;
        m_right = true;
        break;
    case KoFlake::BottomRightHandle:
        m_top = false;
        m_bottom = true;
        m_left = false;
        m_right = true;
        break;
    case KoFlake::BottomMiddleHandle:
        m_top = false;
        m_bottom = true;
        m_left = false;
        m_right = false;
        break;
    case KoFlake::BottomLeftHandle:
        m_top = false;
        m_bottom = true;
        m_left = true;
        m_right = false;
        break;
    case KoFlake::LeftMiddleHandle:
        m_top = false;
        m_bottom = false;
        m_left = true;
        m_right = false;
        break;
    case KoFlake::TopLeftHandle:
        m_top = true;
        m_bottom = false;
        m_left = true;
        m_right = false;
        break;
    default:; // throw exception ?  TODO
    }
    m_initialSize = sel->size();
    m_solidPoint = QPointF(m_initialSize.width() / 2, m_initialSize.height() / 2);

    if (m_top)
        m_solidPoint += QPointF(0, m_initialSize.height() / 2);
    else if (m_bottom)
        m_solidPoint -= QPointF(0, m_initialSize.height() / 2);
    if (m_left)
        m_solidPoint += QPointF(m_initialSize.width() / 2, 0);
    else if (m_right)
        m_solidPoint -= QPointF(m_initialSize.width() / 2, 0);

    QPointF edge;
    qreal angle = 0.0;
    if (m_top) {
        edge = sel->absolutePosition(KoFlake::BottomLeftCorner) - sel->absolutePosition(KoFlake::BottomRightCorner);
        angle = 180.0;
    } else if (m_bottom) {
        edge = sel->absolutePosition(KoFlake::TopRightCorner) - sel->absolutePosition(KoFlake::TopLeftCorner);
        angle = 0.0;
    } else if (m_left) {
        edge = sel->absolutePosition(KoFlake::BottomLeftCorner) - sel->absolutePosition(KoFlake::TopLeftCorner);
        angle = 90.0;
    } else if (m_right) {
        edge = sel->absolutePosition(KoFlake::TopRightCorner) - sel->absolutePosition(KoFlake::BottomRightCorner);
        angle = 270.0;
    }
    qreal currentAngle = atan2(edge.y(), edge.x()) / M_PI * 180;
    m_initialSelectionAngle = currentAngle - angle;

    qDebug() << " PREsol.x=" << m_solidPoint.x() << " sol.y=" << m_solidPoint.y();
    m_solidPoint = tool->canvas()->shapeManager()->selection()->absoluteTransformation(nullptr).map(m_solidPoint);

    // use crossproduct of top edge and left edge of selection bounding rect
    // to determine if the selection is mirrored
    QPointF top = sel->absolutePosition(KoFlake::TopRightCorner) - sel->absolutePosition(KoFlake::TopLeftCorner);
    QPointF left = sel->absolutePosition(KoFlake::BottomLeftCorner) - sel->absolutePosition(KoFlake::TopLeftCorner);
    m_isMirrored = (top.x() * left.y() - top.y() * left.x()) < 0.0;
}

void ShapeShearStrategy::handleMouseMove(const QPointF &point, Qt::KeyboardModifiers modifiers)
{
    Q_UNUSED(modifiers);
    QPointF shearVector = point - m_start;

    QTransform m;
    m.rotate(-m_initialSelectionAngle);
    shearVector = m.map(shearVector);

    qreal shearX = 0, shearY = 0;

    if (m_top || m_left)
        shearVector = -shearVector;
    if (m_top || m_bottom)
        shearX = shearVector.x() / m_initialSize.height();
    if (m_left || m_right)
        shearY = shearVector.y() / m_initialSize.width();

    // if selection is mirrored invert the shear values
    if (m_isMirrored) {
        shearX *= -1.0;
        shearY *= -1.0;
    }

    QTransform matrix;
    matrix.translate(m_solidPoint.x(), m_solidPoint.y());
    matrix.rotate(m_initialSelectionAngle);
    matrix.shear(shearX, shearY);
    matrix.rotate(-m_initialSelectionAngle);
    matrix.translate(-m_solidPoint.x(), -m_solidPoint.y());

    QTransform applyMatrix = matrix * m_shearMatrix.inverted();

    foreach (KoShape *shape, m_selectedShapes) {
        shape->update();
        shape->applyAbsoluteTransformation(applyMatrix);
        shape->update();
    }
    tool()->canvas()->shapeManager()->selection()->applyAbsoluteTransformation(applyMatrix);
    m_shearMatrix = matrix;
}

void ShapeShearStrategy::paint(QPainter &painter, const KoViewConverter &converter)
{
    SelectionDecorator decorator(KoFlake::NoHandle, true, false);
    decorator.setSelection(tool()->canvas()->shapeManager()->selection());
    decorator.setHandleRadius(handleRadius());
    decorator.paint(painter, converter);
}

KUndo2Command *ShapeShearStrategy::createCommand()
{
    QVector<QTransform> newTransforms;
    newTransforms.reserve(m_selectedShapes.count());
    foreach (KoShape *shape, m_selectedShapes)
        newTransforms << shape->transformation();
    KoShapeTransformCommand *cmd = new KoShapeTransformCommand(m_selectedShapes, m_oldTransforms, newTransforms);
    cmd->setText(kundo2_i18n("Shear"));
    KoSelection *sel = tool()->canvas()->shapeManager()->selection();
    new SelectionTransformCommand(sel, m_initialSelectionMatrix, sel->transformation(), cmd);
    return cmd;
}
