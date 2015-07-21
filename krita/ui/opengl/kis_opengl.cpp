/*
 *  Copyright (c) 2007 Adrian Page <adrian@pagenet.plus.com>
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

#include "opengl/kis_opengl.h"

#ifdef HAVE_OPENGL
#include <QOpenGLContext>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#endif

#include <QApplication>
#include <QOffscreenSurface>
#include <QDir>
#include <QFile>
#include <QDesktopServices>
#include <QMessageBox>

#include <klocale.h>

#include <kis_debug.h>
#include <kis_config.h>

namespace
{
#ifdef HAVE_OPENGL
    QOffscreenSurface *SharedSurface = 0;
    QOpenGLContext *SharedContext = 0;
#endif
    bool NeedsFenceWorkaround = false;
}


void KisOpenGL::initialize()
{
#ifdef HAVE_OPENGL
    dbgUI << "OpenGL: initializing";

    KisConfig cfg;;

    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 2);
    if (cfg.disableDoubleBuffering()) {
        format.setSwapBehavior(QSurfaceFormat::SingleBuffer);
    }
    format.setSwapInterval(0); // Disable vertical refresh syncing
    QSurfaceFormat::setDefaultFormat(format);

    if (!SharedSurface) {
        SharedSurface = new QOffscreenSurface();
        SharedSurface->setFormat(format);
        SharedSurface->create();
    }

    if (!SharedContext) {
        SharedContext = new QOpenGLContext(qApp);
        SharedContext->setFormat(format);
        SharedContext->create();
        SharedContext->makeCurrent(SharedSurface);
    }

    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    QFile log(QDesktopServices::storageLocation(QDesktopServices::TempLocation) + "/krita-opengl.txt");
    log.open(QFile::WriteOnly);
    QString vendor((const char*)f->glGetString(GL_VENDOR));
    log.write(vendor.toLatin1());
    log.write(", ");
    QString renderer((const char*)f->glGetString(GL_RENDERER));
    log.write(renderer.toLatin1());
    log.write(", ");
    QString version((const char*)f->glGetString(GL_VERSION));
    log.write(version.toLatin1());

    // Check if we have a bugged driver that needs fence workaround
    bool isOnX11 = false;
#ifdef HAVE_X11
    isOnX11 = true;
#endif

    if ((isOnX11 && renderer.startsWith("AMD")) || cfg.forceOpenGLFenceWorkaround()) {
        NeedsFenceWorkaround = true;
    }
#else
    NeedsFenceWorkaround = false;
#endif
}


bool KisOpenGL::supportsGLSL13()
{
    return true; // QGLFormat::openGLVersionFlags() & QGLFormat::OpenGL_Version_3_0; -- we force 3.2 for now, since that has glFenceSync
}

bool KisOpenGL::needsFenceWorkaround()
{
    return NeedsFenceWorkaround;
}

bool KisOpenGL::hasOpenGL()
{
    // QT5TODO: figure out runtime whether we have opengl...
    return true;
}
