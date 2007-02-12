/* This file is part of the KDE project
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "kis_opengl_canvas2.h"
#ifdef HAVE_OPENGL


#include <QtOpenGL>
#include <QWidget>
#include <QGLWidget>
#include <QGLContext>
#include <QImage>
#include <QBrush>
#include <QPainter>
#include <QPaintEvent>

#include <kdebug.h>
#include <KoToolProxy.h>
#include <KoToolManager.h>

#include "kis_canvas2.h"
#include "kis_opengl_image_context.h"

#define PATTERN_WIDTH 64
#define PATTERN_HEIGHT 64

class KisOpenGLCanvas2::Private
{
public:
    KisCanvas2 * canvas;
    KoToolProxy * toolProxy;
    KisOpenGLImageContextSP openGLImageContext;
    KoViewConverter * viewConverter;
 };

KisOpenGLCanvas2::KisOpenGLCanvas2( KisCanvas2 * canvas, QWidget * parent, KisOpenGLImageContextSP context )
    : QGLWidget( QGLFormat(QGL::SampleBuffers), parent )
{
    m_d = new Private();
    m_d->canvas = canvas;
    m_d->toolProxy = KoToolManager::instance()->createToolProxy(m_d->canvas);
    m_d->openGLImageContext = context;
    m_d->viewConverter = canvas->viewConverter();

    if (isSharing()) {
        kDebug(41001) << "Created QGLWidget with sharing\n";
    } else {
        kDebug(41001) << "Created QGLWidget with no sharing\n";
    }
}



KisOpenGLCanvas2::~KisOpenGLCanvas2()
{
}

void KisOpenGLCanvas2::initializeGL()
{
    qglClearColor(QColor::fromCmykF(0.40, 0.0, 1.0, 0.0));
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void KisOpenGLCanvas2::resizeGL(int w, int h)
{
    glViewport(0, 0, (GLint)w, (GLint)h);
}


void KisOpenGLCanvas2::paintGL()
{
#if 0
    if ( ev.rect().isNull() ) return;


    glDrawBuffer(GL_BACK);

    QColor widgetBackgroundColor = palette().color(QPalette::Mid);

    glClearColor(widgetBackgroundColor.red() / 255.0, widgetBackgroundColor.green() / 255.0, widgetBackgroundColor.blue() / 255.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    KisImageSP img = currentImg();

    if ( !img ) return;
    QRect vr = ev.rect(); // &= QRect(0, 0, m_canvas->width(), m_canvas->height());

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, m_canvas->width(), m_canvas->height());
    glOrtho(0, m_canvas->width(), m_canvas->height(), 0, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBindTexture(GL_TEXTURE_2D, m_d->openGLImageContext->backgroundTexture());

    glTranslatef(m_canvasXOffset, m_canvasYOffset, 0.0);

    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);

    glTexCoord2f(0.0, 0.0);
    glVertex2f(0.0, 0.0);

    glTexCoord2f((img->width() * zoom()) / KisOpenGLImageContext::BACKGROUND_TEXTURE_WIDTH, 0.0);
    glVertex2f(img->width() * zoom(), 0.0);

    glTexCoord2f((img->width() * zoom()) / KisOpenGLImageContext::BACKGROUND_TEXTURE_WIDTH,
                     (img->height() * zoom()) / KisOpenGLImageContext::BACKGROUND_TEXTURE_HEIGHT);
    glVertex2f(img->width() * zoom(), img->height() * zoom());

    glTexCoord2f(0.0, (img->height() * zoom()) / KisOpenGLImageContext::BACKGROUND_TEXTURE_HEIGHT);
    glVertex2f(0.0, img->height() * zoom());

    glEnd();

    glTranslatef(-m_canvasXOffset, -m_canvasYOffset, 0.0);

    glTranslatef(-horzValue(), -vertValue(), 0.0);
    glScalef(zoomFactor(), zoomFactor(), 1.0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    QRect wr = viewToWindow(QRect(0, 0, m_canvas->width(), m_canvas->height()));
    wr &= QRect(0, 0, img->width(), img->height());

    m_d->openGLImageContext->setHDRExposure(HDRExposure());

    for (int x = (wr.left() / m_d->openGLImageContext->imageTextureTileWidth()) * m_d->openGLImageContext->imageTextureTileWidth();
         x <= wr.right();
         x += m_d->openGLImageContext->imageTextureTileWidth()) {
        for (int y = (wr.top() / m_d->openGLImageContext->imageTextureTileHeight()) * m_d->openGLImageContext->imageTextureTileHeight();
             y <= wr.bottom();
             y += m_d->openGLImageContext->imageTextureTileHeight()) {

            glBindTexture(GL_TEXTURE_2D, m_d->openGLImageContext->imageTextureTile(x, y));

            glBegin(GL_QUADS);

            glTexCoord2f(0.0, 0.0);
            glVertex2f(x, y);

            glTexCoord2f(1.0, 0.0);
            glVertex2f(x + m_d->openGLImageContext->imageTextureTileWidth(), y);

            glTexCoord2f(1.0, 1.0);
            glVertex2f(x + m_d->openGLImageContext->imageTextureTileWidth(), y + m_d->openGLImageContext->imageTextureTileHeight());

            glTexCoord2f(0.0, 1.0);
            glVertex2f(x, y + m_d->openGLImageContext->imageTextureTileHeight());

            glEnd();
        }
    }

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    // Unbind the texture otherwise the ATI driver crashes when the canvas context is
    // made current after the textures are deleted following an image resize.
    glBindTexture(GL_TEXTURE_2D, 0);

    // XXX: Draw selections, masks, visualisations, grids, guides
    //m_gridManager->drawGrid(wr, 0, true);
    QPainter gc ( this );
    m_d->toolProxy->paint(gc, *m_d->viewConverter );

    swapBuffers();
#endif

}

KoToolProxy * KisOpenGLCanvas2::toolProxy() {
    return m_d->toolProxy;
}

#include "kis_opengl_canvas2.moc"
#endif // HAVE_OPENGL
