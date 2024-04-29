/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006, 2008 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2007-2010 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2007-2008 C. Boemann <cbo@boemann.dk>
 * SPDX-FileCopyrightText: 2006-2007 Jan Hambrecht <jaham@gmx.net>
 * SPDX-FileCopyrightText: 2009 Thorsten Zachmann <zachmann@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KOCANVASCONTROLLERWIDGET_H
#define KOCANVASCONTROLLERWIDGET_H

#include "flake_export.h"

#include "KoCanvasController.h"
#include <QAbstractScrollArea>
#include <QPointer>

class KoShape;
class KoCanvasBase;

/**
 * KoCanvasController implementation for QWidget based canvases
 */
class FLAKE_EXPORT KoCanvasControllerWidget : public QAbstractScrollArea, public KoCanvasController
{
    Q_OBJECT
public:
    /**
     * Constructor.
     * @param actionCollection the action collection for this widget
     * @param parent the parent this widget will belong to
     */
    explicit KoCanvasControllerWidget(KActionCollection *actionCollection, QWidget *parent = nullptr);
    ~KoCanvasControllerWidget() override;

    /// Reimplemented from QObject
    bool eventFilter(QObject *watched, QEvent *event) override;

    /**
     * Reimplemented from QAbstractScrollArea.
     */
    void scrollContentsBy(int dx, int dy) override;

    QSize viewportSize() const override;

    /// Reimplemented from KoCanvasController

    /**
     * Activate this canvascontroller
     */
    virtual void activate();

    void setDrawShadow(bool drawShadow) override;

    void setCanvas(KoCanvasBase *canvas) override;

    KoCanvasBase *canvas() const override;

    /**
     * Change the actual canvas widget used by the current canvas. This allows the canvas widget
     * to be changed while keeping the current KoCanvasBase canvas and its associated resources as
     * they are. This might be used, for example, to switch from a QWidget to a QOpenGLWidget canvas.
     * @param widget the new canvas widget.
     */
    virtual void changeCanvasWidget(QWidget *widget);

    int visibleHeight() const override;
    int visibleWidth() const override;
    int canvasOffsetX() const override;
    int canvasOffsetY() const override;

    void ensureVisible(const QRectF &rect, bool smooth = false) override;

    void ensureVisible(KoShape *shape) override;

    /**
     * will cause the toolOptionWidgetsChanged to be emitted and all
     * listeners to be updated to the new widget.
     *
     * FIXME: This doesn't belong her and it does an
     * inherits("KoView") so it too much tied to komain
     *
     * @param widgets the map of widgets
     */
    void setToolOptionWidgets(const QList<QPointer<QWidget>> &widgets);

    void zoomIn(const QPoint &center) override;

    void zoomOut(const QPoint &center) override;

    void zoomBy(const QPoint &center, qreal zoom) override;

    void zoomTo(const QRect &rect) override;

    /**
     * Zoom document keeping point \p widgetPoint unchanged
     * \param widgetPoint sticky point in widget pixels
     * \param zoomCoeff the zoom coefficient
     */
    virtual void zoomRelativeToPoint(const QPointF &widgetPoint, qreal zoomCoeff);

    void recenterPreferred() override;

    void setPreferredCenter(const QPointF &viewPoint) override;

    /// Returns the currently set preferred center point in view coordinates (pixels)
    QPointF preferredCenter() const override;

    void pan(const QPoint &distance) override;

    void setMargin(int margin) override;

    QPoint scrollBarValue() const override;

    /**
     * Used by KisCanvasController to correct the scrollbars position
     * after the rotation.
     */
    void setScrollBarValue(const QPoint &value) override;

    void updateDocumentSize(const QSize &sz, bool recalculateCenter = true) override;

    /**
     * Set mouse wheel to zoom behaviour
     * @param zoom if true wheel will zoom instead of scroll, control modifier will scroll
     */
    void setZoomWithWheel(bool zoom) override;

    void setVastScrolling(qreal factor) override;

    /**
     * \internal
     */
    class Private;
    KoCanvasControllerWidget::Private *priv();

Q_SIGNALS:

    /**
     * Emit the new tool option widgets to be used with this canvas.
     */
    void toolOptionWidgetsChanged(const QList<QPointer<QWidget>> &widgets);

private Q_SLOTS:

    /// Called by the horizontal scrollbar when its value changes
    void updateCanvasOffsetX();

    /// Called by the vertical scrollbar when its value changes
    void updateCanvasOffsetY();

protected:
    friend class KisZoomAndPanTest;

    /// reimplemented from QWidget
    void paintEvent(QPaintEvent *event) override;
    /// reimplemented from QWidget
    void resizeEvent(QResizeEvent *resizeEvent) override;
    /// reimplemented from QWidget
    void dragEnterEvent(QDragEnterEvent *event) override;
    /// reimplemented from QWidget
    void dropEvent(QDropEvent *event) override;
    /// reimplemented from QWidget
    void dragMoveEvent(QDragMoveEvent *event) override;
    /// reimplemented from QWidget
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    /// reimplemented from QWidget
    void wheelEvent(QWheelEvent *event) override;
    /// reimplemented from QWidget
    void keyPressEvent(QKeyEvent *event) override;
    /// reimplemented from QWidget
    bool focusNextPrevChild(bool next) override;

private:
    Q_PRIVATE_SLOT(d, void activate())

    Private *const d;
};

#endif
