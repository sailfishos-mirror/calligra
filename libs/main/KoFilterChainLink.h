/* This file is part of the Calligra libraries
   SPDX-FileCopyrightText: 2001 Werner Trobin <trobin@kde.org>

SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KOFILTERCHAINLINK_H
#define KOFILTERCHAINLINK_H

#include <KoFilterChain.h>

class QByteArray;
class KoUpdater;

namespace CalligraFilter
{

/**
 * A small private helper class with represents one single filter
 * (one link of the chain)
 * @internal
 */
class ChainLink
{
public:
    ChainLink(KoFilterChain *chain, KoFilterEntry::Ptr filterEntry, const QByteArray &from, const QByteArray &to);

    ~ChainLink();

    KoFilter::ConversionStatus invokeFilter(const ChainLink *const parentChainLink);

    QByteArray from() const
    {
        return m_from;
    }
    QByteArray to() const
    {
        return m_to;
    }

    // debugging
    void dump() const;

    QPointer<KoUpdater> updater() const
    {
        return m_updater;
    }

private:
    ChainLink(const ChainLink &rhs) = delete;
    ChainLink &operator=(const ChainLink &rhs) = delete;

    void setupCommunication(const KoFilter *const parentFilter) const;
    void setupConnections(const KoFilter *sender, const KoFilter *receiver) const;

    KoFilterChain *m_chain;
    KoFilterEntry::Ptr m_filterEntry;
    QByteArray m_from, m_to;

    // This hack is only needed due to crappy Microsoft design and
    // circular dependencies in their embedded files :}
    KoFilter *m_filter;

    QPointer<KoUpdater> const m_updater;
};

}
#endif // KOFILTERCHAINLINK_H
