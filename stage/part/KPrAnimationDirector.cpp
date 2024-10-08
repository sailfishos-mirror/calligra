/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007-2008 Thorsten Zachmann <zachmann@kde.org>
 * SPDX-FileCopyrightText: 2010 Benjamin Port <port.benjamin@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (  at your option ) any later version.
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

#include "KPrAnimationDirector.h"

#include <QApplication>
#include <QList>
#include <QPaintEvent>
#include <QPainter>
#include <QVariant>
#include <QWidget>

#include <KoPACanvas.h>
#include <KoPAMasterPage.h>
#include <KoPAPageBase.h>
#include <KoPAUtil.h>
#include <KoPAView.h>
#include <KoPAViewMode.h>
#include <KoPageLayout.h>
#include <KoSelection.h>
#include <KoShapeLayer.h>
#include <KoShapeManager.h>
#include <KoShapeManagerPaintingStrategy.h>
#include <KoShapePaintingContext.h>
#include <KoViewConverter.h>

#include "KPrEndOfSlideShowPage.h"
#include "KPrMasterPage.h"
#include "KPrPage.h"
#include "KPrPageApplicationData.h"
#include "KPrPageSelectStrategyActive.h"
#include "KPrPageTransition.h"
#include "KPrShapeAnimations.h"
#include "KPrShapeManagerAnimationStrategy.h"
#include "KPrShapeManagerDisplayMasterStrategy.h"
#include "StageDebug.h"
#include "pageeffects/KPrPageEffect.h"
#include "pageeffects/KPrPageEffectRunner.h"

#include "animations/KPrAnimationCache.h"

KPrAnimationDirector::KPrAnimationDirector(KoPAView *view, KoPACanvas *canvas, const QList<KoPAPageBase *> &pages, KoPAPageBase *currentPage)
    : m_view(view)
    , m_canvas(canvas)
    , m_pages(pages)
    , m_pageEffectRunner(nullptr)
    , m_stepIndex(0)
    , m_maxShapeDuration(0)
    , m_hasAnimation(false)
    , m_animationCache(nullptr)
    , m_state(PresentationState)
{
    Q_ASSERT(!m_pages.empty());
    m_animationCache = new KPrAnimationCache();
    if (!currentPage || !pages.contains(currentPage))
        updateActivePage(m_pages[0]);
    else
        updateActivePage(currentPage);

    m_pageIndex = m_pages.indexOf(m_view->activePage());

    // updatePageAnimation was called from updateZoom() [updateActivePage()]

    connect(&m_timeLine, &QTimeLine::valueChanged, this, &KPrAnimationDirector::animate);
    // this is needed as after a call to m_canvas->showFullScreen the canvas is not made fullscreen right away
    connect(m_canvas, &KoPACanvas::sizeChanged, this, &KPrAnimationDirector::updateZoom);
    m_timeLine.setEasingCurve(QEasingCurve(QEasingCurve::Linear));
    m_timeLine.setUpdateInterval(20);
    // set the animation strategy in the KoShapeManagers
    m_canvas->shapeManager()->setPaintingStrategy(
        new KPrShapeManagerAnimationStrategy(m_canvas->shapeManager(), m_animationCache, new KPrPageSelectStrategyActive(m_view->kopaCanvas())));
    m_canvas->masterShapeManager()->setPaintingStrategy(
        new KPrShapeManagerAnimationStrategy(m_canvas->masterShapeManager(), m_animationCache, new KPrPageSelectStrategyActive(m_view->kopaCanvas())));

    m_autoTransitionTimer.setSingleShot(true);
    connect(&m_autoTransitionTimer, &QTimer::timeout, this, &KPrAnimationDirector::nextPage);
    connect(&m_timeLine, &QTimeLine::finished, this, &KPrAnimationDirector::slotTimelineFinished);
    if (hasAutoSlideTransition()) {
        if (hasPageEffect() || hasAnimation()) {
            nextStep();
        } else {
            startAutoSlideTransition();
        }
    }
}

KPrAnimationDirector::~KPrAnimationDirector()
{
    // free used resources
    delete m_pageEffectRunner;
    delete m_animationCache;
    // set the KoShapeManagerPaintingStrategy in the KoShapeManagers
    m_canvas->shapeManager()->setPaintingStrategy(new KoShapeManagerPaintingStrategy(m_canvas->shapeManager()));
    m_canvas->masterShapeManager()->setPaintingStrategy(
        new KPrShapeManagerDisplayMasterStrategy(m_canvas->masterShapeManager(), new KPrPageSelectStrategyActive(m_view->kopaCanvas())));
}

void KPrAnimationDirector::paint(QPainter &painter, const QRectF &paintRect)
{
    if (m_pageEffectRunner) {
        bool finished = m_pageEffectRunner->isFinished();
        if (!m_pageEffectRunner->paint(painter)) {
            delete m_pageEffectRunner;
            m_pageEffectRunner = nullptr;

            // check if there where a animation to start
            if (hasAnimation()) {
                if (finished) {
                    QRect clipRect = m_pageRect.intersected(paintRect.toRect());
                    painter.setClipRect(clipRect);
                    painter.setRenderHint(QPainter::Antialiasing);
                    paintStep(painter);
                } else {
                    // start the animations
                    startTimeLine(m_animations.at(m_stepIndex)->totalDuration());
                }
            }
        }
    } else {
        QRect clipRect = m_pageRect.intersected(paintRect.toRect());
        painter.setClipRect(clipRect);
        painter.setRenderHint(QPainter::Antialiasing);
        paintStep(painter);
    }
}

void KPrAnimationDirector::paintEvent(QPaintEvent *event)
{
    QPainter painter(m_canvas);
    paint(painter, event->rect());
}

KoViewConverter *KPrAnimationDirector::viewConverter()
{
    return &m_zoomHandler;
}

int KPrAnimationDirector::numPages() const
{
    return m_pages.size();
}

KoPAPageBase *KPrAnimationDirector::page(int index) const
{
    return m_pages.value(index);
}

int KPrAnimationDirector::currentPage() const
{
    return m_pageIndex;
}

int KPrAnimationDirector::numStepsInPage() const
{
    return m_animations.size();
}

int KPrAnimationDirector::currentStep() const
{
    return m_stepIndex;
}

bool KPrAnimationDirector::navigate(Navigation navigation)
{
    bool finished = false;
    if (m_pageEffectRunner) {
        m_pageEffectRunner->finish();
        finishAnimations();
        // finish on first step
        m_timeLine.stop();
        finished = true;
    } else if (m_timeLine.state() == QTimeLine::Running) { // there are still shape animations running
        finishAnimations();
        m_timeLine.stop();
        finished = true;
    }

    bool presentationFinished = false;

    switch (navigation) {
    case FirstPage:
    case PreviousPage:
    case NextPage:
    case LastPage:
        presentationFinished = changePage(navigation);
        break;
    case PreviousStep:
        previousStep();
        break;
    case NextStep:
        if (!finished) {
            presentationFinished = nextStep();
        }
        break;
    default:
        break;
    }

    return presentationFinished;
}

void KPrAnimationDirector::navigateToPage(int index)
{
    if (m_pageEffectRunner) {
        m_pageEffectRunner->finish();
        finishAnimations();
        // finish on first step
        m_timeLine.stop();
    } else if (m_timeLine.state() == QTimeLine::Running) { // there are still shape animations running
        finishAnimations();
        m_timeLine.stop();
    }

    m_pageIndex = index;
    KoPAPageBase *page = m_pages[m_pageIndex];

    m_stepIndex = 0;

    updateActivePage(page);
    updatePageAnimation();
    updateStepAnimation();
    // trigger a repaint
    m_canvas->update();
}

void KPrAnimationDirector::updateActivePage(KoPAPageBase *page)
{
    deactivate();

    if (m_canvas == m_view->kopaCanvas()) {
        m_view->viewMode()->updateActivePage(page);
    } else {
        QList<KoShape *> shapes = page->shapes();
        m_canvas->shapeManager()->setShapes(shapes, KoShapeManager::AddWithoutRepaint);
        // Make the top most layer active
        if (!shapes.isEmpty()) {
            KoShapeLayer *layer = dynamic_cast<KoShapeLayer *>(shapes.last());
            m_canvas->shapeManager()->selection()->setActiveLayer(layer);
        }

        // if the page is not a master page itself set shapes of the master page
        KoPAPage *paPage = dynamic_cast<KoPAPage *>(page);

        Q_ASSERT(paPage);
        KoPAMasterPage *masterPage = paPage->masterPage();
        QList<KoShape *> masterShapes = masterPage->shapes();
        m_canvas->masterShapeManager()->setShapes(masterShapes, KoShapeManager::AddWithoutRepaint);
        // Make the top most layer active
        if (!masterShapes.isEmpty()) {
            KoShapeLayer *layer = dynamic_cast<KoShapeLayer *>(masterShapes.last());
            m_canvas->masterShapeManager()->selection()->setActiveLayer(layer);
        }
    }

    KPrPage *kprPage = dynamic_cast<KPrPage *>(page);
    Q_ASSERT(kprPage);
    if (m_pageIndex > m_pages.size() || m_pageIndex < 0) {
        m_pageIndex = m_pages.indexOf(page);
    }
    m_animations = kprPage->animations().steps();

    // it can be that the pages have different sizes. So we need to recalculate
    // the zoom when we change the page
    updateZoom(m_canvas->size());
}

void KPrAnimationDirector::updatePageAnimation()
{
    m_animationCache->clear();

    m_animationCache->setPageSize(m_pages[m_pageIndex]->size());
    qreal zoom;
    m_zoomHandler.zoom(&zoom, &zoom);
    m_animationCache->setZoom(zoom);
    int i = 0;
    foreach (KPrAnimationStep *step, m_animations) {
        step->init(m_animationCache, i);
        i++;
    }
}

void KPrAnimationDirector::updateStepAnimation()
{
    m_animationCache->startStep(m_stepIndex);
}

bool KPrAnimationDirector::changePage(Navigation navigation)
{
    switch (navigation) {
    case FirstPage:
        m_pageIndex = 0;
        break;
    case PreviousPage:
        m_pageIndex = m_pageIndex > 0 ? m_pageIndex - 1 : 0;
        break;
    case NextPage:
        if (m_pageIndex < m_pages.size() - 1) {
            ++m_pageIndex;
        } else {
            return true;
        }
        break;
    case LastPage:
        m_pageIndex = m_pages.size() - 1;
        if (dynamic_cast<KPrEndOfSlideShowPage *>(m_pages[m_pageIndex]) && m_pageIndex > 0) {
            m_pageIndex--;
        }
        break;
    case PreviousStep:
    case NextStep:
    default:
        // this should not happen
        Q_ASSERT(0);
        break;
    }
    m_stepIndex = 0;

    updateActivePage(m_pages[m_pageIndex]);
    updatePageAnimation();
    updateStepAnimation();

    // trigger a repaint
    m_canvas->update();

    return false;
}

void KPrAnimationDirector::updateZoom(const QSize &size)
{
    KoPageLayout pageLayout = m_view->activePage()->pageLayout();
    KoPAUtil::setZoom(pageLayout, size, m_zoomHandler);
    m_pageRect = KoPAUtil::pageRect(pageLayout, size, m_zoomHandler);
    m_canvas->setDocumentOffset(-m_pageRect.topLeft());

    // reinit page animation, because somi init method contain zoom
    updatePageAnimation();
    updateStepAnimation();
}

void KPrAnimationDirector::paintStep(QPainter &painter)
{
    if (m_pageRect != m_canvas->rect()) {
        painter.setClipping(false);
        painter.fillRect(m_canvas->rect(), Qt::black);
        painter.setClipping(true);
    }
    painter.translate(m_pageRect.topLeft());
    KoShapePaintingContext context;
    m_view->activePage()->paintBackground(painter, m_zoomHandler, context);

    if (m_view->activePage()->displayMasterShapes()) {
        foreach (KoShape *shape, m_canvas->masterShapeManager()->shapes()) {
            shape->waitUntilReady(m_zoomHandler, false);
        }

        m_canvas->masterShapeManager()->paint(painter, m_zoomHandler, true);
    }
    foreach (KoShape *shape, m_canvas->shapeManager()->shapes()) {
        shape->waitUntilReady(m_zoomHandler, false);
    }
    m_canvas->shapeManager()->paint(painter, m_zoomHandler, true);
}

bool KPrAnimationDirector::nextStep()
{
    if (m_stepIndex < numStepsInPage() - 1) {
        // if there are sub steps go to the next substep
        ++m_stepIndex;
        m_state = EntryAnimationState;
        updateStepAnimation();
        startTimeLine(m_animations.at(m_stepIndex)->totalDuration());
    } else {
        // if there are no more sub steps go to the next page
        // The active page and the substeps are updated later as
        // first the current page has to be painted again for the page effect
        if (m_pageIndex < m_pages.size() - 1) {
            ++m_pageIndex;
        } else {
            return true;
        }
        m_stepIndex = 0;

        KPrPageEffect *effect = KPrPage::pageData(m_pages[m_pageIndex])->pageEffect();

        // run page effect if there is one
        if (effect) {
            QPixmap oldPage(m_canvas->size());
            m_canvas->render(&oldPage);

            updateActivePage(m_pages[m_pageIndex]);
            updatePageAnimation();
            updateStepAnimation();
            QPixmap newPage(m_canvas->size());
            newPage.fill(Qt::white); // TODO
            QPainter newPainter(&newPage);
            newPainter.setClipRect(m_pageRect);
            newPainter.setRenderHint(QPainter::Antialiasing);
            paintStep(newPainter);

            m_state = EntryEffectState;
            m_pageEffectRunner = new KPrPageEffectRunner(oldPage, newPage, m_canvas, effect);
            startTimeLine(effect->duration());
        } else {
            updateActivePage(m_pages[m_pageIndex]);
            updatePageAnimation();
            updateStepAnimation();
            m_canvas->update();
            if (hasAnimation()) {
                m_state = EntryAnimationState;
                startTimeLine(m_animations.at(m_stepIndex)->totalDuration());
            } else if (hasAutoSlideTransition()) {
                m_state = PresentationState;
                startAutoSlideTransition();
            } else {
                m_state = PresentationState;
            }
        }
    }
    return false;
}

void KPrAnimationDirector::nextPage()
{
    nextStep();
}

void KPrAnimationDirector::previousStep()
{
    if (m_stepIndex > 0) {
        --m_stepIndex;
    } else {
        if (m_pageIndex > 0) {
            --m_pageIndex;
            updateActivePage(m_pages[m_pageIndex]);
            if (hasAnimation()) {
                m_stepIndex = m_animations.size() - 1;
            } else {
                m_stepIndex = m_animations.size();
            }
            updatePageAnimation();
            // trigger repaint
            m_canvas->update();
            // cancel a running page effect
            delete m_pageEffectRunner;
            m_pageEffectRunner = nullptr;
        }
    }
    // when going back you always go to the end of the effect
    finishAnimations();
}

bool KPrAnimationDirector::hasAutoSlideTransition() const
{
    return KPrPage::pageData(m_pages[m_pageIndex])->pageTransition().type() == KPrPageTransition::Automatic;
}

void KPrAnimationDirector::startAutoSlideTransition()
{
    m_autoTransitionTimer.start(KPrPage::pageData(m_pages[m_pageIndex])->pageTransition().milliseconds());
}

bool KPrAnimationDirector::pageEffectRunning() const
{
    return m_pageEffectRunner;
}

bool KPrAnimationDirector::hasPageEffect() const
{
    return KPrPage::pageData(m_pages[m_pageIndex])->pageEffect() != nullptr;
}

bool KPrAnimationDirector::animationRunning() const
{
    return hasAnimation() && m_timeLine.state() != QTimeLine::NotRunning;
}

bool KPrAnimationDirector::hasAnimation() const
{
    return m_animations.size() > 0;
}
bool KPrAnimationDirector::moreAnimationSteps() const
{
    return m_stepIndex < m_animations.size() - 1;
}

void KPrAnimationDirector::animate()
{
    if (m_pageEffectRunner) {
        m_pageEffectRunner->next(m_timeLine.currentTime());
    } else if (hasAnimation()) { // if there are animations
        // set current time, to the current step
        m_animationCache->next();
        m_animations.at(m_stepIndex)->setCurrentTime(m_timeLine.currentTime());
        m_canvas->update();
    }
}

void KPrAnimationDirector::finishAnimations()
{
    m_animationCache->endStep(m_stepIndex);
    m_canvas->update();
    m_state = PresentationState;
}

void KPrAnimationDirector::startTimeLine(int duration)
{
    if (duration == 0) {
        m_timeLine.setDuration(1);
    } else {
        m_timeLine.setDuration(duration);
    }
    m_timeLine.setCurrentTime(0);
    m_timeLine.start();
}

void KPrAnimationDirector::slotTimelineFinished()
{
    switch (m_state) {
    case PresentationState:
        break;
    case EntryEffectState:
        if (hasAutoSlideTransition()) {
            if (hasAnimation()) {
                nextStep();
            } else {
                m_state = PresentationState;
                startAutoSlideTransition();
            }
        } else {
            m_state = PresentationState;
        }
        break;
    case EntryAnimationState:
        if (hasAutoSlideTransition()) {
            if (moreAnimationSteps()) {
                nextStep();
            } else if (hasAutoSlideTransition()) {
                m_state = PresentationState;
                startAutoSlideTransition();
            } else {
                m_state = PresentationState;
            }
        } else {
            m_state = PresentationState;
        }
        break;
    }
}

void KPrAnimationDirector::deactivate()
{
    m_state = PresentationState;
    m_autoTransitionTimer.stop();
    foreach (KPrAnimationStep *step, m_animations) {
        step->deactivate();
    }
}
