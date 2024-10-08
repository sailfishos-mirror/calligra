/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Pierre Ducroquet <pinaraf@gmail.com>
 * SPDX-FileCopyrightText: 2008 Thorsten Zachmann <zachmann@kde.org>
 * SPDX-FileCopyrightText: 2008 Sebastian Sauer <mail@dipe.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "PageVariable.h"

#include "VariablesDebug.h"

#include <KoProperties.h>
#include <KoShape.h>
#include <KoShapeLoadingContext.h>
#include <KoShapeSavingContext.h>
#include <KoTextDocument.h>
#include <KoTextDocumentLayout.h>
#include <KoTextLayoutRootArea.h>
#include <KoXmlNS.h>
#include <KoXmlReader.h>
#include <KoXmlWriter.h>

#include <QAbstractTextDocumentLayout>
#include <QFontMetricsF>
#include <QTextDocument>
#include <QTextInlineObject>

PageVariable::PageVariable()
    : KoVariable(true)
    , m_type(PageNumber)
    , m_pageselect(KoTextPage::CurrentPage)
    , m_pageadjust(0)
    , m_fixed(false)
{
}

void PageVariable::readProperties(const KoProperties *props)
{
    switch (props->intProperty("vartype")) {
    case 1:
        m_type = PageCount;
        break;
    case 2:
        m_type = PageNumber;
        break;
    case 3:
        m_type = PageContinuation;
        break;
    default:
        Q_ASSERT(false);
        break;
    }
}

void PageVariable::propertyChanged(Property property, const QVariant &value)
{
    switch (m_type) {
    case PageCount:
        if (property == KoInlineObject::PageCount) {
            KoOdfNumberDefinition defaultDefinition; // FIXME Should fetch from pagestyle
            QString newValue = value.toInt() >= 0 ? m_numberFormat.formattedNumber(value.toInt(), &defaultDefinition) : QString();
            setValue(newValue);
        }
        break;
    case PageNumber:
        break;
    case PageContinuation:
        break;
    }
}

void PageVariable::resize(const QTextDocument *document, QTextInlineObject &object, int posInDocument, const QTextCharFormat &format, QPaintDevice *pd)
{
    KoTextPage *page = nullptr;
    if (m_type != PageCount) {
#if 0 // the code is left here to do some testing
        KoTextDocumentLayout *lay = qobject_cast<KoTextDocumentLayout*>(document->documentLayout());
        KoTextLayoutRootArea *rootArea = 0;
        KoTextPage *page2 = 0;
        if (lay) {
            rootArea = lay->rootAreaForPosition(posInDocument);
            if (rootArea) {
                page2 = rootArea->page();
            }
        }
#endif
        page = document->resource(KoTextDocument::LayoutTextPage, KoTextDocument::LayoutTextPageUrl).value<KoTextPage *>();
    }
    int pagenumber = 0;

    switch (m_type) {
    case PageCount:
        break;
    case PageNumber:
        if (page) {
            // the text is not yet layouted therefore we don't get the rootArea
            // if we don't do that we get an endless change of the variable.
            QString currentValue = value();
            if (currentValue.isEmpty() || !m_fixed) {
                pagenumber = page->visiblePageNumber(m_pageselect, m_pageadjust);
                KoOdfNumberDefinition defaultDefinition; // FIXME Should fetch from pagestyle
                QString newValue = pagenumber >= 0 ? m_numberFormat.formattedNumber(pagenumber, &defaultDefinition) : QString();
                // only update value when changed
                if (currentValue != newValue) {
                    setValue(newValue);
                }
            }
        }
        break;
    case PageContinuation:
        if (page) {
            // the text is not yet layouted therefore we don't get the rootArea
            // if we don't do that we get an endless change of the variable.
            pagenumber = page->visiblePageNumber(m_pageselect);
            setValue(pagenumber >= 0 ? m_continuation : QString());
        }
        break;
    }
    KoVariable::resize(document, object, posInDocument, format, pd);
}

void PageVariable::saveOdf(KoShapeSavingContext &context)
{
    KoXmlWriter *writer = &context.xmlWriter();
    switch (m_type) {
    case PageCount:
        // <text:page-count>3</text:page-count>
        writer->startElement("text:page-count", false);
        writer->addTextNode(value());
        writer->endElement();
        break;
    case PageNumber:
        // <text:page-number text:select-page="current" text:page-adjust="2" text:fixed="true">3</text:page-number>
        writer->startElement("text:page-number", false);

        if (m_pageselect == KoTextPage::CurrentPage)
            writer->addAttribute("text:select-page", "current");
        else if (m_pageselect == KoTextPage::PreviousPage)
            writer->addAttribute("text:select-page", "previous");
        else if (m_pageselect == KoTextPage::NextPage)
            writer->addAttribute("text:select-page", "next");

        if (m_pageadjust != 0)
            writer->addAttribute("text:page-adjust", QString::number(m_pageadjust));

        m_numberFormat.saveOdf(writer);

        if (m_fixed)
            writer->addAttribute("text:fixed", "true");

        writer->addTextNode(value());
        writer->endElement();
        break;
    case PageContinuation:
        // <text:page-continuation-string text:select-page="previous">The Text</text:page-continuation-string>
        writer->startElement("page-continuation-string", false);

        if (m_pageselect == KoTextPage::PreviousPage)
            writer->addAttribute("text:select-page", "previous");
        else if (m_pageselect == KoTextPage::NextPage)
            writer->addAttribute("text:select-page", "next");

        writer->addTextNode(m_continuation);
        writer->endElement();
        break;
    }
}

bool PageVariable::loadOdf(const KoXmlElement &element, KoShapeLoadingContext &context)
{
    Q_UNUSED(context);
    const QString localName(element.localName());
    if (localName == "page-count") {
        m_type = PageCount;

        m_numberFormat.loadOdf(element);
    } else if (localName == "page-number") {
        m_type = PageNumber;

        // The text:select-page attribute is used to display the number of the previous or the following
        // page rather than the number of the current page.
        QString pageselect = element.attributeNS(KoXmlNS::text, "select-page", QString());
        if (pageselect == "previous")
            m_pageselect = KoTextPage::PreviousPage;
        else if (pageselect == "next")
            m_pageselect = KoTextPage::NextPage;
        else // "current"
            m_pageselect = KoTextPage::CurrentPage;

        // The value of a page number field can be adjusted by a specified number, allowing the display
        // of page numbers of following or preceding pages. The adjustment amount is specified using
        // the text:page-adjust attribute.
        m_pageadjust = element.attributeNS(KoXmlNS::text, "page-adjust", QString()).toInt();

        m_numberFormat.loadOdf(element);

        // The text:fixed attribute specifies whether or not the value of a field element is fixed. If
        // the value of a field is fixed, the value of the field element to which this attribute is
        // attached is preserved in all future edits of the document. If the value of the field is not
        // fixed, the value of the field may be replaced by a new value when the document is edited.
        m_fixed = element.attributeNS(KoXmlNS::text, "fixed", QString()) == "true";
    } else if (localName == "page-continuation-string") {
        m_type = PageContinuation;

        // This attribute specifies whether to check for a previous or next page and if the page exists, the
        // continuation text is printed.
        QString pageselect = element.attributeNS(KoXmlNS::text, "select-page", QString());
        if (pageselect == "previous")
            m_pageselect = KoTextPage::PreviousPage;
        else if (pageselect == "next")
            m_pageselect = KoTextPage::NextPage;
        else
            m_pageselect = KoTextPage::CurrentPage;

        // The text to display
        m_continuation = element.text();
    }
    return true;
}
