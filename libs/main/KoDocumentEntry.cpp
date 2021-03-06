/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
   SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KoDocumentEntry.h"

#include "KoPart.h"
#include "KoDocument.h"
#include "KoFilter.h"
#include <MainDebug.h>

#include <KoPluginLoader.h>

#include <kservicetype.h>
#include <kpluginfactory.h>

#include <QCoreApplication>

#include <limits.h> // UINT_MAX

KoDocumentEntry::KoDocumentEntry()
        : m_loader(0)
{
}

KoDocumentEntry::KoDocumentEntry(QPluginLoader *loader)
        : m_loader(loader)
{
}

KoDocumentEntry::~KoDocumentEntry()
{
}


QJsonObject KoDocumentEntry::metaData() const
{
    return m_loader ? m_loader->metaData().value("MetaData").toObject() : QJsonObject();
}

QString KoDocumentEntry::fileName() const
{
    return m_loader ? m_loader->fileName() : QString();
}

/**
 * @return TRUE if the service pointer is null
 */
bool KoDocumentEntry::isEmpty() const {
    return (m_loader == 0);
}

/**
 * @return name of the associated service
 */
QString KoDocumentEntry::name() const {
    QJsonObject json = metaData();
    json = json.value("KPlugin").toObject();
    return json.value("Name").toString();
}

/**
 *  Mimetypes (and other service types) which this document can handle.
 */
QStringList KoDocumentEntry::mimeTypes() const {
    QJsonObject json = metaData();
    QJsonObject pluginData = json.value("KPlugin").toObject();
    return pluginData.value("MimeTypes").toVariant().toStringList();
}

/**
 *  @return TRUE if the document can handle the requested mimetype.
 */
bool KoDocumentEntry::supportsMimeType(const QString & _mimetype) const {
    return mimeTypes().contains(_mimetype);
}

KoPart *KoDocumentEntry::createKoPart(QString* errorMsg) const
{
    if (!m_loader) {
        return 0;
    }

    QObject *obj = m_loader->instance();
    KPluginFactory *factory = qobject_cast<KPluginFactory *>(obj);
    KoPart *part = factory->create<KoPart>(0, QVariantList());

    if (!part) {
        if (errorMsg)
            *errorMsg = m_loader->errorString();
        return 0;
    }

    return part;
}

KoDocumentEntry KoDocumentEntry::queryByMimeType(const QString & mimetype)
{
    QList<KoDocumentEntry> vec = query(mimetype);

    if (vec.isEmpty()) {
        warnMain << "Got no results with " << mimetype;
        // Fallback to the old way (which was probably wrong, but better be safe)
        vec = query(mimetype);

        if (vec.isEmpty()) {
            // Still no match. Either the mimetype itself is unknown, or we have no service for it.
            // Help the user debugging stuff by providing some more diagnostics
            if (!KServiceType::serviceType(mimetype)) {
                errorMain << "Unknown Calligra MimeType " << mimetype << "." << endl;
                errorMain << "Check your installation (for instance, run 'kde4-config --path mime' and check the result)." << endl;
            } else {
                errorMain << "Found no Calligra part able to handle " << mimetype << "!" << endl;
                errorMain << "Check your installation (does the desktop file have X-KDE-NativeMimeType and Calligra/Part, did you install Calligra in a different prefix than KDE, without adding the prefix to /etc/kderc ?)" << endl;
            }
            return KoDocumentEntry();
        }
    }

    // Filthy hack alert -- this'll be properly fixed in the mvc branch.
    if (qApp->applicationName() == "flow" && vec.size() == 2) {
        return KoDocumentEntry(vec[1]);
    }

    return KoDocumentEntry(vec[0]);
}

QList<KoDocumentEntry> KoDocumentEntry::query(const QString & mimetype)
{

    QList<KoDocumentEntry> lst;

    // Query the trader
    const QList<QPluginLoader *> offers = KoPluginLoader::pluginLoaders(QStringLiteral("calligra/parts"), mimetype);

    foreach(QPluginLoader *pluginLoader, offers) {
        lst.append(KoDocumentEntry(pluginLoader));
    }

    if (lst.count() > 1 && !mimetype.isEmpty()) {
        warnMain << "KoDocumentEntry::query " << mimetype << " got " << lst.count() << " offers!";
        foreach(const KoDocumentEntry &entry, lst) {
            warnMain << entry.name();
        }
    }

    return lst;
}
