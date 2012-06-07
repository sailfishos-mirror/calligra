/* This file is part of the KDE project
 * Copyright (C) 2012 Arjen Hiemstra <ahiemstra@heimr.nl>
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

#include "kis_shortcut.h"

#include "kis_abstract_input_action.h"
#include <QEvent>
#include <QDebug>
#include <QKeyEvent>

class KisShortcut::Private
{
public:
    QList<Qt::Key> keys;
    QList<Qt::Key> keyState;
    QList<Qt::MouseButton> buttons;
    QList<Qt::MouseButton> buttonState;

    KisAbstractInputAction *action;
};

KisShortcut::KisShortcut() : d(new Private)
{

}

KisShortcut::~KisShortcut()
{

}

int KisShortcut::priority() const
{
    return d->keys.count() + d->buttons.count();
}

KisAbstractInputAction* KisShortcut::action() const
{
    return d->action;
}

void KisShortcut::setAction(KisAbstractInputAction* action)
{
    d->action = action;
}

void KisShortcut::setButtons(const QList<Qt::MouseButton> &buttons)
{
    d->buttons = buttons;
    d->buttonState.clear();
}

void KisShortcut::setKeys(const QList< Qt::Key >& keys)
{
    d->keys = keys;
    d->keyState.clear();
}

KisShortcut::MatchLevel KisShortcut::matchLevel()
{
    if( d->keys.count() == d->keyState.count() && d->buttons.count() == d->buttonState.count() ) {
        return CompleteMatch;
    } else if( d->keyState.count() > 0 || d->buttonState.count() > 0 ) {
        return PartialMatch;
    }

    return NoMatch;
}

void KisShortcut::match(QEvent* event)
{
    switch(event->type()) {
        case QEvent::KeyPress: {
            QKeyEvent *kevent = static_cast<QKeyEvent*>(event);
            if(!kevent->isAutoRepeat()) {
                Qt::Key key = static_cast<Qt::Key>(kevent->key());
                if(d->keys.contains(key) && !d->keyState.contains(key)) {
                    d->keyState.append(key);
                }
            }
            break;
        }
        case QEvent::KeyRelease: {
            QKeyEvent *kevent = static_cast<QKeyEvent*>(event);
            if(!kevent->isAutoRepeat()) {
                Qt::Key key = static_cast<Qt::Key>(kevent->key());
                if( d->keyState.contains(key) ) {
                    d->keyState.removeOne(key);
                }
            }
            break;
        }
        case QEvent::MouseButtonPress: {
            Qt::MouseButton button = static_cast<QMouseEvent*>(event)->button();
            if( d->buttons.contains( button ) && !d->buttonState.contains(button) ) {
                d->buttonState.append( button );
            }
            break;
        }
        case QEvent::MouseButtonRelease: {
            Qt::MouseButton button = static_cast<QMouseEvent*>(event)->button();
            if( d->buttonState.contains( button ) ) {
                d->buttonState.removeOne(button);
            }
            break;
        }
        default:
            break;
    }
}

void KisShortcut::clear()
{
    d->buttonState.clear();
    d->keyState.clear();
}
