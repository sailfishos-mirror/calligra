/* Swinder - Portable library for spreadsheet
   SPDX-FileCopyrightText: 2003-2005 Ariya Hidayat <ariya@kde.org>
   SPDX-FileCopyrightText: 2009, 2010 Sebastian Sauer <sebsauer@kdab.com>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef SWINDER_WORKBOOK_H
#define SWINDER_WORKBOOK_H

#include <QObject>
#include <QRect>
#include <QVariant>
#include <map>
#include <string>

class KoStore;

namespace MSO
{
class OfficeArtDggContainer;
}

namespace Swinder
{

class Sheet;
class Format;
class FormatFont;

class Workbook : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructs a new workbook.
     *
     * @a store An optional implementation of the Store class
     * that is used to write content like images to.
     */
    explicit Workbook(KoStore *store = nullptr);

    /**
     * Destroys the workbook.
     */
    ~Workbook() override;

    /**
     * Returns the used KoStore or NULL if not KoStore was set.
    /*/
    KoStore *store() const;

    /**
     * Clears the workbook, i.e. makes it as if it is just constructed.
     */
    void clear();

    /**
     * Loads the workbook from file. Returns false if error occurred.
     */
    bool load(const char *filename);

    /**
     * Appends a new sheet.
     */
    void appendSheet(Sheet *sheet);

    /**
     * Returns the number of worksheet in this workbook. A newly created
     * workbook has no sheet, i.e. sheetCount() returns 0.
     */
    unsigned sheetCount() const;

    /**
     * Returns a worksheet at given index. If index is invalid (e.g. larger
     * than total number of worksheet), this function returns NULL.
     */
    Sheet *sheet(unsigned index);

    enum PropertyType {
        PIDSI_CODEPAGE = 0x01,
        PIDSI_TITLE = 0x02,
        PIDSI_SUBJECT = 0x03,
        PIDSI_AUTHOR = 0x04,
        PIDSI_KEYWORDS = 0x05,
        PIDSI_COMMENTS = 0x06,
        PIDSI_TEMPLATE = 0x07,
        PIDSI_LASTAUTHOR = 0x08,
        PIDSI_REVNUMBER = 0x09,
        PIDSI_EDITTIME = 0x0a,
        PIDSI_LASTPRINTED_DTM = 0x0b,
        PIDSI_CREATE_DTM = 0x0c,
        PIDSI_LASTSAVED_DTM = 0x0d,
        PIDSI_APPNAME = 0x12
    };

    bool hasProperty(PropertyType type) const;
    QVariant property(PropertyType type, const QVariant &defaultValue = QVariant()) const;
    void setProperty(PropertyType type, const QVariant &value);

    std::map<std::pair<unsigned, QString>, QString> &namedAreas();
    void setNamedArea(unsigned sheet, const QString &name, const QString &formula);

    QList<QRect> filterRanges(unsigned sheet) const;
    QList<QRect> filterRanges(const Sheet *sheet) const;
    void addFilterRange(unsigned sheet, const QRect &range);
    void addFilterRange(const QString &sheet, const QRect &range);

    int activeTab() const;
    void setActiveTab(int tab);

    bool isPasswordProtected() const;
    void setPasswordProtected(bool p);

    unsigned long password() const;
    void setPassword(unsigned long hash);

    int addFormat(const Format &format);
    Format *format(int index) const;
    int formatCount() const;

    void setOfficeArtDggContainer(const MSO::OfficeArtDggContainer &dggContainer);
    MSO::OfficeArtDggContainer *officeArtDggContainer() const;

    void setPictureNames(const QMap<QByteArray, QString> pictureNames);
    QString pictureName(const QByteArray &uid) const;

    void setColorTable(const QList<QColor> &colorTable);
    QList<QColor> colorTable() const;
    QColor customColor(unsigned index) const;
    QColor color(unsigned index) const;

    FormatFont font(unsigned index) const;
    void setFont(unsigned index, const FormatFont &font);

    QDateTime baseDate() const;
    void setBaseDate(const QDateTime &baseDate);

    enum Version {
        Excel95,
        Excel97,
        Excel2000,
        Excel2002,
        Excel2003,
        Excel2007,
        Excel2010,
        Unknown = -1
    };
    Version version() const;
    void setVersion(Version ver);

    void emitProgress(int value);

#ifdef SWINDER_XLS2RAW
    void dumpStats();
#endif
Q_SIGNALS:
    void sigProgress(int value);

private:
    // no copy or assign
    Workbook(const Workbook &);
    Workbook &operator=(const Workbook &);

    class Private;
    Private *d;
};

}

#endif // SWINDER_WORKBOOK_H
