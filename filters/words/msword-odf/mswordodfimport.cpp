/* This file is part of the Calligra project
   Copyright (C) 2002 Werner Trobin <trobin@kde.org>
   Copyright (C) 2002 David Faure <faure@kde.org>
   Copyright (C) 2008 Benjamin Cail <cricketc@gmail.com>
   Copyright (C) 2010, 2011 Matus Uzak <matus.uzak@ixonos.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the Library GNU General Public
   version 2 of the License, or (at your option) version 3 or,
   at the discretion of KDE e.V (which shall act as a proxy as in
   section 14 of the GPLv3), any later version..

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <QFile>
#include <QString>
#include <QBuffer>
//Added by qt3to4:
#include <QByteArray>

#include <kdebug.h>

#include <KoFilterChain.h>
#include <KoOdfWriteStore.h>

#include "mswordodfimport.h"
#include "document.h"
#include "exceptions.h"
#include "msdoc.h"

#include "generated/api.h"
#include "pole.h"

//function prototypes of local functions
bool readStream(POLE::Storage& storage, const char* streampath, QBuffer& buffer);

K_PLUGIN_FACTORY(MSWordOdfImportFactory, registerPlugin<MSWordOdfImport>();)
K_EXPORT_PLUGIN(MSWordOdfImportFactory("calligrafilters"))


MSWordOdfImport::MSWordOdfImport(QObject *parent, const QVariantList&)
        : KoFilter(parent)
{
}

MSWordOdfImport::~MSWordOdfImport()
{
}

struct Finalizer {

    Finalizer(LEInputStream* s) : m_store(0), m_genStyles(0), m_document(0),
                                  m_contentWriter(0), m_bodyWriter(0), m_datastm(s) { }
    ~Finalizer() {
        delete m_store;
        delete m_genStyles;
        delete m_document;
        delete m_contentWriter;
        delete m_bodyWriter;
        delete m_datastm;
    }

    KoStore *m_store;
    KoGenStyles *m_genStyles;
    Document *m_document;
    KoXmlWriter *m_contentWriter;
    KoXmlWriter *m_bodyWriter;
    LEInputStream* m_datastm;
};


KoFilter::ConversionStatus MSWordOdfImport::convert(const QByteArray &from, const QByteArray &to)
{
    // check for proper conversion
    if ((to != "application/vnd.oasis.opendocument.text") ||
        (from != "application/msword")) {
        return KoFilter::NotImplemented;
    }

    kDebug(30513) << "######################## MSWordOdfImport::convert ########################";

    QString inputFile = m_chain->inputFile();
    QString outputFile = m_chain->outputFile();

    /*
     * ************************************************
     *  POLE storage, POLE and LEInput streams
     * ************************************************
     */
    //TODO: POLE:Stream or LEInputStream ?

    POLE::Storage storage(inputFile.toLocal8Bit());
    if (!storage.open()) {
        kDebug(30513) << "Cannot open " << inputFile;
        return KoFilter::InvalidFormat;
    }
    // WordDocument Stream
    QBuffer wordDocumentBuffer;
    if (!readStream(storage, "/WordDocument", wordDocumentBuffer)) {
        return KoFilter::InvalidFormat;
    }
    MSO::FibBase fibBase(wordDocumentBuffer.data().constData(), wordDocumentBuffer.size());
    if (!fibBase) {
        kError(30513) << "Could not parse fibBase";
        return KoFilter::InvalidFormat;
    }

    // document is encrypted or obfuscated
    if (fibBase.fEncrypted) {
        return KoFilter::PasswordProtected;
    }

    // 1Table Stream or 0Table Stream
    const char* tblstm_name = fibBase.fWhichTblStm ? "1Table" : "0Table";
    POLE::Stream tblstm_pole(&storage, tblstm_name);
    if (tblstm_pole.fail()) {
        if (fibBase.nFib >= Word8nFib) {
            kDebug(30513) << "Either the 1Table stream or the 0Table stream MUST be present in the file!";
            return KoFilter::InvalidFormat;
        }
    }

    // Data Stream
    LEInputStream* datastm = 0;
    QBuffer buff3;
    if (!readStream(storage, "/Data", buff3)) {
        kDebug(30513) << "Failed to open /Data stream, no big deal (OPTIONAL).";
    } else {
        datastm = new LEInputStream(&buff3);
    }

    /*
     * ************************************************
     *  Processing file
     * ************************************************
     */
    Finalizer finalizer(datastm);

    // Create output files
    KoStore *storeout;
    storeout = KoStore::createStore(outputFile, KoStore::Write,
                                    "application/vnd.oasis.opendocument.text", KoStore::Zip);
    if (!storeout) {
        kWarning(30513) << "Unable to open output file!";
        return KoFilter::FileNotFound;
    }
    finalizer.m_store = storeout;
    storeout->disallowNameExpansion();
    kDebug(30513) << "created storeout.";
    KoOdfWriteStore oasisStore(storeout);

    kDebug(30513) << "created oasisStore.";

    //create KoGenStyles for writing styles while we're parsing
    KoGenStyles* mainStyles = new KoGenStyles();
    finalizer.m_genStyles = mainStyles; // will delete this as it goes out of scope.

    //create a writer for meta.xml
    QBuffer buf;
    buf.open(QIODevice::WriteOnly);
    KoXmlWriter metaWriter(&buf);

    //create a writer for manifest.xml
    QBuffer manifestBuf;
    manifestBuf.open(QIODevice::WriteOnly);
    KoXmlWriter manifestWriter(&manifestBuf);

    //open contentWriter & bodyWriter *temp* writers
    //so we can write picture files while we parse
    QBuffer contentBuf;
    QBuffer bodyBuf;
    KoXmlWriter *contentWriter = new KoXmlWriter(&contentBuf);
    finalizer.m_contentWriter = contentWriter;
    KoXmlWriter *bodyWriter = new KoXmlWriter(&bodyBuf);
    finalizer.m_bodyWriter = bodyWriter;
    if (!bodyWriter || !contentWriter) {
        //not sure if this is the right error to return
        return KoFilter::CreationError;
    }

    kDebug(30513) << "created temp contentWriter and bodyWriter.";

    //open tags in bodyWriter
    bodyWriter->startElement("office:body");
    bodyWriter->startElement("office:text");

    //create our document object, writing to the temporary buffers
    Document *document = 0;
    LEInputStream wdstm(&wordDocumentBuffer); // XXX: this should be removed!
    try {
        document = new Document(QFile::encodeName(inputFile).data(), this,
                                bodyWriter, &metaWriter, &manifestWriter,
                                storeout, mainStyles,
                                wordDocumentBuffer, tblstm_pole, datastm);
    } catch (InvalidFormatException _e) {
        kDebug(30513) << _e.msg;
        return KoFilter::InvalidFormat;
    } catch (...) {
        kWarning(30513) << "Warning: Caught an unknown exception!";
        return KoFilter::StupidError;
    }

    finalizer.m_document = document;

    //check that we can parse the document?
    if (!document->hasParser()) {
        return KoFilter::WrongFormat;
    }
    //actual parsing & action
    try {
        quint8 ret = document->parse();
        switch (ret) {
        case 1:
            return KoFilter::CreationError;
        case 2:
            return KoFilter::StupidError;
        }
    } catch (InvalidFormatException _e) {
        kDebug(30513) << _e.msg;
        return KoFilter::InvalidFormat;
    } catch (...) {
        kWarning(30513) << "Warning: Caught an unknown exception!";
        return KoFilter::StupidError;
    }

    document->processSubDocQueue(); //process the queues we've created?
    document->finishDocument(); //process footnotes, pictures, ...

    if (!document->bodyFound()) {
        return KoFilter::WrongFormat;
    }
    kDebug(30513) << "finished parsing.";

    //save the office:automatic-styles & and fonts in content.xml
    mainStyles->saveOdfStyles(KoGenStyles::FontFaceDecls, contentWriter);
    mainStyles->saveOdfStyles(KoGenStyles::DocumentAutomaticStyles, contentWriter);

    //close tags in bodyWriter
    bodyWriter->endElement();//office:text
    bodyWriter->endElement();//office:body

    //now create real content/body writers & dump the information there
    KoXmlWriter* realContentWriter = oasisStore.contentWriter();
    if(!realContentWriter) {
        kWarning(30513) << "Error writing content.";
        return KoFilter::CreationError;
    }
    realContentWriter->addCompleteElement(&contentBuf);

    KoXmlWriter *realBodyWriter = oasisStore.bodyWriter();
    realBodyWriter->addCompleteElement(&bodyBuf);

    //now close content & body writers
    if (!oasisStore.closeContentWriter()) {
        kWarning(30513) << "Error closing content.";
        return KoFilter::CreationError;
    }

    kDebug(30513) << "closed content & body writers.";

    //create the settings file
    storeout->open("settings.xml");
    KoStoreDevice settingsDev(storeout);
    KoXmlWriter *settingsWriter = oasisStore.createOasisXmlWriter(&settingsDev, "office:document-settings");
    settingsWriter->startElement("office:settings");
    settingsWriter->startElement("config:config-item-set");
    settingsWriter->addAttribute("config:name", "ooo:configuration-settings");
    settingsWriter->startElement("config:config-item");
    settingsWriter->addAttribute("config:name", "UseFormerLineSpacing");
    settingsWriter->addAttribute("config:type", "boolean");
    settingsWriter->addTextSpan("false");
    settingsWriter->endElement();
    settingsWriter->startElement("config:config-item");
    settingsWriter->addAttribute("config:name", "TabsRelativeToIndent");
    settingsWriter->addAttribute("config:type", "boolean");
    settingsWriter->addTextSpan("false");
    settingsWriter->endElement();
    settingsWriter->endElement(); // config-item-set

    settingsWriter->endElement(); // settings
    settingsWriter->endElement(); // document-settings
    settingsWriter->endDocument();
    delete settingsWriter;
    storeout->close();

    kDebug(30513) << "created settings.xml";

    //create the manifest file
    KoXmlWriter *realManifestWriter = oasisStore.manifestWriter("application/vnd.oasis.opendocument.text");
    //create the styles.xml file
    mainStyles->saveOdfStylesDotXml(storeout, realManifestWriter);
    realManifestWriter->addManifestEntry("content.xml", "text/xml");
    realManifestWriter->addManifestEntry("settings.xml", "text/xml");
    realManifestWriter->addCompleteElement(&manifestBuf);

    kDebug(30513) << "created manifest and styles.xml";

    //create meta.xml
    if (!storeout->open("meta.xml")) {
        return KoFilter::CreationError;
    }

    KoStoreDevice metaDev(storeout);
    KoXmlWriter *meta = KoOdfWriteStore::createOasisXmlWriter(&metaDev, "office:document-meta");
    meta->startElement("office:meta");
    meta->addCompleteElement(&buf);
    meta->endElement(); //office:meta
    meta->endElement(); //office:document-meta
    meta->endDocument();
    delete meta;
    if (!storeout->close()) {
        return KoFilter::CreationError;
    }

    realManifestWriter->addManifestEntry("meta.xml", "text/xml");
    oasisStore.closeManifestWriter();

    kDebug(30513) << "######################## MSWordOdfImport::convert done ####################";
    return KoFilter::OK;
}

void MSWordOdfImport::setProgress(const int percent)
{
    emit sigProgress(percent);
}

/*
 * Read the stream content into buffer.
 * @param storage; POLE storage
 * @param streampath; stream path into the POLE storage
 * @param buffer; buffer provided by the user
 */
bool readStream(POLE::Storage& storage, const char* streampath, QBuffer& buffer)
{
    std::string path(streampath);
    POLE::Stream stream(&storage, path);
    if (stream.fail()) {
        kError(30513) << "Unable to construct " << streampath << "stream";
        return false;
    }

    QByteArray array;
    array.resize(stream.size());
    unsigned long r = stream.read((unsigned char*)array.data(), stream.size());
    if (r != stream.size()) {
        kError(30513) << "Error while reading from " << streampath << "stream";
        return false;
    }
    buffer.setData(array);
    buffer.open(QIODevice::ReadOnly);
    return true;
}

#include <mswordodfimport.moc>
