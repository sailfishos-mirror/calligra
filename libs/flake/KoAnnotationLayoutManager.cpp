/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2013 Mojtaba Shahi Senobari <mojtaba.shahi3000@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include <algorithm>

#include "KoAnnotationLayoutManager.h"

#include <KoCanvasBase.h>
#include <KoShape.h>
#include <KoShapeManager.h>
#include <KoViewConverter.h>
#include <QHash>
#include <QLine>
#include <QLineF>
#include <QList>
#include <QMap>
#include <QPainter>
#include <QPen>
#include <QWidget>
#include <QtAlgorithms>

#include <FlakeDebug.h>

int compare(const QPair<QPointF, KoShape *> &a, const QPair<QPointF, KoShape *> &b)
{
    if (a.first.y() == b.first.y()) {
        return a.first.x() < b.first.x();
    }
    return a.first.y() < b.first.y();
}

class Q_DECL_HIDDEN KoAnnotationLayoutManager::Private
{
public:
    Private()
        : shapeManager(nullptr)
        , canvas(nullptr)
    {
    }

    qreal x; // The x point of annotation shapes position.
    QList<QPair<QPointF, KoShape *>> annotationShapePositions;
    KoShapeManager *shapeManager;
    KoCanvasBase *canvas;
};

KoAnnotationLayoutManager::KoAnnotationLayoutManager(QObject *parent)
    : QObject(parent)
    , d(new Private())
{
}

KoAnnotationLayoutManager::~KoAnnotationLayoutManager()
{
    delete d;
}

void KoAnnotationLayoutManager::setShapeManager(KoShapeManager *shapeManager)
{
    if (d->shapeManager) {
        disconnect(d->shapeManager, &KoShapeManager::shapeChanged, this, &KoAnnotationLayoutManager::updateLayout);
    }
    d->shapeManager = shapeManager;
    connect(d->shapeManager, &KoShapeManager::shapeChanged, this, &KoAnnotationLayoutManager::updateLayout);
}

void KoAnnotationLayoutManager::setCanvasBase(KoCanvasBase *canvas)
{
    d->canvas = canvas;
}

void KoAnnotationLayoutManager::setViewContentWidth(qreal width)
{
    d->x = width;
}

void KoAnnotationLayoutManager::paintConnections(QPainter &painter)
{
    painter.save();

    QPen pen(QColor(230, 216, 87)); // Color of lines.
    pen.setStyle(Qt::DashLine);
    pen.setWidth(2);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);

    painter.setPen(pen);

    QList<QPair<QPointF, KoShape *>>::const_iterator it = d->annotationShapePositions.constBegin();
    while (it != d->annotationShapePositions.constEnd() && !d->annotationShapePositions.isEmpty()) {
        KoShape *shape = it->second;

        // To make it more beautiful start line from shape a little downer of top-left of annotation shape (shape->position().y() + 20.0).
        QPointF shapePosition(d->canvas->viewConverter()->documentToView(QPointF(shape->position().x(), (shape->position().y() + 20.0))));
        QPointF refTextPosition(d->canvas->viewConverter()->documentToView(QPointF(it->first.x(), (it->first.y()))));
        QPointF connectionPoint(d->canvas->viewConverter()->documentToView(QPointF((shape->position().x() - connectionPointLines), (it->first.y()))));
        QPointF pointLine(d->canvas->viewConverter()->documentToView(QPointF(it->first.x(), (it->first.y() + 5))));

        // Draw first line, from shape to connection Point.
        painter.drawLine(shapePosition, connectionPoint);
        // Draw second line, from connection point to reftext position.
        painter.drawLine(connectionPoint, refTextPosition);
        // Draw pointer.
        painter.drawLine(refTextPosition, pointLine);

        it++;
    }
    painter.restore();
}

bool KoAnnotationLayoutManager::isAnnotationShape(KoShape *shape) const
{
    QList<QPair<QPointF, KoShape *>>::ConstIterator it = d->annotationShapePositions.constBegin();
    while (it != d->annotationShapePositions.constEnd()) {
        if (shape == it->second) {
            return true;
        }
        ++it;
    }
    return false;
}

void KoAnnotationLayoutManager::registerAnnotationRefPosition(KoShape *annotationShape, const QPointF &pos)
{
    QList<QPair<QPointF, KoShape *>>::iterator it = d->annotationShapePositions.begin();
    bool yPositionChanged = false;
    while (it != d->annotationShapePositions.end()) {
        KoShape *shape = it->second;
        if (shape == annotationShape) {
            if (it->first.y() != pos.y()) {
                yPositionChanged = true;
            }
            d->annotationShapePositions.erase(it);
            break;
        }
        ++it;
    }
    if (d->annotationShapePositions.isEmpty()) {
        Q_EMIT hasAnnotationsChanged(true);
    }
    d->annotationShapePositions.append(QPair<QPointF, KoShape *>(pos, annotationShape));
    layoutAnnotationShapes();
    if (d->canvas && yPositionChanged) {
        d->canvas->canvasWidget()->update();
    }
}

void KoAnnotationLayoutManager::removeAnnotationShape(KoShape *annotationShape)
{
    QList<QPair<QPointF, KoShape *>>::iterator it = d->annotationShapePositions.begin();
    while (it != d->annotationShapePositions.end()) {
        if (it->second == annotationShape) {
            d->annotationShapePositions.erase(it);
            break;
        }
        ++it;
    }
    layoutAnnotationShapes();
    if (d->annotationShapePositions.isEmpty()) {
        Q_EMIT hasAnnotationsChanged(false);
    }
    // Should update canvas.
    d->canvas->canvasWidget()->update();
}

void KoAnnotationLayoutManager::updateLayout(KoShape *shape)
{
    QList<QPair<QPointF, KoShape *>>::const_iterator it = d->annotationShapePositions.constBegin();
    while (it != d->annotationShapePositions.constEnd() && !d->annotationShapePositions.isEmpty()) {
        if (it->second == shape) {
            layoutAnnotationShapes();
            break;
        }
        ++it;
    }
}

void KoAnnotationLayoutManager::layoutAnnotationShapes()
{
    qreal currentY = 0.0;
    std::stable_sort(d->annotationShapePositions.begin(), d->annotationShapePositions.end(), compare);

    QList<QPair<QPointF, KoShape *>>::const_iterator it = d->annotationShapePositions.constBegin();
    while (it != d->annotationShapePositions.constEnd()) {
        KoShape *shape = it->second;
        qreal refPosition = it->first.y();
        if (refPosition > currentY) {
            currentY = refPosition;
        }
        shape->update();
        shape->setSize(QSize(shapeWidth, shape->size().height()));
        shape->setPosition(QPointF(d->x, currentY));

        // This makes the shape visible. It was set to invisible when it was created.
        shape->setVisible(true);
        shape->update();
        currentY += shape->size().height() + shapeSpace;

        ++it;
    }
}

// only static const integral data members can be initialized within a class
const qreal KoAnnotationLayoutManager::shapeSpace = 10.0; // Distance between annotation shapes.
const qreal KoAnnotationLayoutManager::shapeWidth = 200.0; // Annotation shapes width.
const qreal KoAnnotationLayoutManager::connectionPointLines =
    50.0; // Connection point of lines from shape to this point and from this point to refText position.
