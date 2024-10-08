/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2016 Camilla Boemann <cbo@boemann.dk>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "GammaFilterEffect.h"
#include <KoFilterEffectRenderContext.h>

#include <QImage>
#include <QtMath>

#include <QDebug>

GammaFilterEffect::GammaFilterEffect()
    : KoFilterEffect(GammaFilterEffectId, "Gamma Correction")
    , m_gamma(0)
{
}

GammaFilterEffect::~GammaFilterEffect() = default;

void GammaFilterEffect::save(KoXmlWriter & /*writer*/)
{
}

bool GammaFilterEffect::load(const KoXmlElement & /*element*/, const KoFilterEffectLoadingContext & /*context*/)
{
    return true;
}

void GammaFilterEffect::setGamma(qreal gamma)
{
    m_gamma = gamma;
}

qreal GammaFilterEffect::gamma() const
{
    return m_gamma;
}

QImage GammaFilterEffect::processImage(const QImage &image, const KoFilterEffectRenderContext &context) const
{
    if (m_gamma <= 0)
        return image;

    QImage result = image.convertToFormat(QImage::Format_ARGB32);

    const int bottom = context.filterRegion().bottom();
    const int left = context.filterRegion().left();
    const int right = context.filterRegion().right();
    const int width = result.width();
    const QRgb *src = (const QRgb *)image.constBits();
    QRgb *dst = (QRgb *)result.bits();

    const int max = 255;
    const qreal invMax = 1.0 / max;
    const qreal invGamma = m_gamma > 0 && m_gamma <= 10.0 ? 1 / m_gamma : 1.0;

    for (int row = context.filterRegion().top(); row < bottom; ++row) {
        int index = row * width + left;
        for (int col = left; col < right; ++col, ++index) {
            const QRgb &s = src[index];
            const int red = qPow(invMax * qRed(s), invGamma) * max;
            const int green = qPow(invMax * qGreen(s), invGamma) * max;
            const int blue = qPow(invMax * qBlue(s), invGamma) * max;
            const int alpha = qAlpha(s);
            dst[index] = qRgba(red, green, blue, alpha);
        }
    }
    return result;
}
