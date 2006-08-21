/*
 * This file is part of KSpread
 *
 * Copyright (c) 2005 Cyrille Berger <cberger@cberger.net>
 * Copyright (c) 2006 ISaac Clerencia <isaac@warp.es>
 * Copyright (c) 2006 Sebastian Sauer <mail@dipe.org>
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

#ifndef SCRIPTING_H
#define SCRIPTING_H

#include <kparts/plugin.h>
#include <View.h>

class Scripting : public KParts::Plugin
{
        Q_OBJECT
    public:
        Scripting(QObject *parent, const QStringList &);
        virtual ~Scripting();
    private slots:
        void executionFinished();
    private:
        class Private;
        Private* const d;
};


#endif
