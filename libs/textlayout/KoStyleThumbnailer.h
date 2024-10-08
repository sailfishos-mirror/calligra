/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2007 Sebastian Sauer <mail@dipe.org>
 * SPDX-FileCopyrightText: 2008 Thorsten Zachmann <zachmann@kde.org>
 * SPDX-FileCopyrightText: 2008 Girish Ramakrishnan <girish@forwardbias.in>
 * SPDX-FileCopyrightText: 2009-2011 KO GmbH <cbo@kogmbh.com>
 * SPDX-FileCopyrightText: 2011-2012 Pierre Stirnweiss <pstirnweiss@googlemail.com>
 * SPDX-FileCopyrightText: 2012 Gopalakrishna Bhat A <gopalakbhat@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KOSTYLETHUMBNAILER_H
#define KOSTYLETHUMBNAILER_H

#include "kotextlayout_export.h"

#include <QSize>

class KoCharacterStyle;
class KoParagraphStyle;

class QImage;

/**
 * Helper class to create (and cache) thumbnails of styles
 */
class KOTEXTLAYOUT_EXPORT KoStyleThumbnailer
{
public:
    enum KoStyleThumbnailerFlag {
        NoFlags = 0,
        CenterAlignThumbnail = 1, ///< Vertically Center Align the layout of the thumbnail
                                  ///     i.e the layout is done at the center of the area
        UseStyleNameText = 2, ///< Use the style name as the text that is layouted inside the thumbnail
        ScaleThumbnailFont = 4 ///< If set, then when the layout size is more than the size available
                               ///  the font size is scaled down to fit the space available
    };
    Q_DECLARE_FLAGS(KoStyleThumbnailerFlags, KoStyleThumbnailerFlag)

    /**
     * Create a new style thumbnailer.
     */
    explicit KoStyleThumbnailer();

    /**
     * Destructor.
     */
    virtual ~KoStyleThumbnailer();

    /**
     * @returns a thumbnail representing the @param style, constrained into the @param size.
     * If there is no specified @param size, the thumbnail is the size specified with @fn setThumbnailSize or 250*48 pt if no size was provided.
     * If the given @param size is too small, the font size will be decreased, so the thumbnail fits.
     * The real font size is indicated in this case.
     * If @param recreateThumbnail is true, do not return the cached thumbnail if it exist, but recreate a new one.
     * The created thumbnail is cached.
     */
    QImage thumbnail(KoParagraphStyle *style,
                     const QSize &size = QSize(),
                     bool recreateThumbnail = false,
                     KoStyleThumbnailerFlags flags = KoStyleThumbnailerFlags(CenterAlignThumbnail | UseStyleNameText | ScaleThumbnailFont));

    /**
     * @returns a thumbnail representing the @param characterStyle applied on the given @param paragraphStyle, constrained into the @param size.
     * If there is no specified @param size, the thumbnail is the size specified with @fn setThumbnailSize or 250*48 pt if no size was provided.
     * If the given @param size is too small, the font size will be decreased, so the thumbnail fits.
     * The real font size is indicated in this case.
     * If @param recreateThumbnail is true, do not return the cached thumbnail if it exist, but recreate a new one.
     * The created thumbnail is cached.
     */
    QImage thumbnail(KoCharacterStyle *characterStyle,
                     KoParagraphStyle *paragraphStyle = nullptr,
                     const QSize &size = QSize(),
                     bool recreateThumbnail = false,
                     KoStyleThumbnailerFlags flags = KoStyleThumbnailerFlags(CenterAlignThumbnail | UseStyleNameText | ScaleThumbnailFont));

    /**
     * Sets the size of the thumbnails returned by the @fn thumbnail with no size arguments.
     */
    void setThumbnailSize(const QSize &size);

    /**
     * Sets the text that will be layouted.
     * @param text The text that will be layouted inside the thumbnail
     *If the UseStyleNameText flag is set then this text will not be used
     */
    void setText(const QString &text);

    /**
     * remove all occurrences of the style from the cache
     */
    void removeFromCache(KoParagraphStyle *style);

    /**
     * remove all occurrences of the style from the cache
     */
    void removeFromCache(KoCharacterStyle *style);

private:
    void layoutThumbnail(const QSize &size, QImage *im, KoStyleThumbnailerFlags flags);
    void removeFromCache(const QString &expr);

    class Private;
    Private *const d;
};

#endif
