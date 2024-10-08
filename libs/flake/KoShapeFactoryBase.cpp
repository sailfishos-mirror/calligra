/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006 Boudewijn Rempt (boud@valdyas.org)
 * SPDX-FileCopyrightText: 2006-2007 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2008 C. Boemann <cbo@boemann.dk>
 * SPDX-FileCopyrightText: 2008 Thorsten Zachmann <zachmann@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoShapeFactoryBase.h"

#include "KoDeferredShapeFactoryBase.h"
#include "KoDocumentResourceManager.h"
#include "KoShape.h"
#include "KoShapeLoadingContext.h"

#include <KoOdfLoadingContext.h>
#include <KoPluginLoader.h>
#include <KoProperties.h>
#include <KoStyleStack.h>

#include <KPluginFactory>
#include <QMutex>
#include <QMutexLocker>
#include <QPluginLoader>

#include <FlakeDebug.h>

class Q_DECL_HIDDEN KoShapeFactoryBase::Private
{
public:
    Private(const QString &_id, const QString &_name, const QString &_deferredPluginName)
        : deferredFactory(nullptr)
        , deferredPluginName(_deferredPluginName)
        , id(_id)
        , name(_name)
        , loadingPriority(0)
        , hidden(false)
    {
    }

    ~Private()
    {
        foreach (const KoShapeTemplate &t, templates)
            delete t.properties;
        templates.clear();
    }

    KoDeferredShapeFactoryBase *deferredFactory;
    QMutex pluginLoadingMutex;
    QString deferredPluginName;
    QList<KoShapeTemplate> templates;
    QList<KoShapeConfigFactoryBase *> configPanels;
    const QString id;
    const QString name;
    QString family;
    QString tooltip;
    QString iconName;
    int loadingPriority;
    QList<QPair<QString, QStringList>> xmlElements; // xml name space -> xml element names
    bool hidden;
    QList<KoDocumentResourceManager *> resourceManagers;
};

KoShapeFactoryBase::KoShapeFactoryBase(const QString &id, const QString &name, const QString &deferredPluginName)
    : d(new Private(id, name, deferredPluginName))
{
}

KoShapeFactoryBase::~KoShapeFactoryBase()
{
    delete d;
}

QString KoShapeFactoryBase::toolTip() const
{
    return d->tooltip;
}

QString KoShapeFactoryBase::iconName() const
{
    return d->iconName;
}

QString KoShapeFactoryBase::name() const
{
    return d->name;
}

QString KoShapeFactoryBase::family() const
{
    return d->family;
}

int KoShapeFactoryBase::loadingPriority() const
{
    return d->loadingPriority;
}

QList<QPair<QString, QStringList>> KoShapeFactoryBase::odfElements() const
{
    return d->xmlElements;
}

void KoShapeFactoryBase::addTemplate(const KoShapeTemplate &params)
{
    KoShapeTemplate tmplate = params;
    tmplate.id = d->id;
    d->templates.append(tmplate);
}

void KoShapeFactoryBase::setToolTip(const QString &tooltip)
{
    d->tooltip = tooltip;
}

void KoShapeFactoryBase::setIconName(const QString &iconName)
{
    d->iconName = iconName;
}

void KoShapeFactoryBase::setFamily(const QString &family)
{
    d->family = family;
}

QString KoShapeFactoryBase::id() const
{
    return d->id;
}

void KoShapeFactoryBase::setOptionPanels(const QList<KoShapeConfigFactoryBase *> &panelFactories)
{
    d->configPanels = panelFactories;
}

QList<KoShapeConfigFactoryBase *> KoShapeFactoryBase::panelFactories() const
{
    return d->configPanels;
}

QList<KoShapeTemplate> KoShapeFactoryBase::templates() const
{
    return d->templates;
}

void KoShapeFactoryBase::setLoadingPriority(int priority)
{
    d->loadingPriority = priority;
}

void KoShapeFactoryBase::setXmlElementNames(const QString &nameSpace, const QStringList &names)
{
    d->xmlElements.clear();
    d->xmlElements.append(QPair<QString, QStringList>(nameSpace, names));
}

void KoShapeFactoryBase::setXmlElements(const QList<QPair<QString, QStringList>> &elementNamesList)
{
    d->xmlElements = elementNamesList;
}

bool KoShapeFactoryBase::hidden() const
{
    return d->hidden;
}

void KoShapeFactoryBase::setHidden(bool hidden)
{
    d->hidden = hidden;
}

void KoShapeFactoryBase::newDocumentResourceManager(KoDocumentResourceManager *manager) const
{
    d->resourceManagers.append(manager);
    connect(manager, &QObject::destroyed, this, &KoShapeFactoryBase::pruneDocumentResourceManager);
}

QList<KoDocumentResourceManager *> KoShapeFactoryBase::documentResourceManagers() const
{
    return d->resourceManagers;
}

KoShape *KoShapeFactoryBase::createDefaultShape(KoDocumentResourceManager *documentResources) const
{
    if (!d->deferredPluginName.isEmpty()) {
        const_cast<KoShapeFactoryBase *>(this)->getDeferredPlugin();
        Q_ASSERT(d->deferredFactory);
        if (d->deferredFactory) {
            return d->deferredFactory->createDefaultShape(documentResources);
        }
    }
    return nullptr;
}

KoShape *KoShapeFactoryBase::createShape(const KoProperties *properties, KoDocumentResourceManager *documentResources) const
{
    if (!d->deferredPluginName.isEmpty()) {
        const_cast<KoShapeFactoryBase *>(this)->getDeferredPlugin();
        Q_ASSERT(d->deferredFactory);
        if (d->deferredFactory) {
            return d->deferredFactory->createShape(properties, documentResources);
        }
    }
    return createDefaultShape(documentResources);
}

KoShape *KoShapeFactoryBase::createShapeFromOdf(const KoXmlElement &element, KoShapeLoadingContext &context)
{
    KoShape *shape = createDefaultShape(context.documentResourceManager());
    if (!shape)
        return nullptr;

    if (shape->shapeId().isEmpty())
        shape->setShapeId(id());

    context.odfLoadingContext().styleStack().save();
    bool loaded = shape->loadOdf(element, context);
    context.odfLoadingContext().styleStack().restore();

    if (!loaded) {
        delete shape;
        return nullptr;
    }

    return shape;
}

void KoShapeFactoryBase::getDeferredPlugin()
{
    QMutexLocker lock(&d->pluginLoadingMutex);
    if (d->deferredFactory) {
        return;
    }

    const QList<KPluginFactory *> pluginFactories = KoPluginLoader::instantiatePluginFactories(QStringLiteral("calligra/deferred"));
    Q_ASSERT(pluginFactories.size() > 0);
    for (KPluginFactory *factory : pluginFactories) {
        auto plugin = factory->create<KoDeferredShapeFactoryBase>(this, QVariantList());

        if (plugin && plugin->deferredPluginName() == d->deferredPluginName) {
            d->deferredFactory = plugin;
        } else {
            // not our/valid plugin, so delete the created object
            plugin->deleteLater();
        }
    }
}

void KoShapeFactoryBase::pruneDocumentResourceManager(QObject *obj)
{
    KoDocumentResourceManager *r = qobject_cast<KoDocumentResourceManager *>(obj);
    d->resourceManagers.removeAll(r);
}
