/*
 *  SPDX-FileCopyrightText: 2005 Bart Coppens <kde@bartcoppens.be>
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "KoHistogramProducer.h"

#include <QGlobalStatic>
#include <QList>

#include <KoID.h>

#include "KoBasicHistogramProducers.h"

#include "KoColorSpace.h"

Q_GLOBAL_STATIC(KoHistogramProducerFactoryRegistry, s_instance)

KoHistogramProducerFactoryRegistry::KoHistogramProducerFactoryRegistry() = default;

KoHistogramProducerFactoryRegistry::~KoHistogramProducerFactoryRegistry()
{
    qDeleteAll(values());
}

KoHistogramProducerFactoryRegistry *KoHistogramProducerFactoryRegistry::instance()
{
    return s_instance;
}

QList<QString> KoHistogramProducerFactoryRegistry::keysCompatibleWith(const KoColorSpace *colorSpace) const
{
    QList<QString> list;
    QList<float> preferredList;
    foreach (const QString &id, keys()) {
        KoHistogramProducerFactory *f = value(id);
        if (f->isCompatibleWith(colorSpace)) {
            float preferred = f->preferrednessLevelWith(colorSpace);
            QList<float>::iterator pit = preferredList.begin();
            QList<float>::iterator pend = preferredList.end();
            QList<QString>::iterator lit = list.begin();

            while (pit != pend && preferred <= *pit) {
                ++pit;
                ++lit;
            }

            list.insert(lit, id);
            preferredList.insert(pit, preferred);
        }
    }
    return list;
}
