/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2008-2009 Jan Hambrecht <jaham@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KOSNAPSTRATEGY_H
#define KOSNAPSTRATEGY_H

#include "KoSnapGuide.h"

#include <QLineF>
#include <QVector>

class TestSnapStrategy;
class KoPathPoint;
class KoSnapProxy;
class KoViewConverter;

class QTransform;
class QPainterPath;

class FLAKE_EXPORT KoSnapStrategy
{
public:
    explicit KoSnapStrategy(KoSnapGuide::Strategy type);
    virtual ~KoSnapStrategy() = default;

    virtual bool snap(const QPointF &mousePosition, KoSnapProxy *proxy, qreal maxSnapDistance) = 0;

    /// returns the strategies type
    KoSnapGuide::Strategy type() const;

    static qreal squareDistance(const QPointF &p1, const QPointF &p2);
    static qreal scalarProduct(const QPointF &p1, const QPointF &p2);

    /// returns the snapped position form the last call to snapToPoints
    QPointF snappedPosition() const;

    /// returns the current snap strategy decoration
    virtual QPainterPath decoration(const KoViewConverter &converter) const = 0;

protected:
    /// sets the current snapped position
    void setSnappedPosition(const QPointF &position);

private:
    KoSnapGuide::Strategy m_snapType;
    QPointF m_snappedPosition;
};

/// snaps to x- or y-coordinates of path points
class FLAKE_EXPORT OrthogonalSnapStrategy : public KoSnapStrategy
{
public:
    OrthogonalSnapStrategy();
    bool snap(const QPointF &mousePosition, KoSnapProxy *proxy, qreal maxSnapDistance) override;
    QPainterPath decoration(const KoViewConverter &converter) const override;

private:
    QLineF m_hLine;
    QLineF m_vLine;
};

/// snaps to path points
class FLAKE_EXPORT NodeSnapStrategy : public KoSnapStrategy
{
public:
    NodeSnapStrategy();
    bool snap(const QPointF &mousePosition, KoSnapProxy *proxy, qreal maxSnapDistance) override;
    QPainterPath decoration(const KoViewConverter &converter) const override;
};

/// snaps extension lines of path shapes
class FLAKE_EXPORT ExtensionSnapStrategy : public KoSnapStrategy
{
    friend class TestSnapStrategy;

public:
    ExtensionSnapStrategy();
    bool snap(const QPointF &mousePosition, KoSnapProxy *proxy, qreal maxSnapDistance) override;
    QPainterPath decoration(const KoViewConverter &converter) const override;

private:
    qreal project(const QPointF &lineStart, const QPointF &lineEnd, const QPointF &point);
    QPointF extensionDirection(KoPathPoint *point, const QTransform &matrix);
    bool snapToExtension(QPointF &position, KoPathPoint *point, const QTransform &matrix);
    QVector<QLineF> m_lines;
};

/// snaps to intersections of shapes
class FLAKE_EXPORT IntersectionSnapStrategy : public KoSnapStrategy
{
public:
    IntersectionSnapStrategy();
    bool snap(const QPointF &mousePosition, KoSnapProxy *proxy, qreal maxSnapDistance) override;
    QPainterPath decoration(const KoViewConverter &converter) const override;
};

/// snaps to the canvas grid
class FLAKE_EXPORT GridSnapStrategy : public KoSnapStrategy
{
public:
    GridSnapStrategy();
    bool snap(const QPointF &mousePosition, KoSnapProxy *proxy, qreal maxSnapDistance) override;
    QPainterPath decoration(const KoViewConverter &converter) const override;
};

/// snaps to shape bounding boxes
class FLAKE_EXPORT BoundingBoxSnapStrategy : public KoSnapStrategy
{
    friend class TestSnapStrategy;

public:
    BoundingBoxSnapStrategy();
    bool snap(const QPointF &mousePosition, KoSnapProxy *proxy, qreal maxSnapDistance) override;
    QPainterPath decoration(const KoViewConverter &converter) const override;

private:
    qreal squareDistanceToLine(const QPointF &lineA, const QPointF &lineB, const QPointF &point, QPointF &pointOnLine);
    QPointF m_boxPoints[5];
};

/// snaps to line guides
class FLAKE_EXPORT LineGuideSnapStrategy : public KoSnapStrategy
{
public:
    LineGuideSnapStrategy();
    bool snap(const QPointF &mousePosition, KoSnapProxy *proxy, qreal maxSnapDistance) override;
    QPainterPath decoration(const KoViewConverter &converter) const override;

private:
    int m_orientation;
};

#endif // KOSNAPSTRATEGY_H
