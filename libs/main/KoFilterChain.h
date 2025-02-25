/* This file is part of the Calligra libraries
   SPDX-FileCopyrightText: 2001 Werner Trobin <trobin@kde.org>

SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __KO_FILTER_CHAIN_H__
#define __KO_FILTER_CHAIN_H__

#include <QHash>
#include <QList>
#include <QStringList>

#include "KoFilter.h"
#include "KoFilterChainLinkList.h"
#include "KoFilterEntry.h"
#include "komain_export.h"
#include <KoStoreDevice.h>

class QTemporaryFile;
class KoFilterManager;
class KoDocument;

namespace CalligraFilter
{
class Graph;
class ChainLink;
class Vertex;
class Edge;
}

/**
 * @brief This class represents a chain of plain KoFilter instances.
 *
 * Instances of this class are shared, so please just hold
 * KoFilterChain::Ptr pointers to it.
 *
 * @author Werner Trobin <trobin@kde.org>
 * @todo the class has no constructor and therefore cannot initialize its private class
 */
class KOMAIN_EXPORT KoFilterChain : public QSharedData
{
    // Only Calligra::Graph is allowed to construct instances and
    // add chain links.
    friend class Graph;
    friend class KoFilterManager;

public:
    typedef QExplicitlySharedDataPointer<KoFilterChain> Ptr;

    virtual ~KoFilterChain();

    /**
     * The filter manager returned may be 0!
     */
    const KoFilterManager *manager() const
    {
        return m_manager;
    }

    /**
     * Starts the filtering process.
     * @return The return status of the conversion. KoFilter::OK
     * if everything is alright.
     */
    KoFilter::ConversionStatus invokeChain();

    /**
     * Tells the @ref KoFilterManager the output file of the
     * filter chain in case of an import operation. If it's
     * an empty QString we directly manipulated the document.
     */
    QString chainOutput() const;

    /**
     * Get the current file to read from. This part of the API
     * is for the filters in our chain.
     */
    QString inputFile();
    /**
     * Get the current file to write to. This part of the API
     * is for the filters in our chain.
     */
    QString outputFile();

    /**
     * Get a file from a storage. May return 0!
     * This part of the API is for the filters in our chain.
     * If you call it multiple times with the same stream name
     * the stream will be closed and re-opened.
     * Note: @em Don't delete that KoStoreDevice we return.
     * @param name The name of the stream inside the storage
     * @param mode Whether we want to read or write from/to the stream
     * @return The storage device to access the stream. May be 0!
     */
    KoStoreDevice *storageFile(const QString &name = "root", KoStore::Mode mode = KoStore::Read);

    /**
     * This method allows your filter to work directly on the
     * KoDocument of the application.
     * This part of the API is for the filters in our chain.
     * @return The document containing the data. May return 0 on error.
     */
    KoDocument *inputDocument();
    /**
     * This method allows your filter to work directly on the
     * KoDocument of the application.
     * This part of the API is for the filters in our chain.
     * @return The document you have to write to. May return 0 on error.
     */
    KoDocument *outputDocument();

    /// returns the amount of filters this chain contains representing the weight
    int weight() const;

    // debugging
    void dump();

private:
    // ### API for Calligra::Graph:
    // Construct a filter chain belonging to some KoFilterManager.
    // The parent filter manager may be 0.

    friend class CalligraFilter::Graph;

    explicit KoFilterChain(const KoFilterManager *manager);

    void appendChainLink(KoFilterEntry::Ptr filterEntry, const QByteArray &from, const QByteArray &to);
    void prependChainLink(KoFilterEntry::Ptr filterEntry, const QByteArray &from, const QByteArray &to);

    // These methods are friends of KoFilterManager and provide access
    // to a private part of its API. As I don't want to include
    // koFilterManager.h in this header the direction is "int" here.
    QString filterManagerImportFile() const;
    QString filterManagerExportFile() const;
    KoDocument *filterManagerKoDocument() const;
    int filterManagerDirection() const;
    KoFilterChain *filterManagerParentChain() const;

    // Helper methods which keep track of all the temp files, documents,
    // storages,... and properly delete them as soon as they are not
    // needed anymore.
    void manageIO();
    void finalizeIO();

    bool createTempFile(QTemporaryFile **tempFile, bool autoDelete = true);

    void inputFileHelper(KoDocument *document, const QString &alternativeFile);
    void outputFileHelper(bool autoDelete);
    KoStoreDevice *storageNewStreamHelper(KoStore **storage, KoStoreDevice **device, const QString &name);
    KoStoreDevice *storageHelper(const QString &file, const QString &streamName, KoStore::Mode mode, KoStore **storage, KoStoreDevice **device);
    void storageInit(const QString &file, KoStore::Mode mode, KoStore **storage);
    KoStoreDevice *storageCreateFirstStream(const QString &streamName, KoStore **storage, KoStoreDevice **device);
    KoStoreDevice *storageCleanupHelper(KoStore **storage);

    KoDocument *createDocument(const QString &file);
    KoDocument *createDocument(const QByteArray &mimeType);

    // "A whole is that which has beginning, middle, and end" - Aristotle
    // ...but we also need to signal "Done" state, Mr. Aristotle
    enum Whole {
        Beginning = 1,
        Middle = 2,
        End = 4,
        Done = 8
    };

    // Don't copy or assign filter chains
    KoFilterChain(const KoFilterChain &rhs) = delete;
    KoFilterChain &operator=(const KoFilterChain &rhs) = delete;

    const KoFilterManager *const m_manager;

    CalligraFilter::ChainLinkList m_chainLinks;

    // stuff needed for bookkeeping
    int m_state;

    QString m_inputFile; // Did we pass around plain files?
    QString m_outputFile;

    KoStore *m_inputStorage; // ...or was it a storage+device?
    KoStoreDevice *m_inputStorageDevice;
    KoStore *m_outputStorage;
    KoStoreDevice *m_outputStorageDevice;

    KoDocument *m_inputDocument; // ...or even documents?
    KoDocument *m_outputDocument;

    QTemporaryFile *m_inputTempFile;
    QTemporaryFile *m_outputTempFile;

    // These two flags keep track of the input/output the
    // filter (=user) asked for
    enum IOState {
        Nil,
        File,
        Storage,
        Document
    };
    IOState m_inputQueried, m_outputQueried;

    class Private;
    Private *const d;
};

#endif // __KO_FILTER_CHAIN_H__
