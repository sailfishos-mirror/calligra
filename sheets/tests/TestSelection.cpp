/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2009 Stefan Nikolaus <stefan.nikolaus@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "TestSelection.h"

#include <QTest>

#include "core/Map.h"
#include "core/Sheet.h"
#include "part/Canvas.h"
#include "ui/Selection.h"

using namespace Calligra::Sheets;

void TestSelection::initialize()
{
    Map map;
    Sheet *sheet1 = new Sheet(&map, "Sheet1");
    map.addSheet(sheet1);
    Sheet *sheet2 = new Sheet(&map, "Sheet2");
    map.addSheet(sheet2);
    Canvas canvas(nullptr);
    Selection selection(&canvas);
    selection.setActiveSheet(sheet1);

    QVERIFY(!selection.isEmpty());
    QCOMPARE(selection.name(), QString("A1"));

    // Clearing in normal selection mode, results in A1 as residuum.
    selection.clear();
    QVERIFY(!selection.isEmpty());
    QCOMPARE(selection.name(), QString("Sheet1!A1"));

    selection.initialize(QPoint(2, 4), sheet2);
    QCOMPARE(selection.name(), QString("Sheet2!B4"));

    selection.initialize(QRect(3, 5, 2, 3));
    QCOMPARE(selection.name(), QString("Sheet1!C5:D7"));

    Region region = map.regionFromName("A1:A4", sheet2);
    selection.initialize(region);
    QCOMPARE(selection.name(), QString("Sheet2!A1:A4"));

    region = map.regionFromName("Sheet1!A2:A4", sheet2);
    selection.initialize(region);
    QCOMPARE(selection.name(), QString("Sheet1!A2:A4"));

    // reference mode:
    selection.startReferenceSelection();

    region = map.regionFromName("Sheet1!A2:A4;B3;C5", sheet2);
    selection.initialize(region);
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!B3;Sheet2!C5"));
    QCOMPARE(selection.activeSubRegionName(), QString("A2:A4;Sheet2!B3;Sheet2!C5"));

    selection.setActiveSubRegion(1, 1);
    QCOMPARE(selection.activeSubRegionName(), QString("Sheet2!B3"));
    selection.initialize(QPoint(2, 2));
    QCOMPARE(selection.name(), QString("A2:A4;B2;Sheet2!C5"));
    QCOMPARE(selection.activeSubRegionName(), QString("B2"));

    selection.setActiveSubRegion(1, 2, 2);
    QCOMPARE(selection.activeSubRegionName(), QString("B2;Sheet2!C5"));
    selection.initialize(QPoint(3, 3), sheet2);
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!C3"));
    QCOMPARE(selection.activeSubRegionName(), QString("Sheet2!C3"));

    selection.setActiveSubRegion(1, 0, 2);
    QCOMPARE(selection.activeSubRegionName(), QString());
    selection.initialize(QPoint(4, 4), sheet2);
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!D4;Sheet2!C3"));
    QCOMPARE(selection.activeSubRegionName(), QString("Sheet2!D4"));

    selection.setActiveSubRegion(1, 1, 2);
    QCOMPARE(selection.activeSubRegionName(), QString("Sheet2!D4"));
    selection.clearSubRegion();
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!C3"));
    QCOMPARE(selection.activeSubRegionName(), QString());

    // Two appendings.
    selection.clear();
    selection.setActiveSubRegion(0, 0, 0);
    QCOMPARE(selection.activeSubRegionName(), QString());
    selection.initialize(QPoint(1, 1), sheet1);
    QCOMPARE(selection.activeSubRegionName(), QString("A1"));
    selection.setActiveSubRegion(1, 0, 1);
    QCOMPARE(selection.activeSubRegionName(), QString());
    selection.initialize(QPoint(2, 2), sheet1);
    QCOMPARE(selection.name(), QString("A1;B2"));
    QCOMPARE(selection.activeSubRegionName(), QString("B2"));
}

void TestSelection::update()
{
    Map map;
    Sheet *sheet1 = new Sheet(&map, "Sheet1");
    map.addSheet(sheet1);
    Sheet *sheet2 = new Sheet(&map, "Sheet2");
    map.addSheet(sheet2);
    Canvas canvas(nullptr);
    Selection selection(&canvas);
    selection.setActiveSheet(sheet1);

    QVERIFY(!selection.isEmpty());
    QCOMPARE(selection.name(), QString("A1"));

    // Selection::update(const QPoint&) works always on the active sheet.

    // normal mode:

    // init with location
    selection.initialize(QPoint(1, 6), sheet1);
    QCOMPARE(selection.name(), QString("Sheet1!A6"));

    // init with region
    selection.initialize(map.regionFromName("A3", sheet1));
    QCOMPARE(selection.name(), QString("Sheet1!A3"));

    // init with location after init with region
    selection.initialize(QPoint(1, 5), sheet1);
    QCOMPARE(selection.name(), QString("Sheet1!A5"));

    // TODO

    // reference mode:
    selection.startReferenceSelection();

    Region region = map.regionFromName("Sheet1!A2:A4;B3;C5", sheet2);
    selection.initialize(region);
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!B3;Sheet2!C5"));

    // Prepend new range.
    selection.setActiveSubRegion(0, 0);
    QCOMPARE(selection.activeSubRegionName(), QString());
    selection.update(QPoint(1, 1));
    QCOMPARE(selection.name(), QString("A1;A2:A4;Sheet2!B3;Sheet2!C5"));

    // Insert new range on active sheet.
    selection.setActiveSubRegion(2, 0);
    QCOMPARE(selection.activeSubRegionName(), QString());
    selection.update(QPoint(1, 1));
    QCOMPARE(selection.name(), QString("A1;A2:A4;A1;Sheet2!B3;Sheet2!C5"));

    // Append new range.
    selection.setActiveSubRegion(5, 0);
    QCOMPARE(selection.activeSubRegionName(), QString());
    selection.update(QPoint(1, 1));
    QCOMPARE(selection.name(), QString("A1;A2:A4;A1;Sheet2!B3;Sheet2!C5;A1"));

    // Update range.
    selection.setActiveSubRegion(2, 2);
    QCOMPARE(selection.activeSubRegionName(), QString("A1;Sheet2!B3"));
    selection.update(QPoint(2, 2));
    QCOMPARE(selection.name(), QString("A1;A2:A4;A1:B2;Sheet2!B3;Sheet2!C5;A1"));

    // Try to update range on non-active sheet. Inserts location on active sheet.
    selection.setActiveSubRegion(2, 2, 3);
    QCOMPARE(selection.activeSubRegionName(), QString("A1:B2;Sheet2!B3"));
    selection.update(QPoint(2, 2));
    QCOMPARE(selection.name(), QString("A1;A2:A4;A1:B2;Sheet2!B3;B2;Sheet2!C5;A1"));
}

void TestSelection::extend()
{
    Map map;
    Sheet *sheet1 = new Sheet(&map, "Sheet1");
    map.addSheet(sheet1);
    Sheet *sheet2 = new Sheet(&map, "Sheet2");
    map.addSheet(sheet2);
    Canvas canvas(nullptr);
    Selection selection(&canvas);
    selection.setActiveSheet(sheet1);

    QVERIFY(!selection.isEmpty());
    QCOMPARE(selection.name(), QString("A1"));

    // normal mode:
    // TODO

    // reference mode:
    selection.startReferenceSelection();

    Region region = map.regionFromName("Sheet1!A2:A4;B3;C5", sheet2);
    selection.initialize(region);
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!B3;Sheet2!C5"));

    // Prepend new location.
    selection.setActiveSubRegion(0, 0);
    QCOMPARE(selection.activeSubRegionName(), QString());
    selection.extend(QPoint(1, 1), sheet2);
    QCOMPARE(selection.name(), QString("Sheet2!A1;A2:A4;Sheet2!B3;Sheet2!C5"));
    QCOMPARE(selection.activeSubRegionName(), QString("Sheet2!A1"));

    // Try to prepend new location. Prepending needs length = 0.
    selection.setActiveSubRegion(0, 2); //  results in: 0, 2, 0
    QCOMPARE(selection.activeSubRegionName(), QString("Sheet2!A1;A2:A4"));
    selection.extend(QPoint(1, 1), sheet1);
    QCOMPARE(selection.name(), QString("Sheet2!A1;A1;A2:A4;Sheet2!B3;Sheet2!C5"));
    QCOMPARE(selection.activeSubRegionName(), QString("Sheet2!A1;A1;A2:A4"));

    // Prepend new range.
    selection.setActiveSubRegion(0, 0);
    QCOMPARE(selection.activeSubRegionName(), QString());
    selection.extend(QRect(1, 1, 2, 2), sheet1);
    QCOMPARE(selection.name(), QString("A1:B2;Sheet2!A1;A1;A2:A4;Sheet2!B3;Sheet2!C5"));
    QCOMPARE(selection.activeSubRegionName(), QString("A1:B2"));

    // Try to prepend new range. Prepending needs length = 0.
    selection.setActiveSubRegion(0, 3); //  results in: 0, 3, 0
    QCOMPARE(selection.activeSubRegionName(), QString("A1:B2;Sheet2!A1;A1"));
    selection.extend(QRect(1, 2, 2, 2), sheet2);
    QCOMPARE(selection.name(), QString("A1:B2;Sheet2!A2:B3;Sheet2!A1;A1;A2:A4;Sheet2!B3;Sheet2!C5"));
    QCOMPARE(selection.activeSubRegionName(), QString("A1:B2;Sheet2!A2:B3;Sheet2!A1;A1"));

    selection.clear();
    QVERIFY(selection.isEmpty());
    selection.initialize(region);
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!B3;Sheet2!C5"));

    // Append new location.
    selection.setActiveSubRegion(1, 3, 3); // results in: 1, 2, 3
    QCOMPARE(selection.activeSubRegionName(), QString("Sheet2!B3;Sheet2!C5"));
    selection.extend(QPoint(1, 1), sheet2);
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!B3;Sheet2!C5;Sheet2!A1"));
    QCOMPARE(selection.activeSubRegionName(), QString("Sheet2!B3;Sheet2!C5;Sheet2!A1"));

    // Append new location.
    selection.setActiveSubRegion(4, 1, 3); // results in: 4, 0, 4
    QCOMPARE(selection.activeSubRegionName(), QString());
    selection.extend(QPoint(1, 1), sheet1);
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!B3;Sheet2!C5;Sheet2!A1;A1"));
    QCOMPARE(selection.activeSubRegionName(), QString("A1"));

    // Append new range.
    selection.setActiveSubRegion(5, 0); // results in: 5, 0, 5
    QCOMPARE(selection.activeSubRegionName(), QString());
    selection.extend(QRect(1, 1, 2, 2), sheet1);
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!B3;Sheet2!C5;Sheet2!A1;A1;A1:B2"));
    QCOMPARE(selection.activeSubRegionName(), QString("A1:B2"));

    // Append new range.
    selection.setActiveSubRegion(5, 1, 6);
    QCOMPARE(selection.activeSubRegionName(), QString("A1:B2"));
    selection.extend(QRect(1, 2, 2, 2), sheet2);
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!B3;Sheet2!C5;Sheet2!A1;A1;A1:B2;Sheet2!A2:B3"));
    QCOMPARE(selection.activeSubRegionName(), QString("A1:B2;Sheet2!A2:B3"));

    selection.clear();
    QVERIFY(selection.isEmpty());
    selection.initialize(region);
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!B3;Sheet2!C5"));

    // Insert new location.
    selection.setActiveSubRegion(0, 3, 1);
    QCOMPARE(selection.activeSubRegionName(), QString("A2:A4;Sheet2!B3;Sheet2!C5"));
    selection.extend(QPoint(1, 1), sheet2);
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!B3;Sheet2!A1;Sheet2!C5"));
    QCOMPARE(selection.activeSubRegionName(), QString("A2:A4;Sheet2!B3;Sheet2!A1;Sheet2!C5"));

    // Insert new range.
    selection.setActiveSubRegion(1, 3, 3);
    QCOMPARE(selection.activeSubRegionName(), QString("Sheet2!B3;Sheet2!A1;Sheet2!C5"));
    selection.extend(QRect(1, 1, 2, 2), sheet1);
    QCOMPARE(selection.name(), QString("A2:A4;Sheet2!B3;Sheet2!A1;Sheet2!C5;A1:B2"));
    QCOMPARE(selection.activeSubRegionName(), QString("Sheet2!B3;Sheet2!A1;Sheet2!C5;A1:B2"));
}

void TestSelection::activeElement()
{
    Map map;
    Sheet *sheet1 = new Sheet(&map, "Sheet1");
    map.addSheet(sheet1);
    Sheet *sheet2 = new Sheet(&map, "Sheet2");
    map.addSheet(sheet2);
    Canvas canvas(nullptr);
    Selection selection(&canvas);
    selection.setActiveSheet(sheet1);

    QVERIFY(!selection.isEmpty());
    QCOMPARE(selection.name(), QString("A1"));
    QVERIFY(!selection.activeElement());

    // normal mode:

    // BEGIN Leave the active element unset.
    selection.startReferenceSelection();
    QVERIFY(!selection.activeElement());

    selection.initialize(map.regionFromName("A3:A4;B2;C4:D5", sheet1));
    QCOMPARE(selection.name(sheet1), QString("A3:A4;B2;C4:D5"));
    QVERIFY(!selection.activeElement());

    selection.extend(QRect(1, 1, 4, 4), sheet1);
    QCOMPARE(selection.name(sheet1), QString("A3:A4;B2;C4:D5;A1:D4"));
    QVERIFY(!selection.activeElement());

    selection.update(QPoint(5, 5));
    QCOMPARE(selection.name(sheet1), QString("A3:A4;B2;C4:D5;A1:E5"));
    QVERIFY(!selection.activeElement());

    selection.initialize(QPoint(5, 6), sheet1);
    QCOMPARE(selection.name(sheet1), QString("E6"));
    QVERIFY(!selection.activeElement());

    selection.initialize(QRect(1, 1, 2, 1), sheet1);
    QCOMPARE(selection.name(sheet1), QString("A1:B1"));
    QVERIFY(!selection.activeElement());

    selection.extend(QPoint(1, 3), sheet1);
    QCOMPARE(selection.name(sheet1), QString("A1:B1;A3"));
    QVERIFY(!selection.activeElement());
    // END Leave the active element unset.
}

void TestSelection::referenceSelectionMode()
{
    Map map;
    Sheet *sheet1 = new Sheet(&map, "Sheet1");
    map.addSheet(sheet1);
    Sheet *sheet2 = new Sheet(&map, "Sheet2");
    map.addSheet(sheet2);
    Canvas canvas(nullptr);
    Selection selection(&canvas);
    selection.setActiveSheet(sheet1);

    QVERIFY(!selection.isEmpty());
    QCOMPARE(selection.name(), QString("A1"));

    // reference mode:
    selection.startReferenceSelection();
    QVERIFY(selection.isEmpty());

    selection.initialize(QPoint(2, 2), sheet2);
    QCOMPARE(selection.name(), QString("Sheet2!B2"));

    selection.extend(QPoint(1, 2));
    QCOMPARE(selection.name(), QString("Sheet2!B2;A2"));

    selection.setActiveSubRegion(1, 1, 1);
    QCOMPARE(selection.activeSubRegionName(), QString("A2"));

    selection.endReferenceSelection();
    QCOMPARE(selection.name(), QString("A1"));

    selection.initialize(QPoint(2, 2), sheet1);
    QCOMPARE(selection.name(), QString("B2"));

    // quit it again
    selection.endReferenceSelection();
    QCOMPARE(selection.name(), QString("B2"));

    selection.initialize(QRect(2, 2, 2, 2));
    QCOMPARE(selection.name(), QString("B2:C3"));

    // start it again
    selection.startReferenceSelection();
    QVERIFY(selection.isEmpty());

    selection.initialize(map.regionFromName("A3", sheet1));
    QCOMPARE(selection.name(), QString("A3"));

    // set the active sub-region beyond the last range
    selection.setActiveSubRegion(1, 0, 1);
    QCOMPARE(selection.activeSubRegionName(), QString());

    // quit it again
    selection.endReferenceSelection();
    QCOMPARE(selection.name(), QString("B2:C3"));
}

void TestSelection::covering()
{
    Map map;
    Sheet *sheet1 = new Sheet(&map, "Sheet1");
    map.addSheet(sheet1);
    Sheet *sheet2 = new Sheet(&map, "Sheet2");
    map.addSheet(sheet2);
    Canvas canvas(nullptr);
    Selection selection(&canvas);
    selection.setActiveSheet(sheet1);

    QVERIFY(!selection.isEmpty());
    QCOMPARE(selection.name(), QString("A1"));

    // normal mode:
    // covered ranges get removed

    selection.initialize(map.regionFromName("A3:A4;B2;C4:D5", sheet1));
    QCOMPARE(selection.name(sheet1), QString("A3:A4;B2;C4:D5"));

    selection.extend(QRect(1, 1, 4, 4), sheet1);
    QCOMPARE(selection.name(sheet1), QString("C4:D5;A1:D4"));

    selection.update(QPoint(5, 5));
    QCOMPARE(selection.name(sheet1), QString("A1:E5"));

    // reference mode:
    // covered ranges get ignored

    selection.startReferenceSelection();

    selection.initialize(map.regionFromName("A3:A4;B2;C4:D5", sheet1));
    QCOMPARE(selection.name(), QString("A3:A4;B2;C4:D5"));

    selection.extend(QRect(1, 1, 4, 4), sheet1);
    QCOMPARE(selection.name(), QString("A3:A4;B2;C4:D5;A1:D4"));

    selection.update(QPoint(5, 5));
    QCOMPARE(selection.name(sheet1), QString("A3:A4;B2;C4:D5;A1:E5"));
}

void TestSelection::splitting()
{
    Map map;
    Sheet *sheet1 = new Sheet(&map, "Sheet1");
    map.addSheet(sheet1);
    Sheet *sheet2 = new Sheet(&map, "Sheet2");
    map.addSheet(sheet2);
    Canvas canvas(nullptr);
    Selection selection(&canvas);
    selection.setActiveSheet(sheet1);

    QVERIFY(!selection.isEmpty());
    QCOMPARE(selection.name(), QString("A1"));

    // normal mode:
    // ranges get split

    selection.initialize(map.regionFromName("A1:D5", sheet1));
    QCOMPARE(selection.name(sheet1), QString("A1:D5"));

    selection.extend(QPoint(2, 2), sheet1);
    QCOMPARE(selection.name(sheet1), QString("A3:D5;C2:D2;A2;A1:D1"));

    // reference mode:
    // ranges are not affected

    selection.startReferenceSelection();

    selection.initialize(map.regionFromName("A1:D5", sheet1));
    QCOMPARE(selection.name(), QString("A1:D5"));

    selection.extend(QPoint(2, 2), sheet1);
    QCOMPARE(selection.name(), QString("A1:D5;B2"));
}

QTEST_MAIN(TestSelection)
