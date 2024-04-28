/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Thomas Zander <zander@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "Plugin.h"
#include "snapguidedocker/SnapGuideDockerFactory.h"
#include "shapeproperties/ShapePropertiesDockerFactory.h"
#include "shapecollection/ShapeCollectionDocker.h"

#include <KoDockRegistry.h>

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(PluginFactory, "calligra_docker_defaults.json",
                           registerPlugin<Plugin>();)

Plugin::Plugin(QObject *parent, const QVariantList&)
    : QObject(parent)
{
    Q_UNUSED(parent);
    KoDockRegistry::instance()->add(new SnapGuideDockerFactory());
    KoDockRegistry::instance()->add(new ShapePropertiesDockerFactory());
    KoDockRegistry::instance()->add(new ShapeCollectionDockerFactory());
}

#include <Plugin.moc>

