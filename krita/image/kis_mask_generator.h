/*
 *  Copyright (c) 2008-2009 Cyrille Berger <cberger@cberger.net>
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

#ifndef _KIS_MASK_GENERATOR_H_
#define _KIS_MASK_GENERATOR_H_

#include "krita_export.h"

class QDomElement;
class QDomDocument;

/**
 * This is the base class for mask shapes
 * You should subclass it if you want to create a new
 * shape.
 */
class KRITAIMAGE_EXPORT KisMaskGenerator
{
public:

    virtual ~KisMaskGenerator() {}

    KDE_DEPRECATED KisMaskGenerator(double width, double height, double fh, double fv) : m_radius(width), m_ratio(height/width), m_fh( 2.0 * fh / width), m_fv( 2.0 * fv / height ), m_spikes(2)
    {
    }
    /**
     * This function creates an auto brush shape with the following value :
     * @param w width
     * @param h height
     * @param fh horizontal fade (fh \< w / 2 )
     * @param fv vertical fade (fv \< h / 2 )
     */
    KisMaskGenerator(double radius, double ratio, double fh, double fv, int spikes) : m_radius(radius), m_ratio(ratio), m_fh(fh), m_fv(fv), m_spikes(spikes) { }

    /**
     * @return the alpha value at the position (x,y)
     */
    virtual quint8 valueAt(double x, double y) = 0;

    quint8 interpolatedValueAt(double x, double y);

    virtual void toXML(QDomDocument& , QDomElement&) const;

    /**
     * Unserialise a \ref KisMaskGenerator
     */
    static KisMaskGenerator* fromXML(const QDomElement&);

    double width() const {
        return m_radius;
    }

    double height() const {
        return m_radius * m_ratio;
    }

protected:

    double m_radius, m_ratio;
    double m_fh, m_fv;
    int m_spikes;
};

/**
 * This class allows to create circular shapes.
 */
class KRITAIMAGE_EXPORT KisCircleMaskGenerator : public KisMaskGenerator
{

public:

    virtual ~KisCircleMaskGenerator() {}

    KDE_DEPRECATED KisCircleMaskGenerator(double w, double h, double fh, double fv);
    KisCircleMaskGenerator(double radius, double ratio, double fh, double fv, int spikes);

    virtual quint8 valueAt(double x, double y);

    virtual void toXML(QDomDocument& , QDomElement&) const;

private:

    double norme(double a, double b) {
        return a*a + b * b;
    }

private:
    double m_xcenter, m_ycenter;
    double m_xcoef, m_ycoef;
    double m_xfadecoef, m_yfadecoef;
};

/**
 * This class allows to create rectangular shapes.
 */
class KRITAIMAGE_EXPORT KisRectangleMaskGenerator : public KisMaskGenerator
{

public:

    virtual ~KisRectangleMaskGenerator() {}

    KDE_DEPRECATED KisRectangleMaskGenerator(double w, double h, double fh, double fv);
    KisRectangleMaskGenerator(double radius, double ratio, double fh, double fv, int spikes);

    virtual quint8 valueAt(double x, double y);

    virtual void toXML(QDomDocument& , QDomElement&) const;

private:

    double m_xcenter, m_ycenter, m_c;
};


#endif
