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

import QtQuick 1.1

Item {
    id: base;
    width: parent ? parent.width : 0;
    height: Constants.GridHeight;

    property alias text: input.text;
    property alias placeholder: placeholder.text;
    property alias validator: input.validator;

    property alias background: fill.color;
    property alias border: fill.border;
    property alias radius: fill.radius;
    property alias shadow: dropShadow.visible;

    signal focusLost();
    signal accepted();

    Item {
        anchors.fill: parent;
        anchors.margins: Constants.DefaultMargin;

        DropShadow {
            id: dropShadow;
            anchors.fill: parent;
        }
        Rectangle {
            id: fill;
            anchors.fill: parent;

            Label {
                id: placeholder;
                anchors.left: parent.left;
                anchors.verticalCenter: parent.verticalCenter;
                anchors.margins: Constants.DefaultMargin;
                color: Constants.Theme.SecondaryTextColor;
            }

            TextInput {
                id: input;
                anchors.left: parent.left;
                anchors.right: parent.right;
                anchors.verticalCenter: parent.verticalCenter;
                anchors.margins: Constants.DefaultMargin;
                font.pixelSize: Constants.DefaultFontSize;
                onFocusChanged: {
                    if(focus === false)
                        base.focusLost();
                }
                onAccepted: base.accepted();
            }
        }
    }

    MouseArea {
        anchors.fill: parent;
        onClicked: input.focus = true;
    }

    states: State {
        name: "input";
        when: input.focus || input.text != "";

        PropertyChanges { target: placeholder; opacity: 0.5; }
        AnchorChanges { target: placeholder; anchors.left: undefined; anchors.right: parent.right }
    }

    transitions: Transition {
        ParallelAnimation {
            NumberAnimation { duration: 100; properties: "opacity"; }
            AnchorAnimation { duration: 100; }
        }
    }
}
