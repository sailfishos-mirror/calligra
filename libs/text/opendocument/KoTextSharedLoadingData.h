/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007-2008 Thorsten Zachmann <zachmann@kde.org>
 * SPDX-FileCopyrightText: 2008 Girish Ramakrishnan <girish@forwardbias.in>
 * SPDX-FileCopyrightText: 2010 Thomas Zander <zander@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KOTEXTSHAREDLOADINGDATA_H
#define KOTEXTSHAREDLOADINGDATA_H

#include "kotext_export.h"
#include <KoSharedLoadingData.h>
#include <KoXmlReaderForward.h>

#include <QList>
#include <QPair>
#include <QString>

class KoOdfLoadingContext;
class KoParagraphStyle;
class KoCharacterStyle;
class KoListStyle;
class KoTableStyle;
class KoTableColumnStyle;
class KoTableRowStyle;
class KoTableCellStyle;
class KoSectionStyle;
class KoStyleManager;
class KoShape;
class KoShapeLoadingContext;
class KoOdfNotesConfiguration;
class KoOdfBibliographyConfiguration;
class KoTextTableTemplate;

#define KOTEXT_SHARED_LOADING_ID "KoTextSharedLoadingId"

/**
 * This class is used to cache the loaded styles so that they have to be loaded only once
 * and can be used by all text shapes.
 * When a text shape is loaded it checks if the KoTextSharedLoadingData is already there.
 * If not it is created.
 */
class KOTEXT_EXPORT KoTextSharedLoadingData : public KoSharedLoadingData
{
    friend class KoTextLoader;

public:
    KoTextSharedLoadingData();
    ~KoTextSharedLoadingData() override;

    /**
     * Load the styles
     *
     * If your application uses a style manager call this function from you application with insertOfficeStyles = true
     * to load the custom styles into the style manager before the rest of the loading is started.
     *
     * @param scontext The shape loading context.
     * @param styleManager The style manager too use or 0 if you don't have a style manager.
     */
    void loadOdfStyles(KoShapeLoadingContext &scontext, KoStyleManager *styleManager);

    /**
     * Get the paragraph style for the given name
     *
     * The name is the style:name given in the file
     *
     * @param name The name of the style to get
     * @param stylesDotXml If set the styles from styles.xml are used, if unset styles from content.xml are used.
     * @return The paragraph style for the given name or 0 if not found
     */
    KoParagraphStyle *paragraphStyle(const QString &name, bool stylesDotXml) const;

    /**
     * Return all paragraph styles.
     *
     * @param stylesDotXml If set the styles from styles.xml are used, if unset styles from content.xml are used.
     * @return All paragraph styles from the given file
     */
    QList<KoParagraphStyle *> paragraphStyles(bool stylesDotXml) const;

    /**
     * Get the character style for the given name
     *
     * The name is the style:name given in the file
     *
     * @param name The name of the style to get
     * @param stylesDotXml If set the styles from styles.xml are used, if unset styles from content.xml are used.
     * @return The character style for the given name or 0 if not found
     */
    KoCharacterStyle *characterStyle(const QString &name, bool stylesDotXml) const;

    /**
     * Return all character styles.
     *
     * @param stylesDotXml If set the styles from styles.xml are used, if unset styles from content.xml are used.
     * @return All character styles from the given file
     */
    QList<KoCharacterStyle *> characterStyles(bool stylesDotXml) const;

    /**
     * Get the list style for the given name
     *
     * @param name The name of the style to get
     * @param stylesDotXml If set the styles from styles.xml are used, if unset styles from content.xml are used.
     * @return The list style for the given name or 0 if not found
     */
    KoListStyle *listStyle(const QString &name, bool stylesDotXml) const;

    /**
     * Get the table style for the given name
     *
     * @param name The name of the style to get
     * @param stylesDotXml If set the styles from styles.xml are used, if unset styles from content.xml are used.
     * @return The table style for the given name or 0 if not found
     */
    KoTableStyle *tableStyle(const QString &name, bool stylesDotXml) const;

    /**
     * Get the table column style for the given name
     *
     * @param name The name of the style to get
     * @param stylesDotXml If set the styles from styles.xml are used, if unset styles from content.xml are used.
     * @return The table column style for the given name or 0 if not found
     */
    KoTableColumnStyle *tableColumnStyle(const QString &name, bool stylesDotXml) const;

    /**
     * Get the table row style for the given name
     *
     * @param name The name of the style to get
     * @param stylesDotXml If set the styles from styles.xml are used, if unset styles from content.xml are used.
     * @return The table row style for the given name or 0 if not found
     */
    KoTableRowStyle *tableRowStyle(const QString &name, bool stylesDotXml) const;

    /**
     * Get the table cell style for the given name
     *
     * @param name The name of the style to get
     * @param stylesDotXml If set the styles from styles.xml are used, if unset styles from content.xml are used.
     * @return The table cell style for the given name or 0 if not found
     */
    KoTableCellStyle *tableCellStyle(const QString &name, bool stylesDotXml) const;

    /**
     * Get the section style for the given name
     *
     * @param name The name of the style to get
     * @param stylesDotXml If set the styles from styles.xml are used, if unset styles from content.xml are used.
     * @return The section style for the given name or 0 if not found
     */
    KoSectionStyle *sectionStyle(const QString &name, bool stylesDotXml) const;

    /**
     * Get the document-wide configuration for bibliography this contains information
     * about prefix, suffix, sort by position, sort algorithm etc.
     */
    KoOdfBibliographyConfiguration bibliographyConfiguration() const;

    /**
     * Returns a list of shapes that should be inserted.
     */
    QList<KoShape *> insertedShapes() const;

protected:
    /**
     * This method got called by kotext once a \a KoShape got inserted and an
     * application can implement this to do additional things with shapes once
     * they got inserted.
     * @param shape a shape that has finished loading.
     * @param element the xml element that represents the shape being inserted.
     */
    virtual void shapeInserted(KoShape *shape, const KoXmlElement &element, KoShapeLoadingContext &context);

private:
    enum StyleType {
        ContentDotXml = 1,
        StylesDotXml = 2
    };
    // helper functions for loading of paragraph styles
    void addParagraphStyles(KoShapeLoadingContext &context, const QList<KoXmlElement *> &styleElements, int styleTypes, KoStyleManager *styleManager = nullptr);
    QList<QPair<QString, KoParagraphStyle *>>
    loadParagraphStyles(KoShapeLoadingContext &context, const QList<KoXmlElement *> &styleElements, int styleTypes, KoStyleManager *manager = nullptr);

    void addDefaultParagraphStyle(KoShapeLoadingContext &context, const KoXmlElement *styleElem, const KoXmlElement *appDefault, KoStyleManager *styleManager);

    // helper functions for loading of character styles
    void addCharacterStyles(KoShapeLoadingContext &context, const QList<KoXmlElement *> &styleElements, int styleTypes, KoStyleManager *styleManager = nullptr);
    struct OdfCharStyle {
        QString odfName;
        QString parentStyle;
        KoCharacterStyle *style;
    };
    // declaring OdfCharStyle as movable type seems not possible, due to being a private nested struct

    QVector<OdfCharStyle> loadCharacterStyles(KoShapeLoadingContext &context, const QList<KoXmlElement *> &styleElements);

    void addDefaultCharacterStyle(KoShapeLoadingContext &context, const KoXmlElement *styleElem, const KoXmlElement *appDefault, KoStyleManager *styleManager);

    // helper functions for loading of list styles
    void addListStyles(KoShapeLoadingContext &context, const QList<KoXmlElement *> &styleElements, int styleTypes, KoStyleManager *styleManager = nullptr);
    QList<QPair<QString, KoListStyle *>> loadListStyles(KoShapeLoadingContext &context, const QList<KoXmlElement *> &styleElements);

    // helper functions for loading of table styles
    void addTableStyles(KoOdfLoadingContext &context, const QList<KoXmlElement *> &styleElements, int styleTypes, KoStyleManager *styleManager = nullptr);
    QList<QPair<QString, KoTableStyle *>> loadTableStyles(KoOdfLoadingContext &context, const QList<KoXmlElement *> &styleElements);

    // helper functions for loading of table column styles
    void addTableColumnStyles(KoOdfLoadingContext &context, const QList<KoXmlElement *> &styleElements, int styleTypes, KoStyleManager *styleManager = nullptr);
    QList<QPair<QString, KoTableColumnStyle *>> loadTableColumnStyles(KoOdfLoadingContext &context, const QList<KoXmlElement *> &styleElements);

    // helper functions for loading of table row styles
    void addTableRowStyles(KoOdfLoadingContext &context, const QList<KoXmlElement *> &styleElements, int styleTypes, KoStyleManager *styleManager = nullptr);
    QList<QPair<QString, KoTableRowStyle *>> loadTableRowStyles(KoOdfLoadingContext &context, const QList<KoXmlElement *> &styleElements);

    // helper functions for loading of table cell styles
    void addTableCellStyles(KoShapeLoadingContext &context, const QList<KoXmlElement *> &styleElements, int styleTypes, KoStyleManager *styleManager = nullptr);
    QList<QPair<QString, KoTableCellStyle *>> loadTableCellStyles(KoShapeLoadingContext &context, const QList<KoXmlElement *> &styleElements);

    // helper functions for loading of section styles
    void addSectionStyles(KoOdfLoadingContext &context, const QList<KoXmlElement *> &styleElements, int styleTypes, KoStyleManager *styleManager = nullptr);
    QList<QPair<QString, KoSectionStyle *>> loadSectionStyles(KoOdfLoadingContext &context, const QList<KoXmlElement *> &styleElements);

    void addOutlineStyle(KoShapeLoadingContext &context, KoStyleManager *styleManager);

    void addNotesConfiguration(KoShapeLoadingContext &context, KoStyleManager *styleManager);

    void addBibliographyConfiguration(KoShapeLoadingContext &context);

    void addTableTemplate(KoShapeLoadingContext &context, KoStyleManager *styleManager);
    QList<QPair<QString, KoTextTableTemplate *>> loadTableTemplates(KoShapeLoadingContext &context);

    class Private;
    Private *const d;
};

#endif /* KOTEXTSHAREDLOADINGDATA_H */
