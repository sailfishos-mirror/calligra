// SPDX-FileCopyrightText: 2008, 2010 Brad Hards <bradh@frogmouth.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "rtfreader.h"
#include "AbstractRtfOutput.h"
#include "controlword.h"
#include "rtfdebug.h"

#include "AuthorPcdataDestination.h"
#include "CategoryPcdataDestination.h"
#include "ColorTableDestination.h"
#include "CommentPcdataDestination.h"
#include "CompanyPcdataDestination.h"
#include "DocumentCommentPcdataDestination.h"
#include "DocumentDestination.h"
#include "FontTableDestination.h"
#include "GeneratorPcdataDestination.h"
#include "HLinkBasePcdataDestination.h"
#include "IgnoredDestination.h"
#include "InfoCreatedTimeDestination.h"
#include "InfoDestination.h"
#include "InfoPrintedTimeDestination.h"
#include "InfoRevisedTimeDestination.h"
#include "KeywordsPcdataDestination.h"
#include "ManagerPcdataDestination.h"
#include "OperatorPcdataDestination.h"
#include "PictDestination.h"
#include "StyleSheetDestination.h"
#include "SubjectPcdataDestination.h"
#include "TitlePcdataDestination.h"
#include "UserPropsDestination.h"

#include <QStack>
#include <QTextCursor>
#include <QUrl>

namespace RtfReader
{
Reader::Reader(QObject *parent)
    : QObject(parent)
    , m_inputDevice(nullptr)
{
}

Reader::~Reader()
{
    // Clean up any remaining objects
    qDeleteAll(m_destinationStack);
}

bool Reader::open(const QString &filename)
{
    m_inputDevice = new QFile(filename, this);

    bool result = m_inputDevice->open(QIODevice::ReadOnly);

    return result;
}

void Reader::close()
{
    if (!m_inputDevice)
        return;
    m_inputDevice->close();
    delete m_inputDevice;
    m_inputDevice = nullptr;
}

QString Reader::fileName() const
{
    if (m_inputDevice && m_inputDevice->exists()) {
        return m_inputDevice->fileName();
    } else {
        return QString();
    }
}

bool Reader::parseTo(AbstractRtfOutput *output)
{
    if ((!m_inputDevice) || (!m_inputDevice->isOpen())) {
        return false;
    }

    m_output = output;

    parseFile();

    return true;
}

void Reader::parseFile()
{
    m_tokenizer = new Tokenizer(m_inputDevice);

    if (parseFileHeader()) {
        parseDocument();
    }

    delete m_tokenizer;
}

bool Reader::parseFileHeader()
{
    bool result = true;

    Token token = m_tokenizer->fetchToken();
    if (token.type != OpenGroup) {
        qCDebug(lcRtf) << "Not an RTF file";
        result = false;
    }

    token = m_tokenizer->fetchToken();
    if (token.type != Control) {
        qCDebug(lcRtf) << "Not an RTF file - wrong document type";
        result = false;
    }

    if (!headerFormatIsKnown(token.name, token.parameter.toInt())) {
        qCDebug(lcRtf) << "Not a valid RTF file - unknown header";
        result = false;
    }

    return result;
}

bool Reader::headerFormatIsKnown(const QString &tokenName, int tokenValue)
{
    if (tokenName != QString("rtf")) {
        qCDebug(lcRtf) << "unknown / unexpected header token name:" << tokenName;
        return false;
    }

    if (tokenValue != 1) {
        qCDebug(lcRtf) << "unknown / unexpected header token value:" << tokenValue;
        return false;
    }

    return true;
}

Destination *Reader::makeDestination(const QString &destinationName)
{
    if (destinationName == "colortbl") {
        return new ColorTableDestination(this, m_output, destinationName);
    } else if (destinationName == "creatim") {
        return new InfoCreatedTimeDestination(this, m_output, destinationName);
    } else if (destinationName == "printim") {
        return new InfoPrintedTimeDestination(this, m_output, destinationName);
    } else if (destinationName == "revtim") {
        return new InfoRevisedTimeDestination(this, m_output, destinationName);
    } else if (destinationName == "author") {
        return new AuthorPcdataDestination(this, m_output, destinationName);
    } else if (destinationName == "company") {
        return new CompanyPcdataDestination(this, m_output, destinationName);
    } else if (destinationName == "operator") {
        return new OperatorPcdataDestination(this, m_output, destinationName);
    } else if (destinationName == "comment") {
        return new CommentPcdataDestination(this, m_output, destinationName);
    } else if (destinationName == "doccomm") {
        return new DocumentCommentPcdataDestination(this, m_output, destinationName);
    } else if (destinationName == "title") {
        return new TitlePcdataDestination(this, m_output, destinationName);
    } else if (destinationName == "subject") {
        return new SubjectPcdataDestination(this, m_output, destinationName);
    } else if (destinationName == "manager") {
        return new ManagerPcdataDestination(this, m_output, destinationName);
    } else if (destinationName == "category") {
        return new CategoryPcdataDestination(this, m_output, destinationName);
    } else if (destinationName == "keywords") {
        return new KeywordsPcdataDestination(this, m_output, destinationName);
    } else if (destinationName == "hlinkbase") {
        return new HLinkBasePcdataDestination(this, m_output, destinationName);
    } else if (destinationName == "generator") {
        return new GeneratorPcdataDestination(this, m_output, destinationName);
    } else if (destinationName == "pict") {
        return new PictDestination(this, m_output, destinationName);
    } else if (destinationName == "fonttbl") {
        return new FontTableDestination(this, m_output, destinationName);
    } else if (destinationName == "stylesheet") {
        return new StyleSheetDestination(this, m_output, destinationName);
    } else if (destinationName == "rtf") {
        return new DocumentDestination(this, m_output, destinationName);
    } else if (destinationName == "info") {
        return new InfoDestination(this, m_output, destinationName);
    } else if (destinationName == "userprops") {
        return new UserPropsDestination(this, m_output, destinationName);
    } else if (destinationName == "ignorable") {
        return new IgnoredDestination(this, m_output, destinationName);
    }
    qCDebug(lcRtf) << "creating plain old Destination for" << destinationName;
    return new Destination(this, m_output, destinationName);
}

void Reader::changeDestination(const QString &destinationName)
{
    if (m_destinationStack.top()->name() == QLatin1StringView("ignorable")) {
        // we don't change destinations inside ignored groups
        return;
    }
    // qCDebug(lcRtf) << m_debugIndent << "about to change destination to: " << destinationName;

    Destination *dest = makeDestination(destinationName);

    m_destinationStack.push(dest);
    m_stateStack.top().didChangeDestination = true;
    QStringList destStackElementNames;
    for (int i = 0; i < m_destinationStack.size(); ++i) {
        destStackElementNames << m_destinationStack.at(i)->name();
    }
    qCDebug(lcRtf) << m_debugIndent << "destinationStack after changeDestination (" << destStackElementNames << ")";
}

void Reader::parseDocument()
{
    class RtfGroupState state;

    // Push an end-of-file marker onto the stack
    state.endOfFile = true;
    m_stateStack.push(state);

    // Set up the outer part of the destination stack
    Destination *dest = makeDestination("rtf");
    m_destinationStack.push(dest);
    m_stateStack.top().didChangeDestination = true;

    m_debugIndent = QString('\t');
    // Parse RTF document
    bool atEndOfFile = false;
    bool nextSymbolMightBeDestination = false;
    bool nextSymbolIsIgnorable = false;

    RtfReader::ControlWord controlWord("");

    while (!atEndOfFile) {
        Token token = m_tokenizer->fetchToken();
        // token.dump();
        switch (token.type) {
        case Invalid:
            atEndOfFile = true;
            break;
        case OpenGroup: {
            // Store the current state on the stack
            RtfGroupState state;
            m_stateStack.push(state);
            nextSymbolMightBeDestination = true;
            m_output->startGroup();
            // qCDebug(lcRtf) << m_debugIndent << "opengroup";
            m_debugIndent.append("\t");
            break;
        }
        case CloseGroup: {
            QStringList destStackElementNames;
            for (int i = 0; i < m_destinationStack.size(); ++i) {
                destStackElementNames << m_destinationStack.at(i)->name();
            }
            // qCDebug(lcRtf) << m_debugIndent << "closegroup ( destinationStack:" << destStackElementNames << ")";
            m_debugIndent.remove(0, 1);
            state = m_stateStack.pop();
            if (state.endOfFile) {
                atEndOfFile = true;
            } else {
                m_output->endGroup();
            }

            if (state.didChangeDestination) {
                m_destinationStack.top()->aboutToEndDestination();
                delete m_destinationStack.top();
                m_destinationStack.pop();
            }

            destStackElementNames.clear();
            for (int i = 0; i < m_destinationStack.size(); ++i) {
                destStackElementNames << m_destinationStack.at(i)->name();
            }
            // qCDebug(lcRtf) << m_debugIndent << "destinationStack after CloseGroup: (" << destStackElementNames << ")";
            nextSymbolMightBeDestination = true;
            break;
        }
        case Control:
            controlWord = ControlWord(token.name);
            if (!controlWord.isKnown()) {
                qCDebug(lcRtf) << "*** Unrecognised control word (not in spec 1.9.1): " << token.name;
            }
            // qCDebug(lcRtf) << m_debugIndent << "got controlWord: " << token.name;
            // qCDebug(lcRtf) << m_debugIndent << "isDestination:" << controlWord.isDestination();
            // qCDebug(lcRtf) << m_debugIndent << "isIgnorable:" << nextSymbolIsIgnorable;
            if (nextSymbolMightBeDestination && controlWord.isSupportedDestination()) {
                nextSymbolMightBeDestination = false;
                nextSymbolIsIgnorable = false;
                changeDestination(token.name);
            } else if (nextSymbolMightBeDestination && nextSymbolIsIgnorable) {
                // This is a control word we don't understand
                nextSymbolMightBeDestination = false;
                nextSymbolIsIgnorable = false;
                qCDebug(lcRtf) << "ignorable destination word:" << token.name;
                changeDestination("ignorable");
            } else {
                nextSymbolMightBeDestination = false;
                if (token.name == "*") {
                    nextSymbolMightBeDestination = true;
                    nextSymbolIsIgnorable = true;
                }
                m_destinationStack.top()->handleControlWord(token.name, token.hasParameter, token.parameter.toInt());
            }
            break;
        case Plain:
            m_destinationStack.top()->handlePlainText(token.name);
            break;
        case Binary:
            qCDebug(lcRtf) << "binary data:" << token.name;
            break;
        default:
            qCDebug(lcRtf) << "Unexpected token Type";
        }
    }
}
}
