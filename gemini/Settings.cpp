/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "Settings.h"
#include "DocumentListModel.h"

#include <QApplication>
#include <QMimeDatabase>
#include <QMimeType>
#include <QUrl>
#include <QUrlQuery>

#include <KConfigGroup>
#include <KSharedConfig>

#include "PropertyContainer.h"
#include "Theme.h"
// #include <qtquick/CQTextDocumentCanvas.h>
#include <KoDocumentEntry.h>
#include <part/KWDocument.h>
#include <stage/part/KPrDocument.h>

class Settings::Private
{
public:
    Private()
        : temporaryFile(false)
        , focusItem(nullptr)
    {
    }

    QString currentFile;
    QString currentFileClass;
    bool temporaryFile;
    QQuickItem *focusItem;
    Theme *theme;
};

Settings::Settings(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
    QString theme = KSharedConfig::openConfig()->group("General").readEntry<QString>("theme", "default");
    d->theme = Theme::load(theme, this);
    connect(d->theme, &Theme::fontCacheRebuilt, this, &Settings::themeChanged);
}

Settings::~Settings()
{
    delete d;
}

QString Settings::currentFile() const
{
    return d->currentFile;
}

QString Settings::currentFileClass() const
{
    return d->currentFileClass;
}

void Settings::setCurrentFile(const QString &fileName)
{
    qApp->processEvents();
    if (fileName.isEmpty()) {
        d->currentFile = fileName;
        d->currentFileClass = "No document set, consequently no class. This is expected behaviour, do not report.";
        emit currentFileChanged();
    } else if (fileName != d->currentFile) {
        QUrl url(fileName);
        if (url.scheme() == "newfile") {
            QUrlQuery query(url.query());
            d->currentFileClass = query.queryItemValue("mimetype");
        } else {
            QMimeDatabase db;
            QMimeType mimeType = db.mimeTypeForUrl(url);
            KoDocumentEntry documentEntry = KoDocumentEntry::queryByMimeType(mimeType.name());
            if (documentEntry.supportsMimeType(WORDS_MIME_TYPE)) {
                d->currentFileClass = WORDS_MIME_TYPE;
            } else if (documentEntry.supportsMimeType(STAGE_MIME_TYPE)) {
                d->currentFileClass = STAGE_MIME_TYPE;
            } else {
                d->currentFileClass = QString("Unsupported document! Reported mimetype is %1").arg(mimeType.name());
            }
        }
        d->currentFile = fileName;
        emit currentFileChanged();
    }
}

bool Settings::isTemporaryFile() const
{
    return d->temporaryFile;
}

void Settings::setTemporaryFile(bool temp)
{
    if (temp != d->temporaryFile) {
        d->temporaryFile = temp;
        emit temporaryFileChanged();
    }
}

QQuickItem *Settings::focusItem()
{
    return d->focusItem;
}

void Settings::setFocusItem(QQuickItem *item)
{
    if (item != d->focusItem) {
        d->focusItem = item;
        emit focusItemChanged();
    }
}

QObject *Settings::theme() const
{
    return d->theme;
}

QString Settings::themeID() const
{
    if (d->theme)
        return d->theme->id();

    return QString();
}

void Settings::setThemeID(const QString &id)
{
    if (!d->theme || id != d->theme->id()) {
        if (d->theme) {
            delete d->theme;
            d->theme = nullptr;
        }

        d->theme = Theme::load(id, this);
        KSharedConfig::openConfig()->group("General").writeEntry<QString>("theme", id);

        emit themeChanged();
    }
}

int Settings::mimeTypeToDocumentClass(QString mimeType) const
{
    DocumentListModel::DocumentType documentClass = DocumentListModel::UnknownType;
    if (mimeType == QLatin1String("application/vnd.oasis.opendocument.text") || mimeType == QLatin1String("application/msword")
        || mimeType == QLatin1String("application/rtf") || mimeType == QLatin1String("application/vnd.openxmlformats-officedocument.wordprocessingml.document")
        || mimeType == QLatin1String("application/vnd.openxmlformats-officedocument.wordprocessingml.template")
        || mimeType == QLatin1String("application/vnd.ms-word.document.macroEnabled.12")
        || mimeType == QLatin1String("application/vnd.ms-word.template.macroEnabled.12")) {
        documentClass = DocumentListModel::TextDocumentType;
    } else if (mimeType == QLatin1String("application/vnd.oasis.opendocument.presentation") || mimeType == QLatin1String("application/vnd.ms-powerpoint")
               || mimeType == QLatin1String("application/vnd.openxmlformats-officedocument.presentationml.presentation")
               || mimeType == QLatin1String("application/vnd.openxmlformats-officedocument.presentationml.template")
               || mimeType == QLatin1String("application/vnd.openxmlformats-officedocument.presentationml.slideshow")
               || mimeType == QLatin1String("application/vnd.ms-powerpoint.presentation.macroEnabled.12")
               || mimeType == QLatin1String("application/vnd.ms-powerpoint.template.macroEnabled.12")
               || mimeType == QLatin1String("application/vnd.ms-powerpoint.slideshow.macroEnabled.12")) {
        documentClass = DocumentListModel::PresentationType;
    }
    //     else
    //     if(mimeType == QLatin1String("application/vnd.oasis.opendocument.spreadsheet") ||
    //        mimeType == QLatin1String("application/vnd.ms-excel") ||
    //        mimeType == QLatin1String("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet") ||
    //        mimeType == QLatin1String("application/vnd.openxmlformats-officedocument.spreadsheetml.template") ||
    //        mimeType == QLatin1String("application/vnd.ms-excel.sheet.macroEnabled") ||
    //        mimeType == QLatin1String("application/vnd.ms-excel.sheet.macroEnabled.12") ||
    //        mimeType == QLatin1String("application/vnd.ms-excel.template.macroEnabled.12") )
    //     {
    //         documentClass = DocumentListModel::SpreadSheetType;
    //     }
    return documentClass;
}
