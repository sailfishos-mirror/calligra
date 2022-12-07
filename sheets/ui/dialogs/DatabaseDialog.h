/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2002-2003 Norbert Andres <nandres@web.de>
             SPDX-FileCopyrightText: 2002 Ariya Hidayat <ariya@kde.org>
             SPDX-FileCopyrightText: 2002 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CALLIGRA_SHEETS_DATABASE_DIALOG
#define CALLIGRA_SHEETS_DATABASE_DIALOG

#ifndef QT_NO_SQL

// #include <QRect>
// #include <QFrame>
// #include <QLabel>

#include <QSqlDatabase>
#include <kassistantdialog.h>

class QCheckBox;
class QFrame;
class QLabel;
class QTreeWidget;
class QListWidget;
class QListWidgetItem;
class QRadioButton;
class QPushButton;

class KLineEdit;
class KComboBox;
class KTextEdit;
class KPageWidgetItem;

namespace Calligra
{
namespace Sheets
{
class Selection;

/**
 * \ingroup UI
 * Dialog to import data from a database.
 */
class DatabaseDialog : public KAssistantDialog
{
    Q_OBJECT

public:
    enum PageId { eDatabase = 0, eTables = 1, eColumns = 2, eOptions = 3, eResult = 4 };

    DatabaseDialog(QWidget* parent, Selection* selection);
    ~DatabaseDialog() override;

private Q_SLOTS:
    void orBox_clicked();
    void andBox_clicked();
    void startingCell_clicked();
    void startingRegion_clicked();
    void connectButton_clicked();
    void databaseNameChanged(const QString & s);
    void databaseHostChanged(const QString & s);
    void databaseDriverChanged(int);
    //void popupTableViewMenu( QListWidgetItem *, const QPoint &, int );
    void tableViewClicked(QListWidgetItem *);
    void accept() override;

protected:
    void next() override;
    void back() override;

private:
    int            m_currentPage;
    Selection    * m_selection;
    QRect          m_targetRect;
    QSqlDatabase   m_dbConnection;

    KPageWidgetItem * m_database;
    QLabel       * m_databaseStatus;
    KLineEdit    * m_username;
    KLineEdit    * m_port;
    KLineEdit    * m_databaseName;
    KComboBox    * m_driver;
    KLineEdit    * m_password;
    KLineEdit    * m_host;
    QLabel       * m_Type;
    KPageWidgetItem * m_table;
    KComboBox    * m_databaseList;
    QPushButton  * m_connectButton;
    QLabel       * m_tableStatus;
    QLabel       * m_SelectTableLabel;
    QListWidget  * m_tableView;
    KPageWidgetItem * m_columns;
    QTreeWidget  * m_columnView;
    QLabel       * m_columnsStatus;
    KPageWidgetItem * m_options;
    KComboBox    * m_columns_1;
    KComboBox    * m_columns_2;
    KComboBox    * m_columns_3;
    KComboBox    * m_operator_1;
    KComboBox    * m_operator_2;
    KComboBox    * m_operator_3;
    KLineEdit    * m_operatorValue_1;
    KLineEdit    * m_operatorValue_2;
    KLineEdit    * m_operatorValue_3;
    QRadioButton * m_andBox;
    QRadioButton * m_orBox;
    KComboBox    * m_columnsSort_1;
    KComboBox    * m_columnsSort_2;
    KComboBox    * m_sortMode_1;
    KComboBox    * m_sortMode_2;
    QCheckBox    * m_distinct;
    KPageWidgetItem * m_result;
    KTextEdit    * m_sqlQuery;
    QRadioButton * m_startingRegion;
    KLineEdit    * m_cell;
    KLineEdit    * m_region;
    QRadioButton * m_startingCell;

    void switchPage(int id);
    bool databaseDoNext();
    bool tablesDoNext();
    bool columnsDoNext();
    bool optionsDoNext();

    QString exchangeWildcards(QString const & value);
    QString getWhereCondition(QString const &, QString const &, int);
};

} // namespace Sheets
} // namespace Calligra

#endif // QT_NO_SQL

#endif // CALLIGRA_SHEETS_DATABASE_DIALOG