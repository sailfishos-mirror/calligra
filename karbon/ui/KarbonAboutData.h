/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2004 Rob Buis <buis@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KARBON_ABOUTDATA_H
#define KARBON_ABOUTDATA_H

#include <KAboutData>
#include <KLocalizedString>
#include <kcoreaddons_version.h>

#include <calligra-version.h>

// This is all implemented here so that the executable and the part can share it
// without sharing an object file.
KAboutData *newKarbonAboutData()
{
    KAboutData *aboutData = new KAboutData(QStringLiteral("karbon"),
                                           i18n("Karbon"),
                                           QStringLiteral(CALLIGRA_VERSION_STRING),
                                           i18n("A Vector Graphics Drawing Application."),
                                           KAboutLicense::LGPL,
                                           i18n("(c) 2001-%1, The Karbon Developers", QString::number(CALLIGRA_YEAR)),
                                           QStringLiteral("https://www.calligra.org/karbon/"));
    aboutData->setProductName("karbon"); // for bugs.kde.org
    aboutData->setOrganizationDomain("kde.org");
    aboutData->setDesktopFileName(QStringLiteral("org.kde.calligra.karbon"));
    aboutData->addAuthor(i18n("Anthony Fieroni"), i18n("Maintainer"), QString());
    aboutData->addAuthor(i18n("Carl Schwan"),
                         i18n("Port to Qt6"),
                         QStringLiteral("carl@carlschwan.eu"),
                         QStringLiteral("https://carlschwan.eu"),
                         QUrl(QStringLiteral("https://carlschwan.eu/avatar.png")));
    aboutData->addAuthor(i18n("Rob Buis"), QString(), "buis@kde.org");
    aboutData->addAuthor(i18n("Tomislav Lukman"), QString(), "tomislav.lukman@ck.t-com.hr");
    aboutData->addAuthor(i18n("Benoît Vautrin"), QString(), "benoit.vautrin@free.fr");
    aboutData->addCredit(i18n("Jan Hambrecht"), i18n("Bug fixes and improvements"), "jaham@gmx.net");
    aboutData->addCredit(i18n("Peter Simonsson"), i18n("Bug fixes and improvements"), "psn@linux.se");
    aboutData->addCredit(i18n("Tim Beaulen"), i18n("Bug fixes and improvements"), "tbscope@gmail.com");
    aboutData->addCredit(i18n("Boudewijn Rempt"), i18n("Bug fixes and improvements"), "boud@valdyas.org");
    aboutData->addCredit(i18n("Pierre Stirnweiss"), i18n("Bug fixes and improvements"), "pierre.stirnweiss_kde@gadz.org");
    aboutData->addCredit(i18n("Inge Wallin"), i18n("Bug fixes"), "inge@lysator.liu.se");
    aboutData->addCredit(i18n("Alan Horkan"), i18n("Helpful patches and advice"));
    // TODO: add the names of some helpful souls
    // standard ki18n translator strings
    aboutData->setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    return aboutData;
}

#endif /* KARBON_ABOUTDATA_H */
