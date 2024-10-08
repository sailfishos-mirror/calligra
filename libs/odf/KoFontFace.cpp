/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Nokia Corporation and /or its subsidiary(-ies).

   Contact: Suresh Chande suresh.chande@nokia.com

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KoFontFace.h"
#include <KoXmlWriter.h>
#include <OdfDebug.h>

class KoFontFacePrivate : public QSharedData
{
public:
    KoFontFacePrivate(const QString &_name)
        : name(_name)
        , pitch(KoFontFace::VariablePitch)
    {
    }

    ~KoFontFacePrivate() = default;

    void saveOdf(KoXmlWriter *xmlWriter) const
    {
        xmlWriter->startElement("style:font-face");
        xmlWriter->addAttribute("style:name", name);
        xmlWriter->addAttribute("svg:font-family", family.isEmpty() ? name : family);
        if (!familyGeneric.isEmpty())
            xmlWriter->addAttribute("style:font-family-generic", familyGeneric);
        if (!style.isEmpty())
            xmlWriter->addAttribute("svg:font-style", style);
        xmlWriter->addAttribute("style:font-pitch", pitch == KoFontFace::FixedPitch ? "fixed" : "variable");
        xmlWriter->endElement(); // style:font-face
    }

    QString name; //!< for style:name attribute
    QString family; //!< for svg:font-family attribute
    QString familyGeneric; //!< for style:font-family-generic attribute
    QString style; //!< for svg:font-style attribute
    KoFontFace::Pitch pitch; //!< for style:font-pitch attribute
};

KoFontFace::KoFontFace(const QString &_name)
    : d(new KoFontFacePrivate(_name))
{
}

KoFontFace::KoFontFace(const KoFontFace &other)

    = default;

KoFontFace::~KoFontFace() = default;

KoFontFace &KoFontFace::operator=(const KoFontFace &other) = default;

bool KoFontFace::operator==(const KoFontFace &other) const
{
    if (isNull() && other.isNull())
        return true;
    return d.data() == other.d.data();
}

bool KoFontFace::isNull() const
{
    return d->name.isEmpty();
}

QString KoFontFace::name() const
{
    return d->name;
}

void KoFontFace::setName(const QString &name)
{
    d->name = name;
}

QString KoFontFace::family() const
{
    return d->family;
}

void KoFontFace::setFamily(const QString &family)
{
    d->family = family;
}

QString KoFontFace::familyGeneric() const
{
    return d->familyGeneric;
}

void KoFontFace::setFamilyGeneric(const QString &familyGeneric)
{
    if (familyGeneric == "decorative" || familyGeneric == "modern" || familyGeneric == "roman" || familyGeneric == "script" || familyGeneric == "swiss"
        || familyGeneric == "system") {
        d->familyGeneric = familyGeneric;
    }
}

QString KoFontFace::style() const
{
    return d->style;
}

void KoFontFace::setStyle(const QString &style)
{
    d->style = style;
}

KoFontFace::Pitch KoFontFace::pitch() const
{
    return d->pitch;
}

void KoFontFace::setPitch(KoFontFace::Pitch pitch)
{
    d->pitch = pitch;
}

void KoFontFace::saveOdf(KoXmlWriter *xmlWriter) const
{
    Q_ASSERT(!isNull());
    if (isNull()) {
        warnOdf << "This font face is null and will not be saved: set at least the name";
        return;
    }
    d->saveOdf(xmlWriter);
}
