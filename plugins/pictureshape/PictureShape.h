/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2011 Silvio Heinrich <plassy@web.de>
 * SPDX-FileCopyrightText: 2012 Inge Wallin <inge@lysator.liu.se>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef PICTURESHAPE_H
#define PICTURESHAPE_H

#include <QImage>
#include <QObject>
#include <QPainterPath>
#include <QPixmap>
#include <QRunnable>

#include <KoFrameShape.h>
#include <KoTosContainer.h>
#include <SvgShape.h>

#include "ClippingRect.h"

#define PICTURESHAPEID "PictureShape"

class KoImageData;
class KoImageCollection;
class PictureShape;
class KoClipPath;

namespace _Private
{
/**
 * This class acts as a proxy for the PictureShape class
 * since it is not possible to add slots to it
 * (MOC always complains)
 */
class PictureShapeProxy : public QObject
{
    Q_OBJECT
public:
    explicit PictureShapeProxy(PictureShape *p)
        : m_pictureShape(p)
    {
    }

public Q_SLOTS:
    void setImage(const QString &key, const QImage &image);

private:
    PictureShape *m_pictureShape;
};

/**
 * This class will scale an image to a given size.
 * Instances of this class can be executed in a thread pool
 * therefore the scaling process can be done in the background
 */
class PixmapScaler : public QObject, public QRunnable
{
    Q_OBJECT
public:
    PixmapScaler(PictureShape *pictureShape, const QSize &pixmapSize);
    void run() override;

Q_SIGNALS:
    void finished(const QString &, const QImage &);

private:
    QSize m_size;
    QImage m_image;
    quint64 m_imageKey;
};

/**
 * This method will create an outline path out of the image
 */
QPainterPath generateOutline(const QImage &imageIn, int threshold = 20);
}

class PictureShape : public KoTosContainer, public KoFrameShape, public SvgShape
{
    friend class _Private::PixmapScaler;
    friend class _Private::PictureShapeProxy;

public:
    // Odf 1.2: 20.313  style:mirror
    // The value could be 0, or a combination of one of the Horizontal* and/or Vertical
    // separated by whitespace.
    enum MirrorMode {
        MirrorNone = 0x00,
        MirrorHorizontal = 0x01,
        MirrorHorizontalOnEven = 0x02,
        MirrorHorizontalOnOdd = 0x04,
        MirrorVertical = 0x08,

        MirrorMask = 0x0f // Only used as a mask, never as a value.
    };

    enum ColorMode {
        Standard,
        Greyscale,
        Mono,
        Watermark
    };

    PictureShape();

    // reimplemented
    void paint(QPainter &painter, const KoViewConverter &converter, KoShapePaintingContext &paintcontext) override;
    // reimplemented
    QPainterPath shadowOutline() const override;
    // reimplemented
    void saveOdf(KoShapeSavingContext &context) const override;
    // reimplemented
    bool loadOdf(const KoXmlElement &element, KoShapeLoadingContext &context) override;
    // reimplemented
    void waitUntilReady(const KoViewConverter &converter, bool asynchronous) const override;
    // reimplemented from SvgShape
    bool saveSvg(SvgSavingContext &context) override;
    // reimplemented from SvgShape
    bool loadSvg(const KoXmlElement &element, SvgLoadingContext &context) override;
    /**
     * Get the collection used in the shape.
     */
    KoImageCollection *imageCollection() const;
    KoImageData *imageData() const;
    QFlags<MirrorMode> mirrorMode() const;
    ColorMode colorMode() const;
    QRectF cropRect() const;
    bool isPictureInProportion() const;

    void setImageCollection(KoImageCollection *collection)
    {
        m_imageCollection = collection;
    }
    void setCropRect(const QRectF &rect);
    void setMirrorMode(QFlags<MirrorMode> mode);
    void setColorMode(ColorMode mode);
    void setColoring(qreal red, qreal green, qreal blue, qreal luminance, qreal contrast);
    void setGamma(qreal gamma);
    KoClipPath *generateClipPath();

protected:
    bool loadOdfFrameElement(const KoXmlElement &element, KoShapeLoadingContext &context) override;
    QString saveStyle(KoGenStyle &style, KoShapeSavingContext &context) const override;
    void loadStyle(const KoXmlElement &element, KoShapeLoadingContext &context) override;

private:
    QSize calcOptimalPixmapSize(const QSizeF &shapeSize, const QSizeF &imageSize) const;
    ClippingRect parseClippingRectString(const QString &string) const;

private:
    KoImageCollection *m_imageCollection;
    mutable QImage m_printQualityImage;
    mutable QSizeF m_printQualityRequestedSize;

    QFlags<MirrorMode> m_mirrorMode;
    ColorMode m_colorMode;
    ClippingRect m_clippingRect;

    _Private::PictureShapeProxy m_proxy;
};

#endif
