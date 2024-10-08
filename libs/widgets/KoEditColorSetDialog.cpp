/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2007 Fredy Yanardi <fyanardi@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoEditColorSetDialog.h"

#include <KoIcon.h>

#include <QColorDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QScrollArea>

#include <KLocalizedString>
#include <KMessageBox>

#include <KoColorPatch.h>
#include <KoColorSet.h>
#include <KoColorSpaceRegistry.h>
#include <KoFileDialog.h>

// debug
#include <WidgetsDebug.h>

KoEditColorSetWidget::KoEditColorSetWidget(const QList<KoColorSet *> &palettes, const QString &activePalette, QWidget *parent)
    : QWidget(parent)
    , m_colorSets(palettes)
    , m_gridLayout(nullptr)
    , m_activeColorSet(nullptr)
    , m_activePatch(nullptr)
    , m_initialColorSetCount(palettes.count())
    , m_activeColorSetRequested(false)
{
    widget.setupUi(this);
    foreach (KoColorSet *colorSet, m_colorSets) {
        colorSet->load();
        widget.selector->addItem(colorSet->name());
    }
    connect(widget.selector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &KoEditColorSetWidget::setActiveColorSet);

    // A widget that shows all colors from active palette
    // FIXME no need to handcode the QScrollArea if designer can add QScrollArea (Qt 4.4?)
    m_scrollArea = new QScrollArea(widget.patchesFrame);

    int index = 0;
    foreach (KoColorSet *set, m_colorSets) {
        if (set->name() == activePalette) {
            m_activeColorSet = set;
            index = widget.selector->findText(set->name());
            widget.selector->setCurrentIndex(index);
        }
    }
    if (!m_activeColorSet && !palettes.isEmpty()) {
        m_activeColorSet = palettes.first();
        index = widget.selector->findText(m_activeColorSet->name());
    }

    m_scrollArea->setMinimumWidth(16 * (12 + 2));

    QHBoxLayout *layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_scrollArea);
    widget.patchesFrame->setLayout(layout);

    widget.add->setIcon(koIcon("list-add"));
    widget.remove->setIcon(koIcon("list-remove"));
    widget.open->setIcon(koIcon("document-open"));
    widget.save->setIcon(koIcon("document-save"));

    setEnabled(m_activeColorSet != nullptr);
    setActiveColorSet(index);
    widget.remove->setEnabled(false); // initially no color selected

    connect(widget.add, &QAbstractButton::clicked, this, &KoEditColorSetWidget::addColor);
    connect(widget.remove, &QAbstractButton::clicked, this, &KoEditColorSetWidget::removeColor);
    connect(widget.open, &QAbstractButton::clicked, this, &KoEditColorSetWidget::open);
    connect(widget.save, &QAbstractButton::clicked, this, &KoEditColorSetWidget::save);
}

KoEditColorSetWidget::~KoEditColorSetWidget()
{
    // only delete new color sets
    uint colorSetCount = m_colorSets.count();
    for (uint i = m_initialColorSetCount; i < colorSetCount; ++i) {
        KoColorSet *cs = m_colorSets[i];
        // if the active color set was requested by activeColorSet()
        // the caller takes ownership and then we do not delete it here
        if (cs == m_activeColorSet && m_activeColorSetRequested)
            continue;
        delete cs;
    }
}

void KoEditColorSetWidget::setActiveColorSet(int index)
{
    if (m_gridLayout) {
        delete m_gridLayout;
        m_activePatch = nullptr;
    }

    QWidget *wdg = new QWidget(m_scrollArea);
    m_gridLayout = new QGridLayout();
    m_gridLayout->setContentsMargins({});
    m_gridLayout->setSpacing(2);

    m_activeColorSet = m_colorSets.value(index);
    setEnabled(m_activeColorSet != nullptr);
    if (m_activeColorSet) {
        widget.remove->setEnabled(false);
        for (int i = 0; i < m_activeColorSet->nColors(); i++) {
            KoColorPatch *patch = new KoColorPatch(widget.patchesFrame);
            patch->setColor(m_activeColorSet->getColor(i).color);
            connect(patch, &KoColorPatch::triggered, this, &KoEditColorSetWidget::setTextLabel);
            m_gridLayout->addWidget(patch, i / 16, i % 16);
        }
    }

    wdg->setLayout(m_gridLayout);
    m_scrollArea->setWidget(wdg);
}

void KoEditColorSetWidget::setTextLabel(KoColorPatch *patch)
{
    widget.colorName->setText(patch->color().toQColor().name());
    if (m_activePatch) {
        m_activePatch->setFrameShape(QFrame::NoFrame);
        m_activePatch->setFrameShadow(QFrame::Plain);
    }
    m_activePatch = patch;
    m_activePatch->setFrameShape(QFrame::Panel);
    m_activePatch->setFrameShadow(QFrame::Raised);
    widget.remove->setEnabled(true);
}

void KoEditColorSetWidget::addColor()
{
    QColor color;

    color = QColorDialog::getColor(color);
    if (color.isValid()) {
        KoColorSetEntry newEntry;
        newEntry.color = KoColor(color, KoColorSpaceRegistry::instance()->rgb8());
        newEntry.name = QInputDialog::getText(this, i18n("Add Color To Palette"), i18n("Color name:"));
        KoColorPatch *patch = new KoColorPatch(widget.patchesFrame);
        patch->setColor(newEntry.color);
        connect(patch, &KoColorPatch::triggered, this, &KoEditColorSetWidget::setTextLabel);
        Q_ASSERT(m_gridLayout);
        Q_ASSERT(m_activeColorSet);
        m_gridLayout->addWidget(patch, m_activeColorSet->nColors() / 16, m_activeColorSet->nColors() % 16);
        m_activeColorSet->add(newEntry);
    }
}

void KoEditColorSetWidget::removeColor()
{
    Q_ASSERT(m_activeColorSet);
    for (int i = 0; i < m_activeColorSet->nColors(); i++) {
        if (m_activePatch->color() == m_activeColorSet->getColor(i).color) {
            m_activeColorSet->remove(m_activeColorSet->getColor(i));
            setActiveColorSet(widget.selector->currentIndex());
            break;
        }
    }
}

void KoEditColorSetWidget::open()
{
    Q_ASSERT(m_activeColorSet);
    KoFileDialog dialog(this, KoFileDialog::OpenFile, "OpenColorSet");
    dialog.setDefaultDir(m_activeColorSet->filename());
    dialog.setNameFilter(i18n("Gimp Color Palette (*.gpl)"));
    QString fileName = dialog.filename();
    KoColorSet *colorSet = new KoColorSet(fileName);
    colorSet->load();
    m_colorSets.append(colorSet);
    widget.selector->addItem(colorSet->name());
    widget.selector->setCurrentIndex(widget.selector->count() - 1);
}

void KoEditColorSetWidget::save()
{
    Q_ASSERT(m_activeColorSet);
    if (!m_activeColorSet->save())
        KMessageBox::error(nullptr, i18n("Cannot write to palette file %1. Maybe it is read-only. ", m_activeColorSet->filename()), i18n("Palette"));
}

KoColorSet *KoEditColorSetWidget::activeColorSet()
{
    m_activeColorSetRequested = true;
    return m_activeColorSet;
}

KoEditColorSetDialog::KoEditColorSetDialog(const QList<KoColorSet *> &palettes, const QString &activePalette, QWidget *parent)
    : KoDialog(parent)
{
    ui = new KoEditColorSetWidget(palettes, activePalette, this);
    setMainWidget(ui);
    setCaption(i18n("Add/Remove Colors"));
    enableButton(KoDialog::Ok, ui->isEnabled());
}

KoEditColorSetDialog::~KoEditColorSetDialog()
{
    delete ui;
}

KoColorSet *KoEditColorSetDialog::activeColorSet()
{
    return ui->activeColorSet();
}
