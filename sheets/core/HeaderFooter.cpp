/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 Stefan Nikolaus <stefan.nikolaus@kdemail.net>
   SPDX-FileCopyrightText: 2005 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
   SPDX-FileCopyrightText: 2003 Philipp Müller <philipp.mueller@gmx.de>
   SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "HeaderFooter.h"

#include "engine/SheetsDebug.h"

#include "DocBase.h"
#include "Sheet.h"

#include <KoDocumentInfo.h>

#include <KLocalizedString>
#include <QUrl>

#ifndef Q_OS_WIN
#include <pwd.h>
#endif
#include <unistd.h>

using namespace Calligra::Sheets;

HeaderFooter::HeaderFooter(Sheet *sheet)
    : m_pSheet(sheet)
{
}

HeaderFooter::~HeaderFooter() = default;

void HeaderFooter::replaceHeadFootLineMacro(QString &_text, const QString &_search, const QString &_replace) const
{
    if (_search != _replace)
        _text.replace(QString('<' + _search + '>'), '<' + _replace + '>');
}

QString HeaderFooter::localizeHeadFootLine(const QString &_text) const
{
    QString tmp = _text;

    /*
      i18n:
      Please use the same words (even upper/lower case) as in
      KoPageLayoutDia.cc function setupTab2(), without the brackets "<" and ">"
    */
    replaceHeadFootLineMacro(tmp, "page", i18n("page"));
    replaceHeadFootLineMacro(tmp, "pages", i18n("pages"));
    replaceHeadFootLineMacro(tmp, "file", i18n("file"));
    replaceHeadFootLineMacro(tmp, "name", i18n("name"));
    replaceHeadFootLineMacro(tmp, "time", i18n("time"));
    replaceHeadFootLineMacro(tmp, "date", i18n("date"));
    replaceHeadFootLineMacro(tmp, "author", i18n("author"));
    replaceHeadFootLineMacro(tmp, "email", i18n("email"));
    replaceHeadFootLineMacro(tmp, "org", i18n("org"));
    replaceHeadFootLineMacro(tmp, "sheet", i18n("sheet"));

    return tmp;
}

QString HeaderFooter::delocalizeHeadFootLine(const QString &_text) const
{
    QString tmp = _text;

    /*
      i18n:
      Please use the same words (even upper/lower case) as in
      KoPageLayoutDia.cc function setupTab2(), without the brackets "<" and ">"
    */
    replaceHeadFootLineMacro(tmp, i18n("page"), "page");
    replaceHeadFootLineMacro(tmp, i18n("pages"), "pages");
    replaceHeadFootLineMacro(tmp, i18n("file"), "file");
    replaceHeadFootLineMacro(tmp, i18n("name"), "name");
    replaceHeadFootLineMacro(tmp, i18n("time"), "time");
    replaceHeadFootLineMacro(tmp, i18n("date"), "date");
    replaceHeadFootLineMacro(tmp, i18n("author"), "author");
    replaceHeadFootLineMacro(tmp, i18n("email"), "email");
    replaceHeadFootLineMacro(tmp, i18n("org"), "org");
    replaceHeadFootLineMacro(tmp, i18n("sheet"), "sheet");

    return tmp;
}

void HeaderFooter::setHeadFootLine(const QString &_headl,
                                   const QString &_headm,
                                   const QString &_headr,
                                   const QString &_footl,
                                   const QString &_footm,
                                   const QString &_footr)
{
    m_headLeft = _headl;
    m_headRight = _headr;
    m_headMid = _headm;
    m_footLeft = _footl;
    m_footRight = _footr;
    m_footMid = _footm;
    if (m_pSheet->doc())
        m_pSheet->doc()->setModified(true);
}

QString HeaderFooter::completeHeading(const QString &_data, int _page, int _pageCount, const QString &_sheet) const
{
    QString page(QString::number(_page));
    QString pages(QString::number(_pageCount));

    QString pathFileName(m_pSheet->doc()->url().path());
    if (pathFileName.isNull())
        pathFileName = "";

    QString fileName(m_pSheet->doc()->url().fileName());
    if (fileName.isNull())
        fileName = "";

    QString t(QTime::currentTime().toString());
    QString d(QDate::currentDate().toString());
    QString ta;
    if (!_sheet.isEmpty())
        ta = _sheet;

    KoDocumentInfo *info = m_pSheet->doc()->documentInfo();
    QString full_name;
    QString email_addr;
    QString organization;
    QString tmp;
    if (!info)
        warnSheets << "Author information not found in Document Info !";
    else {
        full_name = info->authorInfo("creator");
        email_addr = info->authorInfo("email");
        organization = info->authorInfo("company");
    }

// FIXME Get user on windows too
#ifndef Q_OS_WIN
    char hostname[80];
    struct passwd *p = getpwuid(getuid());
    gethostname(hostname, sizeof(hostname));

#ifndef Q_OS_ANDROID
    if (full_name.isEmpty())
        full_name = p->pw_gecos;
#endif

    if (email_addr.isEmpty())
        email_addr = QString("%1@%2").arg(p->pw_name).arg(hostname);
#endif

    tmp = _data;
    int pos = 0;
    while ((pos = tmp.indexOf("<page>", pos)) != -1)
        tmp.replace(pos, 6, page);
    pos = 0;
    while ((pos = tmp.indexOf("<pages>", pos)) != -1)
        tmp.replace(pos, 7, pages);
    pos = 0;
    while ((pos = tmp.indexOf("<file>", pos)) != -1)
        tmp.replace(pos, 6, pathFileName);
    pos = 0;
    while ((pos = tmp.indexOf("<name>", pos)) != -1)
        tmp.replace(pos, 6, fileName);
    pos = 0;
    while ((pos = tmp.indexOf("<time>", pos)) != -1)
        tmp.replace(pos, 6, t);
    pos = 0;
    while ((pos = tmp.indexOf("<date>", pos)) != -1)
        tmp.replace(pos, 6, d);
    pos = 0;
    while ((pos = tmp.indexOf("<author>", pos)) != -1)
        tmp.replace(pos, 8, full_name);
    pos = 0;
    while ((pos = tmp.indexOf("<email>", pos)) != -1)
        tmp.replace(pos, 7, email_addr);
    pos = 0;
    while ((pos = tmp.indexOf("<org>", pos)) != -1)
        tmp.replace(pos, 5, organization);
    pos = 0;
    while ((pos = tmp.indexOf("<sheet>", pos)) != -1)
        tmp.replace(pos, 7, ta);

    return tmp;
}
