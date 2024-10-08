/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Marijn Kruisselbrink <mkruisselbrink@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#ifndef MUSIC_RENDERER_H
#define MUSIC_RENDERER_H

#include "core/Global.h"
#include <QColor>
#include <QPointF>

class MusicStyle;
class QPainter;

namespace MusicCore
{
class Sheet;
class Part;
class Staff;
class Voice;
class VoiceElement;
class StaffElement;
class Clef;
class KeySignature;
class TimeSignature;
class Chord;
}

class MusicRenderer
{
public:
    struct RenderState {
        RenderState()
            : clef(nullptr)
        {
        }

        MusicCore::Clef *clef;
    };

    explicit MusicRenderer(MusicStyle *style);

    void renderSheet(QPainter &painter, MusicCore::Sheet *sheet, int firstSystem, int lastSystem);
    void renderPart(QPainter &painter, MusicCore::Part *part, int firstBar, int lastBar, const QColor &color = Qt::black);
    void renderStaff(QPainter &painter, MusicCore::Staff *staff, int firstBar, int lastBar, const QColor &color = Qt::black);
    void renderVoice(QPainter &painter, MusicCore::Voice *voice, int firstBar, int lastBar, const QColor &color = Qt::black);
    void renderElement(QPainter &painter,
                       MusicCore::VoiceElement *element,
                       MusicCore::Voice *voice,
                       const QPointF &pos,
                       RenderState &state,
                       const QColor &color = Qt::black);
    void renderStaffElement(QPainter &painter, MusicCore::StaffElement *element, const QPointF &pos, RenderState &state, const QColor &color = Qt::black);

    void
    renderClef(QPainter &painter, MusicCore::Clef *clef, const QPointF &pos, RenderState &state, const QColor &color = Qt::black, bool ignoreOwnPos = false);
    void renderKeySignature(QPainter &painter,
                            MusicCore::KeySignature *keySignature,
                            const QPointF &pos,
                            RenderState &state,
                            const QColor &color = Qt::black,
                            bool ignoreOwnPos = false);
    void renderTimeSignature(QPainter &painter, MusicCore::TimeSignature *timeSignature, const QPointF &pos, const QColor &color = Qt::black);
    void renderChord(QPainter &painter, MusicCore::Chord *chord, MusicCore::Voice *voice, const QPointF &ref, const QColor &color = Qt::black);
    void renderRest(QPainter &painter, MusicCore::Duration duration, const QPointF &pos, const QColor &color = Qt::black);
    void renderNote(QPainter &painter, MusicCore::Duration duration, const QPointF &pos, qreal stemLength, const QColor &color = Qt::black);
    void renderAccidental(QPainter &painter, int accidentals, const QPointF &pos, const QColor &color = Qt::black);

private:
    MusicStyle *m_style;
    bool m_debug;
};

#endif // MUSIC_RENDERER_H
