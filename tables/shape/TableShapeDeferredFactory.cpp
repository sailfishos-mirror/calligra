/* This file is part of the KDE project
   Copyright 2006 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Local
#include "TableShapeDeferredFactory.h"

#include <QStringList>
#include <QSharedPointer>

#include <kpluginfactory.h>
#include <klocale.h>

#include <KoResourceManager.h>
#include <KoToolRegistry.h>
#include <KoShapeRegistry.h>
#include <KoShapeLoadingContext.h>
#include <KoXmlNS.h>

#include <Map.h>

#include "TableShape.h"
#include "TableToolFactory.h"
#include "TableShapeFactory.h"

using namespace Calligra::Tables;

K_PLUGIN_FACTORY(TableShapePluginFactory, registerPlugin<TableDeferredShapeFactory>();)
K_EXPORT_PLUGIN(TableShapePluginFactory("TableShape"))

TableDeferredShapeFactory::TableDeferredShapeFactory(QObject *parent, const QVariantList&)
        : KoDeferredShapeFactoryBase(parent)
{
    qDebug() << ">>>>>>>>>>>>>>> creating tableshape deferred plugin";
    // only create the tool when this plugin gets loaded.
    KoToolRegistry::instance()->add(new TableToolFactory());
}

TableDeferredShapeFactory::~TableDeferredShapeFactory()
{
}

KoShape *TableDeferredShapeFactory::createDefaultShape(KoResourceManager *documentResources) const
{
    TableShape *shape = new TableShape();
    shape->setShapeId(TableShapeId);
    if (documentResources) {
        Q_ASSERT(documentResources->hasResource(MapResourceId));
        Map *map = static_cast<Map*>(documentResources->resource(MapResourceId).value<void*>());
        shape->setMap(map);
    }
    return shape;
}

void TableDeferredShapeFactory::newDocumentResourceManager(KoResourceManager *manager)
{
    if (manager->hasResource(MapResourceId)) return;
    // One spreadsheet map for all inserted tables to allow referencing cells among them.
    QVariant variant;
    Map* map = new Map();
    // Make the KoResourceManager manage this Map, since we cannot delete it ourselves
    map->setParent(manager);
    QObject::connect(manager, SIGNAL(destroyed()), map, SLOT(deleteLater()));
    variant.setValue<void*>(map);
    manager->setResource(MapResourceId, variant);
}
