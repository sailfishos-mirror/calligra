/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright     2007       David Faure <faure@kde.org>

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
 * Boston, MA 02110-1301, USA.
*/

#include "KoDocumentEntry.h"

#include "KoPart.h"
#include "KoDocument.h"
#include "KoFilter.h"

#include <kservicetype.h>
#include <kdebug.h>
#include <KoServiceLocator.h>
#include <QFile>
#include <QApplication>

#include <limits.h> // UINT_MAX

KoDocumentEntry::KoDocumentEntry()
        : m_service(0)
{
}

KoDocumentEntry::KoDocumentEntry(const KService::Ptr& service)
        : m_service(service)
{
}

KoDocumentEntry::~KoDocumentEntry()
{
}


KService::Ptr KoDocumentEntry::service() const {
    return m_service;
}

/**
 * @return TRUE if the service pointer is null
 */
bool KoDocumentEntry::isEmpty() const {
    return m_service.isNull();
}

/**
 * @return name of the associated service
 */
QString KoDocumentEntry::name() const {
    return m_service->name();
}

/**
 *  Mimetypes (and other service types) which this document can handle.
 */
QStringList KoDocumentEntry::mimeTypes() const {
    return m_service->serviceTypes();
}

/**
 *  @return TRUE if the document can handle the requested mimetype.
 */
bool KoDocumentEntry::supportsMimeType(const QString & _mimetype) const {
    return mimeTypes().contains(_mimetype);
}

KoPart *KoDocumentEntry::createKoPart(QString* errorMsg) const
{
    QString error;
    KoPart* part = m_service->createInstance<KoPart>(0, QVariantList(), &error);

    if (!part) {
        kWarning(30003) << error;
        if (errorMsg)
            *errorMsg = error;
        return 0;
    }

    return part;
}

KoDocumentEntry KoDocumentEntry::queryByMimeType(const QString & mimetype)
{
    QList<KoDocumentEntry> vec = query(mimetype);

    if (vec.isEmpty()) {
        kWarning(30003) << "Got no results with " << mimetype;
        // Fallback to the old way (which was probably wrong, but better be safe)
        vec = query(mimetype);

        if (vec.isEmpty()) {
            // Still no match. Either the mimetype itself is unknown, or we have no service for it.
            // Help the user debugging stuff by providing some more diagnostics
            if (KServiceType::serviceType(mimetype).isNull()) {
                kError(30003) << "Unknown Calligra MimeType " << mimetype << "." << endl;
                kError(30003) << "Check your installation (for instance, run 'kde4-config --path mime' and check the result)." << endl;
            } else {
                kError(30003) << "Found no Calligra part able to handle " << mimetype << "!" << endl;
                kError(30003) << "Check your installation (does the desktop file have X-KDE-NativeMimeType and Calligra/Part, did you install Calligra in a different prefix than KDE, without adding the prefix to /etc/kderc ?)" << endl;
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
    const KService::List offers = KoServiceLocator::instance()->entries("Calligra/Part");

    foreach(KService::Ptr offer, offers) {

        QStringList nativeMimeTypes = offer->property("X-KDE-NativeMimeType", QVariant::StringList).toStringList();
        QStringList extraNativeMimeTypes = offer->property("X-KDE-ExtraNativeMimeTypes", QVariant::StringList).toStringList();
        QStringList serviceTypes = offer->property("ServiceTypes", QVariant::StringList).toStringList();

        if (nativeMimeTypes.contains(mimetype) || extraNativeMimeTypes.contains(mimetype) || serviceTypes.contains(mimetype)) {

            if (offer->noDisplay())
                continue;
            KoDocumentEntry d(offer);
            // Append converted offer
            lst.append(d);

            // And if it's the part that belongs to the current application it's our own, so break off
            if (offer->desktopEntryName() == (qAppName().replace("calligra","") + "part")) {
                lst.clear();
                lst.append(d);
                break;
            }

            // Next service
        }
    }

    if (lst.count() > 1 && !mimetype.isEmpty()) {
        kWarning(30003) << "KoDocumentEntry::query " << mimetype << " got " << lst.count() << " offers!";
        foreach(const KoDocumentEntry &entry, lst) {
            qDebug() << entry.name();
        }
    }

    return lst;
}
