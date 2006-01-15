/*
 *  Copyright (c) 2005 Cyrille Berger <cberger@cberger.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KROSS_KRITACOREKRSIMAGE_H
#define KROSS_KRITACOREKRSIMAGE_H

#include <api/class.h>

#include <kis_types.h>

class KisDoc;

namespace Kross {

namespace KritaCore {

/**
@author Cyrille Berger
*/
    class Image : public Kross::Api::Class<Image>
{
    public:
        Image(KisImageSP image, KisDoc* doc = 0);
        ~Image();
        virtual const QString getClassName() const;
    private:
        Kross::Api::Object::Ptr getActivePaintLayer(Kross::Api::List::Ptr);
        Kross::Api::Object::Ptr getWidth(Kross::Api::List::Ptr);
        Kross::Api::Object::Ptr getHeight(Kross::Api::List::Ptr);
        Kross::Api::Object::Ptr convertToColorspace(Kross::Api::List::Ptr args);
        Kross::Api::Object::Ptr createPaintLayer(Kross::Api::List::Ptr args);
    private:
        KisImageSP m_image;
        KisDoc* m_doc;
};

}

}

#endif
