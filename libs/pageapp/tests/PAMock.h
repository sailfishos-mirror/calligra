/* This file is part of the KDE project
 * Copyright ( C ) 2007 Thorsten Zachmann <zachmann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or ( at your option ) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef PAMOCK_H
#define PAMOCK_H

#include "KoPADocument.h"

#include <KoComponentData.h>
#include <KoOdf.h>
#include <KoPart.h>

#include <KAboutData>

#include <QGraphicsItem>

class MockPart : public KoPart
{
public:
    MockPart()
        : KoPart(KoComponentData(KAboutData(QStringLiteral("test"), QStringLiteral("Test"), QStringLiteral("0.0.9"))), nullptr)
    {
    }
    KoView *createViewInstance(KoDocument *document, QWidget *parent) override
    {
        Q_UNUSED(document);
        Q_UNUSED(parent);
        return nullptr;
    }
    KoMainWindow *createMainWindow() override
    {
        return nullptr;
    }

protected:
    QGraphicsItem *createCanvasItem(KoDocument *document) override
    {
        Q_UNUSED(document);
        return nullptr;
    }
};

class MockDocument : public KoPADocument
{
public:
    MockDocument()
        : KoPADocument(new MockPart)
    {
    }
    const char *odfTagName(bool b) override
    {
        return KoOdf::bodyContentElement(KoOdf::Presentation, b);
    }
    KoOdf::DocumentType documentType() const override
    {
        return KoOdf::Presentation;
    }

    /// reimplemented from KoDocument
    QByteArray nativeFormatMimeType() const override
    {
        return "";
    }
    /// reimplemented from KoDocument
    QByteArray nativeOasisMimeType() const override
    {
        return "";
    }
    /// reimplemented from KoDocument
    QStringList extraNativeMimeTypes() const override
    {
        return QStringList();
    }
};

#endif // PAMOCK_H
