/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006, 2008 Jan Hambrecht <jaham@gmx.net>
 * SPDX-FileCopyrightText: 2006, 2007 Thorsten Zachmann <zachmann@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoPathPointRemoveCommand.h"
#include "KoPathPoint.h"
#include "KoShapeController.h"
#include "KoSubpathRemoveCommand.h"
#include <KLocalizedString>

class KoPathPointRemoveCommandPrivate
{
public:
    KoPathPointRemoveCommandPrivate()
        : deletePoints(false)
    {
    }
    ~KoPathPointRemoveCommandPrivate()
    {
        if (deletePoints)
            qDeleteAll(points);
    }
    QList<KoPathPointData> pointDataList;
    QList<KoPathPoint *> points;
    bool deletePoints;
};

KUndo2Command *KoPathPointRemoveCommand::createCommand(const QList<KoPathPointData> &pointDataList, KoShapeController *shapeController, KUndo2Command *parent)
{
    /*
     * We want to decide if we have to:
     * 1. delete only some points of a path or
     * 2. delete one or more complete subpath or
     * 3. delete a complete path
     */

    QList<KoPathPointData> sortedPointData(pointDataList);
    std::sort(sortedPointData.begin(), sortedPointData.end());

    KoPathPointData last(nullptr, KoPathPointIndex(-1, -1));
    // add last at the end so that the point date before last will also be put in
    // the right places.
    sortedPointData.append(last);

    QList<KoPathPointData> pointsOfSubpath; // points of current subpath
    QList<KoPathPointData> subpathsOfPath; // subpaths of current path
    QList<KoPathPointData> pointsToDelete; // single points to delete
    QList<KoPathPointData> subpathToDelete; // single subpaths to delete
    QList<KoShape *> shapesToDelete; // single paths to delete

    last = sortedPointData.first();

    QList<KoPathPointData>::const_iterator it(sortedPointData.constBegin());
    for (; it != sortedPointData.constEnd(); ++it) {
        // check if we have come to the next subpath of the same or another path
        if (last.pathShape != it->pathShape || last.pointIndex.first != it->pointIndex.first) {
            // check if all points of the last subpath should be deleted
            if (last.pathShape->subpathPointCount(last.pointIndex.first) == pointsOfSubpath.size()) {
                // all points of subpath to be deleted -> mark subpath as to be deleted
                subpathsOfPath.append(pointsOfSubpath.first());
            } else {
                // not all points of subpath to be deleted -> add them to the delete point list
                pointsToDelete += pointsOfSubpath;
            }
            // clear the suboath point list
            pointsOfSubpath.clear();
        }

        // check if we have come to the next shape
        if (last.pathShape != it->pathShape) {
            // check if all subpath of the shape should be deleted
            if (last.pathShape->subpathCount() == subpathsOfPath.size()) {
                // all subpaths of path to be deleted -> add shape to delete shape list
                shapesToDelete.append(last.pathShape);
            } else {
                // not all subpaths of path to be deleted -> add them to delete subpath list
                subpathToDelete += subpathsOfPath;
            }
            subpathsOfPath.clear();
        }
        if (!it->pathShape)
            continue;
        // keep reference to last point
        last = *it;
        // add this point to the current subpath point list
        pointsOfSubpath.append(*it);
    }

    KUndo2Command *cmd = new KUndo2Command(kundo2_i18n("Remove points"), parent);

    if (pointsToDelete.size() > 0) {
        new KoPathPointRemoveCommand(pointsToDelete, cmd);
    }
    foreach (const KoPathPointData &pd, subpathToDelete) {
        new KoSubpathRemoveCommand(pd.pathShape, pd.pointIndex.first, cmd);
    }
    if (shapesToDelete.size() > 0) {
        shapeController->removeShapes(shapesToDelete, cmd);
    }

    return cmd;
}

KoPathPointRemoveCommand::KoPathPointRemoveCommand(const QList<KoPathPointData> &pointDataList, KUndo2Command *parent)
    : KUndo2Command(parent)
    , d(new KoPathPointRemoveCommandPrivate())
{
    QList<KoPathPointData>::const_iterator it(pointDataList.begin());
    for (; it != pointDataList.end(); ++it) {
        KoPathPoint *point = it->pathShape->pointByIndex(it->pointIndex);
        if (point) {
            d->pointDataList.append(*it);
            d->points.append(nullptr);
        }
    }
    std::sort(d->pointDataList.begin(), d->pointDataList.end());
    setText(kundo2_i18n("Remove points"));
}

KoPathPointRemoveCommand::~KoPathPointRemoveCommand()
{
    delete d;
}

void KoPathPointRemoveCommand::redo()
{
    KUndo2Command::redo();
    KoPathShape *lastPathShape = nullptr;
    int updateBefore = d->pointDataList.size();
    for (int i = d->pointDataList.size() - 1; i >= 0; --i) {
        const KoPathPointData &pd = d->pointDataList.at(i);
        pd.pathShape->update();
        d->points[i] = pd.pathShape->removePoint(pd.pointIndex);

        if (lastPathShape != pd.pathShape) {
            if (lastPathShape) {
                QPointF offset = lastPathShape->normalize();

                QTransform matrix;
                matrix.translate(-offset.x(), -offset.y());
                for (int j = i + 1; j < updateBefore; ++j) {
                    d->points.at(j)->map(matrix);
                }
                lastPathShape->update();
                updateBefore = i + 1;
            }
            lastPathShape = pd.pathShape;
        }
    }

    if (lastPathShape) {
        QPointF offset = lastPathShape->normalize();

        QTransform matrix;
        matrix.translate(-offset.x(), -offset.y());
        for (int j = 0; j < updateBefore; ++j) {
            d->points.at(j)->map(matrix);
        }
        lastPathShape->update();
    }

    d->deletePoints = true;
}

void KoPathPointRemoveCommand::undo()
{
    KUndo2Command::undo();
    KoPathShape *lastPathShape = nullptr;
    for (int i = 0; i < d->pointDataList.size(); ++i) {
        const KoPathPointData &pd = d->pointDataList.at(i);
        if (lastPathShape && lastPathShape != pd.pathShape) {
            lastPathShape->normalize();
            lastPathShape->update();
        }
        pd.pathShape->insertPoint(d->points[i], pd.pointIndex);
        lastPathShape = pd.pathShape;
    }
    if (lastPathShape) {
        lastPathShape->normalize();
        lastPathShape->update();
    }
    d->deletePoints = false;
}
