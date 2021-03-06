/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2007 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2016 Friedrich W. H. Kossebau <kossebau@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoPluginLoader.h"

#include <KConfig>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KPluginFactory>
#include <KPluginLoader>

#include <QCoreApplication>
#include <QJsonObject>
#include <QPluginLoader>
#include <QLoggingCategory>
#include <QDebug>


const QLoggingCategory &PLUGIN_LOG()
{
    static const QLoggingCategory category("calligra.lib.plugin");
    return category;
}

#define debugPlugin qCDebug(PLUGIN_LOG)
#define warnPlugin qCWarning(PLUGIN_LOG)


class KoPluginLoaderImpl : public QObject
{
Q_OBJECT
public:
    QStringList loadedDirectories;
};



Q_GLOBAL_STATIC(KoPluginLoaderImpl, pluginLoaderInstance)


void KoPluginLoader::load(const QString & directory, const PluginsConfig &config, QObject* owner)
{
    // Don't load the same plugins again
    if (pluginLoaderInstance->loadedDirectories.contains(directory)) {
        return;
    }
    pluginLoaderInstance->loadedDirectories << directory;

    QList<QPluginLoader *> offers = KoPluginLoader::pluginLoaders(directory);
    QList<QPluginLoader *> plugins;
    bool configChanged = false;
    QList<QString> blacklist; // what we will save out afterwards
    if (config.whiteList && config.blacklist && config.group) {
        debugPlugin << "Loading" << directory << "with checking the config";
        KConfigGroup configGroup(KSharedConfig::openConfig(), config.group);
        QList<QString> whiteList = configGroup.readEntry(config.whiteList, config.defaults);
        QList<QString> knownList;

        // if there was no list of defaults; all plugins are loaded.
        const bool firstStart = !config.defaults.isEmpty() && !configGroup.hasKey(config.whiteList);
        knownList = configGroup.readEntry(config.blacklist, knownList);
        if (firstStart) {
            configChanged = true;
        }
        foreach(QPluginLoader *loader, offers) {
            QJsonObject json = loader->metaData().value("MetaData").toObject();
            json = json.value("KPlugin").toObject();
            const QString pluginName = json.value("Id").toString();
            if (pluginName.isEmpty()) {
                warnPlugin << "Loading plugin" << loader->fileName() << "failed, has no X-KDE-PluginInfo-Name.";
                continue;
            }
            if (whiteList.contains(pluginName)) {
                plugins.append(loader);
            } else if (!firstStart && !knownList.contains(pluginName)) { // also load newly installed plugins.
                plugins.append(loader);
                configChanged = true;
            } else {
                blacklist << pluginName;
            }
        }
    } else {
        plugins = offers;
    }

    QMap<QString, QPluginLoader *> serviceNames;
    foreach(QPluginLoader *loader, plugins) {
        if (serviceNames.contains(loader->fileName())) { // duplicate
            QJsonObject json2 = loader->metaData().value("MetaData").toObject();
            QVariant pluginVersion2 = json2.value("X-Flake-PluginVersion").toVariant();
            if (pluginVersion2.isNull()) { // just take the first one found...
                continue;
            }
            QPluginLoader *currentLoader = serviceNames.value(loader->fileName());
            QJsonObject json = currentLoader->metaData().value("MetaData").toObject();
            QVariant pluginVersion = json.value("X-Flake-PluginVersion").toVariant();
            if (!(pluginVersion.isNull() || pluginVersion.toInt() < pluginVersion2.toInt())) {
                continue; // replace the old one with this one, since its newer.
            }
        }
        serviceNames.insert(loader->fileName(), loader);
    }

    QList<QString> whiteList;
    foreach(QPluginLoader *loader, serviceNames) {
        KPluginFactory *factory = qobject_cast<KPluginFactory *>(loader->instance());
        QObject *plugin = factory ? factory->create<QObject>(owner ? owner : pluginLoaderInstance, QVariantList()) : nullptr;
        if (plugin) {
            QJsonObject json = loader->metaData().value("MetaData").toObject();
            json = json.value("KPlugin").toObject();
            const QString pluginName = json.value("Id").toString();
            whiteList << pluginName;
            debugPlugin << "Loaded plugin" << loader->fileName() << owner;
            if (!owner) {
                delete plugin;
            }
        } else {
            warnPlugin << "Loading plugin" << loader->fileName() << "failed, " << loader->errorString();
        }
    }

    if (configChanged && config.whiteList && config.blacklist && config.group) {
        KConfigGroup configGroup(KSharedConfig::openConfig(), config.group);
        configGroup.writeEntry(config.whiteList, whiteList);
        configGroup.writeEntry(config.blacklist, blacklist);
    }

    qDeleteAll(offers);
}

QList<KPluginFactory *> KoPluginLoader::instantiatePluginFactories(const QString & directory)
{
    QList<KPluginFactory *> pluginFactories;

    const QList<QPluginLoader *> offers = KoPluginLoader::pluginLoaders(directory);

    foreach(QPluginLoader *pluginLoader, offers) {
        QObject* pluginInstance = pluginLoader->instance();
        if (!pluginInstance) {
            warnPlugin << "Loading plugin" << pluginLoader->fileName() << "failed, " << pluginLoader->errorString();
            continue;
        }
        KPluginFactory *factory = qobject_cast<KPluginFactory *>(pluginInstance);
        if (factory == 0) {
            warnPlugin << "Expected a KPluginFactory, got a" << pluginInstance->metaObject()->className();
            delete pluginInstance;
            continue;
        }

        pluginFactories.append(factory);
    }
    qDeleteAll(offers);

    return pluginFactories;
}

QList<QPluginLoader *> KoPluginLoader::pluginLoaders(const QString &directory, const QString &mimeType)
{
    QList<QPluginLoader *>list;
    KPluginLoader::forEachPlugin(directory, [&](const QString &pluginPath) {
        debugPlugin << "Trying to load" << pluginPath;
        QPluginLoader *loader = new QPluginLoader(pluginPath);
        QJsonObject metaData = loader->metaData().value("MetaData").toObject();

        if (metaData.isEmpty()) {
            debugPlugin << pluginPath << "has no MetaData!";
            return;
        }

        if (!mimeType.isEmpty()) {
            QJsonObject pluginData = metaData.value("KPlugin").toObject();
            QStringList mimeTypes = pluginData.value("MimeTypes").toVariant().toStringList();
            mimeTypes += metaData.value("X-KDE-ExtraNativeMimeTypes").toVariant().toStringList();
            mimeTypes += metaData.value("X-KDE-NativeMimeType").toString();
            if (! mimeTypes.contains(mimeType)) {
                return;
            }
        }

        list.append(loader);
    });

    return list;
}
#include "KoPluginLoader.moc"
