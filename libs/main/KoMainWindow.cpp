/* This file is part of the KDE project
   SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>
   SPDX-FileCopyrightText: 2000-2006 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2007, 2009 Thomas zander <zander@kde.org>
   SPDX-FileCopyrightText: 2010 Benjamin Port <port.benjamin@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KoMainWindow.h"

#include "KoApplication.h"
#include "KoComponentData.h"
#include "KoDockFactoryBase.h"
#include "KoDockWidgetTitleBar.h"
#include "KoDockerManager.h"
#include "KoDocument.h"
#include "KoDocumentEntry.h"
#include "KoDocumentInfo.h"
#include "KoDocumentInfoDlg.h"
#include "KoFileDialog.h"
#include "KoFilterManager.h"
#include "KoPart.h"
#include "KoPrintJob.h"
#include "KoResourcePaths.h"
#include "KoVersionDialog.h"
#include "KoView.h"
#include "calligra-version.h"
#include <KoConfig.h>
#include <KoDockRegistry.h>
#include <KoIcon.h>
#include <KoPageLayout.h>
#include <KoPageLayoutDialog.h>

#include <KActionCollection>
#include <KActionMenu>
#include <KFileItem>
#include <KLocalizedString>
#include <KMessageBox>
#include <KToolBar>
#include <KWindowConfig>
#include <KoNetAccess.h>
#include <QTemporaryFile>
#include <kaboutdata.h>
#include <kedittoolbar.h>
#include <khelpmenu.h>
#include <krecentdirs.h>
#include <krecentdocument.h>
#include <krecentfilesaction.h>
#include <ktoggleaction.h>
#include <kxmlguifactory.h>

#include <KEMailClientLauncherJob>
#include <kio_version.h>

//   // qt includes
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFontDatabase>
#include <QLabel>
#include <QLayout>
#include <QMenuBar>
#include <QMimeDatabase>
#include <QMutex>
#include <QMutexLocker>
#include <QPointer>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QProgressBar>
#include <QScreen>
#include <QStatusBar>
#include <QTabBar>

#include "MainDebug.h"

class KoMainWindowPrivate
{
public:
    KoMainWindowPrivate(const QByteArray &_nativeMimeType, const KoComponentData &componentData_, KoMainWindow *w)
        : componentData(componentData_)
    {
        nativeMimeType = _nativeMimeType;
        parent = w;
        rootDocument = nullptr;
        rootPart = nullptr;
        partToOpen = nullptr;
        mainWindowGuiIsBuilt = false;
        forQuit = false;
        activePart = nullptr;
        rootView = nullptr;
        activeView = nullptr;
        firstTime = true;
        progress = nullptr;
        showDocumentInfo = nullptr;
        saveAction = nullptr;
        saveActionAs = nullptr;
        printAction = nullptr;
        printActionPreview = nullptr;
        sendFileAction = nullptr;
        exportPdf = nullptr;
        closeFile = nullptr;
        reloadFile = nullptr;
        showFileVersions = nullptr;
        importFile = nullptr;
        exportFile = nullptr;
        encryptDocument = nullptr;
#ifndef NDEBUG
        uncompressToDir = nullptr;
#endif
        isImporting = false;
        isExporting = false;
        windowSizeDirty = false;
        lastExportSpecialOutputFlag = 0;
        readOnly = false;
        dockWidgetMenu = nullptr;
        dockerManager = nullptr;
        deferredClosingEvent = nullptr;

        m_helpMenu = nullptr;

        // PartManger
        m_activeWidget = nullptr;
        m_activePart = nullptr;

        noCleanup = false;
        openingDocument = false;
    }

    ~KoMainWindowPrivate()
    {
        qDeleteAll(toolbarList);
    }

    void applyDefaultSettings(QPrinter &printer)
    {
        QString title = rootDocument->documentInfo()->aboutInfo("title");
        if (title.isEmpty()) {
            title = rootDocument->url().fileName();
            // strip off the native extension (I don't want foobar.kwd.ps when printing into a file)
            QMimeType mime = QMimeDatabase().mimeTypeForName(rootDocument->outputMimeType());
            if (mime.isValid()) {
                const QString extension = mime.preferredSuffix();

                if (title.endsWith(extension))
                    title.chop(extension.length());
            }
        }

        if (title.isEmpty()) {
            // #139905
            title = i18n("%1 unsaved document (%2)",
                         parent->componentData().componentDisplayName(),
                         QLocale().toString(QDate::currentDate(), QLocale::ShortFormat));
        }
        printer.setDocName(title);
    }

    QByteArray nativeMimeType;

    KoMainWindow *parent;
    QPointer<KoDocument> rootDocument;
    KoView *rootView;

    // PartManager
    QPointer<KoPart> rootPart;
    QPointer<KoPart> partToOpen;
    QPointer<KoPart> activePart;
    QPointer<KoPart> m_activePart;
    QPointer<KoPart> m_registeredPart;

    KoView *activeView;
    QWidget *m_activeWidget;

    QPointer<QProgressBar> progress;
    QMutex progressMutex;

    QList<QAction *> toolbarList;

    bool mainWindowGuiIsBuilt;
    bool forQuit;
    bool firstTime;
    bool windowSizeDirty;
    bool readOnly;

    QAction *showDocumentInfo;
    QAction *saveAction;
    QAction *saveActionAs;
    QAction *printAction;
    QAction *printActionPreview;
    QAction *sendFileAction;
    QAction *exportPdf;
    QAction *closeFile;
    QAction *reloadFile;
    QAction *showFileVersions;
    QAction *importFile;
    QAction *exportFile;
    QAction *encryptDocument;
#ifndef NDEBUG
    QAction *uncompressToDir;
#endif
    KToggleAction *toggleDockers;
    KToggleAction *toggleDockerTitleBars;
    KRecentFilesAction *recent;

    bool isImporting;
    bool isExporting;

    QUrl lastExportUrl;
    QByteArray lastExportedFormat;
    int lastExportSpecialOutputFlag;

    QMap<QString, QDockWidget *> dockWidgetsMap;
    KActionMenu *dockWidgetMenu;
    QMap<QDockWidget *, bool> dockWidgetVisibilityMap;
    KoDockerManager *dockerManager;
    QList<QDockWidget *> dockWidgets;
    QByteArray m_dockerStateBeforeHiding;

    QCloseEvent *deferredClosingEvent;

    KoComponentData componentData;

    KHelpMenu *m_helpMenu;

    bool noCleanup;
    bool openingDocument;
};

KoMainWindow::KoMainWindow(const QByteArray &nativeMimeType, const KoComponentData &componentData)
    : KXmlGuiWindow()
    , d(new KoMainWindowPrivate(nativeMimeType, componentData, this))
{
    setStandardToolBarMenuEnabled(true);

    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    connect(this, &KoMainWindow::restoringDone, this, &KoMainWindow::forceDockTabFonts);

    // PartManager
    // End

    QString doc;
    const QStringList allFiles = KoResourcePaths::findAllResources("data", "calligra/calligra_shell.rc");
    setXMLFile(findMostRecentXMLFile(allFiles, doc));
    setLocalXMLFile(KoResourcePaths::locateLocal("data", "calligra/calligra_shell.rc"));

    actionCollection()->addAction(KStandardAction::New, "file_new", this, SLOT(slotFileNew()));
    actionCollection()->addAction(KStandardAction::Open, "file_open", this, SLOT(slotFileOpen()));
    d->recent = KStandardAction::openRecent(this, SLOT(slotFileOpenRecent(QUrl)), actionCollection());
    connect(d->recent, &KRecentFilesAction::recentListCleared, this, &KoMainWindow::saveRecentFiles);
    d->saveAction = actionCollection()->addAction(KStandardAction::Save, "file_save", this, SLOT(slotFileSave()));
    d->saveActionAs = actionCollection()->addAction(KStandardAction::SaveAs, "file_save_as", this, SLOT(slotFileSaveAs()));
    d->printAction = actionCollection()->addAction(KStandardAction::Print, "file_print", this, SLOT(slotFilePrint()));
    d->printActionPreview = actionCollection()->addAction(KStandardAction::PrintPreview, "file_print_preview", this, SLOT(slotFilePrintPreview()));

    d->exportPdf = new QAction(i18n("Export as PDF..."), this);
    d->exportPdf->setIcon(koIcon("application-pdf"));
    actionCollection()->addAction("file_export_pdf", d->exportPdf);
    connect(d->exportPdf, &QAction::triggered, this, [this]() {
        exportToPdf();
    });

    d->sendFileAction = actionCollection()->addAction(KStandardAction::Mail, "file_send_file", this, SLOT(slotEmailFile()));

    d->closeFile = actionCollection()->addAction(KStandardAction::Close, "file_close", this, SLOT(slotFileClose()));
    actionCollection()->addAction(KStandardAction::Quit, "file_quit", this, SLOT(slotFileQuit()));

    d->reloadFile = new QAction(i18n("Reload"), this);
    actionCollection()->addAction("file_reload_file", d->reloadFile);
    connect(d->reloadFile, &QAction::triggered, this, &KoMainWindow::slotReloadFile);

    d->showFileVersions = new QAction(i18n("Versions..."), this);
    actionCollection()->addAction("file_versions_file", d->showFileVersions);
    connect(d->showFileVersions, &QAction::triggered, this, &KoMainWindow::slotVersionsFile);

    d->importFile = new QAction(koIcon("document-import"), i18n("Open ex&isting Document as Untitled Document..."), this);
    actionCollection()->addAction("file_import_file", d->importFile);
    connect(d->importFile, &QAction::triggered, this, &KoMainWindow::slotImportFile);

    d->exportFile = new QAction(koIcon("document-export"), i18n("E&xport..."), this);
    actionCollection()->addAction("file_export_file", d->exportFile);
    connect(d->exportFile, &QAction::triggered, this, &KoMainWindow::slotExportFile);

    d->encryptDocument = new QAction(i18n("En&crypt Document"), this);
    actionCollection()->addAction("file_encrypt_doc", d->encryptDocument);
    connect(d->encryptDocument, &QAction::triggered, this, &KoMainWindow::slotEncryptDocument);

#ifndef NDEBUG
    d->uncompressToDir = new QAction(i18n("&Uncompress to Directory"), this);
    actionCollection()->addAction("file_uncompress_doc", d->uncompressToDir);
    connect(d->uncompressToDir, &QAction::triggered, this, &KoMainWindow::slotUncompressToDir);
#endif

    QAction *actionNewView = new QAction(koIcon("window-new"), i18n("&New View"), this);
    actionCollection()->addAction("view_newview", actionNewView);
    connect(actionNewView, &QAction::triggered, this, &KoMainWindow::newView);

    /* The following entry opens the document information dialog.  Since the action is named so it
        intends to show data this entry should not have a trailing ellipses (...).  */
    d->showDocumentInfo = new QAction(koIcon("document-properties"), i18n("Document Information"), this);
    actionCollection()->addAction("file_documentinfo", d->showDocumentInfo);
    connect(d->showDocumentInfo, &QAction::triggered, this, &KoMainWindow::slotDocumentInfo);

    KStandardAction::keyBindings(this, SLOT(slotConfigureKeys()), actionCollection());
    KStandardAction::configureToolbars(this, SLOT(slotConfigureToolbars()), actionCollection());

    d->showDocumentInfo->setEnabled(false);
    d->saveActionAs->setEnabled(false);
    d->reloadFile->setEnabled(false);
    d->showFileVersions->setEnabled(false);
    d->importFile->setEnabled(true); // always enabled like File --> Open
    d->exportFile->setEnabled(false);
    d->saveAction->setEnabled(false);
    d->printAction->setEnabled(false);
    d->printActionPreview->setEnabled(false);
    d->sendFileAction->setEnabled(false);
    d->exportPdf->setEnabled(false);
    d->closeFile->setEnabled(false);
    d->encryptDocument->setEnabled(false);
#ifndef NDEBUG
    d->uncompressToDir->setEnabled(false);
#endif

    KToggleAction *fullscreenAction = new KToggleAction(koIcon("view-fullscreen"), i18n("Full Screen Mode"), this);
    actionCollection()->addAction("view_fullscreen", fullscreenAction);
    actionCollection()->setDefaultShortcut(fullscreenAction, QKeySequence::FullScreen);
    connect(fullscreenAction, &QAction::toggled, this, &KoMainWindow::viewFullscreen);

    d->toggleDockers = new KToggleAction(i18n("Show Dockers"), this);
    d->toggleDockers->setChecked(true);
    actionCollection()->addAction("view_toggledockers", d->toggleDockers);
    connect(d->toggleDockers, &QAction::toggled, this, &KoMainWindow::toggleDockersVisibility);

    d->toggleDockerTitleBars = new KToggleAction(i18nc("@action:inmenu", "Show Docker Titlebars"), this);
    KConfigGroup configGroupInterface = KSharedConfig::openConfig()->group("Interface");
    d->toggleDockerTitleBars->setChecked(configGroupInterface.readEntry("ShowDockerTitleBars", true));
    d->toggleDockerTitleBars->setVisible(false);
    actionCollection()->addAction("view_toggledockertitlebars", d->toggleDockerTitleBars);
    connect(d->toggleDockerTitleBars, &QAction::toggled, this, &KoMainWindow::showDockerTitleBars);

    d->dockWidgetMenu = new KActionMenu(i18n("Dockers"), this);
    actionCollection()->addAction("settings_dockers_menu", d->dockWidgetMenu);
    d->dockWidgetMenu->setVisible(false);

    // Load list of recent files
    KSharedConfigPtr configPtr = componentData.config();
    d->recent->loadEntries(configPtr->group("RecentFiles"));

    createMainwindowGUI();
    d->mainWindowGuiIsBuilt = true;

    // we first figure out some good default size and restore the x,y position. See bug 285804Z.
    KConfigGroup cfg(KSharedConfig::openConfig(), "MainWindow");
    QByteArray geom = QByteArray::fromBase64(cfg.readEntry("ko_geometry", QByteArray()));
    if (!restoreGeometry(geom)) {
        auto screen = QGuiApplication::primaryScreen();
        Q_ASSERT(screen);

        quint32 w = 0;
        quint32 h = 0;

        // Default size -- maximize on small screens, something useful on big screens
        const int deskWidth = screen->availableSize().width();
        if (deskWidth > 1024) {
            // a nice width, and slightly less than total available
            // height to componensate for the window decs
            w = (deskWidth / 3) * 2;
            h = (screen->availableSize().height() / 3) * 2;
        } else {
            w = screen->availableSize().width();
            h = screen->availableSize().height();
        }

        setGeometry(geometry().x(), geometry().y(), w, h);
    }
    restoreState(QByteArray::fromBase64(cfg.readEntry("ko_windowstate", QByteArray())));

    d->dockerManager = new KoDockerManager(this);
}

void KoMainWindow::setNoCleanup(bool noCleanup)
{
    d->noCleanup = noCleanup;
}

KoMainWindow::~KoMainWindow()
{
    KConfigGroup cfg(KSharedConfig::openConfig(), "MainWindow");
    cfg.writeEntry("ko_geometry", saveGeometry().toBase64());
    cfg.writeEntry("ko_windowstate", saveState().toBase64());

    // Explicitly delete the docker manager to ensure that it is deleted before the dockers
    delete d->dockerManager;
    d->dockerManager = nullptr;

    // The doc and view might still exist (this is the case when closing the window)
    if (d->rootPart)
        d->rootPart->removeMainWindow(this);

    if (d->partToOpen) {
        d->partToOpen->removeMainWindow(this);
        delete d->partToOpen;
    }

    // safety first ;)
    setActivePart(nullptr, nullptr);

    if (d->rootView != d->activeView) {
        delete d->activeView;
        d->activeView = nullptr;
    }

    if (d->noCleanup)
        return;
    // We have to check if this was a root document.
    // This has to be checked from queryClose, too :)
    if (d->rootPart && d->rootPart->viewCount() == 0) {
        // debugMain <<"Destructor. No more views, deleting old doc" << d->rootDoc;
        delete d->rootDocument;
    }

    delete d;
}

void KoMainWindow::setRootDocument(KoDocument *doc, KoPart *part, bool deletePrevious)
{
    if (d->rootDocument == doc)
        return;

    if (d->partToOpen && d->partToOpen->document() != doc) {
        d->partToOpen->removeMainWindow(this);
        if (deletePrevious)
            delete d->partToOpen;
    }
    d->partToOpen = nullptr;

    // debugMain <<"KoMainWindow::setRootDocument this =" << this <<" doc =" << doc;
    KoView *oldRootView = d->rootView;
    d->rootView = nullptr;
    KoDocument *oldRootDoc = d->rootDocument;
    KoPart *oldRootPart = d->rootPart;

    if (oldRootDoc) {
        oldRootDoc->disconnect(this);
        oldRootPart->removeMainWindow(this);

        if (dockerManager()) {
            dockerManager()->resetToolDockerWidgets();
        }

        // Hide all dockwidgets and remember their old state
        d->dockWidgetVisibilityMap.clear();

        foreach (QDockWidget *dockWidget, d->dockWidgetsMap) {
            d->dockWidgetVisibilityMap.insert(dockWidget, dockWidget->isVisible());
            dockWidget->setVisible(false);
        }

        d->toggleDockerTitleBars->setVisible(false);
        d->dockWidgetMenu->setVisible(false);
    }

    d->rootDocument = doc;
    // XXX remove this after the splitting
    if (!part && doc) {
        d->rootPart = doc->documentPart();
    } else {
        d->rootPart = part;
    }

    if (doc) {
        d->toggleDockerTitleBars->setVisible(true);
        d->dockWidgetMenu->setVisible(true);
        d->m_registeredPart = d->rootPart.data();

        KoView *view = d->rootPart->createView(doc, this);
        setCentralWidget(view);
        d->rootView = view;

        view->show();
        view->setFocus();

        // The addMainWindow has been done already if using openUrl
        if (!d->rootPart->mainWindows().contains(this)) {
            d->rootPart->addMainWindow(this);
        }
    }

    bool enable = d->rootDocument != nullptr ? true : false;
    d->showDocumentInfo->setEnabled(enable);
    d->saveAction->setEnabled(enable);
    d->saveActionAs->setEnabled(enable);
    d->importFile->setEnabled(enable);
    d->exportFile->setEnabled(enable);
    d->encryptDocument->setEnabled(enable);
#ifndef NDEBUG
    d->uncompressToDir->setEnabled(enable);
#endif
    d->printAction->setEnabled(enable);
    d->printActionPreview->setEnabled(enable);
    d->sendFileAction->setEnabled(enable);
    d->exportPdf->setEnabled(enable);
    d->closeFile->setEnabled(enable);
    updateCaption();

    setActivePart(d->rootPart, d->rootView);
    Q_EMIT restoringDone();

    delete oldRootView;

    if (oldRootPart && oldRootPart->viewCount() == 0) {
        // debugMain <<"No more views, deleting old doc" << oldRootDoc;
        oldRootDoc->clearUndoHistory();
        if (deletePrevious)
            delete oldRootDoc;
    }

    if (doc && !d->dockWidgetVisibilityMap.isEmpty()) {
        foreach (QDockWidget *dockWidget, d->dockWidgetsMap) {
            dockWidget->setVisible(d->dockWidgetVisibilityMap.value(dockWidget));
        }
    }

    if (!d->rootDocument) {
        statusBar()->setVisible(false);
    } else {
#ifdef Q_OS_MAC
        statusBar()->setMaximumHeight(28);
#endif
        connect(d->rootDocument.data(), &KoDocument::titleModified, this, &KoMainWindow::slotDocumentTitleModified);
    }
}

void KoMainWindow::updateReloadFileAction(KoDocument *doc)
{
    d->reloadFile->setEnabled(doc && !doc->url().isEmpty());
}

void KoMainWindow::updateVersionsFileAction(KoDocument *doc)
{
    // TODO activate it just when we save it in oasis file format
    d->showFileVersions->setEnabled(
        doc && !doc->url().isEmpty()
        && (doc->outputMimeType() == doc->nativeOasisMimeType() || doc->outputMimeType() == doc->nativeOasisMimeType() + "-template"));
}

void KoMainWindow::setReadWrite(bool readwrite)
{
    d->saveAction->setEnabled(readwrite);
    d->importFile->setEnabled(readwrite);
    d->readOnly = !readwrite;
    updateCaption();
}

void KoMainWindow::addRecentURL(const QUrl &url)
{
    debugMain << "KoMainWindow::addRecentURL url=" << url.toDisplayString();
    // Add entry to recent documents list
    // (call coming from KoDocument because it must work with cmd line, template dlg, file/open, etc.)
    if (!url.isEmpty()) {
        bool ok = true;
        if (url.isLocalFile()) {
            QString path = url.adjusted(QUrl::StripTrailingSlash).toLocalFile();
            const QStringList tmpDirs = QStandardPaths::standardLocations(QStandardPaths::TempLocation);
            foreach (const QString &tmpDir, tmpDirs) {
                if (path.startsWith(tmpDir)) {
                    ok = false; // it's in the tmp resource
                    break;
                }
            }
            if (ok) {
                KRecentDocument::add(QUrl::fromLocalFile(path));
                KRecentDirs::add(":OpenDialog", QFileInfo(path).dir().canonicalPath());
            }
        } else {
            KRecentDocument::add(url.adjusted(QUrl::StripTrailingSlash));
        }
        if (ok) {
            d->recent->addUrl(url);
        }
        saveRecentFiles();
    }
}

void KoMainWindow::saveRecentFiles()
{
    // Save list of recent files
    KSharedConfigPtr config = componentData().config();
    debugMain << this << " Saving recent files list into config. componentData()=" << componentData().componentName();
    d->recent->saveEntries(config->group("RecentFiles"));
    config->sync();

    // Tell all windows to reload their list, after saving
    // Doesn't work multi-process, but it's a start
    foreach (KMainWindow *window, KMainWindow::memberList())
        static_cast<KoMainWindow *>(window)->reloadRecentFileList();
}

void KoMainWindow::reloadRecentFileList()
{
    KSharedConfigPtr config = componentData().config();
    d->recent->loadEntries(config->group("RecentFiles"));
}

KoPart *KoMainWindow::createPart() const
{
    KoDocumentEntry entry = KoDocumentEntry::queryByMimeType(d->nativeMimeType);
    QString errorMsg;
    KoPart *part = entry.createKoPart(&errorMsg);

    if (!part || !errorMsg.isEmpty()) {
        return nullptr;
    }
    return part;
}

void KoMainWindow::updateCaption()
{
    debugMain << "KoMainWindow::updateCaption()";
    if (!d->rootDocument) {
        updateCaption(QString(), false);
    } else {
        QString caption(d->rootDocument->caption());
        if (d->readOnly) {
            caption += ' ' + i18n("(write protected)");
        }

        updateCaption(caption, d->rootDocument->isModified());
        if (!rootDocument()->url().fileName().isEmpty())
            d->saveAction->setToolTip(i18n("Save as %1", d->rootDocument->url().fileName()));
        else
            d->saveAction->setToolTip(i18n("Save"));
    }
}

void KoMainWindow::updateCaption(const QString &caption, bool mod)
{
    debugMain << "KoMainWindow::updateCaption(" << caption << "," << mod << ")";
#ifdef CALLIGRA_ALPHA
    setCaption(QString("ALPHA %1: %2").arg(CALLIGRA_ALPHA).arg(caption), mod);
    return;
#endif
#ifdef CALLIGRA_BETA
    setCaption(QString("BETA %1: %2").arg(CALLIGRA_BETA).arg(caption), mod);
    return;
#endif
#ifdef CALLIGRA_RC
    setCaption(QString("RELEASE CANDIDATE %1: %2").arg(CALLIGRA_RC).arg(caption), mod);
    return;
#endif

    setCaption(caption, mod);
}

KoDocument *KoMainWindow::rootDocument() const
{
    return d->rootDocument;
}

KoView *KoMainWindow::rootView() const
{
    if (d->rootView != d->activeView)
        return d->activeView;
    return d->rootView;
}

bool KoMainWindow::openDocument(const QUrl &url)
{
    if (!KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, nullptr)) {
        KMessageBox::error(nullptr, i18n("The file %1 does not exist.", url.url()));
        d->recent->removeUrl(url); // remove the file from the recent-opened-file-list
        saveRecentFiles();
        return false;
    }
    return openDocumentInternal(url);
}

bool KoMainWindow::openDocument(KoPart *newPart, const QUrl &url)
{
    // the part always has a document; the document doesn't know about the part.
    KoDocument *newdoc = newPart->document();
    if (!KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, nullptr)) {
        newdoc->initEmpty(); // create an empty document
        setRootDocument(newdoc, newPart);
        newdoc->setUrl(url);
        QMimeType mime = QMimeDatabase().mimeTypeForUrl(url);
        QString mimetype = (!mime.isValid() || mime.isDefault()) ? newdoc->nativeFormatMimeType() : mime.name();
        newdoc->setMimeTypeAfterLoading(mimetype);
        updateCaption();
        return true;
    }
    return openDocumentInternal(url, newPart, newdoc);
}

bool KoMainWindow::openDocumentInternal(const QUrl &url, KoPart *newpart, KoDocument *newdoc)
{
    debugMain << "KoMainWindow::openDocument" << url.url();

    if (!newpart)
        newpart = createPart();

    if (!newpart)
        return false;

    if (!newdoc)
        newdoc = newpart->document();

    d->firstTime = true;
    connect(newdoc, &KoDocument::sigProgress, this, &KoMainWindow::slotProgress);
    connect(newdoc, &KoDocument::completed, this, &KoMainWindow::slotLoadCompleted);
    connect(newdoc, &KoDocument::canceled, this, &KoMainWindow::slotLoadCanceled);
    d->openingDocument = true;
    newpart->addMainWindow(this); // used by openUrl
    bool openRet = (!isImporting()) ? newdoc->openUrl(url) : newdoc->importDocument(url);
    if (!openRet) {
        newpart->removeMainWindow(this);
        delete newdoc;
        delete newpart;
        d->openingDocument = false;
        return false;
    }
    updateReloadFileAction(newdoc);
    updateVersionsFileAction(newdoc);

    KFileItem file(url, newdoc->mimeType(), KFileItem::Unknown);
    if (!file.isWritable()) {
        setReadWrite(false);
    }
    return true;
}

// Separate from openDocument to handle async loading (remote URLs)
void KoMainWindow::slotLoadCompleted()
{
    debugMain << "KoMainWindow::slotLoadCompleted";
    KoDocument *newdoc = qobject_cast<KoDocument *>(sender());
    KoPart *newpart = newdoc->documentPart();

    if (d->rootDocument && d->rootDocument->isEmpty()) {
        // Replace current empty document
        setRootDocument(newdoc);
        Q_EMIT loadCompleted(this);
    } else if (d->rootDocument && !d->rootDocument->isEmpty()) {
        // Open in a new main window
        // (Note : could create the main window first and the doc next for this
        // particular case, that would give a better user feedback...)
        KoMainWindow *s = newpart->createMainWindow();
        s->show();
        newpart->removeMainWindow(this);
        s->setRootDocument(newdoc, newpart);
        Q_EMIT loadCompleted(s);
    } else {
        // We had no document, set the new one
        setRootDocument(newdoc);
        Q_EMIT loadCompleted(this);
    }

    slotProgress(-1);
    disconnect(newdoc, &KoDocument::sigProgress, this, &KoMainWindow::slotProgress);
    disconnect(newdoc, &KoDocument::completed, this, &KoMainWindow::slotLoadCompleted);
    disconnect(newdoc, &KoDocument::canceled, this, &KoMainWindow::slotLoadCanceled);
    d->openingDocument = false;
}

void KoMainWindow::slotLoadCanceled(const QString &errMsg)
{
    debugMain << "KoMainWindow::slotLoadCanceled";
    if (!errMsg.isEmpty()) // empty when canceled by user
        KMessageBox::error(this, errMsg);
    // ... can't delete the document, it's the one who emitted the signal...

    KoDocument *doc = qobject_cast<KoDocument *>(sender());
    Q_ASSERT(doc);
    disconnect(doc, &KoDocument::sigProgress, this, &KoMainWindow::slotProgress);
    disconnect(doc, &KoDocument::completed, this, &KoMainWindow::slotLoadCompleted);
    disconnect(doc, &KoDocument::canceled, this, &KoMainWindow::slotLoadCanceled);
    d->openingDocument = false;
    Q_EMIT loadCanceled();
}

void KoMainWindow::slotSaveCanceled(const QString &errMsg)
{
    debugMain << "KoMainWindow::slotSaveCanceled";
    if (!errMsg.isEmpty()) // empty when canceled by user
        KMessageBox::error(this, errMsg);
    slotSaveCompleted();
}

void KoMainWindow::slotSaveCompleted()
{
    debugMain << "KoMainWindow::slotSaveCompleted";
    KoDocument *doc = qobject_cast<KoDocument *>(sender());
    Q_ASSERT(doc);
    disconnect(doc, &KoDocument::sigProgress, this, &KoMainWindow::slotProgress);
    disconnect(doc, &KoDocument::completed, this, &KoMainWindow::slotSaveCompleted);
    disconnect(doc, &KoDocument::canceled, this, &KoMainWindow::slotSaveCanceled);

    if (d->deferredClosingEvent) {
        KXmlGuiWindow::closeEvent(d->deferredClosingEvent);
    }
}

// returns true if we should save, false otherwise.
bool KoMainWindow::exportConfirmation(const QByteArray &outputFormat)
{
    KConfigGroup group = KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName());
    if (!group.readEntry("WantExportConfirmation", true)) {
        return true;
    }

    QMimeType mime = QMimeDatabase().mimeTypeForName(outputFormat);
    QString comment = mime.isValid() ? mime.comment() : i18n("%1 (unknown file type)", QString::fromLatin1(outputFormat));

    // Warn the user
    int ret;
    if (!isExporting()) { // File --> Save
        ret = KMessageBox::warningContinueCancel(this,
                                                 i18n("<qt>Saving as a %1 may result in some loss of formatting."
                                                      "<p>Do you still want to save in this format?</qt>",
                                                      QString("<b>%1</b>").arg(comment)), // in case we want to remove the bold later
                                                 i18n("Confirm Save"),
                                                 KStandardGuiItem::save(),
                                                 KStandardGuiItem::cancel(),
                                                 "NonNativeSaveConfirmation");
    } else { // File --> Export
        ret = KMessageBox::warningContinueCancel(this,
                                                 i18n("<qt>Exporting as a %1 may result in some loss of formatting."
                                                      "<p>Do you still want to export to this format?</qt>",
                                                      QString("<b>%1</b>").arg(comment)), // in case we want to remove the bold later
                                                 i18n("Confirm Export"),
                                                 KGuiItem(i18n("Export")),
                                                 KStandardGuiItem::cancel(),
                                                 "NonNativeExportConfirmation" // different to the one used for Save (above)
        );
    }

    return (ret == KMessageBox::Continue);
}

bool KoMainWindow::saveDocument(bool saveas, bool silent, int specialOutputFlag)
{
    if (!d->rootDocument || !d->rootPart) {
        return true;
    }
    bool reset_url;

    if (d->rootDocument->isEmpty() || d->rootDocument->url().isEmpty()) {
        Q_EMIT saveDialogShown();
        reset_url = true;
        saveas = true;
    } else {
        reset_url = false;
    }

    connect(d->rootDocument.data(), &KoDocument::sigProgress, this, &KoMainWindow::slotProgress);
    connect(d->rootDocument.data(), &KoDocument::completed, this, &KoMainWindow::slotSaveCompleted);
    connect(d->rootDocument.data(), &KoDocument::canceled, this, &KoMainWindow::slotSaveCanceled);

    QUrl oldURL = d->rootDocument->url();
    QString oldFile = d->rootDocument->localFilePath();

    QByteArray _native_format = d->rootDocument->nativeFormatMimeType();
    QByteArray oldOutputFormat = d->rootDocument->outputMimeType();

    int oldSpecialOutputFlag = d->rootDocument->specialOutputFlag();

    QUrl suggestedURL = d->rootDocument->url();

    QStringList mimeFilter;
    QMimeType mime = QMimeDatabase().mimeTypeForName(_native_format);
    if (!mime.isValid())
        // QT5TODO: find if there is no better way to get an object for the default type
        mime = QMimeDatabase().mimeTypeForName(QStringLiteral("application/octet-stream"));
    if (specialOutputFlag & KoDocumentBase::SaveEncrypted) {
        // ecryption only supported for the native mimetype
        mimeFilter << _native_format;
    } else if (specialOutputFlag == 0) {
        mimeFilter = KoFilterManager::mimeFilter(_native_format, KoFilterManager::Export, d->rootDocument->extraNativeMimeTypes());
    } else {
        mimeFilter = mime.globPatterns();
    }

    qWarning() << _native_format << mimeFilter;

    if (!mimeFilter.contains(oldOutputFormat) && !isExporting()) {
        debugMain << "KoMainWindow::saveDocument no export filter for" << oldOutputFormat;

        // --- don't setOutputMimeType in case the user cancels the Save As
        // dialog and then tries to just plain Save ---

        // suggest a different filename extension (yes, we fortunately don't all live in a world of magic :))
        QString suggestedFilename = suggestedURL.fileName();
        if (!suggestedFilename.isEmpty()) { // ".kra" looks strange for a name
            int c = suggestedFilename.lastIndexOf('.');

            QString ext = mime.preferredSuffix();
            if (!ext.isEmpty()) {
                ext.prepend('.');
                if (c < 0)
                    suggestedFilename += ext;
                else
                    suggestedFilename = suggestedFilename.left(c) + ext;
            } else { // current filename extension wrong anyway
                if (c > 0) {
                    // this assumes that a . signifies an extension, not just a .
                    suggestedFilename = suggestedFilename.left(c);
                }
            }

            suggestedURL = suggestedURL.adjusted(QUrl::RemoveFilename);
            suggestedURL.setPath(suggestedURL.path() + suggestedFilename);
        }

        // force the user to choose outputMimeType
        saveas = true;
    }

    bool ret = false;

    if (d->rootDocument->url().isEmpty() || saveas) {
        // if you're just File/Save As'ing to change filter options you
        // don't want to be reminded about overwriting files etc.
        bool justChangingFilterOptions = false;

        if (suggestedURL.isEmpty()) {
            suggestedURL = d->rootDocument->defaultUrl();
        }
        const auto localfile = suggestedURL.toLocalFile();
        const bool overrideDefaultDir = !suggestedURL.adjusted(QUrl::RemoveFilename).isEmpty();
        KoFileDialog dialog(this, KoFileDialog::SaveFile, "SaveDocument");
        dialog.setCaption(i18n("untitled"));
        dialog.setDefaultDir(localfile, overrideDefaultDir);
        dialog.setMimeTypeFilters(mimeFilter);
        QUrl newURL = QUrl::fromUserInput(dialog.filename());
        debugMain << newURL;
        ret = !newURL.isEmpty();
        if (ret) {
            QByteArray outputFormat = _native_format;

            if (!specialOutputFlag) {
                QMimeType mime = QMimeDatabase().mimeTypeForUrl(newURL);
                outputFormat = mime.name().toLatin1();
            }

            if (!isExporting())
                justChangingFilterOptions =
                    (newURL == d->rootDocument->url()) && (outputFormat == d->rootDocument->mimeType()) && (specialOutputFlag == oldSpecialOutputFlag);
            else
                justChangingFilterOptions =
                    (newURL == d->lastExportUrl) && (outputFormat == d->lastExportedFormat) && (specialOutputFlag == d->lastExportSpecialOutputFlag);

            bool wantToSave = true;

            // don't change this line unless you know what you're doing :)
            if (!justChangingFilterOptions || d->rootDocument->confirmNonNativeSave(isExporting())) {
                if (!d->rootDocument->isNativeFormat(outputFormat))
                    wantToSave = exportConfirmation(outputFormat);
            }

            if (wantToSave) {
                //
                // Note:
                // If the user is stupid enough to Export to the current URL,
                // we do _not_ change this operation into a Save As.  Reasons
                // follow:
                //
                // 1. A check like "isExporting() && oldURL == newURL"
                //    doesn't _always_ work on case-insensitive filesystems
                //    and inconsistent behaviour is bad.
                // 2. It is probably not a good idea to change d->rootDocument->mimeType
                //    and friends because the next time the user File/Save's,
                //    (not Save As) they won't be expecting that they are
                //    using their File/Export settings
                //
                // As a bad side-effect of this, the modified flag will not
                // be updated and it is possible that what is currently on
                // their screen is not what is stored on disk (through loss
                // of formatting).  But if you are dumb enough to change
                // mimetype but not the filename, then arguably, _you_ are
                // the "bug" :)
                //
                // - Clarence
                //

                d->rootDocument->setOutputMimeType(outputFormat, specialOutputFlag);
                if (!isExporting()) { // Save As
                    ret = d->rootDocument->saveAs(newURL);

                    if (ret) {
                        debugMain << "Successful Save As!";
                        addRecentURL(newURL);
                        setReadWrite(true);
                    } else {
                        debugMain << "Failed Save As!";
                        d->rootDocument->setUrl(oldURL);
                        d->rootDocument->setLocalFilePath(oldFile);
                        d->rootDocument->setOutputMimeType(oldOutputFormat, oldSpecialOutputFlag);
                    }
                } else { // Export
                    ret = d->rootDocument->exportDocument(newURL);

                    if (ret) {
                        // a few file dialog convenience things
                        d->lastExportUrl = newURL;
                        d->lastExportedFormat = outputFormat;
                        d->lastExportSpecialOutputFlag = specialOutputFlag;
                    }

                    // always restore output format
                    d->rootDocument->setOutputMimeType(oldOutputFormat, oldSpecialOutputFlag);
                }

                if (silent) // don't let the document change the window caption
                    d->rootDocument->setTitleModified();
            }
        }
    } else { // saving

        bool needConfirm = d->rootDocument->confirmNonNativeSave(false) && !d->rootDocument->isNativeFormat(oldOutputFormat);

        if (!needConfirm || (needConfirm && exportConfirmation(oldOutputFormat /* not so old :) */))) {
            // be sure d->rootDocument has the correct outputMimeType!
            if (isExporting() || d->rootDocument->isModified() || d->rootDocument->alwaysAllowSaving()) {
                ret = d->rootDocument->save();
            }

            if (!ret) {
                debugMain << "Failed Save!";
                d->rootDocument->setUrl(oldURL);
                d->rootDocument->setLocalFilePath(oldFile);
            }
        } else
            ret = false;
    }

    if (!ret && reset_url)
        d->rootDocument->resetURL(); // clean the suggested filename as the save dialog was rejected

    updateReloadFileAction(d->rootDocument);
    updateCaption();

    return ret;
}

void KoMainWindow::closeEvent(QCloseEvent *e)
{
    // If we are in the process of opening a new document, rootDocument() may not have been set yet,
    // so we must prevent closing to avoid crash.
    if (d->openingDocument || (rootDocument() && rootDocument()->isLoading())) {
        e->setAccepted(false);
        return;
    }
    if (queryClose()) {
        d->deferredClosingEvent = e;
        if (d->partToOpen) {
            // The open pane is visible
            d->partToOpen->deleteOpenPane(true);
        }
        if (!d->m_dockerStateBeforeHiding.isEmpty()) {
            restoreState(d->m_dockerStateBeforeHiding);
        }
        statusBar()->setVisible(true);
        menuBar()->setVisible(true);

        saveWindowSettings();
        if (d->noCleanup)
            return;
        setRootDocument(nullptr);
        if (!d->dockWidgetVisibilityMap.isEmpty()) { // re-enable dockers for persistency
            foreach (QDockWidget *dockWidget, d->dockWidgetsMap)
                dockWidget->setVisible(d->dockWidgetVisibilityMap.value(dockWidget));
        }
    } else {
        e->setAccepted(false);
    }
}

void KoMainWindow::saveWindowSettings()
{
    KSharedConfigPtr config = componentData().config();

    if (d->windowSizeDirty) {
        // Save window size into the config file of our componentData
        // TODO: check if this is ever read again, seems lost over the years
        debugMain << "KoMainWindow::saveWindowSettings";
        KConfigGroup mainWindowConfigGroup = config->group("MainWindow");
        KWindowConfig::saveWindowSize(windowHandle(), mainWindowConfigGroup);
        config->sync();
        d->windowSizeDirty = false;
    }

    if (rootDocument() && d->rootPart) {
        // Save toolbar position into the config file of the app, under the doc's component name
        KConfigGroup group = KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName());
        // debugMain <<"KoMainWindow::closeEvent -> saveMainWindowSettings rootdoc's componentData=" << d->rootPart->componentData().componentName();
        saveMainWindowSettings(group);

        // Save collapsable state of dock widgets
        for (QMap<QString, QDockWidget *>::const_iterator i = d->dockWidgetsMap.constBegin(); i != d->dockWidgetsMap.constEnd(); ++i) {
            if (i.value()->widget()) {
                KConfigGroup dockGroup = group.group(QString("DockWidget ") + i.key());
                dockGroup.writeEntry("Collapsed", i.value()->widget()->isHidden());
                dockGroup.writeEntry("Locked", i.value()->property("Locked").toBool());
                dockGroup.writeEntry("DockArea", (int)dockWidgetArea(i.value()));
            }
        }
    }

    KSharedConfig::openConfig()->sync();
    resetAutoSaveSettings(); // Don't let KMainWindow override the good stuff we wrote down
}

void KoMainWindow::resizeEvent(QResizeEvent *e)
{
    d->windowSizeDirty = true;
    KXmlGuiWindow::resizeEvent(e);
}

bool KoMainWindow::queryClose()
{
    if (rootDocument() == nullptr)
        return true;
    // debugMain <<"KoMainWindow::queryClose() viewcount=" << rootDocument()->viewCount()
    //                << " mainWindowCount=" << rootDocument()->mainWindowCount() << Qt::endl;
    if (!d->forQuit && d->rootPart && d->rootPart->mainwindowCount() > 1)
        // there are more open, and we are closing just one, so no problem for closing
        return true;

    // main doc + internally stored child documents
    if (d->rootDocument->isModified()) {
        auto url = d->rootDocument->defaultUrl();
        int res = KMessageBox::warningTwoActionsCancel(this,
                                                       i18n("<p>The document <b>'%1'</b> has been modified.</p><p>Do you want to save it?</p>", url.fileName()),
                                                       QString(),
                                                       KStandardGuiItem::save(),
                                                       KStandardGuiItem::discard());

        switch (res) {
        case KMessageBox::PrimaryAction: {
            bool isNative = (d->rootDocument->outputMimeType() == d->rootDocument->nativeFormatMimeType());
            if (!saveDocument(!isNative))
                return false;
            break;
        }
        case KMessageBox::SecondaryAction:
            rootDocument()->removeAutoSaveFiles();
            rootDocument()->setModified(false); // Now when queryClose() is called by closeEvent it won't do anything.
            break;
        default: // case KMessageBox::Cancel :
            return false;
        }
    }

    return true;
}

// Helper method for slotFileNew and slotFileClose
void KoMainWindow::chooseNewDocument(InitDocFlags initDocFlags)
{
    KoDocument *doc = rootDocument();
    KoPart *newpart = createPart();
    KoDocument *newdoc = newpart->document();

    if (!newdoc)
        return;

    disconnect(newdoc, &KoDocument::sigProgress, this, &KoMainWindow::slotProgress);

    if ((!doc && initDocFlags == InitDocFileNew) || (doc && !doc->isEmpty())) {
        KoMainWindow *s = newpart->createMainWindow();
        s->show();
        newpart->addMainWindow(s);
        newpart->showStartUpWidget(s, true /*Always show widget*/);
        return;
    }

    if (doc) {
        setRootDocument(nullptr);
    }

    newpart->addMainWindow(this);
    newpart->showStartUpWidget(this, true /*Always show widget*/);
}

void KoMainWindow::slotFileNew()
{
    chooseNewDocument(InitDocFileNew);
}

void KoMainWindow::slotFileOpen()
{
    QUrl url;
    if (!isImporting()) {
        KoFileDialog dialog(this, KoFileDialog::OpenFile, "OpenDocument");
        dialog.setCaption(i18n("Open Document"));
        dialog.setDefaultDir(qApp->applicationName().contains("karbon") ? QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
                                                                        : QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        dialog.setMimeTypeFilters(koApp->mimeFilter(KoFilterManager::Import));
        dialog.setHideNameFilterDetailsOption();
        url = QUrl::fromUserInput(dialog.filename());
    } else {
        KoFileDialog dialog(this, KoFileDialog::ImportFile, "OpenDocument");
        dialog.setCaption(i18n("Import Document"));
        dialog.setDefaultDir(qApp->applicationName().contains("karbon") ? QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
                                                                        : QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        dialog.setMimeTypeFilters(koApp->mimeFilter(KoFilterManager::Import));
        dialog.setHideNameFilterDetailsOption();
        url = QUrl::fromUserInput(dialog.filename());
    }

    if (url.isEmpty())
        return;

    (void)openDocument(url);
}

void KoMainWindow::slotFileOpenRecent(const QUrl &url)
{
    // Create a copy, because the original QUrl in the map of recent files in
    // KRecentFilesAction may get deleted.
    (void)openDocument(QUrl(url));
}

void KoMainWindow::slotFileSave()
{
    if (saveDocument())
        Q_EMIT documentSaved();
}

void KoMainWindow::slotFileSaveAs()
{
    // Need the specialOutputFlag here to avoid encrypted docs silently beeing saved unencrypted.
    if (saveDocument(true, false, d->rootDocument->specialOutputFlag()))
        Q_EMIT documentSaved();
}

void KoMainWindow::slotEncryptDocument()
{
    if (saveDocument(false, false, KoDocument::SaveEncrypted))
        Q_EMIT documentSaved();
}

void KoMainWindow::slotUncompressToDir()
{
    if (saveDocument(true, false, KoDocument::SaveAsDirectoryStore))
        Q_EMIT documentSaved();
}

void KoMainWindow::slotDocumentInfo()
{
    if (!rootDocument())
        return;

    KoDocumentInfo *docInfo = rootDocument()->documentInfo();

    if (!docInfo)
        return;

    KoDocumentInfoDlg *dlg = d->rootDocument->createDocumentInfoDialog(this, docInfo);

    if (dlg->exec()) {
        if (dlg->isDocumentSaved()) {
            rootDocument()->setModified(false);
        } else {
            rootDocument()->setModified(true);
        }
        rootDocument()->setTitleModified();
    }

    delete dlg;
}

void KoMainWindow::slotFileClose()
{
    if (queryClose()) {
        saveWindowSettings();
        setRootDocument(nullptr);
        chooseNewDocument(InitDocFileClose);
    }
}

void KoMainWindow::slotFileQuit()
{
    close();
}

void KoMainWindow::slotFilePrint()
{
    if (!rootView())
        return;
    KoPrintJob *printJob = rootView()->createPrintJob();
    if (printJob == nullptr)
        return;
    d->applyDefaultSettings(printJob->printer());
    QPrintDialog *printDialog = rootView()->createPrintDialog(printJob, this);
    if (printDialog && printDialog->exec() == QDialog::Accepted)
        printJob->startPrinting(KoPrintJob::DeleteWhenDone);
    else
        delete printJob;
    delete printDialog;
}

void KoMainWindow::slotFilePrintPreview()
{
    if (!rootView())
        return;
    KoPrintJob *printJob = rootView()->createPrintJob();
    if (printJob == nullptr)
        return;

    /* Sets the startPrinting() slot to be blocking.
     The Qt print-preview dialog requires the printing to be completely blocking
     and only return when the full document has been printed.
     By default the KoPrintingDialog is non-blocking and
     multithreading, setting blocking to true will allow it to be used in the preview dialog */
    printJob->setProperty("blocking", true);
    QPrintPreviewDialog *preview = new QPrintPreviewDialog(&printJob->printer(), this);
    printJob->setParent(preview); // will take care of deleting the job
    connect(preview, &QPrintPreviewDialog::paintRequested, printJob, [printJob]() {
        printJob->startPrinting();
    });
    preview->exec();
    delete preview;
}

KoPrintJob *KoMainWindow::exportToPdf(const QString &pdfFileName)
{
    if (!rootView())
        return nullptr;
    KoPageLayout pageLayout;
    pageLayout = rootView()->pageLayout();
    return exportToPdf(pageLayout, pdfFileName);
}

KoPrintJob *KoMainWindow::exportToPdf(const KoPageLayout &_pageLayout, const QString &_pdfFileName)
{
    if (!rootView())
        return nullptr;

    KoPageLayout pageLayout = _pageLayout;
    QString pdfFileName = _pdfFileName;

    if (pdfFileName.isEmpty()) {
        KConfigGroup group = KSharedConfig::openConfig()->group("File Dialogs");
        QString defaultDir = group.readEntry("SavePdfDialog");
        if (defaultDir.isEmpty())
            defaultDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        QUrl startUrl = QUrl::fromLocalFile(defaultDir);
        KoDocument *pDoc = rootDocument();
        /** if document has a file name, take file name and replace extension with .pdf */
        if (pDoc && pDoc->url().isValid()) {
            startUrl = pDoc->url();
            QString fileName = startUrl.fileName();
            fileName = fileName.replace(QRegularExpression("\\.\\w{2,5}$", QRegularExpression::CaseInsensitiveOption), ".pdf");
            startUrl = startUrl.adjusted(QUrl::RemoveFilename);
            startUrl.setPath(startUrl.path() + fileName);
        }

        QPointer<KoPageLayoutDialog> layoutDlg(new KoPageLayoutDialog(this, pageLayout));
        layoutDlg->setWindowModality(Qt::WindowModal);
        if (layoutDlg->exec() != QDialog::Accepted || !layoutDlg) {
            delete layoutDlg;
            return nullptr;
        }
        pageLayout = layoutDlg->pageLayout();
        delete layoutDlg;

        KoFileDialog dialog(this, KoFileDialog::SaveFile, "SaveDocument");
        dialog.setCaption(i18n("Export as PDF"));
        dialog.setDefaultDir(startUrl.toLocalFile());
        dialog.setMimeTypeFilters(QStringList() << "application/pdf");
        QUrl url = QUrl::fromUserInput(dialog.filename());

        pdfFileName = url.toLocalFile();
        if (pdfFileName.isEmpty())
            return nullptr;
    }

    KoPrintJob *printJob = rootView()->createPdfPrintJob();
    if (printJob == nullptr)
        return nullptr;
    if (isHidden()) {
        printJob->setProperty("noprogressdialog", true);
    }

    d->applyDefaultSettings(printJob->printer());
    // TODO for remote files we have to first save locally and then upload.
    printJob->printer().setOutputFileName(pdfFileName);
    printJob->printer().setColorMode(QPrinter::Color);

    if (pageLayout.format == KoPageFormat::CustomSize) {
        printJob->printer().setPageSize(QPageSize{QSizeF(pageLayout.width, pageLayout.height), QPageSize::Millimeter});
    } else {
        printJob->printer().setPageSize(KoPageFormat::printerPageSize(pageLayout.format));
    }

    switch (pageLayout.orientation) {
    case KoPageFormat::Portrait:
        printJob->printer().setPageOrientation(QPageLayout::Portrait);
        break;
    case KoPageFormat::Landscape:
        printJob->printer().setPageOrientation(QPageLayout::Landscape);
        break;
    }

    printJob->printer().setPageMargins(QMargins(pageLayout.leftMargin, pageLayout.topMargin, pageLayout.rightMargin, pageLayout.bottomMargin),
                                       QPageLayout::Unit::Millimeter);

    // before printing check if the printer can handle printing
    if (!printJob->canPrint()) {
        KMessageBox::error(this, i18n("Cannot export to the specified file"));
    }

    printJob->startPrinting(KoPrintJob::DeleteWhenDone);
    return printJob;
}

void KoMainWindow::slotConfigureKeys()
{
    QAction *undoAction = nullptr;
    QAction *redoAction = nullptr;
    QString oldUndoText;
    QString oldRedoText;
    if (currentView()) {
        // The undo/redo action text is "undo" + command, replace by simple text while inside editor
        undoAction = currentView()->actionCollection()->action("edit_undo");
        redoAction = currentView()->actionCollection()->action("edit_redo");
        oldUndoText = undoAction->toolTip();
        oldRedoText = redoAction->toolTip();
        undoAction->setToolTip(i18n("Undo"));
        redoAction->setToolTip(i18n("Redo"));
    }

    guiFactory()->showConfigureShortcutsDialog();

    if (currentView()) {
        undoAction->setToolTip(oldUndoText);
        redoAction->setToolTip(oldRedoText);
    }

    Q_EMIT keyBindingsChanged();
}

void KoMainWindow::slotConfigureToolbars()
{
    if (rootDocument()) {
        KConfigGroup componentConfigGroup = KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName());
        saveMainWindowSettings(componentConfigGroup);
    }

    KEditToolBar edit(factory(), this);
    connect(&edit, &KEditToolBar::newToolBarConfig, this, &KoMainWindow::slotNewToolbarConfig);
    (void)edit.exec();
}

void KoMainWindow::slotNewToolbarConfig()
{
    if (rootDocument()) {
        KConfigGroup componentConfigGroup = KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName());
        applyMainWindowSettings(componentConfigGroup);
    }

    KXMLGUIFactory *factory = guiFactory();
    Q_UNUSED(factory);

    // Check if there's an active view
    if (!d->activeView)
        return;

    plugActionList("toolbarlist", d->toolbarList);
}

void KoMainWindow::slotToolbarToggled(bool toggle)
{
    // debugMain <<"KoMainWindow::slotToolbarToggled" << sender()->name() <<" toggle=" << true;
    //  The action (sender) and the toolbar have the same name
    KToolBar *bar = toolBar(sender()->objectName());
    if (bar) {
        if (toggle)
            bar->show();
        else
            bar->hide();

        if (rootDocument()) {
            KConfigGroup componentConfigGroup = KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName());
            saveMainWindowSettings(componentConfigGroup);
        }
    } else
        warnMain << "slotToolbarToggled : Toolbar " << sender()->objectName() << " not found!";
}

bool KoMainWindow::toolbarIsVisible(const char *tbName)
{
    QWidget *tb = toolBar(tbName);
    return !tb->isHidden();
}

void KoMainWindow::showToolbar(const char *tbName, bool shown)
{
    QWidget *tb = toolBar(tbName);
    if (!tb) {
        warnMain << "KoMainWindow: toolbar " << tbName << " not found.";
        return;
    }
    if (shown)
        tb->show();
    else
        tb->hide();

    // Update the action appropriately
    foreach (QAction *action, d->toolbarList) {
        if (action->objectName() != tbName) {
            // debugMain <<"KoMainWindow::showToolbar setChecked" << shown;
            static_cast<KToggleAction *>(action)->setChecked(shown);
            break;
        }
    }
}

void KoMainWindow::viewFullscreen(bool fullScreen)
{
    if (fullScreen) {
        window()->setWindowState(window()->windowState() | Qt::WindowFullScreen); // set
    } else {
        window()->setWindowState(window()->windowState() & ~Qt::WindowFullScreen); // reset
    }
}

void KoMainWindow::slotProgress(int value)
{
    QMutexLocker locker(&d->progressMutex);
    debugMain << "KoMainWindow::slotProgress" << value;
    if (value <= -1 || value >= 100) {
        if (d->progress) {
            statusBar()->removeWidget(d->progress);
            delete d->progress;
            d->progress = nullptr;
            statusBar()->hide();
        }
        d->firstTime = true;
        return;
    }
    if (d->firstTime || !d->progress) {
        // The statusbar might not even be created yet.
        // So check for that first, and create it if necessary
        QStatusBar *bar = findChild<QStatusBar *>();
        if (!bar) {
            statusBar()->show();
            QApplication::sendPostedEvents(this, QEvent::ChildAdded);
        }

        if (d->progress) {
            statusBar()->removeWidget(d->progress);
            delete d->progress;
            d->progress = nullptr;
        }

        d->progress = new QProgressBar(statusBar());
        d->progress->setMaximumHeight(statusBar()->fontMetrics().height());
        d->progress->setRange(0, 100);
        statusBar()->addPermanentWidget(d->progress);
        d->progress->show();
        d->firstTime = false;
    }
    if (!d->progress.isNull()) {
        d->progress->setValue(value);
    }
    locker.unlock();
    qApp->processEvents();
}

void KoMainWindow::setMaxRecentItems(uint _number)
{
    d->recent->setMaxItems(_number);
}

void KoMainWindow::slotEmailFile()
{
    if (!rootDocument())
        return;

    // Subject = Document file name
    // Attachment = The current file
    // Message Body = The current document in HTML export? <-- This may be an option.
    QString theSubject;
    QList<QUrl> urls;
    QUrl fileURL;
    if (rootDocument()->url().isEmpty() || rootDocument()->isModified()) {
        // Save the file as a temporary file
        bool const tmp_modified = rootDocument()->isModified();
        QUrl const tmp_url = rootDocument()->url();
        QByteArray const tmp_mimetype = rootDocument()->outputMimeType();

        // a little open, close, delete dance to make sure we have a nice filename
        // to use, but won't block windows from creating a new file with this name.
        QTemporaryFile *tmpfile = new QTemporaryFile();
        tmpfile->open();
        QString fileName = tmpfile->fileName();
        tmpfile->close();
        delete tmpfile;

        QUrl u = QUrl::fromLocalFile(fileName);
        rootDocument()->setUrl(u);
        rootDocument()->setModified(true);
        rootDocument()->setOutputMimeType(rootDocument()->nativeFormatMimeType());

        saveDocument(false, true);

        fileURL = QUrl::fromLocalFile(fileName);
        theSubject = i18n("Document");
        urls.append(fileURL);

        rootDocument()->setUrl(tmp_url);
        rootDocument()->setModified(tmp_modified);
        rootDocument()->setOutputMimeType(tmp_mimetype);
    } else {
        fileURL = rootDocument()->url();
        theSubject = i18n("Document - %1", rootDocument()->url().fileName());
        urls.append(fileURL);
    }

    debugMain << "(" << fileURL << ")";

    if (!fileURL.isEmpty()) {
        auto *job = new KEMailClientLauncherJob;
        job->setSubject(theSubject);
        job->setAttachments(urls);
        job->start();
    }
}

void KoMainWindow::slotVersionsFile()
{
    if (!rootDocument())
        return;
    auto dlg = new KoVersionDialog(this, rootDocument());
    dlg->exec();
    delete dlg;
}

void KoMainWindow::slotReloadFile()
{
    KoDocument *pDoc = rootDocument();
    if (!pDoc || pDoc->url().isEmpty() || !pDoc->isModified())
        return;

    bool bOk = KMessageBox::questionTwoActions(this,
                                               i18n("You will lose all changes made since your last save\n"
                                                    "Do you want to continue?"),
                                               i18n("Warning"),
                                               KStandardGuiItem::cont(),
                                               KStandardGuiItem::cancel())
        == KMessageBox::PrimaryAction;
    if (!bOk)
        return;

    QUrl url = pDoc->url();
    if (!pDoc->isEmpty()) {
        saveWindowSettings();
        setRootDocument(nullptr); // don't delete this main window when deleting the document
        if (d->rootDocument)
            d->rootDocument->clearUndoHistory();
        delete d->rootDocument;
        d->rootDocument = nullptr;
    }
    openDocument(url);
    return;
}

void KoMainWindow::slotImportFile()
{
    debugMain << "slotImportFile()";

    d->isImporting = true;
    slotFileOpen();
    d->isImporting = false;
}

void KoMainWindow::slotExportFile()
{
    debugMain << "slotExportFile()";

    d->isExporting = true;
    slotFileSaveAs();
    d->isExporting = false;
}

bool KoMainWindow::isImporting() const
{
    return d->isImporting;
}

bool KoMainWindow::isExporting() const
{
    return d->isExporting;
}

void KoMainWindow::setPartToOpen(KoPart *part)
{
    d->partToOpen = part;
}

KoComponentData KoMainWindow::componentData() const
{
    return d->componentData;
}

QDockWidget *KoMainWindow::createDockWidget(KoDockFactoryBase *factory)
{
    QDockWidget *dockWidget = nullptr;

    if (!d->dockWidgetsMap.contains(factory->id())) {
        dockWidget = factory->createDockWidget();

        // It is quite possible that a dock factory cannot create the dock; don't
        // do anything in that case.
        if (!dockWidget)
            return nullptr;
        d->dockWidgets.push_back(dockWidget);

        KoDockWidgetTitleBar *titleBar = nullptr;
        // Check if the dock widget is supposed to be collapsable
        if (!dockWidget->titleBarWidget()) {
            titleBar = new KoDockWidgetTitleBar(dockWidget);
            dockWidget->setTitleBarWidget(titleBar);
            titleBar->setCollapsable(factory->isCollapsable());
        }

        dockWidget->setObjectName(factory->id());
        dockWidget->setParent(this);

        Qt::DockWidgetArea side = Qt::RightDockWidgetArea;
        bool visible = factory->defaultVisible();

        switch (factory->defaultDockPosition()) {
        case KoDockFactoryBase::DockTornOff:
            dockWidget->setFloating(true); // position nicely?
            break;
        case KoDockFactoryBase::DockTop:
            side = Qt::TopDockWidgetArea;
            break;
        case KoDockFactoryBase::DockLeft:
            side = Qt::LeftDockWidgetArea;
            break;
        case KoDockFactoryBase::DockBottom:
            side = Qt::BottomDockWidgetArea;
            break;
        case KoDockFactoryBase::DockRight:
            side = Qt::RightDockWidgetArea;
            break;
        case KoDockFactoryBase::DockMinimized:
        default:
            side = Qt::RightDockWidgetArea;
            visible = false;
        }

        if (rootDocument()) {
            KConfigGroup group = KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName()).group("DockWidget " + factory->id());
            side = static_cast<Qt::DockWidgetArea>(group.readEntry("DockArea", static_cast<int>(side)));
            if (side == Qt::NoDockWidgetArea)
                side = Qt::RightDockWidgetArea;
        }
        addDockWidget(side, dockWidget);
        if (dockWidget->features() & QDockWidget::DockWidgetClosable) {
            d->dockWidgetMenu->addAction(dockWidget->toggleViewAction());
            if (!visible)
                dockWidget->hide();
        }

        bool collapsed = factory->defaultCollapsed();
        bool locked = false;
        if (rootDocument()) {
            KConfigGroup group = KSharedConfig::openConfig()->group(d->rootPart->componentData().componentName()).group("DockWidget " + factory->id());
            collapsed = group.readEntry("Collapsed", collapsed);
            locked = group.readEntry("Locked", locked);
        }
        if (titleBar && collapsed)
            titleBar->setCollapsed(true);
        if (titleBar && locked)
            titleBar->setLocked(true);

        if (titleBar) {
            KConfigGroup configGroupInterface = KSharedConfig::openConfig()->group("Interface");
            titleBar->setVisible(configGroupInterface.readEntry("ShowDockerTitleBars", true));
        }

        d->dockWidgetsMap.insert(factory->id(), dockWidget);
    } else {
        dockWidget = d->dockWidgetsMap[factory->id()];
    }

#ifdef Q_OS_MAC
    dockWidget->setAttribute(Qt::WA_MacSmallSize, true);
#endif
    dockWidget->setFont(KoDockRegistry::dockFont());

    connect(dockWidget, &QDockWidget::dockLocationChanged, this, &KoMainWindow::forceDockTabFonts);

    return dockWidget;
}

void KoMainWindow::forceDockTabFonts()
{
    QObjectList chis = children();
    for (int i = 0; i < chis.size(); ++i) {
        if (chis.at(i)->inherits("QTabBar")) {
            ((QTabBar *)chis.at(i))->setFont(KoDockRegistry::dockFont());
        }
    }
}

QList<QDockWidget *> KoMainWindow::dockWidgets() const
{
    return d->dockWidgetsMap.values();
}

QList<KoCanvasObserverBase *> KoMainWindow::canvasObservers() const
{
    QList<KoCanvasObserverBase *> observers;

    foreach (QDockWidget *docker, dockWidgets()) {
        KoCanvasObserverBase *observer = dynamic_cast<KoCanvasObserverBase *>(docker);
        if (observer) {
            observers << observer;
        }
    }
    return observers;
}

KoDockerManager *KoMainWindow::dockerManager() const
{
    return d->dockerManager;
}

void KoMainWindow::toggleDockersVisibility(bool visible)
{
    if (!visible) {
        d->m_dockerStateBeforeHiding = saveState();

        foreach (QObject *widget, children()) {
            if (widget->inherits("QDockWidget")) {
                QDockWidget *dw = static_cast<QDockWidget *>(widget);
                if (dw->isVisible()) {
                    dw->hide();
                }
            }
        }
    } else {
        restoreState(d->m_dockerStateBeforeHiding);
    }
}

KRecentFilesAction *KoMainWindow::recentAction() const
{
    return d->recent;
}

KoView *KoMainWindow::currentView() const
{
    return rootView();
}

void KoMainWindow::newView()
{
    Q_ASSERT((d != nullptr && d->activeView && d->activePart && d->activeView->koDocument()));

    KoMainWindow *mainWindow = d->activePart->createMainWindow();
    mainWindow->setRootDocument(d->activeView->koDocument(), d->activePart);
    mainWindow->show();
}

void KoMainWindow::createMainwindowGUI()
{
    if (isHelpMenuEnabled() && !d->m_helpMenu) {
        d->m_helpMenu = new KHelpMenu(this, componentData().aboutData(), true);

        KActionCollection *actions = actionCollection();
        QAction *helpContentsAction = d->m_helpMenu->action(KHelpMenu::menuHelpContents);
        QAction *whatsThisAction = d->m_helpMenu->action(KHelpMenu::menuWhatsThis);
        QAction *reportBugAction = d->m_helpMenu->action(KHelpMenu::menuReportBug);
        QAction *switchLanguageAction = d->m_helpMenu->action(KHelpMenu::menuSwitchLanguage);
        QAction *aboutAppAction = d->m_helpMenu->action(KHelpMenu::menuAboutApp);
        QAction *aboutKdeAction = d->m_helpMenu->action(KHelpMenu::menuAboutKDE);

        if (helpContentsAction) {
            actions->addAction(helpContentsAction->objectName(), helpContentsAction);
        }
        if (whatsThisAction) {
            actions->addAction(whatsThisAction->objectName(), whatsThisAction);
        }
        if (reportBugAction) {
            actions->addAction(reportBugAction->objectName(), reportBugAction);
        }
        if (switchLanguageAction) {
            actions->addAction(switchLanguageAction->objectName(), switchLanguageAction);
        }
        if (aboutAppAction) {
            actions->addAction(aboutAppAction->objectName(), aboutAppAction);
        }
        if (aboutKdeAction) {
            actions->addAction(aboutKdeAction->objectName(), aboutKdeAction);
        }
    }

    QString f = xmlFile();
    setXMLFile(QStandardPaths::locate(QStandardPaths::ConfigLocation, QStringLiteral("ui/ui_standards.rc")));
    if (!f.isEmpty())
        setXMLFile(f, true);
    else {
        QString auto_file(componentData().componentName() + "ui.rc");
        setXMLFile(auto_file, true);
    }

    guiFactory()->addClient(this);
}

// PartManager

void KoMainWindow::removePart(KoPart *part)
{
    if (d->m_registeredPart.data() != part) {
        return;
    }
    d->m_registeredPart = nullptr;
    if (part == d->m_activePart) {
        setActivePart(nullptr, nullptr);
    }
}

void KoMainWindow::setActivePart(KoPart *part, QWidget *widget)
{
    if (part && d->m_registeredPart.data() != part) {
        warnMain << "trying to activate a non-registered part!" << part->objectName();
        return; // don't allow someone call setActivePart with a part we don't know about
    }

    // don't activate twice
    if (d->m_activePart && part && d->m_activePart == part && (!widget || d->m_activeWidget == widget))
        return;

    KoPart *oldActivePart = d->m_activePart;
    QWidget *oldActiveWidget = d->m_activeWidget;

    d->m_activePart = part;
    d->m_activeWidget = widget;

    if (oldActivePart) {
        KoPart *savedActivePart = part;
        QWidget *savedActiveWidget = widget;

        if (oldActiveWidget) {
            disconnect(oldActiveWidget, &QObject::destroyed, this, &KoMainWindow::slotWidgetDestroyed);
        }

        d->m_activePart = savedActivePart;
        d->m_activeWidget = savedActiveWidget;
    }

    if (d->m_activePart && d->m_activeWidget) {
        connect(d->m_activeWidget, &QObject::destroyed, this, &KoMainWindow::slotWidgetDestroyed);
    }
    // Set the new active instance in KGlobal
    //     KGlobal::setActiveComponent(d->m_activePart ? d->m_activePart->componentData() : KGlobal::mainComponent());

    // old slot called from part manager
    KoPart *newPart = static_cast<KoPart *>(d->m_activePart.data());

    if (d->activePart && d->activePart == newPart) {
        // debugMain <<"no need to change the GUI";
        return;
    }

    KXMLGUIFactory *factory = guiFactory();

    if (d->activeView) {
        factory->removeClient(d->activeView);

        unplugActionList("toolbarlist");
        qDeleteAll(d->toolbarList);
        d->toolbarList.clear();
    }

    if (!d->mainWindowGuiIsBuilt) {
        createMainwindowGUI();
    }

    if (newPart && d->m_activeWidget && d->m_activeWidget->inherits("KoView")) {
        d->activeView = qobject_cast<KoView *>(d->m_activeWidget);
        d->activeView->actionCollection()->addAction("view_newview", actionCollection()->action("view_newview"));
        d->activePart = newPart;
        // debugMain <<"new active part is" << d->activePart;

        factory->addClient(d->activeView);

        // Position and show toolbars according to user's preference
        setAutoSaveSettings(newPart->componentData().componentName(), false);

        KConfigGroup configGroupInterface = KSharedConfig::openConfig()->group("Interface");
        const bool showDockerTitleBar = configGroupInterface.readEntry("ShowDockerTitleBars", true);
        foreach (QDockWidget *wdg, d->dockWidgets) {
            if ((wdg->features() & QDockWidget::DockWidgetClosable) == 0) {
                if (wdg->titleBarWidget()) {
                    wdg->titleBarWidget()->setVisible(showDockerTitleBar);
                }
                wdg->setVisible(true);
            }
        }

        // Create and plug toolbar list for Settings menu
        foreach (QWidget *it, factory->containers("ToolBar")) {
            KToolBar *toolBar = ::qobject_cast<KToolBar *>(it);
            if (toolBar) {
                KToggleAction *act = new KToggleAction(i18n("Show %1 Toolbar", toolBar->windowTitle()), this);
                actionCollection()->addAction(toolBar->objectName().toUtf8(), act);
                act->setCheckedState(KGuiItem(i18n("Hide %1 Toolbar", toolBar->windowTitle())));
                connect(act, &QAction::toggled, this, &KoMainWindow::slotToolbarToggled);
                act->setChecked(!toolBar->isHidden());
                d->toolbarList.append(act);
            } else
                warnMain << "Toolbar list contains a " << it->metaObject()->className() << " which is not a toolbar!";
        }
        plugActionList("toolbarlist", d->toolbarList);

    } else {
        d->activeView = nullptr;
        d->activePart = nullptr;
    }

    if (d->activeView) {
        d->activeView->guiActivateEvent(true);
    }
}

void KoMainWindow::slotWidgetDestroyed()
{
    debugMain;
    if (static_cast<const QWidget *>(sender()) == d->m_activeWidget)
        setActivePart(nullptr, nullptr); // do not remove the part because if the part's widget dies, then the
    // part will delete itself anyway, invoking removePart() in its destructor
}

void KoMainWindow::slotDocumentTitleModified(const QString &caption, bool mod)
{
    updateCaption(caption, mod);
    updateReloadFileAction(d->rootDocument);
    updateVersionsFileAction(d->rootDocument);
}

void KoMainWindow::showDockerTitleBars(bool show)
{
    foreach (QDockWidget *dock, dockWidgets()) {
        if (dock->titleBarWidget()) {
            dock->titleBarWidget()->setVisible(show);
        }
    }

    KConfigGroup configGroupInterface = KSharedConfig::openConfig()->group("Interface");
    configGroupInterface.writeEntry("ShowDockerTitleBars", show);
}
