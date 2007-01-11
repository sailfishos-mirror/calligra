/*
   This file is part of the KDE project
   Copyright 2001, 2002, 2003, 2004 Nicolas GOUTTE <goutte@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include <QBuffer>
#include <QImage>
//Added by qt3to4:
#include <Q3ValueList>

#include <kdebug.h>

#include <KoPicture.h>

#include "KWEFStructures.h"
#include "KWEFBaseWorker.h"
#include "KWEFKWordLeader.h"

void KWEFBaseWorker::registerKWordLeader(KWEFKWordLeader* leader)
{
    m_kwordLeader=leader;
}

//
// At first, define all methods that do something real!
//

bool KWEFBaseWorker::doAbortFile(void)
{
    // Mostly, aborting is the same than closing the file!
    return doCloseFile();
}

bool KWEFBaseWorker::doFullDocument (const Q3ValueList<ParaData>& paraList)
{
    if (!doOpenTextFrameSet())
        return false;
    if (!doFullAllParagraphs(paraList))
        return false;
    if (!doCloseTextFrameSet())
        return false;

    return true;
}

bool KWEFBaseWorker::doFullAllParagraphs (const Q3ValueList<ParaData>& paraList)
{
    Q3ValueList<ParaData>::ConstIterator it;
	Q3ValueList<ParaData>::ConstIterator end(paraList.end());
    for (it=paraList.begin();it!=end;++it)
    {
        if (!doFullParagraph((*it).text,(*it).layout,(*it).formattingList))
            return false;
    }
    return true;
}

bool KWEFBaseWorker::loadSubFile(const QString& fileName, QByteArray& array) const
// return value:
//   true if the file is not empty
//   false if the file is empty or if an error occurred
{
    bool flag=false;
    if (m_kwordLeader)
    {
        flag=m_kwordLeader->loadSubFile(fileName,array);
    }
    else
    {
        kWarning(30508) << "Leader is unknown! (KWEFBaseWorker::loadSubFile)" << endl;
    }
    return flag;
}

QIODevice* KWEFBaseWorker::getSubFileDevice(const QString& fileName) const
{
    if (!m_kwordLeader)
    {
        kWarning(30508) << "Leader is unknown! (KWEFBaseWorker::getSubFileDevice)" << endl;
        return NULL;
    }
    return m_kwordLeader->getSubFileDevice(fileName);
}

QImage KWEFBaseWorker::loadAndConvertToImage(const QString& strName, const QString& inExtension) const
{
    QIODevice* io=getSubFileDevice(strName);
    if (!io)
    {
        // NO message error, as there must be already one
        return QImage();
    }

    kDebug(30508) << "Picture " << strName << " has size: " << io->size() << endl;
    
    KoPicture picture;
    if (!picture.load(io, inExtension)) // we do not care about KoPictureKey
    {
        kWarning(30508) << "Could not read picture: " << strName << " (KWEFBaseWorker::loadAndConvertToImage)" << endl;
        return QImage();
    }
    
    return picture.generateImage(picture.getOriginalSize()); // ### TODO: KoPicture::getOriginalSize is bad for cliparts
}

bool KWEFBaseWorker::loadAndConvertToImage(const QString& strName, const QString& inExtension, const QString& outExtension, QByteArray& image) const
{
    QImage qimage(loadAndConvertToImage(strName,inExtension));
    
    if (qimage.isNull())
    {
        kWarning(30508) << "Could not load image (KWEFBaseWorker::loadAndConvertToImage)" <<endl;
        return false;
    }
#ifdef __GNUC__
#warning "kde4: port it"
#endif
#if 0    
    QImageIO imageIO;
    imageIO.setImage(qimage);

    QBuffer buffer(&image); // A QBuffer is a QIODevice
    if (!buffer.open(QIODevice::WriteOnly))
    {
        kWarning(30508) << "Could not open buffer! (KWEFBaseWorker::loadAndConvertToImage)" << endl;
        return false;
    }

    imageIO.setIODevice(&buffer);
    imageIO.setFormat(outExtension.utf8());

    if (!imageIO.write())
    {
        kWarning(30508) << "Could not write converted image! (KWEFBaseWorker::loadAndConvertToImage)" << endl;
        return false;
    }
    buffer.close();
#endif
    return true;
}


//
// Secondly, define all methods returning false
//

#define DO_FALSE_DEFINITION(string) \
    bool KWEFBaseWorker::string \
    {\
        kWarning(30508) << "KWEFBaseWorker::" << #string << " was called (Worker not correctly defined?)" << endl; \
        return false;\
    }

DO_FALSE_DEFINITION (doOpenFile (const QString& , const QString& ))
DO_FALSE_DEFINITION (doCloseFile (void))
DO_FALSE_DEFINITION (doOpenDocument (void))
DO_FALSE_DEFINITION (doCloseDocument (void))

// The following is not generated by the leader
DO_FALSE_DEFINITION (doFullParagraph(const QString&, const LayoutData&, const ValueListFormatData&))

//
// Thirdly, define all methods returning true
//

#define DO_TRUE_DEFINITION(string) \
    bool KWEFBaseWorker::string \
    {\
        return true;\
    }

DO_TRUE_DEFINITION (doFullDocumentInfo (const KWEFDocumentInfo&))
DO_TRUE_DEFINITION (doVariableSettings (const VariableSettingsData &))
DO_TRUE_DEFINITION (doFullPaperFormat (const int, const double, const double, const int))
DO_TRUE_DEFINITION (doFullPaperBorders (const double, const double, const double, const double))
DO_TRUE_DEFINITION (doFullPaperFormatOther ( const int, const double, const int ) )
DO_TRUE_DEFINITION (doPageInfo(int,int))
DO_TRUE_DEFINITION (doOpenHead (void))
DO_TRUE_DEFINITION (doCloseHead (void))
DO_TRUE_DEFINITION (doOpenBody (void))
DO_TRUE_DEFINITION (doCloseBody (void))
DO_TRUE_DEFINITION (doOpenStyles (void))
DO_TRUE_DEFINITION (doCloseStyles (void))
DO_TRUE_DEFINITION (doFullDefineStyle (LayoutData&))
DO_TRUE_DEFINITION (doOpenSpellCheckIgnoreList (void))
DO_TRUE_DEFINITION (doCloseSpellCheckIgnoreList (void))
DO_TRUE_DEFINITION (doFullSpellCheckIgnoreWord (const QString&))
DO_TRUE_DEFINITION (doHeader(const HeaderData&))
DO_TRUE_DEFINITION (doFooter(const FooterData&))
DO_TRUE_DEFINITION ( doDeclareNonInlinedFramesets( Q3ValueList<FrameAnchor>&, Q3ValueList<FrameAnchor>& ) )

//  The following are not generated by the leader
DO_TRUE_DEFINITION (doOpenTextFrameSet (void))
DO_TRUE_DEFINITION (doCloseTextFrameSet (void))
