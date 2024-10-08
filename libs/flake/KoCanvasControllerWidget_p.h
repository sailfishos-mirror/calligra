/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2006, 2008-2009 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2006 Peter Simonsson <peter.simonsson@gmail.com>
 * SPDX-FileCopyrightText: 2006, 2009 Thorsten Zachmann <zachmann@kde.org>
 * SPDX-FileCopyrightText: 2007-2010 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2007 C. Boemann <cbo@boemann.dk>
 * SPDX-FileCopyrightText: 2006-2008 Jan Hambrecht <jaham@gmx.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#ifndef KoCanvasControllerWidget_p_h
#define KoCanvasControllerWidget_p_h

#include "KoCanvasControllerWidget.h"

#include "KoCanvasBase.h"
#include "KoCanvasControllerWidgetViewport_p.h"
#include "KoCanvasObserverBase.h"
#include "KoCanvasSupervisor.h"
#include "KoShape.h"
#include "KoToolManager_p.h"
#include "KoViewConverter.h"

#include <FlakeDebug.h>
#include <QDockWidget>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTimer>

#include <KoConfig.h>

class Q_DECL_HIDDEN KoCanvasControllerWidget::Private
{
public:
    Private(KoCanvasControllerWidget *qq)
        : q(qq)
        , canvas(nullptr)
        , ignoreScrollSignals(false)
        , zoomWithWheel(false)
        , vastScrollingFactor(0)
    {
    }

    /**
     * Gets called by the tool manager if this canvas controller is the current active canvas controller.
     */
    void setDocumentOffset();

    void resetScrollBars();
    void emitPointerPositionChangedSignals(QEvent *event);

    void activate();
    void unsetCanvas();

    KoCanvasControllerWidget *q;
    KoCanvasBase *canvas;
    Viewport *viewportWidget;
    bool ignoreScrollSignals;
    bool zoomWithWheel;
    qreal vastScrollingFactor;
};

#endif
