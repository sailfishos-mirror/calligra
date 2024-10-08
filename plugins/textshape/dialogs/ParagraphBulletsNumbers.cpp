/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007, 2009-2010 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2008 Girish Ramakrishnan <girish@forwardbias.in>
 * SPDX-FileCopyrightText: 2012 Gopalakrishna Bhat A <gopalakbhat@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ParagraphBulletsNumbers.h"

#include <KoDialog.h>
#include <KoFileDialog.h>
#include <KoImageCollection.h>
#include <KoImageData.h>
#include <KoListLevelProperties.h>
#include <KoParagraphStyle.h>
#include <KoUnit.h>

#include <QDebug>
#include <QUrl>

#include <KIO/StoredTransferJob>
#include <kcharselect.h>

ParagraphBulletsNumbers::ParagraphBulletsNumbers(QWidget *parent)
    : QWidget(parent)
    , m_alignmentMode(false)
    , m_imageCollection(nullptr)
    , m_data(nullptr)
    , m_fontSize(0)
{
    widget.setupUi(this);

    widget.alignment->addItem(i18nc("Automatic horizontal alignment", "Auto"));
    widget.alignment->addItem(i18nc("Text alignment", "Left"));
    widget.alignment->addItem(i18nc("Text alignment", "Right"));
    widget.alignment->addItem(i18nc("Text alignment", "Centered"));

    widget.labelFollowedBy->addItem(i18nc("Tab follows the bullet or number", "Tab Stop"));
    widget.labelFollowedBy->addItem(i18nc("Space", "Space"));
    widget.labelFollowedBy->addItem(i18nc("None", "Nothing"));

    widget.doubleSpinBox->setSingleStep(0.05);
    widget.doubleSpinBox_2->setSingleStep(0.05);
    widget.doubleSpinBox_3->setSingleStep(0.05);

    connect(widget.labelFollowedBy, &QComboBox::currentIndexChanged, this, &ParagraphBulletsNumbers::labelFollowedByIndexChanged);
    connect(widget.listTypes, &QListWidget::currentRowChanged, this, &ParagraphBulletsNumbers::styleChanged);
    connect(widget.customCharacter, &QAbstractButton::clicked, this, &ParagraphBulletsNumbers::customCharButtonPressed);
    connect(widget.letterSynchronization, &QCheckBox::toggled, widget.startValue, &ListsSpinBox::setLetterSynchronization);
    connect(widget.prefix, &QLineEdit::textChanged, this, &ParagraphBulletsNumbers::recalcPreview);
    connect(widget.suffix, &QLineEdit::textChanged, this, &ParagraphBulletsNumbers::recalcPreview);
    connect(widget.depth, &QSpinBox::valueChanged, this, &ParagraphBulletsNumbers::recalcPreview);
    connect(widget.levels, &QSpinBox::valueChanged, this, &ParagraphBulletsNumbers::recalcPreview);
    connect(widget.startValue, &QSpinBox::valueChanged, this, &ParagraphBulletsNumbers::recalcPreview);
    connect(widget.insertImage, &QAbstractButton::clicked, this, &ParagraphBulletsNumbers::selectListImage);
    connect(widget.imageHeight, &QDoubleSpinBox::valueChanged, this, &ParagraphBulletsNumbers::recalcPreview);
    connect(widget.imageWidth, &QDoubleSpinBox::valueChanged, this, &ParagraphBulletsNumbers::recalcPreview);
    connect(widget.restartNumbering, &QAbstractButton::clicked, this, &ParagraphBulletsNumbers::recalcPreview);
}

int ParagraphBulletsNumbers::addStyle(const Lists::ListStyleItem &lsi)
{
    m_mapping.insert(widget.listTypes->count(), lsi.labelType);
    widget.listTypes->addItem(lsi.name);
    return widget.listTypes->count() - 1;
}

void ParagraphBulletsNumbers::setDisplay(KoParagraphStyle *style, int level, bool directFormattingMode)
{
    Q_UNUSED(directFormattingMode);
    KoListStyle *listStyle = style->listStyle();
    widget.listPropertiesPane->setEnabled(listStyle != nullptr);
    widget.customCharacter->setText("-");
    if (listStyle == nullptr) {
        widget.listTypes->setCurrentRow(0);
        return;
    }

    KoListLevelProperties llp = listStyle->levelProperties(level);
    m_previousLevel = llp.level();
    widget.prefix->setText(llp.listItemPrefix());
    widget.suffix->setText(llp.listItemSuffix());
    widget.letterSynchronization->setChecked(llp.letterSynchronization());
    KoListStyle::LabelType labelType = llp.labelType();
    foreach (int row, m_mapping.keys()) {
        if (m_mapping[row] == labelType) {
            widget.listTypes->setCurrentRow(row);
            break;
        }
    }
    int align;
    if (llp.alignment() == (Qt::AlignLeft | Qt::AlignAbsolute))
        align = 1;
    else if (llp.alignment() == (Qt::AlignRight | Qt::AlignAbsolute))
        align = 2;
    else if (llp.alignment() == Qt::AlignCenter)
        align = 3;
    else
        align = 0;

    widget.alignment->setCurrentIndex(align);
    widget.depth->setValue(llp.level());
    widget.levels->setValue(llp.displayLevel());
    widget.startValue->setValue(llp.startValue());
    if (labelType == KoListStyle::BulletCharLabelType)
        widget.customCharacter->setText(llp.bulletCharacter());

    if (labelType == KoListStyle::ImageLabelType) {
        m_data = llp.bulletImage();
        widget.imageHeight->setValue(llp.height());
        widget.imageWidth->setValue(llp.width());
    } else {
        m_data = nullptr;
        widget.imageHeight->setValue(0);
        widget.imageWidth->setValue(0);
    }

    if (llp.alignmentMode() == false) { // for list-level-position-and-space-mode=label-width-and-position disable the following options
        widget.label_8->setEnabled(false);
        widget.label_9->setEnabled(false);
        widget.label_10->setEnabled(false);
        widget.label_11->setEnabled(false);

        widget.labelFollowedBy->setEnabled(false);
        widget.doubleSpinBox->setEnabled(false);
        widget.doubleSpinBox_2->setEnabled(false);
        widget.doubleSpinBox_3->setEnabled(false);
    } else {
        m_alignmentMode = true;
        switch (llp.labelFollowedBy()) {
        case KoListStyle::ListTab:
            widget.doubleSpinBox->setEnabled(true);
            widget.labelFollowedBy->setCurrentIndex(0);
            widget.doubleSpinBox->setValue(KoUnit::toCentimeter(llp.tabStopPosition()));
            break;
        case KoListStyle::Space:
            widget.doubleSpinBox->setEnabled(false);
            widget.labelFollowedBy->setCurrentIndex(1);
            break;
        case KoListStyle::Nothing:
            widget.doubleSpinBox->setEnabled(false);
            widget.labelFollowedBy->setCurrentIndex(2);
            break;
        default:
            Q_ASSERT(false);
        }

        widget.doubleSpinBox_2->setValue(KoUnit::toCentimeter(llp.margin()));
        widget.doubleSpinBox_3->setValue(KoUnit::toCentimeter(llp.margin()) + KoUnit::toCentimeter(llp.textIndent()));
    }

    // *** features not in GUI;
    // character style
    // relative bullet size (percent)
    // minimum label width
    recalcPreview();
}

void ParagraphBulletsNumbers::save(KoParagraphStyle *savingStyle)
{
    Q_ASSERT(savingStyle);

    KoUnit unit(KoUnit::Centimeter);

    const int currentRow = widget.listTypes->currentRow();
    KoListStyle::LabelType labelType = m_mapping[currentRow];
    if (labelType == KoListStyle::None) {
        savingStyle->setListStyle(nullptr);
        return;
    }
    if (savingStyle->listStyle() == nullptr) {
        KoListStyle *listStyle = new KoListStyle(savingStyle);
        savingStyle->setListStyle(listStyle);
    }
    KoListStyle *listStyle = savingStyle->listStyle();
    KoListLevelProperties llp = listStyle->levelProperties(widget.depth->value());
    llp.setLabelType(labelType);
    llp.setLevel(widget.depth->value());
    llp.setDisplayLevel(widget.levels->value());
    llp.setStartValue(widget.startValue->value());
    llp.setListItemPrefix(widget.prefix->text());
    llp.setListItemSuffix(widget.suffix->text());
    llp.setLetterSynchronization(widget.letterSynchronization->isVisible() && widget.letterSynchronization->isChecked());

    if (m_alignmentMode == true) {
        llp.setAlignmentMode(true);
        switch (widget.labelFollowedBy->currentIndex()) {
        case 0:
            llp.setLabelFollowedBy(KoListStyle::ListTab);
            llp.setTabStopPosition(unit.fromUserValue(widget.doubleSpinBox->value()));
            break;
        case 1:
            llp.setLabelFollowedBy(KoListStyle::Space);
            break;
        case 2:
            llp.setLabelFollowedBy(KoListStyle::Nothing);
            break;
        default:
            Q_ASSERT(false);
        }

        llp.setMargin(unit.fromUserValue(widget.doubleSpinBox_2->value()));
        llp.setTextIndent(unit.fromUserValue(widget.doubleSpinBox_3->value()) - unit.fromUserValue(widget.doubleSpinBox_2->value()));
    }

    if (labelType == KoListStyle::ImageLabelType) {
        if (m_data) {
            llp.setBulletImage(m_data);
        }
        llp.setWidth(widget.imageWidth->value());
        llp.setHeight(widget.imageHeight->value());
    } else if (labelType == KoListStyle::BulletCharLabelType) {
        llp.setBulletCharacter((currentRow == m_blankCharIndex) ? QChar() : widget.customCharacter->text().remove('&').at(0));
    }
    // it is important to not use 45 for BulletCharLabelType as it is also char based
    else if (!KoListStyle::isNumberingStyle(labelType)) {
        llp.setRelativeBulletSize(45); // for non-numbering bullets the default relative bullet size is 45%(The spec does not say it; we take it)
    }

    Qt::Alignment align;
    switch (widget.alignment->currentIndex()) {
    case 0:
        align = Qt::AlignLeft;
        break;
    case 1:
        align = Qt::AlignLeft | Qt::AlignAbsolute;
        break;
    case 2:
        align = Qt::AlignRight | Qt::AlignAbsolute;
        break;
    case 3:
        align = Qt::AlignCenter;
        break;
    default:
        Q_ASSERT(false);
    }
    llp.setAlignment(align);

    if (llp.level() != m_previousLevel)
        listStyle->removeLevelProperties(m_previousLevel);
    listStyle->setLevelProperties(llp);
}

void ParagraphBulletsNumbers::styleChanged(int index)
{
    KoListStyle::LabelType labelType = m_mapping[index];
    bool showLetterSynchronization = false;

    if (labelType == KoListStyle::ImageLabelType) {
        widget.startValue->setValue(1);
        widget.startValue->setEnabled(false);
        widget.levels->setValue(1);
        widget.levels->setEnabled(false);
        widget.insertImage->setEnabled(true);
        widget.imageHeight->setEnabled(true);
        widget.imageWidth->setEnabled(true);

        if (widget.imageHeight->value() == 0 && widget.imageWidth->value() == 0) {
            widget.imageHeight->setValue(m_fontSize);
            widget.imageWidth->setValue(m_fontSize);
        }
    } else if (!KoListStyle::isNumberingStyle(labelType)) {
        // widget.startValue->setNumberFormat();
        widget.startValue->setValue(1);
        widget.startValue->setEnabled(false);
        widget.levels->setValue(1);
        widget.levels->setEnabled(false);
        widget.insertImage->setEnabled(false);
        widget.imageHeight->setEnabled(false);
        widget.imageWidth->setEnabled(false);
        widget.imageHeight->setValue(0);
        widget.imageWidth->setValue(0);
    } else {
        widget.levels->setEnabled(true);
        widget.startValue->setEnabled(true);
        // widget.startValue->setNumberFormat();
        int value = widget.startValue->value();
        widget.startValue->setValue(value + 1);
        widget.startValue->setValue(value); // surely to trigger a change event.
        widget.insertImage->setEnabled(false);
        widget.imageHeight->setEnabled(false);
        widget.imageWidth->setEnabled(false);
        widget.imageHeight->setValue(0);
        widget.imageWidth->setValue(0);
    }

    widget.customCharacter->setEnabled(labelType == KoListStyle::BulletCharLabelType && index != m_blankCharIndex);
    widget.letterSynchronization->setVisible(showLetterSynchronization);
    widget.listPropertiesPane->setEnabled(labelType != KoListStyle::None);
    recalcPreview();
}

void ParagraphBulletsNumbers::customCharButtonPressed()
{
    KoDialog *dialog = new KoDialog(this);
    dialog->setModal(true);
    dialog->setButtons(KoDialog::Ok | KoDialog::Cancel);
    dialog->setDefaultButton(KoDialog::Ok);

    KCharSelect *kcs =
        new KCharSelect(dialog,
                        nullptr,
                        KCharSelect::SearchLine | KCharSelect::FontCombo | KCharSelect::BlockCombos | KCharSelect::CharacterTable | KCharSelect::DetailBrowser);

    dialog->setMainWidget(kcs);
    if (dialog->exec() == KoDialog::Accepted) {
        QChar character = kcs->currentChar();
        widget.customCharacter->setText(character);

        // also switch to the custom list style.
        foreach (int row, m_mapping.keys()) {
            if (m_mapping[row] == KoListStyle::BulletCharLabelType) {
                widget.listTypes->setCurrentRow(row);
                break;
            }
        }
    }
    delete dialog;
    recalcPreview();
}

void ParagraphBulletsNumbers::recalcPreview()
{
    Q_EMIT parStyleChanged();
}

void ParagraphBulletsNumbers::labelFollowedByIndexChanged(int index)
{
    if (index == 1 || index == 2) {
        widget.doubleSpinBox->setEnabled(false);
    } else {
        widget.doubleSpinBox->setEnabled(true);
    }
    Q_EMIT parStyleChanged();
    recalcPreview();
}

void ParagraphBulletsNumbers::setImageCollection(KoImageCollection *imageCollection)
{
    m_imageCollection = imageCollection;
}

void ParagraphBulletsNumbers::selectListImage()
{
    KoFileDialog dlg(nullptr, KoFileDialog::OpenFile, "bullets");
    dlg.setCaption(i18n("Select a list image"));
    QUrl url = QUrl::fromLocalFile(dlg.filename());
    if (!url.isEmpty()) {
        KIO::StoredTransferJob *job = KIO::storedGet(url, KIO::NoReload, {});
        connect(job, &KJob::result, this, &ParagraphBulletsNumbers::setImageData);
    }
}

void ParagraphBulletsNumbers::setImageData(KJob *job)
{
    KIO::StoredTransferJob *transferJob = qobject_cast<KIO::StoredTransferJob *>(job);
    Q_ASSERT(transferJob);

    if (m_imageCollection) {
        m_data = m_imageCollection->createImageData(transferJob->data());
    }
    recalcPreview();
}

void ParagraphBulletsNumbers::setFontSize(const KoCharacterStyle *style)
{
    m_fontSize = style->fontPointSize();
}
