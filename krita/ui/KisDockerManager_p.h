/* This file is part of the KDE project
 *
 * Copyright (c) 2008-2012 C. Boemann <cbo@boemann.dk>
 * Copyright (c) 2009 Cyrille Berger <cberger@cberger.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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
#ifndef KisDockerManager_p_h
#define KisDockerManager_p_h

#include "KisDockerManager.h"
#include "KoDockFactoryBase.h"

#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <ktoolbar.h>

#include "KoToolDocker.h"

#include "KisView.h"
#include "KisMainWindow.h"

class KisDockerManager::Private
{
public:
    Private(KisMainWindow *mw) :
        mainWindow(mw)
        ,ignore(true)
        ,showOptionsDocker(true)
    {
    }

    KoToolDocker *toolOptionsDocker;
    KisMainWindow *mainWindow;
    bool ignore;
    bool showOptionsDocker;

    void restoringDone()
    {
        if (ignore) {
            ignore = false;
            toolOptionsDocker->setVisible(showOptionsDocker);
        }
    }
};

#endif
