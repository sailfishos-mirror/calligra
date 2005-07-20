/*
 * gray_plugin.cc -- Part of Krita
 *
 * Copyright (c) 2004 Boudewijn Rempt (boud@valdyas.org)
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <klocale.h>
#include <kiconloader.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <kdebug.h>
#include <kgenericfactory.h>

#include <kis_factory.h>
#include <kis_colorspace_registry.h>

#include "gray_plugin.h"

#include "kis_strategy_colorspace_grayscale.h"

typedef KGenericFactory<GrayPlugin> GrayPluginFactory;
K_EXPORT_COMPONENT_FACTORY( kritagrayplugin, GrayPluginFactory( "kritacore" ) )


GrayPlugin::GrayPlugin(QObject *parent, const char *name, const QStringList &)
	: KParts::Plugin(parent, name)
{
       	setInstance(GrayPluginFactory::instance());

 	kdDebug(DBG_AREA_PLUGINS) << "GRAY Color model plugin. Class: "
 		  << className()
 		  << ", Parent: "
 		  << parent -> className()
 		  << "\n";

	// This is not a gui plugin; only load it when the doc is created.
	if ( parent->inherits("KisFactory") )
	{
		m_StrategyColorSpaceGrayA = new KisStrategyColorSpaceGrayscale();
		Q_CHECK_PTR(m_StrategyColorSpaceGrayA);
		KisColorSpaceRegistry::instance() -> add(m_StrategyColorSpaceGrayA);
	}

}

GrayPlugin::~GrayPlugin()
{
}

#include "gray_plugin.moc"
