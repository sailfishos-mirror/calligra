/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Johannes Schaub <litb_devel@web.de>
   SPDX-FileCopyrightText: 2011 Arjen Hiemstra <ahiemstra@heimr.nl>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "KoZoomMode.h"
#include <klocalizedstring.h>

const char* const KoZoomMode::modes[] =
{
    I18N_NOOP("%1%"),
    I18N_NOOP("Fit Page Width"),
    I18N_NOOP("Fit Page"),
    nullptr,
    I18N_NOOP("Actual Pixels"),
    nullptr,
    nullptr,
    nullptr,
    I18N_NOOP("Fit Text Width")
};

qreal KoZoomMode::minimumZoomValue = 0.2;
qreal KoZoomMode::maximumZoomValue = 5.0;

QString KoZoomMode::toString(Mode mode)
{
    return i18n(modes[mode]);
}

KoZoomMode::Mode KoZoomMode::toMode(const QString& mode)
{
    if(mode == i18n(modes[ZOOM_WIDTH]))
        return ZOOM_WIDTH;
    else if(mode == i18n(modes[ZOOM_PAGE]))
        return ZOOM_PAGE;
    else if(mode == i18n(modes[ZOOM_PIXELS]))
        return ZOOM_PIXELS;
    else if(mode == i18n(modes[ZOOM_TEXT]))
        return ZOOM_TEXT;
    else
       return ZOOM_CONSTANT;
    // we return ZOOM_CONSTANT else because then we can pass '10%' or '15%'
    // or whatever, it's automatically converted. ZOOM_CONSTANT is
    // changeable, whereas all other zoom modes (non-constants) are normal
    // text like "Fit to xxx". they let the view grow/shrink according
    // to windowsize, hence the term 'non-constant'
}

qreal KoZoomMode::minimumZoom()
{
    return minimumZoomValue;
}

qreal KoZoomMode::maximumZoom()
{
    return maximumZoomValue;
}

qreal KoZoomMode::clampZoom(qreal zoom)
{
    return qMin(maximumZoomValue, qMax(minimumZoomValue, zoom));
}

void KoZoomMode::setMinimumZoom(qreal zoom)
{
    Q_ASSERT(zoom > 0.0f);
    minimumZoomValue = zoom;
}

void KoZoomMode::setMaximumZoom(qreal zoom)
{
    Q_ASSERT(zoom > 0.0f);
    maximumZoomValue = zoom;
}
