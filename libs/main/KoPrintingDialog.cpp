/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007, 2009 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2011 Boudewijn Rempt <boud@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "KoPrintingDialog.h"
#include "KoPrintingDialog_p.h"
#include "KoProgressUpdater.h"

#include <KoProgressBar.h>
#include <KoShape.h>
#include <KoShapeManager.h>
#include <KoZoomHandler.h>

#include <KLocalizedString>
#include <MainDebug.h>
#include <QCoreApplication>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QPrinter>
#include <QPushButton>
#include <QThread>
#include <QTimer>

class PrintDialog : public QDialog
{
public:
    PrintDialog(KoPrintingDialogPrivate *d, QWidget *parent)
        : QDialog(parent)
    {
        setModal(true);
        QGridLayout *grid = new QGridLayout(this);
        setLayout(grid);

        d->pageNumber = new QLabel(this);
        d->pageNumber->setMinimumWidth(200);
        grid->addWidget(d->pageNumber, 0, 0, 1, 2);
        KoProgressBar *bar = new KoProgressBar(this);
        d->progress = new KoProgressUpdater(bar);
        grid->addWidget(bar, 1, 0, 1, 2);
        d->button = new QPushButton(i18n("Stop"), this);
        grid->addWidget(d->button, 2, 1);
        grid->setColumnStretch(0, 1);
    }
};

KoPrintingDialog::KoPrintingDialog(QWidget *parent)
    : KoPrintJob(parent)
    , d(new KoPrintingDialogPrivate(this))
{
    d->dialog = new PrintDialog(d, parent);

    connect(d->button, &QPushButton::released, this, [this]() {
        d->stopPressed();
    });
}

KoPrintingDialog::~KoPrintingDialog()
{
    d->stopPressed();
    delete d;
}

void KoPrintingDialog::setShapeManager(KoShapeManager *sm)
{
    d->shapeManager = sm;
}

KoShapeManager *KoPrintingDialog::shapeManager() const
{
    return d->shapeManager;
}

void KoPrintingDialog::setPageRange(const QList<int> &pages)
{
    if (d->index == 0) // can't change after we started
        d->pageRange = pages;
}

QPainter &KoPrintingDialog::painter() const
{
    if (d->painter == nullptr) {
        d->painter = new QPainter(d->printer);
        d->painter->save(); // state before page preparation (3)
    }
    return *d->painter;
}

bool KoPrintingDialog::isStopped() const
{
    return d->stop;
}

void KoPrintingDialog::startPrinting(RemovePolicy removePolicy)
{
    d->removePolicy = removePolicy;
    d->pages = d->pageRange;
    if (d->pages.isEmpty()) { // auto-fill from min/max
        switch (d->printer->printRange()) {
        case QPrinter::AllPages:
            for (int i = documentFirstPage(); i <= documentLastPage(); i++)
                d->pages.append(i);
            break;
        case QPrinter::PageRange:
            for (int i = d->printer->fromPage(); i <= d->printer->toPage(); i++)
                d->pages.append(i);
            break;
        case QPrinter::CurrentPage:
            d->pages.append(documentCurrentPage());
            break;
        default:
            return;
        }
    }
    if (d->pages.isEmpty()) {
        qWarning(/*30004*/) << "KoPrintingDialog::startPrinting: No pages to print, did you forget to call setPageRange()?";
        return;
    }

    const bool blocking = property("blocking").toBool();
    const bool noprogressdialog = property("noprogressdialog").toBool();
    if (d->index == 0 && d->pages.count() > 0 && d->printer) {
        if (!blocking && !noprogressdialog)
            d->dialog->show();
        d->stop = false;
        delete d->painter;
        d->painter = nullptr;
        d->zoomer.setZoom(1.0);
        d->zoomer.setDpi(d->printer->resolution(), d->printer->resolution());

        d->progress->start(100, i18n("Printing"));

        if (d->printer->copyCount() > 1) {
            QList<int> oldPages = d->pages;
            if (d->printer->collateCopies()) { // means we print whole doc at once
                for (int count = 1; count < d->printer->copyCount(); ++count)
                    d->pages.append(oldPages);
            } else {
                d->pages.clear();
                foreach (int page, oldPages) {
                    for (int count = 1; count < d->printer->copyCount(); ++count)
                        d->pages.append(page);
                }
            }
        }
        if (d->printer->pageOrder() == QPrinter::LastPageFirst) {
            const QList<int> pages = d->pages;
            d->pages.clear();
            QList<int>::ConstIterator iter = pages.end();
            do {
                --iter;
                d->pages << *iter;
            } while (iter != pages.begin());
        }

        d->resetValues();
        foreach (int page, d->pages) {
            d->index++;
            d->updaters.append(d->progress->startSubtask()); // one per page
            d->preparePage(page);
            d->printPage(page);
            if (!blocking) {
                qApp->processEvents();
            }
        }
        d->painter->end();
        if (blocking) {
            printingDone();
        } else {
            d->printingDone();
        }
        d->stop = true;
        d->resetValues();
    }
}

QPrinter &KoPrintingDialog::printer()
{
    return *d->printer;
}

void KoPrintingDialog::printPage(int, QPainter &)
{
}

QRectF KoPrintingDialog::preparePage(int)
{
    return QRectF();
}

// have to include this because of Q_PRIVATE_SLOT
#include <moc_KoPrintingDialog.cpp>
