/*
 * SPDX-FileCopyrightText: 2015 Stefano Bonicatti <smjert@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "KoMD5Generator.h"

#include <QCryptographicHash>
#include <QFile>
#include <QIODevice>

KoMD5Generator::KoMD5Generator() = default;

KoMD5Generator::~KoMD5Generator() = default;

QByteArray KoMD5Generator::generateHash(const QByteArray &array)
{
    if (!array.isEmpty()) {
        QCryptographicHash md5(QCryptographicHash::Md5);
        md5.addData(array);
        return md5.result();
    }

    return array;
}

QByteArray KoMD5Generator::generateHash(const QString &filename)
{
    QByteArray result;

    QFile f(filename);
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        QByteArray ba = f.readAll();
        result = generateHash(ba);
    }

    return result;
}
