/* This file is part of the KDE project
   Copyright (C) 2004 Peter Simonsson <psn@linux.se>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KOLINESTYLEACTION_H
#define KOLINESTYLEACTION_H

#include "koselectaction.h"

class KoLineStyleAction : public KoSelectAction
{
  Q_OBJECT
  public:
    KoLineStyleAction(const QString& text, const QString& icon, QObject* parent = 0, const char* name = 0);
    KoLineStyleAction(const QString& text, const QString& icon, const QObject* receiver,
      const char* slot, QObject* parent, const char* name = 0);
    ~KoLineStyleAction();
    
  protected:
    void createMenu();
  
  private:
    class KoLineStyleActionPrivate;
    KoLineStyleActionPrivate* d;
};

#endif
