/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2008-2009 Thorsten Zachmann <zachmann@kde.org>
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

#include "KPrPlaceholderTool.h"

#include <QPainter>

#include <KLocalizedString>

#include <kundo2command.h>

#include <KoCanvasBase.h>
#include <KoSelection.h>
#include <KoShapeController.h>
#include <KoShapeManager.h>
#include <KoToolManager.h>

#include "KPrPlaceholderShape.h"

KPrPlaceholderTool::KPrPlaceholderTool(KoCanvasBase *canvas)
    : KoToolBase(canvas)
{
}

KPrPlaceholderTool::~KPrPlaceholderTool() = default;

void KPrPlaceholderTool::paint(QPainter & /*painter*/, const KoViewConverter & /*converter*/)
{
}

void KPrPlaceholderTool::mousePressEvent(KoPointerEvent *event)
{
    Q_UNUSED(event);
}

void KPrPlaceholderTool::mouseMoveEvent(KoPointerEvent *event)
{
    Q_UNUSED(event);
}

void KPrPlaceholderTool::mouseReleaseEvent(KoPointerEvent *event)
{
    Q_UNUSED(event);
}

void KPrPlaceholderTool::activate(ToolActivation toolActivation, const QSet<KoShape *> &shapes)
{
    Q_UNUSED(toolActivation);
    QList<KPrPlaceholderShape *> selectedShapes;

    foreach (KoShape *shape, shapes) {
        if (KPrPlaceholderShape *ps = dynamic_cast<KPrPlaceholderShape *>(shape)) {
            selectedShapes.append(ps);
        }
    }

    if (selectedShapes.isEmpty()) {
        Q_EMIT done();
        return;
    }

    KPrPlaceholderShape *shape = selectedShapes.at(0);

    KoShape *newShape = shape->createShape(canvas()->shapeController()->resourceManager());
    // only do anything when we got a shape back
    if (newShape) {
        // copy settings from placeholder shape
        newShape->setParent(shape->parent());
        newShape->setZIndex(shape->zIndex());
        newShape->setSize(shape->size());
        newShape->setPosition(shape->position());
        newShape->setAdditionalAttribute("presentation:class", shape->additionalAttribute("presentation:class"));

        KUndo2Command *cmd = new KUndo2Command(kundo2_i18n("Edit Shape"));

        // replace placeholder by shape
        canvas()->shapeController()->removeShape(shape, cmd);
        canvas()->shapeController()->addShapeDirect(newShape, cmd);
        canvas()->addCommand(cmd);

        // activate the correct tool for the shape
        QList<KoShape *> shapes;
        shapes.append(newShape);
        canvas()->shapeManager()->selection()->select(newShape);
        Q_EMIT activateTool(KoToolManager::instance()->preferredToolForSelection(shapes));
    } else {
        Q_EMIT done();
    }
}
