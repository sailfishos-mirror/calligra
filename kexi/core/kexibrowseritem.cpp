/* This file is part of the KDE project
   Copyright (C) 2002   Lucijan Busch <lucijan@gmx.at>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kexiprojecthandler.h"
#include "kexiprojecthandlerproxy.h"
#include "kexiprojecthandleritem.h"

#include "kexibrowseritem.h"

KexiBrowserItem::KexiBrowserItem(KListView *parent, QString mime, QString name, QString identifier)
 : KListViewItem(parent, name)
{
	m_mime = mime;
	m_name = name;
	m_identifier = identifier;

	m_proxy = 0;
	m_item = 0;
}

KexiBrowserItem::KexiBrowserItem(KListView *parent, KexiProjectPartProxy *proxy)
 : KListViewItem(parent, proxy->part()->name())
{
	m_mime = proxy->part()->mime();
	m_name = proxy->part()->name();
	m_identifier = QString::null;

	m_proxy = proxy;
	m_item = 0;
}

KexiBrowserItem::KexiBrowserItem(KListView *parent, KexiProjectPartItem *item)
 : KListViewItem(parent, item->name())
{
	m_mime = item->mime();
	m_name = item->name();
	m_identifier = item->identifier();
	m_item = item;
	m_proxy = 0;
}

KexiBrowserItem::KexiBrowserItem(KListViewItem *parent, QString mime, QString name, QString identifier)
 : KListViewItem(parent, name)
{
	m_mime = mime;
	m_name = name;
	m_identifier = identifier;
	m_proxy = 0;
	m_item = 0;
}

KexiBrowserItem::KexiBrowserItem(KListViewItem *parent, KexiProjectPartItem *item)
 : KListViewItem(parent, item->name())
{
	m_mime = item->mime();
	m_name = item->name();
	m_identifier = item->identifier();
	m_item = item;
	m_proxy = 0;
}

KexiProjectPartProxy*
KexiBrowserItem::proxy()
{
	return m_proxy;
}

KexiProjectPartItem*
KexiBrowserItem::item()
{
	return m_item;
}

QString
KexiBrowserItem::mime()
{
	return m_mime;
}

QString
KexiBrowserItem::name()
{
	return m_name;
}

QString
KexiBrowserItem::identifier()
{
	return m_identifier;
}

void
KexiBrowserItem::clearChildren()
{
	KexiBrowserItem* child;
	
	while((child = static_cast<KexiBrowserItem*>(firstChild())))
	{
		delete child;
	}
}

//#include "kexibrowseritem.moc"
