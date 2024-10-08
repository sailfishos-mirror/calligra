/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009-2010 Benjamin Port <port.benjamin@gmail.com>
   SPDX-FileCopyrightText: 2009 Yannick Motta <yannick.motta@gmail.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KPrHtmlExport.h"

#include <QDesktopServices>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextStream>
#include <QUrl>

#include <KIO/JobUiDelegateFactory>
#include <KMessageBox>
#include <kio/copyjob.h>
#include <kzip.h>

#include "KPrPage.h"
#include "KPrView.h"

KPrHtmlExport::KPrHtmlExport() = default;

KPrHtmlExport::~KPrHtmlExport() = default;

void KPrHtmlExport::exportHtml(const KPrHtmlExport::Parameter &parameters)
{
    m_parameters = parameters;

    // Create a temporary dir
    QTemporaryDir tmpDir;
    m_tmpDirPath = tmpDir.path();
    tmpDir.setAutoRemove(false);
    extractStyle();
    exportImageToTmpDir();
    generateHtml();
    generateToc();
    copyFromTmpToDest();
}

void KPrHtmlExport::extractStyle()
{
    KZip zip(m_parameters.styleUrl.toLocalFile());
    zip.open(QIODevice::ReadOnly);
    zip.directory()->copyTo(m_tmpDirPath, true);
}

QUrl KPrHtmlExport::exportPreview(const Parameter &parameters)
{
    m_parameters = parameters;

    // Create a temporary dir
    QTemporaryDir tmpDir;
    tmpDir.setAutoRemove(false);
    m_tmpDirPath = tmpDir.path();
    extractStyle();
    exportImageToTmpDir();
    generateHtml();

    QUrl previewUrl = QUrl::fromLocalFile(tmpDir.path() + QLatin1String("/slide0.html"));
    return previewUrl;
}

void KPrHtmlExport::exportImageToTmpDir()
{
    // Export slides as image into the temporary export directory
    QUrl fileUrl;
    for (int i = 0; i < m_parameters.slides.size(); ++i) {
        fileUrl = QUrl::fromLocalFile(m_tmpDirPath + QString::fromLatin1("/slide%1.png").arg(i));
        KoPAPageBase *slide = m_parameters.slides.at(i);
        m_parameters.kprView->exportPageThumbnail(slide, fileUrl, slide->size().toSize(), "PNG", -1);
    }
}

void KPrHtmlExport::generateHtml()
{
    QFile file(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("calligrastage/templates/exportHTML/slides.html")));
    file.open(QIODevice::ReadOnly);
    QString slideContent = file.readAll();
    file.close();
    // Create html slide file
    int nbSlides = m_parameters.slides.size();
    for (int i = 0; i < m_parameters.slidesNames.size(); ++i) {
        QString content = slideContent;
        content.replace("::TITLE::", m_parameters.title);
        content.replace("::AUTHOR::", m_parameters.author);
        content.replace("::IMAGE_PATH::", QString("slide%1.png").arg(i));
        content.replace("::SLIDE_NUM::", QString("%1").arg(i + 1));
        content.replace("::NB_SLIDES::", QString("%1").arg(nbSlides));
        content.replace("::TITLE_SLIDE::", m_parameters.slidesNames.at(i));
        content.replace("::LAST_PATH::", QString("slide%1.html").arg(nbSlides - 1));
        content.replace("::NEXT_PATH::", QString("slide%1.html").arg(((i + 1) < nbSlides) ? i + 1 : i));
        content.replace("::PREVIOUS_PATH::", QString("slide%1.html").arg((i > 0) ? i - 1 : 0));
        content.replace("::FIRST_PATH::", QString("slide0.html"));
        writeHtmlFileToTmpDir(QString("slide%1.html").arg(i), content);
    }
}

void KPrHtmlExport::generateToc()
{
    QString toc = "<ul>";
    for (int i = 0; i < m_parameters.slidesNames.size(); ++i) {
        toc.append(QString("<li><a href=\"slide%1.html\">%2</a></li>").arg(i).arg(m_parameters.slidesNames.at(i)));
    }
    toc.append("</ul>");
    QFile file(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("calligrastage/templates/exportHTML/toc.html")));
    file.open(QIODevice::ReadOnly);
    QString content = file.readAll();
    file.close();
    content.replace("::TITLE::", m_parameters.title);
    content.replace("::AUTHOR::", m_parameters.author);
    content.replace("::TOC::", toc);
    writeHtmlFileToTmpDir("index.html", content);
}

void KPrHtmlExport::writeHtmlFileToTmpDir(const QString &fileName, const QString &htmlBody)
{
    const QString filePath = m_tmpDirPath + QLatin1Char('/') + fileName;
    QFile file(filePath);
    file.open(QIODeviceBase::WriteOnly);
    QTextStream stream(&file);
    stream << htmlBody;
}

void KPrHtmlExport::copyFromTmpToDest()
{
    KIO::CopyJob *job = KIO::moveAs(QUrl::fromLocalFile(m_tmpDirPath), m_parameters.destination);
    job->setWriteIntoExistingDirectories(true);
    job->setUiDelegate(KIO::createDefaultJobUiDelegate());
    connect(job, &KJob::result, this, &KPrHtmlExport::moveResult);
    job->exec();
}

void KPrHtmlExport::moveResult(KJob *job)
{
    QDir((m_tmpDirPath)).removeRecursively();
    if (job->error()) {
        KMessageBox::error(m_parameters.kprView, job->errorText());
    } else {
        if (m_parameters.openBrowser) {
            QUrl url = m_parameters.destination;
            url.setPath(url.path() + QLatin1String("/index.html"));
            QDesktopServices::openUrl(url);
        }
    }
}
