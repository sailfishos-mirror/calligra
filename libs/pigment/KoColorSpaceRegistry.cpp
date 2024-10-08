/*
 *  SPDX-FileCopyrightText: 2003 Patrick Julien <freak@codepimps.org>
 *  SPDX-FileCopyrightText: 2004, 2010 Cyrille Berger <cberger@cberger.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "KoColorSpaceRegistry.h"

#include <QHash>

#include <QDir>
#include <QGlobalStatic>
#include <QReadWriteLock>
#include <QStringList>

#include "DebugPigment.h"
#include "KoBasicHistogramProducers.h"
#include "KoColorConversionCache.h"
#include "KoColorConversionSystem.h"
#include "KoColorProfile.h"
#include "KoColorSpace.h"
#include "KoGenericRegistry.h"
#include "KoPluginLoader.h"

#include "KoColorSpace_p.h"
#include "colorspaces/KoAlphaColorSpace.h"
#include "colorspaces/KoLabColorSpace.h"
#include "colorspaces/KoRgbU16ColorSpace.h"
#include "colorspaces/KoRgbU8ColorSpace.h"
#include "colorspaces/KoSimpleColorSpaceEngine.h"

Q_GLOBAL_STATIC(KoColorSpaceRegistry, s_instance)

struct Q_DECL_HIDDEN KoColorSpaceRegistry::Private {
    KoGenericRegistry<KoColorSpaceFactory *> colorSpaceFactoryRegistry;
    QList<KoColorSpaceFactory *> localFactories;
    QHash<QString, KoColorProfile *> profileMap;
    QHash<QString, QString> profileAlias;
    QHash<QString, const KoColorSpace *> csMap;
    KoColorConversionSystem *colorConversionSystem;
    KoColorConversionCache *colorConversionCache;
    const KoColorSpace *rgbU8sRGB;
    const KoColorSpace *lab16sLAB;
    const KoColorSpace *alphaCs;
    QReadWriteLock registrylock;
};

KoColorSpaceRegistry *KoColorSpaceRegistry::instance()
{
    if (!s_instance.exists()) {
        s_instance->init();
    }
    return s_instance;
}

void KoColorSpaceRegistry::init()
{
    d->rgbU8sRGB = nullptr;
    d->lab16sLAB = nullptr;
    d->alphaCs = nullptr;

    d->colorConversionSystem = new KoColorConversionSystem;
    d->colorConversionCache = new KoColorConversionCache;

    KoColorSpaceEngineRegistry::instance()->add(new KoSimpleColorSpaceEngine());

    addProfile(new KoDummyColorProfile);

    // Create the built-in colorspaces
    d->localFactories << new KoLabColorSpaceFactory() << new KoRgbU8ColorSpaceFactory() << new KoRgbU16ColorSpaceFactory();
    foreach (KoColorSpaceFactory *factory, d->localFactories) {
        add(factory);
    }

    d->alphaCs = new KoAlphaColorSpace();
    d->alphaCs->d->deletability = OwnedByRegistryRegistryDeletes;

    KoPluginLoader::PluginsConfig config;
    config.whiteList = "ColorSpacePlugins";
    config.blacklist = "ColorSpacePluginsDisabled";
    config.group = "calligra";
    KoPluginLoader::load(QStringLiteral("calligra/colorspaces"), config);

    KoPluginLoader::PluginsConfig configExtensions;
    configExtensions.whiteList = "ColorSpaceExtensionsPlugins";
    configExtensions.blacklist = "ColorSpaceExtensionsPluginsDisabled";
    configExtensions.group = "calligra";
    KoPluginLoader::load(QStringLiteral("calligra/colorspaceextensions"), configExtensions);

    dbgPigment << "Loaded the following colorspaces:";
    foreach (const KoID &id, listKeys()) {
        dbgPigment << "\t" << id.id() << "," << id.name();
    }
}

KoColorSpaceRegistry::KoColorSpaceRegistry()
    : d(new Private())
{
    d->colorConversionSystem = nullptr;
    d->colorConversionCache = nullptr;
}

KoColorSpaceRegistry::~KoColorSpaceRegistry()
{
    // Just leak on exit... It's faster.
    //    delete d->colorConversionSystem;
    //    foreach(KoColorProfile* profile, d->profileMap) {
    //        delete profile;
    //    }
    //    d->profileMap.clear();

    //    foreach(const KoColorSpace * cs, d->csMap) {
    //        cs->d->deletability = OwnedByRegistryRegistryDeletes;
    //    }
    //    d->csMap.clear();

    //    // deleting colorspaces calls a function in the cache
    //    delete d->colorConversionCache;
    //    d->colorConversionCache = 0;

    //    // Delete the colorspace factories
    //    qDeleteAll(d->localFactories);

    //    delete d->rgbU8sRGB;
    //    delete d->lab16sLAB;
    //    delete d->alphaCs;

    delete d;
}

void KoColorSpaceRegistry::add(KoColorSpaceFactory *item)
{
    {
        QWriteLocker l(&d->registrylock);
        d->colorSpaceFactoryRegistry.add(item);
    }
    d->colorConversionSystem->insertColorSpace(item);
}

void KoColorSpaceRegistry::remove(KoColorSpaceFactory *item)
{
    d->registrylock.lockForRead();
    QList<QString> toremove;
    foreach (const KoColorSpace *cs, d->csMap) {
        if (cs->id() == item->id()) {
            toremove.push_back(idsToCacheName(cs->id(), cs->profile()->name()));
            cs->d->deletability = OwnedByRegistryRegistryDeletes;
        }
    }
    d->registrylock.unlock();
    d->registrylock.lockForWrite();
    foreach (const QString &id, toremove) {
        d->csMap.remove(id);
        // TODO: should not it delete the color space when removing it from the map ?
    }
    d->colorSpaceFactoryRegistry.remove(item->id());
    d->registrylock.unlock();
}

void KoColorSpaceRegistry::addProfileAlias(const QString &name, const QString &to)
{
    QWriteLocker l(&d->registrylock);
    d->profileAlias[name] = to;
}

QString KoColorSpaceRegistry::profileAlias(const QString &_name) const
{
    QReadLocker l(&d->registrylock);
    return d->profileAlias.value(_name, _name);
}

const KoColorProfile *KoColorSpaceRegistry::profileByName(const QString &_name) const
{
    QReadLocker l(&d->registrylock);
    return d->profileMap.value(profileAlias(_name), 0);
}

QList<const KoColorProfile *> KoColorSpaceRegistry::profilesFor(const QString &id) const
{
    return profilesFor(d->colorSpaceFactoryRegistry.value(id));
}

const KoColorSpace *KoColorSpaceRegistry::colorSpace(const KoID &csID, const QString &profileName)
{
    return colorSpace(csID.id(), profileName);
}

const KoColorSpace *KoColorSpaceRegistry::colorSpace(const QString &colorModelId, const QString &colorDepthId, const KoColorProfile *profile)
{
    return colorSpace(colorSpaceId(colorModelId, colorDepthId), profile);
}

const KoColorSpace *KoColorSpaceRegistry::colorSpace(const QString &colorModelId, const QString &colorDepthId, const QString &profileName)
{
    return colorSpace(colorSpaceId(colorModelId, colorDepthId), profileName);
}

QList<const KoColorProfile *> KoColorSpaceRegistry::profilesFor(const KoColorSpaceFactory *csf) const
{
    QReadLocker l(&d->registrylock);
    QList<const KoColorProfile *> profiles;
    if (csf == nullptr)
        return profiles;

    QHash<QString, KoColorProfile *>::Iterator it;
    for (it = d->profileMap.begin(); it != d->profileMap.end(); ++it) {
        KoColorProfile *profile = it.value();
        if (csf->profileIsCompatible(profile)) {
            Q_ASSERT(profile);
            //         if (profile->colorSpaceSignature() == csf->colorSpaceSignature()) {
            profiles.push_back(profile);
        }
    }
    return profiles;
}

QList<const KoColorSpaceFactory *> KoColorSpaceRegistry::colorSpacesFor(const KoColorProfile *_profile) const
{
    QReadLocker l(&d->registrylock);
    QList<const KoColorSpaceFactory *> csfs;
    foreach (KoColorSpaceFactory *csf, d->colorSpaceFactoryRegistry.values()) {
        if (csf->profileIsCompatible(_profile)) {
            csfs.push_back(csf);
        }
    }
    return csfs;
}

QList<const KoColorProfile *> KoColorSpaceRegistry::profilesFor(const KoID &id) const
{
    return profilesFor(id.id());
}

void KoColorSpaceRegistry::addProfileToMap(KoColorProfile *p)
{
    Q_ASSERT(p);
    if (p->valid()) {
        d->profileMap[p->name()] = p;
    }
}

void KoColorSpaceRegistry::addProfile(KoColorProfile *p)
{
    Q_ASSERT(p);
    if (p->valid()) {
        d->profileMap[p->name()] = p;
        d->colorConversionSystem->insertColorProfile(p);
    }
}

void KoColorSpaceRegistry::addProfile(const KoColorProfile *profile)
{
    addProfile(profile->clone());
}

void KoColorSpaceRegistry::removeProfile(KoColorProfile *profile)
{
    d->profileMap.remove(profile->name());
}

const KoColorSpace *KoColorSpaceRegistry::getCachedColorSpace(const QString &csID, const QString &profileName) const
{
    auto it = d->csMap.find(idsToCacheName(csID, profileName));

    if (it != d->csMap.end()) {
        return it.value();
    }

    return nullptr;
}

QString KoColorSpaceRegistry::idsToCacheName(const QString &csID, const QString &profileName) const
{
    return csID + "<comb>" + profileName;
}

const KoColorSpaceFactory *KoColorSpaceRegistry::colorSpaceFactory(const QString &colorSpaceId) const
{
    QReadLocker l(&d->registrylock);
    return d->colorSpaceFactoryRegistry.get(colorSpaceId);
}

const KoColorSpace *KoColorSpaceRegistry::colorSpace(const QString &csID, const QString &pName)
{
    QString profileName = pName;

    if (profileName.isEmpty()) {
        QReadLocker l(&d->registrylock);
        KoColorSpaceFactory *csf = d->colorSpaceFactoryRegistry.value(csID);

        if (!csf) {
            dbgPigmentCSRegistry << "Unknown color space type : " << csID;
            return nullptr;
        }

        profileName = csf->defaultProfile();
    }

    if (profileName.isEmpty()) {
        return nullptr;
    }

    const KoColorSpace *cs = nullptr;
    {
        QReadLocker l(&d->registrylock);
        cs = getCachedColorSpace(csID, profileName);
    }

    if (!cs) {
        KoColorSpaceFactory *csf = d->colorSpaceFactoryRegistry.value(csID);

        if (!csf) {
            dbgPigmentCSRegistry << "Unknown color space type :" << csf;
            return nullptr;
        }

        // last attempt at getting a profile, sometimes the default profile, like adobe cmyk isn't available.
        const KoColorProfile *p = profileByName(profileName);
        if (!p) {
            dbgPigmentCSRegistry << "Profile not found :" << profileName;

            /**
             * If the requested profile is not available, try fetching the
             * default one
             */
            profileName = csf->defaultProfile();
            p = profileByName(profileName);

            /**
             * If there is no luck, try to fetch the first one
             */
            if (!p) {
                QList<const KoColorProfile *> profiles = profilesFor(csID);
                if (!profiles.isEmpty()) {
                    p = profiles[0];
                    Q_ASSERT(p);
                }
            }
        }

        // We did our best, but still have no profile: and since csf->grabColorSpace
        // needs the profile to find the colorspace, we have to give up.
        if (!p) {
            return nullptr;
        }
        profileName = p->name();

        QWriteLocker l(&d->registrylock);
        /*
         * We need to check again here, a thread requesting the same colorspace could've added it
         * already, in between the read unlock and write lock.
         * TODO: We also potentially changed profileName content, which means we maybe are going to
         * create a colorspace that's actually in the space registry cache, but currently this might
         * not be an issue because the colorspace should be cached also by the factory, so it won't
         * create a new instance. That being said, having two caches with the same stuff doesn't make
         * much sense.
         */
        cs = getCachedColorSpace(csID, profileName);
        if (!cs) {
            cs = csf->grabColorSpace(p);
            if (!cs) {
                dbgPigmentCSRegistry << "Unable to create color space";
                return nullptr;
            }

            dbgPigmentCSRegistry << "colorspace count: " << d->csMap.count() << ", adding name: " << idsToCacheName(cs->id(), cs->profile()->name())
                                 << "\n\tcsID" << csID << "\n\tprofileName" << profileName << "\n\tcs->id()" << cs->id() << "\n\tcs->profile()->name()"
                                 << cs->profile()->name() << "\n\tpName" << pName;
            Q_ASSERT(cs->id() == csID);
            Q_ASSERT(cs->profile()->name() == profileName);
            d->csMap[idsToCacheName(cs->id(), cs->profile()->name())] = cs;
            cs->d->deletability = OwnedByRegistryDoNotDelete;
        }
    } else {
        Q_ASSERT(cs->id() == csID);
        Q_ASSERT(cs->profile()->name() == profileName);
    }

    return cs;
}

const KoColorSpace *KoColorSpaceRegistry::colorSpace(const QString &csID, const KoColorProfile *profile)
{
    if (csID.isEmpty()) {
        return nullptr;
    }
    if (profile) {
        d->registrylock.lockForRead();
        const KoColorSpace *cs = getCachedColorSpace(csID, profile->name());
        d->registrylock.unlock();

        if (!d->profileMap.contains(profile->name())) {
            addProfile(profile);
        }

        if (!cs) {
            // The profile was not stored and thus not the combination either
            d->registrylock.lockForRead();
            KoColorSpaceFactory *csf = d->colorSpaceFactoryRegistry.value(csID);
            d->registrylock.unlock();
            if (!csf) {
                dbgPigmentCSRegistry << "Unknown color space type :" << csf;
                return nullptr;
            }
            if (!csf->profileIsCompatible(profile)) {
                return nullptr;
            }

            QWriteLocker l(&d->registrylock);
            // Check again, anything could've happened between the unlock and the write lock
            cs = getCachedColorSpace(csID, profile->name());
            if (!cs) {
                cs = csf->grabColorSpace(profile);
                if (!cs)
                    return nullptr;

                QString name = idsToCacheName(csID, profile->name());
                d->csMap[name] = cs;
                cs->d->deletability = OwnedByRegistryDoNotDelete;
                dbgPigmentCSRegistry << "colorspace count: " << d->csMap.count() << ", adding name: " << name;
            }
        }

        return cs;
    } else {
        return colorSpace(csID);
    }
}

const KoColorSpace *KoColorSpaceRegistry::alpha8()
{
    Q_ASSERT(d->alphaCs);
    return d->alphaCs;
}

const KoColorSpace *KoColorSpaceRegistry::rgb8(const QString &profileName)
{
    if (profileName.isEmpty()) {
        if (!d->rgbU8sRGB) {
            d->rgbU8sRGB = colorSpace(KoRgbU8ColorSpace::colorSpaceId());
        }
        Q_ASSERT(d->rgbU8sRGB);
        return d->rgbU8sRGB;
    }
    return colorSpace(KoRgbU8ColorSpace::colorSpaceId(), profileName);
}

const KoColorSpace *KoColorSpaceRegistry::rgb8(const KoColorProfile *profile)
{
    if (profile == nullptr) {
        if (!d->rgbU8sRGB) {
            d->rgbU8sRGB = colorSpace(KoRgbU8ColorSpace::colorSpaceId());
        }
        Q_ASSERT(d->rgbU8sRGB);
        return d->rgbU8sRGB;
    }
    return colorSpace(KoRgbU8ColorSpace::colorSpaceId(), profile);
}

const KoColorSpace *KoColorSpaceRegistry::rgb16(const QString &profileName)
{
    return colorSpace(KoRgbU16ColorSpace::colorSpaceId(), profileName);
}

const KoColorSpace *KoColorSpaceRegistry::rgb16(const KoColorProfile *profile)
{
    return colorSpace(KoRgbU16ColorSpace::colorSpaceId(), profile);
}

const KoColorSpace *KoColorSpaceRegistry::lab16(const QString &profileName)
{
    if (profileName.isEmpty()) {
        if (!d->lab16sLAB) {
            d->lab16sLAB = colorSpace(KoLabColorSpace::colorSpaceId(), profileName);
        }
        return d->lab16sLAB;
    }
    return colorSpace(KoLabColorSpace::colorSpaceId(), profileName);
}

const KoColorSpace *KoColorSpaceRegistry::lab16(const KoColorProfile *profile)
{
    if (profile == nullptr) {
        if (!d->lab16sLAB) {
            d->lab16sLAB = colorSpace(KoLabColorSpace::colorSpaceId(), profile);
        }
        Q_ASSERT(d->lab16sLAB);
        return d->lab16sLAB;
    }
    return colorSpace(KoLabColorSpace::colorSpaceId(), profile);
}

QList<KoID> KoColorSpaceRegistry::colorModelsList(ColorSpaceListVisibility option) const
{
    QReadLocker l(&d->registrylock);
    QList<KoID> ids;
    QList<KoColorSpaceFactory *> factories = d->colorSpaceFactoryRegistry.values();
    foreach (KoColorSpaceFactory *factory, factories) {
        if (!ids.contains(factory->colorModelId()) && (option == AllColorSpaces || factory->userVisible())) {
            ids << factory->colorModelId();
        }
    }
    return ids;
}
QList<KoID> KoColorSpaceRegistry::colorDepthList(const KoID &colorModelId, ColorSpaceListVisibility option) const
{
    return colorDepthList(colorModelId.id(), option);
}

QList<KoID> KoColorSpaceRegistry::colorDepthList(const QString &colorModelId, ColorSpaceListVisibility option) const
{
    QReadLocker l(&d->registrylock);
    QList<KoID> ids;
    QList<KoColorSpaceFactory *> factories = d->colorSpaceFactoryRegistry.values();
    foreach (KoColorSpaceFactory *factory, factories) {
        if (!ids.contains(KoID(factory->colorDepthId())) && factory->colorModelId().id() == colorModelId
            && (option == AllColorSpaces || factory->userVisible())) {
            ids << factory->colorDepthId();
        }
    }
    return ids;
}

QString KoColorSpaceRegistry::colorSpaceId(const QString &colorModelId, const QString &colorDepthId) const
{
    QReadLocker l(&d->registrylock);
    QList<KoColorSpaceFactory *> factories = d->colorSpaceFactoryRegistry.values();
    foreach (KoColorSpaceFactory *factory, factories) {
        if (factory->colorModelId().id() == colorModelId && factory->colorDepthId().id() == colorDepthId) {
            return factory->id();
        }
    }
    return "";
}

QString KoColorSpaceRegistry::colorSpaceId(const KoID &colorModelId, const KoID &colorDepthId) const
{
    return colorSpaceId(colorModelId.id(), colorDepthId.id());
}

KoID KoColorSpaceRegistry::colorSpaceColorModelId(const QString &_colorSpaceId) const
{
    QReadLocker l(&d->registrylock);
    KoColorSpaceFactory *factory = d->colorSpaceFactoryRegistry.get(_colorSpaceId);
    if (factory) {
        return factory->colorModelId();
    } else {
        return KoID();
    }
}

KoID KoColorSpaceRegistry::colorSpaceColorDepthId(const QString &_colorSpaceId) const
{
    QReadLocker l(&d->registrylock);
    KoColorSpaceFactory *factory = d->colorSpaceFactoryRegistry.get(_colorSpaceId);
    if (factory) {
        return factory->colorDepthId();
    } else {
        return KoID();
    }
}

const KoColorConversionSystem *KoColorSpaceRegistry::colorConversionSystem() const
{
    return d->colorConversionSystem;
}

KoColorConversionCache *KoColorSpaceRegistry::colorConversionCache() const
{
    return d->colorConversionCache;
}

const KoColorSpace *KoColorSpaceRegistry::permanentColorspace(const KoColorSpace *_colorSpace)
{
    if (_colorSpace->d->deletability != NotOwnedByRegistry) {
        return _colorSpace;
    } else if (*_colorSpace == *d->alphaCs) {
        return d->alphaCs;
    } else {
        const KoColorSpace *cs = colorSpace(_colorSpace->id(), _colorSpace->profile());
        Q_ASSERT(cs);
        Q_ASSERT(*cs == *_colorSpace);
        return cs;
    }
}

QList<KoID> KoColorSpaceRegistry::listKeys() const
{
    QReadLocker l(&d->registrylock);
    QList<KoID> answer;
    foreach (const QString &key, d->colorSpaceFactoryRegistry.keys()) {
        answer.append(KoID(key, d->colorSpaceFactoryRegistry.get(key)->name()));
    }

    return answer;
}

const KoColorProfile *KoColorSpaceRegistry::createColorProfile(const QString &colorModelId, const QString &colorDepthId, const QByteArray &rawData)
{
    QReadLocker l(&d->registrylock);
    KoColorSpaceFactory *factory_ = d->colorSpaceFactoryRegistry.get(colorSpaceId(colorModelId, colorDepthId));
    return factory_->colorProfile(rawData);
}

QList<const KoColorSpace *> KoColorSpaceRegistry::allColorSpaces(ColorSpaceListVisibility visibility, ColorSpaceListProfilesSelection pSelection)
{
    QList<const KoColorSpace *> colorSpaces;

    d->registrylock.lockForRead();
    QList<KoColorSpaceFactory *> factories = d->colorSpaceFactoryRegistry.values();
    d->registrylock.unlock();

    foreach (KoColorSpaceFactory *factory, factories) {
        // Don't test with ycbcr for now, since we don't have a default profile for it.
        if (factory->colorModelId().id().startsWith("Y"))
            continue;
        if (visibility == AllColorSpaces || factory->userVisible()) {
            if (pSelection == OnlyDefaultProfile) {
                const KoColorSpace *cs = colorSpace(factory->id());
                if (cs) {
                    colorSpaces.append(cs);
                } else {
                    warnPigment << "Could not create colorspace for id" << factory->id() << "since there is no working default profile";
                }
            } else {
                QList<const KoColorProfile *> profiles = KoColorSpaceRegistry::instance()->profilesFor(factory->id());
                foreach (const KoColorProfile *profile, profiles) {
                    const KoColorSpace *cs = colorSpace(factory->id(), profile);
                    if (cs) {
                        colorSpaces.append(cs);
                    } else {
                        warnPigment << "Could not create colorspace for id" << factory->id() << "and profile" << profile->name();
                    }
                }
            }
        }
    }

    return colorSpaces;
}
