/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2005 Raphael Langerhorst <raphael.langerhorst@kdemail.net>
             SPDX-FileCopyrightText: 2002-2004 Ariya Hidayat <ariya@kde.org>
             SPDX-FileCopyrightText: 2002-2003 Norbert Andres <nandres@web.de>
             SPDX-FileCopyrightText: 2000-2005 Laurent Montel <montel@kde.org>
             SPDX-FileCopyrightText: 2002 John Dailey <dailey@vt.edu>
             SPDX-FileCopyrightText: 2002 Philipp Mueller <philipp.mueller@gmx.de>
             SPDX-FileCopyrightText: 2001-2002 David Faure <faure@kde.org>
             SPDX-FileCopyrightText: 2001 Werner Trobin <trobin@kde.org>
             SPDX-FileCopyrightText: 2000 Bernd Johannes Wuebben <wuebben@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// Local
#include "PreferenceDialog.h"

#include <KoIcon.h>

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QScrollBar>
#include <QPushButton>

#include <KConfigGroup>
#include <kcombobox.h>
#include <kconfig.h>
#include <KCompletion>

#include <KPluginMetaData>
#include <KPluginInfo>
#include <KPluginSelector>
#include <ksharedconfig.h>
#include <sonnet/configwidget.h>

#include <KoConfigAuthorPage.h>
#include <KoUnit.h>
#include <KoComponentData.h>

#include "ApplicationSettings.h"
#include "CalculationSettings.h"
#include "part/Doc.h"
#include "part/Factory.h"
#include "FunctionModuleRegistry.h"
#include "Global.h"
#include "Localization.h"
#include "Map.h"
#include "Sheet.h"
#include "part/View.h"

#include "ui_FileOptionsWidget.h"
#include "ui_InterfaceOptionsWidget.h"

using namespace Calligra::Sheets;

class PreferenceDialog::Private
{
public:
    View* view;
    KPageWidgetItem* page2;
    KPageWidgetItem* page3;
    KPageWidgetItem* page4;
    KPageWidgetItem* pluginPage;

    // Interface Options
    Ui::InterfaceOptionsWidget interfaceOptions;
    MoveTo oldCursorMovement;
    MethodOfCalc oldFunction;
    KoUnit oldUnit;
    double oldIndentationStep;
    QColor oldGridColor;
    QColor oldPageOutlineColor;

    // Open/Save Options
    Ui::FileOptionsWidget fileOptions;
    int oldRecentFilesEntries;
    int oldAutoSaveDelay;
    bool oldCreateBackupFile;

    // Plugin Options
    KPluginSelector* pluginSelector;

    // Spellchecker Options
    Sonnet::ConfigWidget* spellCheckPage;

    // Author Options
    KoConfigAuthorPage *authorPage;

public:
    // Interface Options
    void applyInterfaceOptions();
    void defaultInterfaceOptions();
    void resetInterfaceOptions();

    // Open/Save Options
    void applyOpenSaveOptions();
    void defaultOpenSaveOptions();
    void resetOpenSaveOptions();
};

void PreferenceDialog::Private::applyInterfaceOptions()
{
    KSharedConfigPtr config = Factory::global().config();
    KConfigGroup parameterGroup = config->group("Parameters");

    oldCursorMovement = view->doc()->map()->settings()->moveToValue();
    oldFunction = view->doc()->map()->settings()->getTypeOfCalc();
    oldUnit = view->doc()->unit();

    const int cursorMovementIndex = interfaceOptions.m_cursorMovement->currentIndex();
    const MoveTo cursorMovement = (MoveTo)interfaceOptions.m_cursorMovement->itemData(cursorMovementIndex).toInt();
    if (cursorMovement != view->doc()->map()->settings()->moveToValue()) {
        view->doc()->map()->settings()->setMoveToValue(cursorMovement);
        parameterGroup.writeEntry("Move", (int)cursorMovement);
        oldCursorMovement = cursorMovement;
    }

    const int functionIndex = interfaceOptions.m_statusBarFunction->currentIndex();
    const MethodOfCalc function = (MethodOfCalc)interfaceOptions.m_statusBarFunction->itemData(functionIndex).toInt();
    if (function != view->doc()->map()->settings()->getTypeOfCalc()) {
        view->doc()->map()->settings()->setTypeOfCalc(function);
        parameterGroup.writeEntry("Method of Calc", (int)function);
        view->calcStatusBarOp();
        view->initCalcMenu();
        oldFunction = function;
    }

    const int unitIndex = interfaceOptions.m_unit->currentIndex();
    const KoUnit unit = KoUnit::fromListForUi(unitIndex, KoUnit::ListAll);
    if (unit != view->doc()->unit()) {
        view->doc()->setUnit(unit);
        // TODO: this is never read, still needed?
        parameterGroup.writeEntry("Unit", static_cast<int>(unit.type()));
        oldUnit = unit;
    }

    const double value = interfaceOptions.m_indentationStep->value();
    if (value != view->doc()->map()->settings()->indentValue()) {
        view->doc()->map()->settings()->setIndentValue(value);
        parameterGroup.writeEntry("Indent", unit.fromUserValue(value));
        oldIndentationStep = value;
    }

    const QColor gridColor = interfaceOptions.m_gridColor->color();
    if (gridColor != view->doc()->map()->settings()->gridColor()) {
        view->doc()->map()->settings()->setGridColor(gridColor);
        config->group("KSpread Color").writeEntry("GridColor", gridColor);
        oldGridColor = gridColor;
    }

    const QColor pageOutlineColor = interfaceOptions.m_pageOutlineColor->color();
    if (pageOutlineColor != view->doc()->map()->settings()->pageOutlineColor()) {
        view->doc()->map()->settings()->changePageOutlineColor(pageOutlineColor);
        config->group("KSpread Color").writeEntry("PageOutlineColor", pageOutlineColor);
        oldPageOutlineColor = pageOutlineColor;
    }

#if 0 // CALLIGRA_SHEETS_COMPLETION_MODE_SETTING
    KCompletion::CompletionMode tmpCompletion = KCompletion::CompletionNone;
    switch (typeCompletion->currentIndex()) {
    case 0:
        tmpCompletion = KCompletion::CompletionNone;
        break;
    case 1:
        tmpCompletion = KCompletion::CompletionShell;
        break;
    case 2:
        tmpCompletion = KCompletion::CompletionPopup;
        break;
    case 3:
        tmpCompletion = KCompletion::CompletionAuto;
        break;
    case 4:
        tmpCompletion = KCompletion::CompletionMan;
        break;
    }


    if (comboChanged) {
        view->doc()->map()->settings()->setCompletionMode(tmpCompletion);
        parameterGroup.writeEntry("Completion Mode", (int)tmpCompletion);
    }
#endif
}

void PreferenceDialog::Private::defaultInterfaceOptions()
{
    interfaceOptions.m_cursorMovement->setCurrentIndex(0);
    interfaceOptions.m_statusBarFunction->setCurrentIndex(0);
    interfaceOptions.m_unit->setCurrentIndex(0);
    interfaceOptions.m_indentationStep->changeValue(10.0);
    interfaceOptions.m_gridColor->setColor(Qt::lightGray);
    interfaceOptions.m_pageOutlineColor->setColor(Qt::red);
#if 0 // CALLIGRA_SHEETS_COMPLETION_MODE_SETTING
    typeCompletion->setCurrentIndex(3);
#endif
}

void PreferenceDialog::Private::resetInterfaceOptions()
{
    KSharedConfigPtr config = Factory::global().config();
    const KConfigGroup parameterGroup = config->group("Parameters");

    oldCursorMovement = view->doc()->map()->settings()->moveToValue();
    oldFunction = view->doc()->map()->settings()->getTypeOfCalc();
    oldUnit = view->doc()->unit();
    oldIndentationStep = view->doc()->map()->settings()->indentValue();

    const KConfigGroup colorGroup = config->group("KSpread Color");
    oldGridColor = colorGroup.readEntry("GridColor", QColor(Qt::lightGray));
    oldPageOutlineColor = colorGroup.readEntry("PageOutlineColor", QColor(Qt::red));

    const int moveToIndex = interfaceOptions.m_cursorMovement->findData(oldCursorMovement);
    interfaceOptions.m_cursorMovement->setCurrentIndex(moveToIndex);
    const int functionIndex = interfaceOptions.m_statusBarFunction->findData(oldFunction);
    interfaceOptions.m_statusBarFunction->setCurrentIndex(functionIndex);
    interfaceOptions.m_unit->setCurrentIndex(oldUnit.indexInListForUi(KoUnit::ListAll));
    interfaceOptions.m_indentationStep->changeValue(oldIndentationStep);
    interfaceOptions.m_gridColor->setColor(oldGridColor);
    interfaceOptions.m_pageOutlineColor->setColor(oldPageOutlineColor);
}

void PreferenceDialog::Private::applyOpenSaveOptions()
{
    KSharedConfigPtr config = Factory::global().config();
    KConfigGroup parameterGroup = config->group("Parameters");
    Doc* doc = view->doc();

    int value = fileOptions.m_recentFilesEntries->value();
    if (value != oldRecentFilesEntries) {
        parameterGroup.writeEntry("NbRecentFile", value);
        view->changeNbOfRecentFiles(value);
        oldRecentFilesEntries = value;
    }

    value = fileOptions.m_autoSaveDelay->value();
    if (value != oldAutoSaveDelay) {
        parameterGroup.writeEntry("AutoSave", value);
        doc->setAutoSave(value * 60);
        oldAutoSaveDelay = value;
    }

    bool state = fileOptions.m_createBackupFile->isChecked();
    if (state != oldCreateBackupFile) {
        parameterGroup.writeEntry("BackupFile", state);
        doc->setBackupFile(state);
        oldCreateBackupFile = state;
    }
}

void PreferenceDialog::Private::defaultOpenSaveOptions()
{
    fileOptions.m_recentFilesEntries->setValue(10);
    fileOptions.m_autoSaveDelay->setValue(KoDocument::defaultAutoSave() / 60);
    fileOptions.m_createBackupFile->setChecked(true);
}

void PreferenceDialog::Private::resetOpenSaveOptions()
{
    KSharedConfigPtr config = Factory::global().config();
    const KConfigGroup parameterGroup = config->group("Parameters");

    oldCreateBackupFile = parameterGroup.readEntry("BackupFile", true);
    oldRecentFilesEntries = parameterGroup.readEntry("NbRecentFile", 10);
    oldAutoSaveDelay = parameterGroup.readEntry("AutoSave", KoDocument::defaultAutoSave() / 60);

    fileOptions.m_createBackupFile->setChecked(oldCreateBackupFile);
    fileOptions.m_recentFilesEntries->setValue(oldRecentFilesEntries);
    fileOptions.m_autoSaveDelay->setValue(oldAutoSaveDelay);
}

QList<KPluginInfo> pluginInfos(const QString &directory)
{
    QList<KPluginInfo> result;
    QVector<KPluginMetaData> pluginMetaDataList = KPluginLoader::findPlugins(directory);
    result.reserve(pluginMetaDataList.size());
    foreach(const KPluginMetaData &metaData, pluginMetaDataList) {
        result.append(KPluginInfo::fromMetaData(metaData));
    }
    return result;
}


PreferenceDialog::PreferenceDialog(View* view)
        : KPageDialog(view)
        , d(new Private)
{
    setObjectName(QLatin1String("PreferenceDialog"));
    setWindowTitle(i18nc("@title:window", "Configure"));
    setFaceType(List);
    setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults | QDialogButtonBox::Reset);
    button(QDialogButtonBox::Ok)->setDefault(true);

    d->view = view;

    connect(this, &QDialog::accepted, this, &PreferenceDialog::slotApply);
    connect(button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked, this, &PreferenceDialog::slotDefault);
    connect(button(QDialogButtonBox::Reset), &QAbstractButton::clicked, this, &PreferenceDialog::slotReset);

    QWidget* widget = 0;
    KPageWidgetItem* page = 0;

    // Interface Options Widget
    widget = new QWidget(this);
    d->interfaceOptions.setupUi(widget);
    page = new KPageWidgetItem(widget, i18n("Interface"));
    page->setIcon(koIcon("preferences-desktop-theme"));
    addPage(page);
    d->page2 = page;

    d->interfaceOptions.m_cursorMovement->addItem(i18n("Down"), Bottom);
    d->interfaceOptions.m_cursorMovement->addItem(i18n("Up"), Top);
    d->interfaceOptions.m_cursorMovement->addItem(i18n("Right"), Right);
    d->interfaceOptions.m_cursorMovement->addItem(i18n("Left"), Left);
    d->interfaceOptions.m_cursorMovement->addItem(i18n("Down, First Column"), BottomFirst);
    d->interfaceOptions.m_cursorMovement->addItem(i18n("None"), NoMovement);

    d->interfaceOptions.m_statusBarFunction->addItem(i18n("Sum"), SumOfNumber);
    d->interfaceOptions.m_statusBarFunction->addItem(i18n("Min"), Min);
    d->interfaceOptions.m_statusBarFunction->addItem(i18n("Max"), Max);
    d->interfaceOptions.m_statusBarFunction->addItem(i18n("Average"), Average);
    d->interfaceOptions.m_statusBarFunction->addItem(i18n("Count"), Count);
    d->interfaceOptions.m_statusBarFunction->addItem(i18n("CountA"), CountA);
    d->interfaceOptions.m_statusBarFunction->addItem(i18n("None"), NoneCalc);

    KComboBox* unitComboBox = d->interfaceOptions.m_unit;
    unitComboBox->addItems(KoUnit::listOfUnitNameForUi(KoUnit::ListAll));
    connect(unitComboBox, QOverload<int>::of(&KComboBox::currentIndexChanged),
            this, &PreferenceDialog::unitChanged);
    unitChanged(0);

    d->interfaceOptions.m_indentationStep->setMinMaxStep(0.0, 400.0, 10.0);

    d->resetInterfaceOptions(); // initialize values

    // Open/Save Options Widget
    widget = new QWidget(this);
    d->fileOptions.setupUi(widget);
    page = new KPageWidgetItem(widget, i18n("Open/Save"));
    page->setIcon(koIcon("document-save"));
    addPage(page);
    d->page3 = page;

    d->resetOpenSaveOptions(); // initialize values

    // Plugin Options Widget
    d->pluginSelector = new KPluginSelector(this);
    const QList<KPluginInfo> functionPluginInfos = pluginInfos(QStringLiteral("calligrasheets/functions"));
    const QList<KPluginInfo> toolPluginInfos = pluginInfos(QStringLiteral("calligrasheets/tools"));
    d->pluginSelector->addPlugins(functionPluginInfos, KPluginSelector::ReadConfigFile,
                                  i18n("Function Modules"), "FunctionModule");
    d->pluginSelector->addPlugins(toolPluginInfos, KPluginSelector::ReadConfigFile,
                                  i18n("Tools"), "Tool");
    d->pluginSelector->load();
    page = new KPageWidgetItem(d->pluginSelector, i18n("Plugins"));
    page->setIcon(koIcon("preferences-plugin"));
    addPage(page);
    d->pluginPage = page;

    // Spell Checker Options
    KSharedConfig::Ptr sharedConfigPtr = Factory::global().config();
    d->spellCheckPage = new Sonnet::ConfigWidget(this);
    page = new KPageWidgetItem(d->spellCheckPage, i18n("Spelling"));
    page->setIcon(koIcon("tools-check-spelling"));
    page->setHeader(i18n("Spell Checker Behavior"));
    addPage(page);
    d->page4 = page;

    d->authorPage = new KoConfigAuthorPage();
    page = new KPageWidgetItem(d->spellCheckPage, i18n("Spelling"));
    page = addPage(d->authorPage, i18nc("@title:tab Author page", "Author"));
    page->setHeader(i18n("Author"));
    page->setIcon(koIcon("user-identity"));
}

PreferenceDialog::~PreferenceDialog()
{
    delete d;
}

void PreferenceDialog::openPage(int flags)
{
    if (flags & InterfacePage)
        setCurrentPage(d->page2);
    else if (flags & OpenSavePage)
        setCurrentPage(d->page3);
    else if (flags & SpellCheckerPage)
        setCurrentPage(d->page4);
    else if (flags & PluginPage)
        setCurrentPage(d->pluginPage);
}

void PreferenceDialog::slotApply()
{
    d->applyInterfaceOptions();
    d->applyOpenSaveOptions();

    // Plugin Options
    d->pluginSelector->save();
    FunctionModuleRegistry::instance()->loadFunctionModules();

    d->spellCheckPage->save();

    d->authorPage->apply();

    // The changes affect the document, not just a single view,
    // so all user interfaces have to be updated.
    d->view->doc()->updateAllViews();
}

void PreferenceDialog::slotDefault()
{
    if (currentPage() == d->page2) {
        d->defaultInterfaceOptions();
    } else if (currentPage() == d->page3) {
        d->defaultOpenSaveOptions();
    } else if (currentPage() == d->page4) {
        d->spellCheckPage->slotDefault();
    } else if (currentPage() == d->pluginPage) {
        d->pluginSelector->load();
    }
}

void PreferenceDialog::slotReset()
{
    if (currentPage() == d->page2) {
        d->resetInterfaceOptions();
    } else if (currentPage() == d->page3) {
        d->resetOpenSaveOptions();
    } else if (currentPage() == d->page4) {
        // TODO
    } else if (currentPage() == d->pluginPage) {
        d->pluginSelector->load();
    }
}

void PreferenceDialog::unitChanged(int index)
{
    const KoUnit unit = KoUnit::fromListForUi(index, KoUnit::ListAll);
    d->interfaceOptions.m_indentationStep->setUnit(unit);
}


#if 0 // CALLIGRA_SHEETS_COMPLETION_MODE_SETTING
QLabel *label = new QLabel(i18n("&Completion mode:"), tmpQGroupBox);

typeCompletion = new KComboBox(tmpQGroupBox);
label->setBuddy(typeCompletion);
typeCompletion->setWhatsThis(i18n("Lets you choose the (auto) text completion mode from a range of options in the drop down selection box."));

listType += i18n("None");
listType += i18n("Manual");
listType += i18n("Popup");
listType += i18n("Automatic");
listType += i18n("Semi-Automatic");
typeCompletion->insertItems(0, listType);
typeCompletion->setCurrentIndex(0);
comboChanged = false;
connect(typeCompletion, SIGNAL(activated(QString)), this, SLOT(slotTextComboChanged(QString)));
#endif
