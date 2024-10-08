/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007-2009 Thorsten Zachmann <zachmann@kde.org>
 * SPDX-FileCopyrightText: 2008 Jim Courtiau <jeremy.courtiau@gmail.com>
 * SPDX-FileCopyrightText: 2009 Alexia Allanic <alexia_allanic@yahoo.fr>
 * SPDX-FileCopyrightText: 2009 Jean-Nicolas Artaud <jeannicolasartaud@gmail.com>
 * SPDX-FileCopyrightText: 2009 Jérémy Lugagne <jejewindsurf@hotmail.com>
 * SPDX-FileCopyrightText: 2009 Johann Hingue <yoan1703@hotmail.fr>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KPrPresentationTool.h"

#include <QAbstractTextDocumentLayout>
#ifdef WITH_QTDBUS
#include <QDBusConnection>
#endif
#include <QDesktopServices>
#include <QFrame>
#include <QKeyEvent>
#include <QPainter>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <KoEventAction.h>
#include <KoPACanvasBase.h>
#include <KoPointedAt.h>
#include <KoPointerEvent.h>
#include <KoShape.h>
#include <KoShapeManager.h>
#include <KoTextLayoutRootArea.h>
#include <KoTextShapeData.h>

#include "KPrPresentationBlackStrategy.h"
#include "KPrPresentationDrawStrategy.h"
#include "KPrPresentationHighlightStrategy.h"
#include "KPrPresentationStrategy.h"
#include "KPrViewModePresentation.h"
#include "ui/KPrPresentationToolWidget.h"

#ifdef WITH_QTDBUS
#include "KPrPresentationToolAdaptor.h"
#endif

KPrPresentationTool::KPrPresentationTool(KPrViewModePresentation &viewMode)
    : KoToolBase(viewMode.canvas())
    , m_viewMode(viewMode)
    , m_strategy(new KPrPresentationStrategy(this))
#ifdef WITH_QTDBUS
    , m_bus(new KPrPresentationToolAdaptor(this))
#endif
{
#ifdef WITH_QTDBUS
    QDBusConnection::sessionBus().registerObject("/kpresenter/PresentationTools", this);
#endif

    // tool box
    m_frame = new QFrame(m_viewMode.canvas()->canvasWidget());

    QVBoxLayout *frameLayout = new QVBoxLayout();

    m_presentationToolWidget = new KPrPresentationToolWidget(m_viewMode.canvas()->canvasWidget());
    frameLayout->addWidget(m_presentationToolWidget, 0, Qt::AlignLeft | Qt::AlignBottom);
    m_frame->setLayout(frameLayout);
    m_frame->show();

    m_presentationToolWidget->raise();
    m_presentationToolWidget->setVisible(false);
    m_presentationToolWidget->installEventFilter(this);

    // Connections of button clicked to slots
    connect(m_presentationToolWidget->presentationToolUi().penButton, &QAbstractButton::clicked, this, &KPrPresentationTool::drawOnPresentation);
    connect(m_presentationToolWidget->presentationToolUi().highLightButton, &QAbstractButton::clicked, this, &KPrPresentationTool::highlightPresentation);
    connect(m_presentationToolWidget->presentationToolUi().blackButton, &QAbstractButton::clicked, this, &KPrPresentationTool::blackPresentation);
}

KPrPresentationTool::~KPrPresentationTool()
{
    delete m_strategy;
}

bool KPrPresentationTool::wantsAutoScroll() const
{
    return false;
}

void KPrPresentationTool::paint(QPainter &painter, const KoViewConverter &converter)
{
    Q_UNUSED(painter);
    Q_UNUSED(converter);
}

void KPrPresentationTool::mousePressEvent(KoPointerEvent *event)
{
    if (event->button() & Qt::LeftButton) {
        event->accept();
        finishEventActions();
        KoShape *shapeClicked = canvas()->shapeManager()->shapeAt(event->point);
        if (shapeClicked) {
            QString link;
            if (checkHyperlink(event, shapeClicked, link)) {
                runHyperlink(link);
                return;
            }

            m_eventActions = shapeClicked->eventActions();
            if (!m_eventActions.isEmpty()) {
                foreach (KoEventAction *eventAction, m_eventActions) {
                    eventAction->start();
                }
                // don't do next step if a action was executed
                return;
            }
        }
        m_viewMode.navigate(KPrAnimationDirector::NextStep);
    } else if (event->button() & Qt::RightButton) {
        event->accept();
        finishEventActions();
        m_viewMode.navigate(KPrAnimationDirector::PreviousStep);
    }
}

void KPrPresentationTool::mouseDoubleClickEvent(KoPointerEvent *event)
{
    Q_UNUSED(event);
}

void KPrPresentationTool::mouseMoveEvent(KoPointerEvent *event)
{
    KoShape *shape = canvas()->shapeManager()->shapeAt(event->point);

    QString link;
    if (checkHyperlink(event, shape, link)) {
        canvas()->setCursor(Qt::PointingHandCursor);
        return;
    }

    canvas()->setCursor(Qt::ArrowCursor);
}

void KPrPresentationTool::mouseReleaseEvent(KoPointerEvent *event)
{
    Q_UNUSED(event);
}

void KPrPresentationTool::keyPressEvent(QKeyEvent *event)
{
    finishEventActions();
    // first try to handle the event in the strategy if it is done there no need to use the default action
    if (!m_strategy->keyPressEvent(event)) {
        switch (event->key()) {
        case Qt::Key_Escape:
            m_viewMode.activateSavedViewMode();
            break;
        case Qt::Key_Home:
            m_viewMode.navigate(KPrAnimationDirector::FirstPage);
            break;
        case Qt::Key_Up:
        case Qt::Key_PageUp:
            m_viewMode.navigate(KPrAnimationDirector::PreviousPage);
            break;
        case Qt::Key_Backspace:
        case Qt::Key_Left:
            m_viewMode.navigate(KPrAnimationDirector::PreviousStep);
            break;
        case Qt::Key_Right:
        case Qt::Key_Space:
            m_viewMode.navigate(KPrAnimationDirector::NextStep);
            break;
        case Qt::Key_Down:
        case Qt::Key_PageDown:
            m_viewMode.navigate(KPrAnimationDirector::NextPage);
            break;
        case Qt::Key_End:
            m_viewMode.navigate(KPrAnimationDirector::LastPage);
            break;
        default:
            event->ignore();
            break;
        }
    }
}

void KPrPresentationTool::keyReleaseEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
}

void KPrPresentationTool::wheelEvent(KoPointerEvent *event)
{
    Q_UNUSED(event);
}

void KPrPresentationTool::activate(ToolActivation toolActivation, const QSet<KoShape *> &shapes)
{
    Q_UNUSED(toolActivation);
    Q_UNUSED(shapes);
    m_frame->setGeometry(canvas()->canvasWidget()->geometry());
    m_presentationToolWidget->setVisible(false);
    // redirect event to tool widget
    m_frame->installEventFilter(this);
    // activate tracking for show/hide tool buttons
    m_frame->setMouseTracking(true);
}

void KPrPresentationTool::deactivate()
{
    switchStrategy(new KPrPresentationStrategy(this));
    finishEventActions();
}

void KPrPresentationTool::finishEventActions()
{
    foreach (KoEventAction *eventAction, m_eventActions) {
        eventAction->finish();
    }
}

void KPrPresentationTool::switchStrategy(KPrPresentationStrategyBase *strategy)
{
    Q_ASSERT(strategy);
    Q_ASSERT(m_strategy != strategy);
    delete m_strategy;
    m_strategy = strategy;
}

// SLOTS
void KPrPresentationTool::highlightPresentation()
{
    KPrPresentationStrategyBase *strategy;
    if (dynamic_cast<KPrPresentationHighlightStrategy *>(m_strategy)) {
        strategy = new KPrPresentationStrategy(this);
    } else {
        strategy = new KPrPresentationHighlightStrategy(this);
    }
    switchStrategy(strategy);
}

void KPrPresentationTool::drawOnPresentation()
{
    KPrPresentationStrategyBase *strategy;
    if (dynamic_cast<KPrPresentationDrawStrategy *>(m_strategy)) {
        strategy = new KPrPresentationStrategy(this);
    } else {
        strategy = new KPrPresentationDrawStrategy(this);
    }
    switchStrategy(strategy);
}

void KPrPresentationTool::blackPresentation()
{
    KPrPresentationStrategyBase *strategy;
    if (dynamic_cast<KPrPresentationBlackStrategy *>(m_strategy)) {
        strategy = new KPrPresentationStrategy(this);
    } else {
        strategy = new KPrPresentationBlackStrategy(this);
    }
    switchStrategy(strategy);
}

bool KPrPresentationTool::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QWidget *source = static_cast<QWidget *>(obj);
        QPoint pos = source->mapFrom(m_viewMode.canvas()->canvasWidget(), mouseEvent->pos());

        QSize buttonSize = m_presentationToolWidget->size() + QSize(20, 20);
        QRect geometry = QRect(0, m_frame->height() - buttonSize.height(), buttonSize.width(), buttonSize.height());
        if (geometry.contains(pos)) {
            m_presentationToolWidget->setVisible(true);
        } else {
            m_presentationToolWidget->setVisible(false);
        }
    }
    return false;
}

bool KPrPresentationTool::checkHyperlink(KoPointerEvent *event, KoShape *shape, QString &hyperLink)
{
    if (!shape) {
        return false;
    }

    KoTextShapeData *textShapeData = qobject_cast<KoTextShapeData *>(shape->userData());
    if (textShapeData) {
        if (!textShapeData->rootArea()) {
            return false; // not layouted yet
        }
        QPointF p = shape->absoluteTransformation(nullptr).inverted().map(event->point);
        p = p + QPointF(0.0, textShapeData->documentOffset());

        KoPointedAt pointedAt = textShapeData->rootArea()->hitTest(p, Qt::ExactHit);

        if (!textShapeData->isDirty() && !pointedAt.externalHRef.isEmpty()) {
            hyperLink = pointedAt.externalHRef;
            return true;
        }
    }
    return false;
}

void KPrPresentationTool::runHyperlink(const QString &hyperLink)
{
    QUrl url = QUrl::fromUserInput(hyperLink);

    QDesktopServices::openUrl(url);
}

KPrPresentationStrategyBase *KPrPresentationTool::strategy()
{
    return m_strategy;
}

KPrViewModePresentation &KPrPresentationTool::viewModePresentation()
{
    return m_viewMode;
}

void KPrPresentationTool::normalPresentation()
{
    switchStrategy(new KPrPresentationStrategy(this));
}
