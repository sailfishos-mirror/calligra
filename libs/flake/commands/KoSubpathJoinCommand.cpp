/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006 Jan Hambrecht <jaham@gmx.net>
 * SPDX-FileCopyrightText: 2006, 2007 Thorsten Zachmann <zachmann@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoSubpathJoinCommand.h"
#include <KLocalizedString>

KoSubpathJoinCommand::KoSubpathJoinCommand(const KoPathPointData &pointData1, const KoPathPointData &pointData2, KUndo2Command *parent)
    : KUndo2Command(parent)
    , m_pointData1(pointData1)
    , m_pointData2(pointData2)
    , m_splitIndex(KoPathPointIndex(-1, -1))
    , m_oldProperties1(KoPathPoint::Normal)
    , m_oldProperties2(KoPathPoint::Normal)
    , m_reverse(0)
{
    Q_ASSERT(m_pointData1.pathShape == m_pointData2.pathShape);
    KoPathShape *pathShape = m_pointData1.pathShape;
    Q_ASSERT(!pathShape->isClosedSubpath(m_pointData1.pointIndex.first));
    Q_ASSERT(m_pointData1.pointIndex.second == 0 || m_pointData1.pointIndex.second == pathShape->subpathPointCount(m_pointData1.pointIndex.first) - 1);
    Q_ASSERT(!pathShape->isClosedSubpath(m_pointData2.pointIndex.first));
    Q_ASSERT(m_pointData2.pointIndex.second == 0 || m_pointData2.pointIndex.second == pathShape->subpathPointCount(m_pointData2.pointIndex.first) - 1);
    // TODO check that points are not the same

    if (m_pointData2 < m_pointData1)
        qSwap(m_pointData1, m_pointData2);

    if (m_pointData1.pointIndex.first != m_pointData2.pointIndex.first) {
        if (m_pointData1.pointIndex.second == 0 && pathShape->subpathPointCount(m_pointData1.pointIndex.first) > 1)
            m_reverse |= ReverseFirst;
        if (m_pointData2.pointIndex.second != 0)
            m_reverse |= ReverseSecond;
        setText(kundo2_i18n("Close subpath"));
    } else {
        setText(kundo2_i18n("Join subpaths"));
    }

    KoPathPoint *point1 = pathShape->pointByIndex(m_pointData1.pointIndex);
    KoPathPoint *point2 = pathShape->pointByIndex(m_pointData2.pointIndex);

    m_oldControlPoint1 = QPointF(pathShape->shapeToDocument(m_reverse & 1 ? point1->controlPoint1() : point1->controlPoint2()));
    m_oldControlPoint2 = QPointF(pathShape->shapeToDocument(m_reverse & 2 ? point2->controlPoint1() : point2->controlPoint2()));
    m_oldProperties1 = point1->properties();
    m_oldProperties2 = point2->properties();
}

KoSubpathJoinCommand::~KoSubpathJoinCommand() = default;

void KoSubpathJoinCommand::redo()
{
    KUndo2Command::redo();
    KoPathShape *pathShape = m_pointData1.pathShape;

    bool closeSubpath = m_pointData1.pointIndex.first == m_pointData2.pointIndex.first;

    KoPathPoint *point1 = pathShape->pointByIndex(m_pointData1.pointIndex);
    KoPathPoint *point2 = pathShape->pointByIndex(m_pointData2.pointIndex);

    // if the endpoint is has a control point create a control point for the new segment to be
    // at the symmetric position to the exiting one
    if (m_reverse & ReverseFirst || closeSubpath) {
        if (point1->activeControlPoint2())
            point1->setControlPoint1(2.0 * point1->point() - point1->controlPoint2());
    } else if (point1->activeControlPoint1())
        point1->setControlPoint2(2.0 * point1->point() - point1->controlPoint1());
    if (m_reverse & ReverseSecond || closeSubpath) {
        if (point2->activeControlPoint1())
            point2->setControlPoint2(2.0 * point2->point() - point2->controlPoint1());
    } else if (point2->activeControlPoint2())
        point2->setControlPoint1(2.0 * point2->point() - point2->controlPoint2());

    if (closeSubpath) {
        pathShape->closeSubpath(m_pointData1.pointIndex);
    } else {
        if (m_reverse & ReverseFirst) {
            pathShape->reverseSubpath(m_pointData1.pointIndex.first);
        }
        if (m_reverse & ReverseSecond) {
            pathShape->reverseSubpath(m_pointData2.pointIndex.first);
        }
        pathShape->moveSubpath(m_pointData2.pointIndex.first, m_pointData1.pointIndex.first + 1);
        m_splitIndex = m_pointData1.pointIndex;
        m_splitIndex.second = pathShape->subpathPointCount(m_pointData1.pointIndex.first) - 1;
        pathShape->join(m_pointData1.pointIndex.first);
    }
    pathShape->normalize();
    pathShape->update();
}

void KoSubpathJoinCommand::undo()
{
    KUndo2Command::undo();
    KoPathShape *pathShape = m_pointData1.pathShape;
    pathShape->update();
    if (m_pointData1.pointIndex.first == m_pointData2.pointIndex.first) {
        pathShape->openSubpath(m_pointData1.pointIndex);
    } else {
        pathShape->breakAfter(m_splitIndex);
        pathShape->moveSubpath(m_pointData1.pointIndex.first + 1, m_pointData2.pointIndex.first);

        if (m_reverse & ReverseSecond) {
            pathShape->reverseSubpath(m_pointData2.pointIndex.first);
        }
        if (m_reverse & ReverseFirst) {
            pathShape->reverseSubpath(m_pointData1.pointIndex.first);
        }
    }
    KoPathPoint *point1 = pathShape->pointByIndex(m_pointData1.pointIndex);
    KoPathPoint *point2 = pathShape->pointByIndex(m_pointData2.pointIndex);

    // restore the old end points
    if (m_reverse & ReverseFirst)
        point1->setControlPoint1(pathShape->documentToShape(m_oldControlPoint1));
    else
        point1->setControlPoint2(pathShape->documentToShape(m_oldControlPoint1));

    point1->setProperties(m_oldProperties1);

    if (m_reverse & ReverseSecond)
        point2->setControlPoint1(pathShape->documentToShape(m_oldControlPoint2));
    else
        point2->setControlPoint2(pathShape->documentToShape(m_oldControlPoint2));

    point2->setProperties(m_oldProperties2);

    pathShape->normalize();
    pathShape->update();
}
