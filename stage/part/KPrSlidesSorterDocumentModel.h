/* This file is part of the KDE project
 *
 * SPDX-FileCopyrightText: 2011 Paul Mendez <paulestebanms@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KPRSLIDESSORTERDOCUMENTMODEL_H
#define KPRSLIDESSORTERDOCUMENTMODEL_H

#include <QAbstractListModel>

class KPrViewModeSlidesSorter;
class KoPADocument;
class KoPAPageBase;

/**
 * Class meant to hold the model for Slides Sorter
 * This model lets display slides as thumbnails in a List view, using standard QT
 * view/model framework. It supports copy and move of slides, and include a context
 * menu to present options when dropping.
 */
class KPrSlidesSorterDocumentModel : public QAbstractListModel
{
    Q_OBJECT
public:
    KPrSlidesSorterDocumentModel(KPrViewModeSlidesSorter *viewModeSlidesSorter, QWidget *parent, KoPADocument *document = nullptr);
    ~KPrSlidesSorterDocumentModel() override;

    /**
     * Set the current document
     * @param document a KoPADocument that holds current document
     */
    void setDocument(KoPADocument *document);

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    int rowCount(const QModelIndex &parent) const override;

    QStringList mimeTypes() const override;

    QMimeData *mimeData(const QModelIndexList &indexes) const override;

    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;

    bool removeRows(int row, int count, const QModelIndex &parent) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    /**
     * Drop selected slides (copy/move) if a modifier key is pressed
     * or display a context menu with alternatives.
     * @param slides list of slides to be dropped
     * @param pageAfter destination of the drop
     * @param action the drop action
     */
    void doDrop(QList<KoPAPageBase *> slides, KoPAPageBase *pageAfter, Qt::DropAction action);

    /**
     * @brief Return the page that match the give index in the list
     *
     / @param index the index of the slide to be retrieved
     * @return a Page in the document if it was found or a null pointer if not.
     */
    KoPAPageBase *pageByIndex(const QModelIndex &index) const;

    /**
     * @brief Deletes a given list of slides from the current document
     *
     * @param slides list of slides to be removed
     * @return true if the command execution was successful
     */
    bool removeSlides(const QList<KoPAPageBase *> &slides);

    /**
     * @brief Add a new slide after the current active page
     *
     * @return true if the command execution was successful
     */
    bool addNewSlide();

    /**
     * @brief copy a given list of slides
     *
     * @param slides list of slides to be copied
     * @return true if the command execution was successful
     */
    bool copySlides(const QList<KoPAPageBase *> &slides);

    /**
     * @brief Paste slides from clipboard
     *
     * @return true if the command execution was successful
     */
    bool pasteSlides();

    /**
     * @brief Moves a given list of slides after pageAfter slide
     *
     * @param slides list of slides to be moved
     * @param pageAfter indicates where the slides will be moved
     * @return true if the command execution was successful
     */
    bool moveSlides(const QList<KoPAPageBase *> &slides, KoPAPageBase *pageAfter);

public Q_SLOTS:
    /** emit signals indicating a change in the model layout or items */
    void update();

private:
    // A reference to current document
    KoPADocument *m_document;
    // A reference to Slides sorter class
    KPrViewModeSlidesSorter *m_viewModeSlidesSorter;
};

#endif // KPRSLIDESSORTERDOCUMENTMODEL_H
