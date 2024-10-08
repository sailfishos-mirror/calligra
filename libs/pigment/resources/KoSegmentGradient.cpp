/*
   SPDX-FileCopyrightText: 2000 Matthias Elter <elter@kde.org>
                 2001 John Califf
                 2004 Boudewijn Rempt <boud@valdyas.org>
                 2004 Adrian Page <adrian@pagenet.plus.com>
                 2004, 2007 Sven Langkamp <sven.langkamp@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "KoSegmentGradient.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

#include <QBuffer>
#include <QByteArray>
#include <QFile>
#include <QImage>
#include <QTextStream>

#include "KoColorSpace.h"
#include "KoColorSpaceRegistry.h"
#include "KoMixColorsOp.h"

#include <DebugPigment.h>
#include <KLocalizedString>

KoGradientSegment::RGBColorInterpolationStrategy *KoGradientSegment::RGBColorInterpolationStrategy::m_instance = nullptr;
KoGradientSegment::HSVCWColorInterpolationStrategy *KoGradientSegment::HSVCWColorInterpolationStrategy::m_instance = nullptr;
KoGradientSegment::HSVCCWColorInterpolationStrategy *KoGradientSegment::HSVCCWColorInterpolationStrategy::m_instance = nullptr;

KoGradientSegment::LinearInterpolationStrategy *KoGradientSegment::LinearInterpolationStrategy::m_instance = nullptr;
KoGradientSegment::CurvedInterpolationStrategy *KoGradientSegment::CurvedInterpolationStrategy::m_instance = nullptr;
KoGradientSegment::SineInterpolationStrategy *KoGradientSegment::SineInterpolationStrategy::m_instance = nullptr;
KoGradientSegment::SphereIncreasingInterpolationStrategy *KoGradientSegment::SphereIncreasingInterpolationStrategy::m_instance = nullptr;
KoGradientSegment::SphereDecreasingInterpolationStrategy *KoGradientSegment::SphereDecreasingInterpolationStrategy::m_instance = nullptr;

KoSegmentGradient::KoSegmentGradient(const QString &file)
    : KoAbstractGradient(file)
{
}

KoSegmentGradient::~KoSegmentGradient()
{
    for (int i = 0; i < m_segments.count(); i++) {
        delete m_segments[i];
        m_segments[i] = 0;
    }
}

KoSegmentGradient::KoSegmentGradient(const KoSegmentGradient &rhs)
    : KoAbstractGradient(rhs)
{
    foreach (KoGradientSegment *segment, rhs.m_segments) {
        pushSegment(new KoGradientSegment(*segment));
    }
}

KoAbstractGradient *KoSegmentGradient::clone() const
{
    return new KoSegmentGradient(*this);
}

bool KoSegmentGradient::load()
{
    QFile file(filename());
    if (!file.open(QIODevice::ReadOnly)) {
        warnPigment << "Can't open file " << filename();
        return false;
    }
    bool res = loadFromDevice(&file);
    file.close();
    return res;
}

bool KoSegmentGradient::loadFromDevice(QIODevice *dev)
{
    QByteArray data = dev->readAll();

    QTextStream fileContent(data, QIODevice::ReadOnly);
    fileContent.setAutoDetectUnicode(true);

    QString header = fileContent.readLine();

    if (header != "GIMP Gradient") {
        return false;
    }

    QString nameDefinition = fileContent.readLine();
    QString numSegmentsText;

    if (nameDefinition.startsWith("Name: ")) {
        QString nameText = nameDefinition.right(nameDefinition.length() - 6);
        setName(nameText);

        numSegmentsText = fileContent.readLine();
    } else {
        // Older format without name.

        numSegmentsText = nameDefinition;
    }

    dbgPigment << "Loading gradient: " << name();

    int numSegments;
    bool ok;

    numSegments = numSegmentsText.toInt(&ok);

    if (!ok || numSegments < 1) {
        return false;
    }

    dbgPigment << "Number of segments = " << numSegments;

    const KoColorSpace *rgbColorSpace = KoColorSpaceRegistry::instance()->rgb8();

    for (int i = 0; i < numSegments; i++) {
        QString segmentText = fileContent.readLine();
        QTextStream segmentFields(&segmentText);
        QStringList values = segmentText.split(' ');

        qreal leftOffset = values[0].toDouble();
        qreal middleOffset = values[1].toDouble();
        qreal rightOffset = values[2].toDouble();

        qreal leftRed = values[3].toDouble();
        qreal leftGreen = values[4].toDouble();
        qreal leftBlue = values[5].toDouble();
        qreal leftAlpha = values[6].toDouble();

        qreal rightRed = values[7].toDouble();
        qreal rightGreen = values[8].toDouble();
        qreal rightBlue = values[9].toDouble();
        qreal rightAlpha = values[10].toDouble();

        int interpolationType = values[11].toInt();
        int colorInterpolationType = values[12].toInt();

        quint8 data[4];
        data[2] = static_cast<quint8>(leftRed * 255 + 0.5);
        data[1] = static_cast<quint8>(leftGreen * 255 + 0.5);
        data[0] = static_cast<quint8>(leftBlue * 255 + 0.5);
        data[3] = static_cast<quint8>(leftAlpha * OPACITY_OPAQUE_U8 + 0.5);

        KoColor leftColor(data, rgbColorSpace);

        data[2] = static_cast<quint8>(rightRed * 255 + 0.5);
        data[1] = static_cast<quint8>(rightGreen * 255 + 0.5);
        data[0] = static_cast<quint8>(rightBlue * 255 + 0.5);
        data[3] = static_cast<quint8>(rightAlpha * OPACITY_OPAQUE_U8 + 0.5);

        KoColor rightColor(data, rgbColorSpace);

        KoGradientSegment *segment =
            new KoGradientSegment(interpolationType, colorInterpolationType, leftOffset, middleOffset, rightOffset, leftColor, rightColor);
        Q_CHECK_PTR(segment);

        if (!segment->isValid()) {
            delete segment;
            return false;
        }

        m_segments.push_back(segment);
    }

    if (!m_segments.isEmpty()) {
        updatePreview();
        setValid(true);
        return true;
    } else {
        return false;
    }
}

bool KoSegmentGradient::save()
{
    QFile file(filename());

    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    saveToDevice(&file);
    file.close();

    return true;
}

bool KoSegmentGradient::saveToDevice(QIODevice *dev) const
{
    QTextStream fileContent(dev);
    fileContent << "GIMP Gradient\n";
    fileContent << "Name: " << name() << "\n";
    fileContent << m_segments.count() << "\n";

    foreach (KoGradientSegment *segment, m_segments) {
        fileContent << QString::number(segment->startOffset(), 'f') << " " << QString::number(segment->middleOffset(), 'f') << " "
                    << QString::number(segment->endOffset(), 'f') << " ";

        QColor startColor = segment->startColor().toQColor();
        QColor endColor = segment->endColor().toQColor();
        fileContent << QString::number(startColor.redF(), 'f') << " " << QString::number(startColor.greenF(), 'f') << " "
                    << QString::number(startColor.blueF(), 'f') << " " << QString::number(startColor.alphaF(), 'f') << " ";
        fileContent << QString::number(endColor.redF(), 'f') << " " << QString::number(endColor.greenF(), 'f') << " " << QString::number(endColor.blueF(), 'f')
                    << " " << QString::number(endColor.alphaF(), 'f') << " ";

        fileContent << (int)segment->interpolation() << " " << (int)segment->colorInterpolation() << "\n";
    }

    KoResource::saveToDevice(dev);

    return true;
}

KoGradientSegment *KoSegmentGradient::segmentAt(qreal t) const
{
    Q_ASSERT(t >= 0 || t <= 1);
    Q_ASSERT(!m_segments.empty());

    for (QList<KoGradientSegment *>::const_iterator it = m_segments.begin(); it != m_segments.end(); ++it) {
        if (t > (*it)->startOffset() - DBL_EPSILON && t < (*it)->endOffset() + DBL_EPSILON) {
            return *it;
        }
    }

    return nullptr;
}

void KoSegmentGradient::colorAt(KoColor &dst, qreal t) const
{
    const KoGradientSegment *segment = segmentAt(t);
    Q_ASSERT(segment != nullptr);

    if (segment) {
        segment->colorAt(dst, t);
    }
}

QGradient *KoSegmentGradient::toQGradient() const
{
    QGradient *gradient = new QLinearGradient();

    QColor color;
    foreach (KoGradientSegment *segment, m_segments) {
        segment->startColor().toQColor(&color);
        gradient->setColorAt(segment->startOffset(), color);
        segment->endColor().toQColor(&color);
        gradient->setColorAt(segment->endOffset(), color);
    }
    return gradient;
}

QString KoSegmentGradient::defaultFileExtension() const
{
    return QString(".ggr");
}

KoGradientSegment::KoGradientSegment(int interpolationType,
                                     int colorInterpolationType,
                                     qreal startOffset,
                                     qreal middleOffset,
                                     qreal endOffset,
                                     const KoColor &startColor,
                                     const KoColor &endColor)
{
    m_interpolator = nullptr;

    switch (interpolationType) {
    case INTERP_LINEAR:
        m_interpolator = LinearInterpolationStrategy::instance();
        break;
    case INTERP_CURVED:
        m_interpolator = CurvedInterpolationStrategy::instance();
        break;
    case INTERP_SINE:
        m_interpolator = SineInterpolationStrategy::instance();
        break;
    case INTERP_SPHERE_INCREASING:
        m_interpolator = SphereIncreasingInterpolationStrategy::instance();
        break;
    case INTERP_SPHERE_DECREASING:
        m_interpolator = SphereDecreasingInterpolationStrategy::instance();
        break;
    }

    m_colorInterpolator = nullptr;

    switch (colorInterpolationType) {
    case COLOR_INTERP_RGB:
        m_colorInterpolator = RGBColorInterpolationStrategy::instance();
        break;
    case COLOR_INTERP_HSV_CCW:
        m_colorInterpolator = HSVCCWColorInterpolationStrategy::instance();
        break;
    case COLOR_INTERP_HSV_CW:
        m_colorInterpolator = HSVCWColorInterpolationStrategy::instance();
        break;
    }

    if (startOffset < DBL_EPSILON) {
        m_startOffset = 0;
    } else if (startOffset > 1 - DBL_EPSILON) {
        m_startOffset = 1;
    } else {
        m_startOffset = startOffset;
    }

    if (middleOffset < m_startOffset + DBL_EPSILON) {
        m_middleOffset = m_startOffset;
    } else if (middleOffset > 1 - DBL_EPSILON) {
        m_middleOffset = 1;
    } else {
        m_middleOffset = middleOffset;
    }

    if (endOffset < m_middleOffset + DBL_EPSILON) {
        m_endOffset = m_middleOffset;
    } else if (endOffset > 1 - DBL_EPSILON) {
        m_endOffset = 1;
    } else {
        m_endOffset = endOffset;
    }

    m_length = m_endOffset - m_startOffset;

    if (m_length < DBL_EPSILON) {
        m_middleT = 0.5;
    } else {
        m_middleT = (m_middleOffset - m_startOffset) / m_length;
    }

    m_startColor = startColor;
    m_endColor = endColor;
}

const KoColor &KoGradientSegment::startColor() const
{
    return m_startColor;
}

const KoColor &KoGradientSegment::endColor() const
{
    return m_endColor;
}

qreal KoGradientSegment::startOffset() const
{
    return m_startOffset;
}

qreal KoGradientSegment::middleOffset() const
{
    return m_middleOffset;
}

qreal KoGradientSegment::endOffset() const
{
    return m_endOffset;
}

void KoGradientSegment::setStartOffset(qreal t)
{
    m_startOffset = t;
    m_length = m_endOffset - m_startOffset;

    if (m_length < DBL_EPSILON) {
        m_middleT = 0.5;
    } else {
        m_middleT = (m_middleOffset - m_startOffset) / m_length;
    }
}
void KoGradientSegment::setMiddleOffset(qreal t)
{
    m_middleOffset = t;

    if (m_length < DBL_EPSILON) {
        m_middleT = 0.5;
    } else {
        m_middleT = (m_middleOffset - m_startOffset) / m_length;
    }
}

void KoGradientSegment::setEndOffset(qreal t)
{
    m_endOffset = t;
    m_length = m_endOffset - m_startOffset;

    if (m_length < DBL_EPSILON) {
        m_middleT = 0.5;
    } else {
        m_middleT = (m_middleOffset - m_startOffset) / m_length;
    }
}

int KoGradientSegment::interpolation() const
{
    return m_interpolator->type();
}

void KoGradientSegment::setInterpolation(int interpolationType)
{
    switch (interpolationType) {
    case INTERP_LINEAR:
        m_interpolator = LinearInterpolationStrategy::instance();
        break;
    case INTERP_CURVED:
        m_interpolator = CurvedInterpolationStrategy::instance();
        break;
    case INTERP_SINE:
        m_interpolator = SineInterpolationStrategy::instance();
        break;
    case INTERP_SPHERE_INCREASING:
        m_interpolator = SphereIncreasingInterpolationStrategy::instance();
        break;
    case INTERP_SPHERE_DECREASING:
        m_interpolator = SphereDecreasingInterpolationStrategy::instance();
        break;
    }
}

int KoGradientSegment::colorInterpolation() const
{
    return m_colorInterpolator->type();
}

void KoGradientSegment::setColorInterpolation(int colorInterpolationType)
{
    switch (colorInterpolationType) {
    case COLOR_INTERP_RGB:
        m_colorInterpolator = RGBColorInterpolationStrategy::instance();
        break;
    case COLOR_INTERP_HSV_CCW:
        m_colorInterpolator = HSVCCWColorInterpolationStrategy::instance();
        break;
    case COLOR_INTERP_HSV_CW:
        m_colorInterpolator = HSVCWColorInterpolationStrategy::instance();
        break;
    }
}

void KoGradientSegment::colorAt(KoColor &dst, qreal t) const
{
    Q_ASSERT(t > m_startOffset - DBL_EPSILON && t < m_endOffset + DBL_EPSILON);

    qreal segmentT;

    if (m_length < DBL_EPSILON) {
        segmentT = 0.5;
    } else {
        segmentT = (t - m_startOffset) / m_length;
    }

    qreal colorT = m_interpolator->valueAt(segmentT, m_middleT);

    m_colorInterpolator->colorAt(dst, colorT, m_startColor, m_endColor);
}

bool KoGradientSegment::isValid() const
{
    if (m_interpolator == nullptr || m_colorInterpolator == nullptr)
        return false;
    return true;
}

KoGradientSegment::RGBColorInterpolationStrategy::RGBColorInterpolationStrategy()
    : m_colorSpace(KoColorSpaceRegistry::instance()->rgb8())
    , buffer(m_colorSpace)
    , m_start(m_colorSpace)
    , m_end(m_colorSpace)
{
}

KoGradientSegment::RGBColorInterpolationStrategy *KoGradientSegment::RGBColorInterpolationStrategy::instance()
{
    if (m_instance == nullptr) {
        m_instance = new RGBColorInterpolationStrategy();
        Q_CHECK_PTR(m_instance);
    }

    return m_instance;
}

void KoGradientSegment::RGBColorInterpolationStrategy::colorAt(KoColor &dst, qreal t, const KoColor &start, const KoColor &end) const
{
    KoColor startDummy, endDummy;
    // hack to get a color space with the bitdepth of the gradients(8bit), but with the colour profile of the image//
    const KoColorSpace *mixSpace = KoColorSpaceRegistry::instance()->rgb8(dst.colorSpace()->profile());
    // convert to the right colorspace for the start and end if we have our mixSpace.
    if (mixSpace) {
        startDummy = KoColor(start, mixSpace);
        endDummy = KoColor(end, mixSpace);
    } else {
        startDummy = start;
        endDummy = end;
    }

    m_start.fromKoColor(start);
    m_end.fromKoColor(end);

    const quint8 *colors[2];
    colors[0] = startDummy.data();
    colors[1] = endDummy.data();

    qint16 colorWeights[2];
    colorWeights[0] = static_cast<quint8>((1.0 - t) * 255 + 0.5);
    colorWeights[1] = 255 - colorWeights[0];

    // check if our mixspace exists, it doesn't at startup.
    if (mixSpace) {
        if (!(*buffer.colorSpace() == *mixSpace)) {
            buffer = KoColor(mixSpace);
        }
        mixSpace->mixColorsOp()->mixColors(colors, colorWeights, 2, buffer.data());
    } else {
        buffer = KoColor(m_colorSpace);
        m_colorSpace->mixColorsOp()->mixColors(colors, colorWeights, 2, buffer.data());
    }

    dst.fromKoColor(buffer);
}

KoGradientSegment::HSVCWColorInterpolationStrategy::HSVCWColorInterpolationStrategy()
    : m_colorSpace(KoColorSpaceRegistry::instance()->rgb8())
{
}

KoGradientSegment::HSVCWColorInterpolationStrategy *KoGradientSegment::HSVCWColorInterpolationStrategy::instance()
{
    if (m_instance == nullptr) {
        m_instance = new HSVCWColorInterpolationStrategy();
        Q_CHECK_PTR(m_instance);
    }

    return m_instance;
}

void KoGradientSegment::HSVCWColorInterpolationStrategy::colorAt(KoColor &dst, qreal t, const KoColor &start, const KoColor &end) const
{
    QColor sc;
    QColor ec;

    start.toQColor(&sc);
    end.toQColor(&ec);

    int s = static_cast<int>(sc.saturation() + t * (ec.saturation() - sc.saturation()) + 0.5);
    int v = static_cast<int>(sc.value() + t * (ec.value() - sc.value()) + 0.5);
    int h;

    if (ec.hue() < sc.hue()) {
        h = static_cast<int>(ec.hue() + (1 - t) * (sc.hue() - ec.hue()) + 0.5);
    } else {
        h = static_cast<int>(ec.hue() + (1 - t) * (360 - ec.hue() + sc.hue()) + 0.5);

        if (h > 359) {
            h -= 360;
        }
    }
    // XXX: added an explicit cast. Is this correct?
    quint8 opacity = static_cast<quint8>(sc.alpha() + t * (ec.alpha() - sc.alpha()));

    QColor result;
    result.setHsv(h, s, v);
    result.setAlpha(opacity);
    dst.fromQColor(result);
}

KoGradientSegment::HSVCCWColorInterpolationStrategy::HSVCCWColorInterpolationStrategy()
    : m_colorSpace(KoColorSpaceRegistry::instance()->rgb8())
{
}

KoGradientSegment::HSVCCWColorInterpolationStrategy *KoGradientSegment::HSVCCWColorInterpolationStrategy::instance()
{
    if (m_instance == nullptr) {
        m_instance = new HSVCCWColorInterpolationStrategy();
        Q_CHECK_PTR(m_instance);
    }

    return m_instance;
}

void KoGradientSegment::HSVCCWColorInterpolationStrategy::colorAt(KoColor &dst, qreal t, const KoColor &start, const KoColor &end) const
{
    QColor sc;
    QColor se;

    start.toQColor(&sc);
    end.toQColor(&se);

    int s = static_cast<int>(sc.saturation() + t * (se.saturation() - sc.saturation()) + 0.5);
    int v = static_cast<int>(sc.value() + t * (se.value() - sc.value()) + 0.5);
    int h;

    if (sc.hue() < se.hue()) {
        h = static_cast<int>(sc.hue() + t * (se.hue() - sc.hue()) + 0.5);
    } else {
        h = static_cast<int>(sc.hue() + t * (360 - sc.hue() + se.hue()) + 0.5);

        if (h > 359) {
            h -= 360;
        }
    }
    // XXX: Added an explicit static cast
    quint8 opacity = static_cast<quint8>(sc.alpha() + t * (se.alpha() - sc.alpha()));

    QColor result;
    result.setHsv(h, s, v);
    result.setAlpha(opacity);
    dst.fromQColor(result);
}

KoGradientSegment::LinearInterpolationStrategy *KoGradientSegment::LinearInterpolationStrategy::instance()
{
    if (m_instance == nullptr) {
        m_instance = new LinearInterpolationStrategy();
        Q_CHECK_PTR(m_instance);
    }

    return m_instance;
}

qreal KoGradientSegment::LinearInterpolationStrategy::calcValueAt(qreal t, qreal middle)
{
    Q_ASSERT(t > -DBL_EPSILON && t < 1 + DBL_EPSILON);
    Q_ASSERT(middle > -DBL_EPSILON && middle < 1 + DBL_EPSILON);

    qreal value = 0;

    if (t <= middle) {
        if (middle < DBL_EPSILON) {
            value = 0;
        } else {
            value = (t / middle) * 0.5;
        }
    } else {
        if (middle > 1 - DBL_EPSILON) {
            value = 1;
        } else {
            value = ((t - middle) / (1 - middle)) * 0.5 + 0.5;
        }
    }

    return value;
}

qreal KoGradientSegment::LinearInterpolationStrategy::valueAt(qreal t, qreal middle) const
{
    return calcValueAt(t, middle);
}

KoGradientSegment::CurvedInterpolationStrategy::CurvedInterpolationStrategy()
{
    m_logHalf = log(0.5);
}

KoGradientSegment::CurvedInterpolationStrategy *KoGradientSegment::CurvedInterpolationStrategy::instance()
{
    if (m_instance == nullptr) {
        m_instance = new CurvedInterpolationStrategy();
        Q_CHECK_PTR(m_instance);
    }

    return m_instance;
}

qreal KoGradientSegment::CurvedInterpolationStrategy::valueAt(qreal t, qreal middle) const
{
    Q_ASSERT(t > -DBL_EPSILON && t < 1 + DBL_EPSILON);
    Q_ASSERT(middle > -DBL_EPSILON && middle < 1 + DBL_EPSILON);

    qreal value = 0;

    if (middle < DBL_EPSILON) {
        middle = DBL_EPSILON;
    }

    value = pow(t, m_logHalf / log(middle));

    return value;
}

KoGradientSegment::SineInterpolationStrategy *KoGradientSegment::SineInterpolationStrategy::instance()
{
    if (m_instance == nullptr) {
        m_instance = new SineInterpolationStrategy();
        Q_CHECK_PTR(m_instance);
    }

    return m_instance;
}

qreal KoGradientSegment::SineInterpolationStrategy::valueAt(qreal t, qreal middle) const
{
    qreal lt = LinearInterpolationStrategy::calcValueAt(t, middle);
    qreal value = (sin(-M_PI_2 + M_PI * lt) + 1.0) / 2.0;

    return value;
}

KoGradientSegment::SphereIncreasingInterpolationStrategy *KoGradientSegment::SphereIncreasingInterpolationStrategy::instance()
{
    if (m_instance == nullptr) {
        m_instance = new SphereIncreasingInterpolationStrategy();
        Q_CHECK_PTR(m_instance);
    }

    return m_instance;
}

qreal KoGradientSegment::SphereIncreasingInterpolationStrategy::valueAt(qreal t, qreal middle) const
{
    qreal lt = LinearInterpolationStrategy::calcValueAt(t, middle) - 1;
    qreal value = sqrt(1 - lt * lt);

    return value;
}

KoGradientSegment::SphereDecreasingInterpolationStrategy *KoGradientSegment::SphereDecreasingInterpolationStrategy::instance()
{
    if (m_instance == nullptr) {
        m_instance = new SphereDecreasingInterpolationStrategy();
        Q_CHECK_PTR(m_instance);
    }

    return m_instance;
}

qreal KoGradientSegment::SphereDecreasingInterpolationStrategy::valueAt(qreal t, qreal middle) const
{
    qreal lt = LinearInterpolationStrategy::calcValueAt(t, middle);
    qreal value = 1 - sqrt(1 - lt * lt);

    return value;
}

void KoSegmentGradient::createSegment(int interpolation,
                                      int colorInterpolation,
                                      double startOffset,
                                      double endOffset,
                                      double middleOffset,
                                      const QColor &left,
                                      const QColor &right)
{
    pushSegment(new KoGradientSegment(interpolation,
                                      colorInterpolation,
                                      startOffset,
                                      middleOffset,
                                      endOffset,
                                      KoColor(left, colorSpace()),
                                      KoColor(right, colorSpace())));
}

const QList<double> KoSegmentGradient::getHandlePositions() const
{
    QList<double> handlePositions;

    handlePositions.push_back(m_segments[0]->startOffset());
    for (int i = 0; i < m_segments.count(); i++) {
        handlePositions.push_back(m_segments[i]->endOffset());
    }
    return handlePositions;
}

const QList<double> KoSegmentGradient::getMiddleHandlePositions() const
{
    QList<double> middleHandlePositions;

    for (int i = 0; i < m_segments.count(); i++) {
        middleHandlePositions.push_back(m_segments[i]->middleOffset());
    }
    return middleHandlePositions;
}

void KoSegmentGradient::moveSegmentStartOffset(KoGradientSegment *segment, double t)
{
    QList<KoGradientSegment *>::iterator it = std::find(m_segments.begin(), m_segments.end(), segment);
    if (it != m_segments.end()) {
        if (it == m_segments.begin()) {
            segment->setStartOffset(0.0);
            return;
        }
        KoGradientSegment *previousSegment = (*(it - 1));
        if (t > segment->startOffset()) {
            if (t > segment->middleOffset())
                t = segment->middleOffset();
        } else {
            if (t < previousSegment->middleOffset())
                t = previousSegment->middleOffset();
        }
        previousSegment->setEndOffset(t);
        segment->setStartOffset(t);
    }
}

void KoSegmentGradient::moveSegmentEndOffset(KoGradientSegment *segment, double t)
{
    QList<KoGradientSegment *>::iterator it = std::find(m_segments.begin(), m_segments.end(), segment);
    if (it != m_segments.end()) {
        if (it + 1 == m_segments.end()) {
            segment->setEndOffset(1.0);
            return;
        }
        KoGradientSegment *followingSegment = (*(it + 1));
        if (t < segment->endOffset()) {
            if (t < segment->middleOffset())
                t = segment->middleOffset();
        } else {
            if (t > followingSegment->middleOffset())
                t = followingSegment->middleOffset();
        }
        followingSegment->setStartOffset(t);
        segment->setEndOffset(t);
    }
}

void KoSegmentGradient::moveSegmentMiddleOffset(KoGradientSegment *segment, double t)
{
    if (segment) {
        if (t > segment->endOffset())
            segment->setMiddleOffset(segment->endOffset());
        else if (t < segment->startOffset())
            segment->setMiddleOffset(segment->startOffset());
        else
            segment->setMiddleOffset(t);
    }
}

void KoSegmentGradient::splitSegment(KoGradientSegment *segment)
{
    Q_ASSERT(segment != nullptr);
    QList<KoGradientSegment *>::iterator it = std::find(m_segments.begin(), m_segments.end(), segment);
    if (it != m_segments.end()) {
        KoColor midleoffsetColor(segment->endColor().colorSpace());
        segment->colorAt(midleoffsetColor, segment->middleOffset());
        KoGradientSegment *newSegment = new KoGradientSegment(segment->interpolation(),
                                                              segment->colorInterpolation(),
                                                              segment->startOffset(),
                                                              (segment->middleOffset() - segment->startOffset()) / 2 + segment->startOffset(),
                                                              segment->middleOffset(),
                                                              segment->startColor(),
                                                              midleoffsetColor);
        m_segments.insert(it, newSegment);
        segment->setStartColor(midleoffsetColor);
        segment->setStartOffset(segment->middleOffset());
        segment->setMiddleOffset((segment->endOffset() - segment->startOffset()) / 2 + segment->startOffset());
    }
}

void KoSegmentGradient::duplicateSegment(KoGradientSegment *segment)
{
    Q_ASSERT(segment != nullptr);
    QList<KoGradientSegment *>::iterator it = std::find(m_segments.begin(), m_segments.end(), segment);
    if (it != m_segments.end()) {
        double middlePositionPercentage = (segment->middleOffset() - segment->startOffset()) / segment->length();
        double center = segment->startOffset() + segment->length() / 2;
        KoGradientSegment *newSegment = new KoGradientSegment(segment->interpolation(),
                                                              segment->colorInterpolation(),
                                                              segment->startOffset(),
                                                              segment->length() / 2 * middlePositionPercentage + segment->startOffset(),
                                                              center,
                                                              segment->startColor(),
                                                              segment->endColor());
        m_segments.insert(it, newSegment);
        segment->setStartOffset(center);
        segment->setMiddleOffset(segment->length() * middlePositionPercentage + segment->startOffset());
    }
}

void KoSegmentGradient::mirrorSegment(KoGradientSegment *segment)
{
    Q_ASSERT(segment != nullptr);
    KoColor tmpColor = segment->startColor();
    segment->setStartColor(segment->endColor());
    segment->setEndColor(tmpColor);
    segment->setMiddleOffset(segment->endOffset() - (segment->middleOffset() - segment->startOffset()));

    if (segment->interpolation() == INTERP_SPHERE_INCREASING)
        segment->setInterpolation(INTERP_SPHERE_DECREASING);
    else if (segment->interpolation() == INTERP_SPHERE_DECREASING)
        segment->setInterpolation(INTERP_SPHERE_INCREASING);

    if (segment->colorInterpolation() == COLOR_INTERP_HSV_CW)
        segment->setColorInterpolation(COLOR_INTERP_HSV_CCW);
    else if (segment->colorInterpolation() == COLOR_INTERP_HSV_CCW)
        segment->setColorInterpolation(COLOR_INTERP_HSV_CW);
}

KoGradientSegment *KoSegmentGradient::removeSegment(KoGradientSegment *segment)
{
    Q_ASSERT(segment != nullptr);
    if (m_segments.count() < 2)
        return nullptr;
    QList<KoGradientSegment *>::iterator it = std::find(m_segments.begin(), m_segments.end(), segment);
    if (it != m_segments.end()) {
        double middlePositionPercentage;
        KoGradientSegment *nextSegment;
        if (it == m_segments.begin()) {
            nextSegment = (*(it + 1));
            middlePositionPercentage = (nextSegment->middleOffset() - nextSegment->startOffset()) / nextSegment->length();
            nextSegment->setStartOffset(segment->startOffset());
            nextSegment->setMiddleOffset(middlePositionPercentage * nextSegment->length() + nextSegment->startOffset());
        } else {
            nextSegment = (*(it - 1));
            middlePositionPercentage = (nextSegment->middleOffset() - nextSegment->startOffset()) / nextSegment->length();
            nextSegment->setEndOffset(segment->endOffset());
            nextSegment->setMiddleOffset(middlePositionPercentage * nextSegment->length() + nextSegment->startOffset());
        }

        delete segment;
        m_segments.erase(it);
        return nextSegment;
    }
    return nullptr;
}

bool KoSegmentGradient::removeSegmentPossible() const
{
    if (m_segments.count() < 2)
        return false;
    return true;
}

const QList<KoGradientSegment *> &KoSegmentGradient::segments() const
{
    return m_segments;
}
