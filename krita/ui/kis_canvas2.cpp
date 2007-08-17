/* This file is part of the KDE project
 *
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2006
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA..
 */
#include "kis_canvas2.h"

#include <QWidget>
#include <QTime>
#include <QLabel>

#include <kdebug.h>

#include "KoUnit.h"
#include "KoZoomHandler.h"
#include "KoViewConverter.h"
#include "KoShapeManager.h"
#include "colorprofiles/KoIccColorProfile.h"
#include "KoColorSpaceRegistry.h"
#include "KoCanvasController.h"
#include "KoDocument.h"
#include "KoZoomAction.h"
#include "KoToolProxy.h"
#include "KoSelection.h"

#include "kis_image.h"
#include "kis_doc2.h"
#include "kis_shape_layer.h"
#include "kis_resource_provider.h"
#include "kis_view2.h"
#include "kis_config.h"
#include "kis_config_notifier.h"
#include "kis_abstract_canvas_widget.h"
#include "kis_qpainter_canvas.h"
#include "kis_opengl_canvas2.h"
#include "kis_group_layer.h"
#include "kis_opengl_image_textures.h"
#include "kis_shape_controller.h"
#include "kis_layer_manager.h"
#include "kis_selection.h"

#ifdef HAVE_OPENGL
#include <QGLFormat>
#endif

class KisCanvas2::KisCanvas2Private {

public:

    KisCanvas2Private( KoCanvasBase * parent, KoViewConverter * viewConverter, KisView2 * view )
        : viewConverter( viewConverter )
        , view( view )
        , canvasWidget( 0 )
        , shapeManager( new KoShapeManager(parent) )
        , monitorProfile( 0 )
        , currentCanvasIsOpenGL( false )
        , currentCanvasUsesOpenGLShaders( false )
        , toolProxy( new KoToolProxy(parent) )
        , fastZooming( false )
        {
        }

    ~KisCanvas2Private()
        {
            delete shapeManager;
            delete monitorProfile;
            delete toolProxy;
        }

    KoViewConverter * viewConverter;
    KisView2 * view;
    KisAbstractCanvasWidget * canvasWidget;
    KoShapeManager * shapeManager;
    KoColorProfile * monitorProfile;
    bool currentCanvasIsOpenGL;
    bool currentCanvasUsesOpenGLShaders;
    KoToolProxy * toolProxy;
    QImage canvasCache; // XXX: use KisQPainterImageContext to share
                        // cache data between views. Finish that
                        // class. Or another way to tile the canvas
                        // cache instead of having a QImage as big as
                        // the image in pixels.
    QPoint documentOffset;
    KoShapeControllerBase * sc;
    bool fastZooming;

#ifdef HAVE_OPENGL
    KisOpenGLImageTexturesSP openGLImageTextures;
#endif
    bool updateAllOfQPainterCanvas;
};

KisCanvas2::KisCanvas2(KoViewConverter * viewConverter, KisView2 * view, KoShapeControllerBase * sc)
    : KoCanvasBase(sc)
    , m_d ( new KisCanvas2Private(this, viewConverter, view) )
{
    KisConfig cfg;

    m_d->updateAllOfQPainterCanvas = cfg.updateAllOfQPainterCanvas();
    m_d->fastZooming = cfg.fastZoom();
    if ( m_d->canvasWidget && m_d->canvasWidget->widget() )
        m_d->canvasWidget->widget()->update();

    resetMonitorProfile();
    createCanvas();
    connect( view->canvasController(), SIGNAL( moveDocumentOffset( const QPoint& ) ),
             this, SLOT( documentOffsetMoved( const QPoint& ) ) );
    connect(KisConfigNotifier::instance(), SIGNAL(configChanged()), SLOT(slotConfigChanged()));

}

KisCanvas2::~KisCanvas2()
{
    delete m_d;
}

void KisCanvas2::createQPainterCanvas()
{
#ifdef HAVE_OPENGL
    m_d->openGLImageTextures = 0;
#endif
    setCanvasWidget( new KisQPainterCanvas( this, m_d->view ) );
    m_d->currentCanvasIsOpenGL = false;
     KisConfig cfg;
     m_d->updateAllOfQPainterCanvas = cfg.updateAllOfQPainterCanvas();
}

void KisCanvas2::createOpenGLCanvas()
{
#ifdef HAVE_OPENGL
    if ( QGLFormat::hasOpenGL() ) {
        // XXX: The image isn't done loading here!
        m_d->openGLImageTextures = KisOpenGLImageTextures::getImageTextures(m_d->view->image(), m_d->monitorProfile);
        setCanvasWidget( new KisOpenGLCanvas2( this, m_d->view, m_d->openGLImageTextures ) );
        m_d->currentCanvasIsOpenGL = true;
        m_d->currentCanvasUsesOpenGLShaders = m_d->openGLImageTextures->usingHDRExposureProgram();
    }
    else {
        kWarning() << "Tried to create OpenGL widget when system doesn't have OpenGL\n";
        createQPainterCanvas();
    }
#endif
}

void KisCanvas2::createCanvas()
{
    KisConfig cfg;
    if ( cfg.useOpenGL() ) {
#ifdef HAVE_OPENGL
        createOpenGLCanvas();
#else
        kWarning() << "OpenGL requested while its not available, starting qpainter canvas";
        createQPainterCanvas();
#endif
    }
    else {
        createQPainterCanvas();

    }

}

void KisCanvas2::setCanvasWidget(QWidget * widget)
{
    KisAbstractCanvasWidget * tmp = dynamic_cast<KisAbstractCanvasWidget*>( widget );
    Q_ASSERT_X( tmp, "setCanvasWidget", "Cannot cast the widget to a KisAbstractCanvasWidget" );
    m_d->canvasWidget = tmp;
    widget->setAutoFillBackground( false );
    widget->setAttribute( Qt::WA_OpaquePaintEvent );
    widget->setMouseTracking( true );
    widget->setAcceptDrops( true );
    KoCanvasController *controller = canvasController();
    if (controller) {
        Q_ASSERT(controller->canvas() == this);
        controller->changeCanvasWidget(widget);
    }
}

KisView2* KisCanvas2::view()
{
    return m_d->view;
}

void KisCanvas2::gridSize(double *horizontal, double *vertical) const
{
    Q_ASSERT(horizontal);
    Q_ASSERT(vertical);
    *horizontal = m_d->view->document()->gridData().gridX();
    *vertical = m_d->view->document()->gridData().gridY();
}

bool KisCanvas2::snapToGrid() const
{
    return m_d->view->document()->gridData().snapToGrid();
}

void KisCanvas2::addCommand(QUndoCommand *command)
{
    m_d->view->koDocument()->addCommand( command );
}

KoShapeManager* KisCanvas2::shapeManager() const
{
    if (!m_d->view) return m_d->shapeManager;
    if (!m_d->view->layerManager()) return m_d->shapeManager;

    KisLayerSP activeLayer = m_d->view->layerManager()->activeLayer();
    if ( activeLayer ) {
        KisShapeLayer * shapeLayer = dynamic_cast<KisShapeLayer*>( activeLayer.data() );
        if ( shapeLayer )
            return shapeLayer->shapeManager();
    }
    return m_d->shapeManager;
}

KoShapeManager * KisCanvas2::globalShapeManager() const
{
    return m_d->shapeManager;
}

QRect KisCanvas2::viewRectFromDoc( const QRectF & rc )
{
    QRect viewRect = m_d->viewConverter->documentToView(rc).toAlignedRect();
    viewRect = viewRect.translated( -m_d->documentOffset );
    viewRect = viewRect.intersected( QRect( 0, 0, m_d->canvasWidget->widget()->width(), m_d->canvasWidget->widget()->height() ) );
    return viewRect;
}


QRect KisCanvas2::viewRectFromImagePixels( const QRect & rc )
{
    double pppx,pppy;
    pppx = image()->xRes();
    pppy = image()->yRes();

    QRectF docRect;
    docRect.setCoords((rc.left() - 2) / pppx, (rc.top() - 2) / pppy, (rc.right() + 2) / pppx, (rc.bottom() + 2) / pppy);

    QRect viewRect = m_d->viewConverter->documentToView(docRect).toAlignedRect();
    viewRect = viewRect.translated( -m_d->documentOffset );
    viewRect = viewRect.intersected( QRect( 0, 0, m_d->canvasWidget->widget()->width(), m_d->canvasWidget->widget()->height() ) );

    return viewRect;

}

void KisCanvas2::updateCanvas(const QRectF& rc)
{
    // updateCanvas is called from tools, never from the projection
    // updates, so no need to prescale!

    // First convert from document coordinated to widget coordinates
    if ( m_d->updateAllOfQPainterCanvas && !m_d->currentCanvasIsOpenGL ) {
        m_d->canvasWidget->widget()->update();
    }
    else {
        QRect vRect = viewRectFromDoc( rc );
        if ( !vRect.isEmpty() ) {
            m_d->canvasWidget->widget()->update( vRect );
        }
    }
}

void KisCanvas2::updateCanvasProjection( const QRect & rc )
{
#ifdef HAVE_OPENGL
    // Should never have an OpenGL image context and get here as that
    // connects to the image directly.
    Q_ASSERT( m_d->openGLImageTextures.isNull() );
#endif
    // XXX: Use the KisQPainterImageContext here

    // When using fast zoom, we don't have a canvas cache
    if ( !m_d->fastZooming ) {
        QPainter p( &m_d->canvasCache );
        p.setCompositionMode( QPainter::CompositionMode_Source );

        QImage updateImage = image()->convertToQImage(rc.x(), rc.y(), rc.width(), rc.height(),
                                                      m_d->monitorProfile,
                                                      m_d->view->resourceProvider()->HDRExposure());

        KisLayerSP layer = resourceProvider()->resource( KisResourceProvider::CurrentKritaLayer ).value<KisLayerSP>();
        if (!layer) return;

        KisPaintDeviceSP dev = layer->paintDevice();
        if (!dev) return;

        if (dev->hasSelection()){
            KisSelectionSP selection = dev->selection();

            QTime t;
            t.start();
            selection->paint(&updateImage, rc);
            kDebug(41010) << "Mask visualisation rendering took: " << t.elapsed();

        }

        p.drawImage( rc.x(), rc.y(), updateImage, 0, 0, rc.width(), rc.height() );
        p.end();

    }
    else {
        // XXX: Draw the selection also if we're using fast scaling
        // instead of the canvas cache
    }

    QRect vRect = viewRectFromImagePixels( rc );

    if ( !vRect.isEmpty() ) {

        m_d->canvasWidget->preScale( vRect );

        if ( m_d->updateAllOfQPainterCanvas ) {
            m_d->canvasWidget->widget()->update();
        }
        else {
            m_d->canvasWidget->widget()->update( vRect );
        }
    }
}


void KisCanvas2::updateCanvas()
{
    m_d->canvasWidget->widget()->update();
}


const KoViewConverter* KisCanvas2::viewConverter() const
{
    return m_d->viewConverter;
}

QWidget* KisCanvas2::canvasWidget()
{
    return m_d->canvasWidget->widget();
}


KoUnit KisCanvas2::unit() const
{
    return KoUnit(KoUnit::Pixel);
}

KoToolProxy * KisCanvas2::toolProxy() const {
    return m_d->toolProxy;
}

KisImageSP KisCanvas2::image()
{
    return m_d->view->image();

}

QImage KisCanvas2::canvasCache()
{
    return m_d->canvasCache;
}

KoColorProfile *  KisCanvas2::monitorProfile()
{
    if (m_d->monitorProfile == 0) {
        resetMonitorProfile();
    }
    return m_d->monitorProfile;
}


void KisCanvas2::resetMonitorProfile()
{
    // XXX: The X11 monitor profile overrides the settings
    m_d->monitorProfile = KoIccColorProfile::getScreenProfile();

    if (m_d->monitorProfile == 0) {
        KisConfig cfg;
        QString monitorProfileName = cfg.monitorProfile();
        m_d->monitorProfile = KoColorSpaceRegistry::instance()->profileByName(monitorProfileName);
    }
}

KisImageSP KisCanvas2::currentImage()
{
    return m_d->view->image();
}

void KisCanvas2::setImageSize( qint32 w, qint32 h )
{
    // Fast zoom takes the pixels directly from the QImage and doesn't
    // cache the image in a qimage
    if ( !m_d->fastZooming )
        m_d->canvasCache = QImage( w, h, QImage::Format_ARGB32 );
}

void KisCanvas2::connectCurrentImage()
{
#ifdef HAVE_OPENGL
    if (m_d->openGLImageTextures) {
        connect(m_d->openGLImageTextures, SIGNAL(sigImageUpdated(const QRect &)), SLOT(updateCanvas()));
        connect(m_d->openGLImageTextures, SIGNAL(sigSizeChanged(qint32, qint32)), SLOT(setImageSize(qint32, qint32)));
    } else {
#endif
        connect(m_d->view->image(), SIGNAL(sigImageUpdated(const QRect &)), SLOT(updateCanvasProjection(const QRect &)));
        connect(m_d->view->image(), SIGNAL(sigSizeChanged(qint32, qint32)), SLOT(setImageSize( qint32, qint32)) );
#ifdef HAVE_OPENGL
    }
#endif
}

void KisCanvas2::disconnectCurrentImage()
{
#ifdef HAVE_OPENGL
    if (m_d->openGLImageTextures) {
        m_d->openGLImageTextures->disconnect(this);
    }
#endif
    m_d->view->image()->disconnect( this );
}

void KisCanvas2::resetCanvas()
{
    resetMonitorProfile();
    KisConfig cfg;

#if HAVE_OPENGL

    if ((cfg.useOpenGL() != m_d->currentCanvasIsOpenGL) ||
        (m_d->currentCanvasIsOpenGL && (cfg.useOpenGLShaders() != m_d->currentCanvasUsesOpenGLShaders))) {

        disconnectCurrentImage();
        createCanvas();
        connectCurrentImage();
    }

    if (cfg.useOpenGL()) {
        m_d->openGLImageTextures->setMonitorProfile(monitorProfile());
    } else {
        if (image()) {
            updateCanvasProjection(image()->bounds());
        }
    }
#endif
    m_d->updateAllOfQPainterCanvas = cfg.updateAllOfQPainterCanvas();
    m_d->fastZooming = cfg.fastZoom();
    m_d->canvasWidget->widget()->update();
}

void KisCanvas2::documentOffsetMoved( const QPoint &documentOffset )
{
    m_d->documentOffset = documentOffset;
    m_d->canvasWidget->documentOffsetMoved( documentOffset );
}

void KisCanvas2::preScale()
{
    m_d->canvasWidget->preScale();
}

bool KisCanvas2::usingHDRExposureProgram()
{
#ifdef HAVE_OPENGL
    if (m_d->currentCanvasIsOpenGL) {
        if (m_d->openGLImageTextures->usingHDRExposureProgram()) {
            return true;
        }
    }
#endif
    return false;
}

bool KisCanvas2::useFastZooming()
{
    return m_d->fastZooming;
}

void KisCanvas2::slotConfigChanged()
{
    resetCanvas();
}

void KisCanvas2::updateInputMethodInfo() {
    // TODO call (the protected) QWidget::updateMicroFocus() on the proper canvas widget...
}

#include "kis_canvas2.moc"
