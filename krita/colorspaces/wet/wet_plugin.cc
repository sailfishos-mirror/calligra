/*
 * wet_plugin.cc -- Part of Krita
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include <stdlib.h>
#include <vector>

#include <QObject>
#include <QApplication>
#include <QClipboard>
#include <QPoint>
#include <QLabel>
#include <QWidget>

#include <klocale.h>
#include <kiconloader.h>
#include <kcomponentdata.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kgenericfactory.h>

#include <KoMainWindow.h>


#include <kis_factory.h>
#include <kis_image.h>
#include <kis_types.h>
#include <kis_view.h>
#include <KoColorSpaceRegistry.h>
#include <kis_tool_registry.h>
#include <kis_paintop_registry.h>
#include <kis_canvas_subject.h>
#include <kis_basic_histogram_producers.h>

#include "wet_plugin.h"
#include "kis_wet_palette_widget.h"
#include "kis_wet_colorspace.h"
#include "kis_wetop.h"
#include "kis_wetness_visualisation_filter.h"
#include "kis_texture_filter.h"
#include "wetphysicsfilter.h"

typedef KGenericFactory<WetPlugin> WetPluginFactory;
K_EXPORT_COMPONENT_FACTORY( kritawetplugin, WetPluginFactory( "kritacore" ) )


WetPlugin::WetPlugin(QObject *parent, const QStringList &)
    : KParts::Plugin(parent)
{
    setComponentData(WetPluginFactory::componentData());

    // This is not a gui plugin; only load it when the doc is created.
    if ( parent->inherits("KoColorSpaceRegistry") ) {
        KoColorSpaceRegistry * f = dynamic_cast<KoColorSpaceRegistry*>(parent);

        KoColorSpace* colorSpaceWet = new KisWetColorSpace(f, 0);

        KoColorSpaceFactory * csf = new KisWetColorSpaceFactory();
        Q_CHECK_PTR(colorSpaceWet);

        // colorspace
        f->add(csf);

        // histogram producer
        KisHistogramProducerFactoryRegistry::instance()->add(
                new KisBasicHistogramProducerFactory<KisBasicU16HistogramProducer>
                (KoID("WETHISTO", i18n("Wet Histogram")), colorSpaceWet) );

        // wet brush op
        KisPaintOpRegistry::instance()->add(KisPaintOpFactorySP(new KisWetOpFactory));

        // Dry filter
        // KisFilterRegistry::instance()->add( new WetPhysicsFilter() );

        // Texture Action:
        f->addPaintDeviceAction(colorSpaceWet, new WetPaintDevAction);
    }
    else if (parent->inherits("KisView"))
    {
        setComponentData(WetPluginFactory::componentData());

setXMLFile(KStandardDirs::locate("data","kritaplugins/wetplugin.rc"),
true);

        m_view = dynamic_cast<KisView*>(parent);
        // Wetness visualization
        WetnessVisualisationFilter * wf = new WetnessVisualisationFilter(m_view);

        KToggleAction *action = new KToggleAction(i18n("Wetness Visualisation"), actionCollection(), "wetnessvisualisation");
        connect(action, SIGNAL(triggered()), wf, SLOT(slotActivated()));

        wf->setAction(action);

        // Create the wet palette
        KisWetPaletteWidget * w = new KisWetPaletteWidget(m_view);
        Q_CHECK_PTR(w);

        w->setWindowTitle(i18n("Watercolors"));

        m_view->createDock(i18n("Watercolors"), w);
        m_view->canvasSubject()->attach(w);
    }


}

WetPlugin::~WetPlugin()
{
}

#include "wet_plugin.moc"
