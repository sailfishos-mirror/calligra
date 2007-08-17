/*
 *  Copyright (c) 2006 Boudewijn Rempt <boud@valdyas.org>
 *
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
#ifndef _KIS_EFFECT_MASK_
#define _KIS_EFFECT_MASK_

#include "kicon.h"

#include "kis_types.h"
#include "kis_mask.h"
/**
   An effect mask is a single channel mask that applies a particular
   effect to the layer the mask belongs to. It differs from an
   adjustment layer in that it only works on its parent layer, while
   adjustment layers work on all layers below it in its layer group.
   The effect could be a filter, a transformation or anything else
   that messes up pixels.
*/
const QString KIS_EFFECT_MASK_ID = "KisEffectMask";

class KRITAIMAGE_EXPORT KisEffectMask : public KisMask
{

    Q_OBJECT

public:

    /**
     * Create an empty effect mask. There is filter and no layer
     * associated with this mask.
     */
    KisEffectMask();

    virtual ~KisEffectMask();
    KisEffectMask( const KisEffectMask& rhs );

    QIcon icon() const
        {
            return KIcon(""); // XXX: Find nice icon for the subclasses.
        }

    virtual QString id() { return KIS_EFFECT_MASK_ID; }


    virtual QString nodeType()
        {
            return id();
        }

    virtual bool canHaveChildren()
        {
            return false;
        }

    /**
     * Apply the effect the projection using the mask as a selection.
     */
    virtual void apply( KisPaintDeviceSP projection, const QRect & rc ) const = 0;

};

#endif //_KIS_EFFECT_MASK_
