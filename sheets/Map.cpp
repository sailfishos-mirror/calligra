/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2010 Marijn Kruisselbrink <mkruisselbrink@kde.org>
   SPDX-FileCopyrightText: 1998, 1999 Torben Weis <weis@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

// Local
#include "Map.h"

#include <stdlib.h>
#include <time.h>

#include <QTimer>

#include <kcodecs.h>
#include <kcompletion.h>

#include <KoGlobal.h>
#include <KoEmbeddedDocumentSaver.h>
#include <KoStyleManager.h>
#include <KoParagraphStyle.h>
#include <KoUpdater.h>
#include <KoProgressUpdater.h>

#include "ApplicationSettings.h"
#include "BindingManager.h"
#include "CalculationSettings.h"
#include "CellStorage.h"
#include "Damages.h"
#include "DependencyManager.h"
#include "DocBase.h"
#include "LoadingInfo.h"
#include "Localization.h"
#include "NamedAreaManager.h"
#include "RecalcManager.h"
#include "RowColumnFormat.h"
#include "Sheet.h"
#include "StyleManager.h"
#include "ValueCalc.h"
#include "ValueConverter.h"
#include "ValueFormatter.h"
#include "ValueParser.h"

// database
#include "database/DatabaseManager.h"

using namespace Calligra::Sheets;

class Q_DECL_HIDDEN Map::Private
{
public:
    DocBase* doc;

    /**
     * List of all sheets in this map.
     */
    QList<Sheet*> lstSheets;
    QList<Sheet*> lstDeletedSheets;

    // used to give every Sheet a unique default name.
    int tableId;

    // used to determine the loading progress
    int overallRowCount;
    int loadedRowsCounter;

    LoadingInfo* loadingInfo;
    bool readwrite;

    BindingManager* bindingManager;
    DatabaseManager* databaseManager;
    DependencyManager* dependencyManager;
    NamedAreaManager* namedAreaManager;
    RecalcManager* recalcManager;
    StyleManager* styleManager;
    KoStyleManager* textStyleManager;

    ApplicationSettings* applicationSettings;
    CalculationSettings* calculationSettings;
    ValueCalc* calc;
    ValueConverter* converter;
    ValueFormatter* formatter;
    ValueParser* parser;

    // default objects
    ColumnFormat* defaultColumnFormat;
    RowFormat* defaultRowFormat;

    QList<Damage*> damages;
    bool isLoading;

    int syntaxVersion;

    KCompletion listCompletion;
};


Map::Map(DocBase* doc, int syntaxVersion)
        : QObject(doc),
        d(new Private)
{
    setObjectName(QLatin1String("Map")); // necessary for D-Bus
    d->doc = doc;
    d->tableId = 1;
    d->overallRowCount = 0;
    d->loadedRowsCounter = 0;
    d->loadingInfo = 0;
    d->readwrite = true;

    d->bindingManager = new BindingManager(this);
    d->databaseManager = new DatabaseManager(this);
    d->dependencyManager = new DependencyManager(this);
    d->namedAreaManager = new NamedAreaManager(this);
    d->recalcManager = new RecalcManager(this);
    d->styleManager = new StyleManager();
    d->textStyleManager = new KoStyleManager(this);
    d->applicationSettings = new ApplicationSettings();
    d->calculationSettings = new CalculationSettings();

    d->parser = new ValueParser(d->calculationSettings);
    d->converter = new ValueConverter(d->parser);
    d->calc = new ValueCalc(d->converter);
    d->formatter = new ValueFormatter(d->converter);

    d->defaultColumnFormat = new ColumnFormat();
    d->defaultRowFormat = new RowFormat();

    QFont font(KoGlobal::defaultFont());
    d->defaultRowFormat->setHeight(font.pointSizeF() + 4);
    d->defaultColumnFormat->setWidth((font.pointSizeF() + 4) * 5);

    d->isLoading = false;

    // default document properties
    d->syntaxVersion = syntaxVersion;

    connect(this, &Map::sheetAdded,
            d->dependencyManager, &DependencyManager::addSheet);
    connect(this, &Map::sheetAdded,
            d->recalcManager, &RecalcManager::addSheet);
    connect(this, &Map::sheetRemoved,
            d->dependencyManager, &DependencyManager::removeSheet);
    connect(this, &Map::sheetRemoved,
            d->recalcManager, &RecalcManager::removeSheet);
    connect(this, &Map::sheetRevived,
            d->dependencyManager, &DependencyManager::addSheet);
    connect(this, &Map::sheetRevived,
            d->recalcManager, &RecalcManager::addSheet);
    connect(d->namedAreaManager, &NamedAreaManager::namedAreaModified,
            d->dependencyManager, &DependencyManager::namedAreaModified);
    connect(this, &Map::damagesFlushed,
            this, &Map::handleDamages);
}

Map::~Map()
{
    // Because some of the shapes might be using a sheet in this map, delete
    // all shapes in each sheet before all sheets are deleted together.
    foreach(Sheet *sheet, d->lstSheets)
        sheet->deleteShapes();
    // we have to explicitly delete the Sheets, not let QObject take care of that
    // as the sheet in its destructor expects the Map to still exist
    qDeleteAll(d->lstSheets);
    d->lstSheets.clear();

    deleteLoadingInfo();

    delete d->bindingManager;
    delete d->databaseManager;
    delete d->dependencyManager;
    delete d->namedAreaManager;
    delete d->recalcManager;
    delete d->styleManager;

    delete d->parser;
    delete d->formatter;
    delete d->converter;
    delete d->calc;
    delete d->calculationSettings;
    delete d->applicationSettings;

    delete d->defaultColumnFormat;
    delete d->defaultRowFormat;

    delete d;
}

DocBase* Map::doc() const
{
    return d->doc;
}

void Map::setReadWrite(bool readwrite)
{
    d->readwrite = readwrite;
}

bool Map::isReadWrite() const
{
    return d->readwrite;
}

bool Map::completeLoading(KoStore *store)
{
    Q_UNUSED(store);

    QPointer<KoUpdater> dependencyUpdater, recalcUpdater;
    if (doc() && doc()->progressUpdater()) {
        dependencyUpdater = doc()->progressUpdater()->startSubtask(1, "Calligra::Sheets::DependencyManager::updateAllDependencies");
        recalcUpdater = doc()->progressUpdater()->startSubtask(1, "Calligra::Sheets::RecalcManager::recalc");
    }

    // Initial build of all cell dependencies.
    d->dependencyManager->updateAllDependencies(this, dependencyUpdater);
    // Recalc the whole workbook now, since there may be formulas other spreadsheets support,
    // but Calligra Sheets does not.
    d->recalcManager->recalcMap(recalcUpdater);

    return true;
}

bool Map::completeSaving(KoStore *store, KoXmlWriter *manifestWriter, KoShapeSavingContext * context)
{
    Q_UNUSED(store);
    Q_UNUSED(manifestWriter);
    Q_UNUSED(context);
    return true;
}

BindingManager* Map::bindingManager() const
{
    return d->bindingManager;
}

DatabaseManager* Map::databaseManager() const
{
    return d->databaseManager;
}

DependencyManager* Map::dependencyManager() const
{
    return d->dependencyManager;
}

NamedAreaManager* Map::namedAreaManager() const
{
    return d->namedAreaManager;
}

RecalcManager* Map::recalcManager() const
{
    return d->recalcManager;
}

StyleManager* Map::styleManager() const
{
    return d->styleManager;
}

KoStyleManager* Map::textStyleManager() const
{
    return d->textStyleManager;
}

ValueParser* Map::parser() const
{
    return d->parser;
}

ValueFormatter* Map::formatter() const
{
    return d->formatter;
}

ValueConverter* Map::converter() const
{
    return d->converter;
}

ValueCalc* Map::calc() const
{
    return d->calc;
}

const ColumnFormat* Map::defaultColumnFormat() const
{
    return d->defaultColumnFormat;
}

const RowFormat* Map::defaultRowFormat() const
{
    return d->defaultRowFormat;
}

void Map::setDefaultColumnWidth(double width)
{
    d->defaultColumnFormat->setWidth(width);
}

void Map::setDefaultRowHeight(double height)
{
    d->defaultRowFormat->setHeight(height);
}

ApplicationSettings* Map::settings() const
{
    return d->applicationSettings;
}

CalculationSettings* Map::calculationSettings() const
{
    return d->calculationSettings;
}

Sheet* Map::createSheet(const QString& name)
{
    QString sheetName(i18n("Sheet%1", d->tableId++));
    if ( !name.isEmpty() )
        sheetName = name;
    Sheet* sheet = new Sheet(this, sheetName);
    connect(sheet, &Sheet::statusMessage,
            this, &Map::statusMessage);
    return sheet;
}

void Map::addSheet(Sheet *_sheet)
{
    d->lstSheets.append(_sheet);
    emit sheetAdded(_sheet);
}

Sheet *Map::addNewSheet(const QString& name)
{
    Sheet *t = createSheet(name);
    addSheet(t);
    return t;
}

void Map::moveSheet(const QString & _from, const QString & _to, bool _before)
{
    Sheet* sheetfrom = findSheet(_from);
    Sheet* sheetto = findSheet(_to);

    int from = d->lstSheets.indexOf(sheetfrom) ;
    int to = d->lstSheets.indexOf(sheetto) ;
    if (!_before)
        ++to;

    if (to > (int)d->lstSheets.count()) {
        d->lstSheets.append(sheetfrom);
        d->lstSheets.removeAt(from);
    } else if (from < to) {
        d->lstSheets.insert(to, sheetfrom);
        d->lstSheets.removeAt(from);
    } else {
        d->lstSheets.removeAt(from);
        d->lstSheets.insert(to, sheetfrom);
    }
}

QDomElement Map::save(QDomDocument& doc)
{
    QDomElement spread = doc.documentElement();

    QDomElement locale = static_cast<Localization*>(d->calculationSettings->locale())->save(doc);
    spread.appendChild(locale);

    QDomElement areaname = d->namedAreaManager->saveXML(doc);
    spread.appendChild(areaname);

    QDomElement defaults = doc.createElement("defaults");
    defaults.setAttribute("row-height", QString::number(d->defaultRowFormat->height()));
    defaults.setAttribute("col-width", QString::number(d->defaultColumnFormat->width()));
    spread.appendChild(defaults);

    QDomElement s = d->styleManager->save(doc);
    spread.appendChild(s);

    QDomElement mymap = doc.createElement("map");

    QByteArray password;
    this->password(password);
    if (!password.isNull()) {
        if (password.size() > 0) {
            QByteArray str = KCodecs::base64Encode(password);
            mymap.setAttribute("protected", QString(str.data()));
        } else {
            mymap.setAttribute("protected", "");
        }
    }

    foreach(Sheet* sheet, d->lstSheets) {
        QDomElement e = sheet->saveXML(doc);
        if (e.isNull())
            return e;
        mymap.appendChild(e);
    }
    return mymap;
}

bool Map::loadXML(const KoXmlElement& mymap)
{
    d->isLoading = true;
    loadingInfo()->setFileFormat(LoadingInfo::NativeFormat);
    const QString activeSheet = mymap.attribute("activeTable");
    const QPoint marker(mymap.attribute("markerColumn").toInt(), mymap.attribute("markerRow").toInt());
    loadingInfo()->setCursorPosition(findSheet(activeSheet), marker);
    const QPointF offset(mymap.attribute("xOffset").toDouble(), mymap.attribute("yOffset").toDouble());
    loadingInfo()->setScrollingOffset(findSheet(activeSheet), offset);

    KoXmlNode n = mymap.firstChild();
    if (n.isNull()) {
        // We need at least one sheet !
        doc()->setErrorMessage(i18n("This document has no sheets (tables)."));
        d->isLoading = false;
        return false;
    }
    while (!n.isNull()) {
        KoXmlElement e = n.toElement();
        if (!e.isNull() && e.tagName() == "table") {
            Sheet *t = addNewSheet();
            if (!t->loadXML(e)) {
                d->isLoading = false;
                return false;
            }
        }
        n = n.nextSibling();
    }

    loadXmlProtection(mymap);

    if (!activeSheet.isEmpty()) {
        // Used by View's constructor
        loadingInfo()->setInitialActiveSheet(findSheet(activeSheet));
    }

    d->isLoading = false;
    return true;
}

Sheet* Map::findSheet(const QString & _name) const
{
    foreach(Sheet* sheet, d->lstSheets) {
        if (_name.toLower() == sheet->sheetName().toLower())
            return sheet;
    }
    return 0;
}

Sheet * Map::nextSheet(Sheet * currentSheet) const
{
    if (currentSheet == d->lstSheets.last())
        return currentSheet;
    int index = 0;
    foreach(Sheet* sheet, d->lstSheets) {
        if (sheet == currentSheet)
            return d->lstSheets.value(++index);
        ++index;
    }
    return 0;
}

Sheet * Map::previousSheet(Sheet * currentSheet) const
{
    if (currentSheet == d->lstSheets.first())
        return currentSheet;
    int index = 0;
    foreach(Sheet* sheet, d->lstSheets) {
        if (sheet  == currentSheet)
            return d->lstSheets.value(--index);
        ++index;
    }
    return 0;
}

bool Map::loadChildren(KoStore * _store)
{
    foreach(Sheet* sheet, d->lstSheets) {
        if (!sheet->loadChildren(_store))
            return false;
    }
    return true;
}

void Map::removeSheet(Sheet* sheet)
{
    d->lstSheets.removeAll(sheet);
    d->lstDeletedSheets.append(sheet);
    d->namedAreaManager->remove(sheet);
    emit sheetRemoved(sheet);
}

void Map::reviveSheet(Sheet* sheet)
{
    d->lstDeletedSheets.removeAll(sheet);
    d->lstSheets.append(sheet);
    emit sheetRevived(sheet);
}

// FIXME cache this for faster operation
QStringList Map::visibleSheets() const
{
    QStringList result;
    foreach(Sheet* sheet, d->lstSheets) {
        if (!sheet->isHidden())
            result.append(sheet->sheetName());
    }
    return result;
}

// FIXME cache this for faster operation
QStringList Map::hiddenSheets() const
{
    QStringList result;
    foreach(Sheet* sheet, d->lstSheets) {
        if (sheet->isHidden())
            result.append(sheet->sheetName());
    }
    return result;
}

Sheet* Map::sheet(int index) const
{
    return d->lstSheets.value(index);
}

int Map::indexOf(Sheet* sheet) const
{
    return d->lstSheets.indexOf(sheet);
}

QList<Sheet*>& Map::sheetList() const
{
    return d->lstSheets;
}

int Map::count() const
{
    return d->lstSheets.count();
}

void Map::setOverallRowsCounter(int number)
{
    d->overallRowCount = number;
}

int Map::increaseLoadedRowsCounter(int number)
{
    d->loadedRowsCounter += number;
    if (d->overallRowCount) {
        return 100 * d->loadedRowsCounter / d->overallRowCount;
    }
    return -1;
}

bool Map::isLoading() const
{
    // The KoDocument state is necessary to avoid damages while importing a file (through a filter).
    return d->isLoading || (d->doc && d->doc->isLoading());
}

void Map::setLoading(bool l) {
    d->isLoading = l;
}

int Map::syntaxVersion() const
{
    return d->syntaxVersion;
}

void Map::setSyntaxVersion(int version)
{
    d->syntaxVersion = version;
}

LoadingInfo* Map::loadingInfo() const
{
    if (!d->loadingInfo) {
        d->loadingInfo = new LoadingInfo();
    }
    return d->loadingInfo;
}

void Map::deleteLoadingInfo()
{
    delete d->loadingInfo;
    d->loadingInfo = 0;
}

KCompletion& Map::stringCompletion()
{
    return d->listCompletion;
}

void Map::addStringCompletion(const QString &stringCompletion)
{
    if (d->listCompletion.items().contains(stringCompletion) == 0) {
        d->listCompletion.addItem(stringCompletion);
    }
}

void Map::addDamage(Damage* damage)
{
    // Do not create a new Damage, if we are in loading process. Check for it before
    // calling this function. This prevents unnecessary memory allocations (new).
    // see FIXME in Sheet::setSheetName().
//     Q_ASSERT(!isLoading());
    Q_CHECK_PTR(damage);

#ifndef NDEBUG
    if (damage->type() == Damage::Cell) {
        debugSheetsDamage << "Adding\t" << *static_cast<CellDamage*>(damage);
    } else if (damage->type() == Damage::Sheet) {
        debugSheetsDamage << "Adding\t" << *static_cast<SheetDamage*>(damage);
    } else if (damage->type() == Damage::Selection) {
        debugSheetsDamage << "Adding\t" << *static_cast<SelectionDamage*>(damage);
    } else {
        debugSheetsDamage << "Adding\t" << *damage;
    }
#endif

    d->damages.append(damage);

    if (d->damages.count() == 1) {
        QTimer::singleShot(0, this, &Map::flushDamages);
    }
}

void Map::flushDamages()
{
    // Copy the damages to process. This allows new damages while processing.
    QList<Damage*> damages = d->damages;
    d->damages.clear();
    emit damagesFlushed(damages);
    qDeleteAll(damages);
}

void Map::handleDamages(const QList<Damage*>& damages)
{
    Region bindingChangedRegion;
    Region formulaChangedRegion;
    Region namedAreaChangedRegion;
    Region valueChangedRegion;
    WorkbookDamage::Changes workbookChanges = WorkbookDamage::None;

    QList<Damage*>::ConstIterator end(damages.end());
    for (QList<Damage*>::ConstIterator it = damages.begin(); it != end; ++it) {
        Damage* damage = *it;

        if (damage->type() == Damage::Cell) {
            CellDamage* cellDamage = static_cast<CellDamage*>(damage);
            debugSheetsDamage << "Processing\t" << *cellDamage;
            Sheet* const damagedSheet = cellDamage->sheet();
            const Region& region = cellDamage->region();
            const CellDamage::Changes changes = cellDamage->changes();

            // TODO Stefan: Detach the style cache from the CellView cache.
            if ((changes.testFlag(CellDamage::Appearance))) {
                // Rebuild the style storage cache.
                damagedSheet->cellStorage()->invalidateStyleCache(); // FIXME more fine-grained
            }
            if ((cellDamage->changes() & CellDamage::Binding) &&
                    !workbookChanges.testFlag(WorkbookDamage::Value)) {
                bindingChangedRegion.add(region, damagedSheet);
            }
            if ((cellDamage->changes() & CellDamage::Formula) &&
                    !workbookChanges.testFlag(WorkbookDamage::Formula)) {
                formulaChangedRegion.add(region, damagedSheet);
            }
            if ((cellDamage->changes() & CellDamage::NamedArea) &&
                    !workbookChanges.testFlag(WorkbookDamage::Formula)) {
                namedAreaChangedRegion.add(region, damagedSheet);
            }
            if ((cellDamage->changes() & CellDamage::Value) &&
                    !workbookChanges.testFlag(WorkbookDamage::Value)) {
                valueChangedRegion.add(region, damagedSheet);
            }
            continue;
        }

        if (damage->type() == Damage::Sheet) {
            SheetDamage* sheetDamage = static_cast<SheetDamage*>(damage);
            debugSheetsDamage << "Processing\t" << *sheetDamage;
//             Sheet* damagedSheet = sheetDamage->sheet();

            if (sheetDamage->changes() & SheetDamage::PropertiesChanged) {
            }
            continue;
        }

        if (damage->type() == Damage::Workbook) {
            WorkbookDamage* workbookDamage = static_cast<WorkbookDamage*>(damage);
            debugSheetsDamage << "Processing\t" << *damage;

            workbookChanges |= workbookDamage->changes();
            if (workbookDamage->changes() & WorkbookDamage::Formula) {
                formulaChangedRegion.clear();
            }
            if (workbookDamage->changes() & WorkbookDamage::Value) {
                valueChangedRegion.clear();
            }
            continue;
        }
//         debugSheetsDamage <<"Unhandled\t" << *damage;
    }

    // Update the named areas.
    if (!namedAreaChangedRegion.isEmpty()) {
        d->namedAreaManager->regionChanged(namedAreaChangedRegion);
    }
    // First, update the dependencies.
    if (!formulaChangedRegion.isEmpty()) {
        d->dependencyManager->regionChanged(formulaChangedRegion);
    }
    // Tell the RecalcManager which cells have had a value change.
    if (!valueChangedRegion.isEmpty()) {
        d->recalcManager->regionChanged(valueChangedRegion);
    }
    if (workbookChanges.testFlag(WorkbookDamage::Formula)) {
        d->namedAreaManager->updateAllNamedAreas();
        d->dependencyManager->updateAllDependencies(this);
    }
    if (workbookChanges.testFlag(WorkbookDamage::Value)) {
        d->recalcManager->recalcMap();
        d->bindingManager->updateAllBindings();
    }
    // Update the bindings
    if (!bindingChangedRegion.isEmpty()) {
        d->bindingManager->regionChanged(bindingChangedRegion);
    }
}

void Map::addCommand(KUndo2Command *command)
{
    emit commandAdded(command);
}

KoDocumentResourceManager* Map::resourceManager() const
{
    if (!doc()) return 0;
    return doc()->resourceManager();
}
