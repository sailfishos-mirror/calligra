/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003 Ariya Hidayat <ariya@kde.org>
   SPDX-FileCopyrightText: 2003 Norbert Andres <nandres@web.de>
   SPDX-FileCopyrightText: 2002 Laurent Montel <montel@kde.org>
   SPDX-FileCopyrightText: 1999 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 1999 Boris Wedl <boris.wedl@kfunigraz.ac.at>
   SPDX-FileCopyrightText: 1998-2000 Torben Weis <weis@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TABBAR_H
#define TABBAR_H

#include <QWidget>

namespace Calligra
{
namespace Sheets
{
class TabBarPrivate;

/**
 * The TabBar class provides a tab bar, for use to switch active
 * page/sheet in a document.
 *
 * The tab bar is typically used in the main view.
 * It is the small widget on the bottom left corner.
 * Pages/sheets are displayed as tabs, clicking on
 * one of the tab will activate the corresponding sheet (this is actually
 * done in main view). Current active page/sheet is marked by bold text.
 *
 * The tab bar supports page/sheet reorder by dragging a certain tab
 * and move it to another location. The main view should handle the necessary
 * action to perform the actual reorder.
 *
 * There are four scroll buttons available in the tab bar. They are used
 * to shift the tabs in left and right direction, for example when there
 * are more tabs than the space available to display all tabs.
 *
 * Since a page/sheet can be hidden, the tab bar only shows the visible page/sheet.
 * When a hidden page or sheet is shown again, it will be on the same position as
 * before it was hidden.
 *
 * When the document is protected, it is necessary to set the tab bar as
 * read-only using setReadOnly (see also readOnly). If it is set to true,
 * tabs can not be moved by dragging and context menu will not be displayed.
 *
 * @short A bar with tabs and scroll buttons.
 */
class TabBar : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString activeTab READ activeTab WRITE setActiveTab)
    Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly)
    Q_PROPERTY(QStringList tabs READ tabs WRITE setTabs)
    Q_PROPERTY(unsigned count READ count)

public:
    /**
     * Creates a new tabbar.
     */
    explicit TabBar(QWidget *parent = nullptr, const char *name = nullptr);

    /**
     * Destroy the tabbar.
     */
    ~TabBar() override;

    /**
     * Returns true if the tab bar is read only.
     */
    bool readOnly() const;

    /**
     * Returns all the tab as list of strings.
     */
    QStringList tabs() const;

    /**
     * Returns number of tabs.
     * This is the same as TabBar::tabs().count()
     */
    unsigned count() const;

    /**
     * Returns the active tab.
     */
    QString activeTab() const;

    /**
     * Returns true if it is possible to scroll one tab back.
     *
     * \sa scrollBack
     */
    bool canScrollBack() const;

    /**
     * Returns true if it is possible to scroll one tab forward.
     *
     * \sa scrollForward
     */
    bool canScrollForward() const;

    /**
     * Ensures that specified tab is visible.
     */
    void ensureVisible(const QString &tab);

public Q_SLOTS:

    /**
     * Replaces all tabs with the list of strings.
     */
    void setTabs(const QStringList &list);

    /**
     * Sets the tab bar to be read only.
     *
     * If the tab bar is read only, tab reordering is not allowed.
     * This means that signal tabMoved, contextMenu and doubleClicked
     * would not be emitted.
     */
    void setReadOnly(bool ro);

    /**
     * Adds a tab to the tab bar.
     */
    void addTab(const QString &text);

    /**
     * Removes a tab from the bar. If the tab was the active one then
     * no tab will be active.
     * It is recommended to call setActiveTab after a call to this function.
     */
    void removeTab(const QString &text);

    /**
     * Renames a tab.
     */
    void renameTab(const QString &old_name, const QString &new_name);

    /**
     * Moves a tab to another position and reorder other tabs.
     *
     * Example 1: if there are four tabs A - B - C - D, then
     * moveTab(2,1) will yield A - C - B - D. This is because
     * 2nd tab (i.e C) is moved to a position before 1th tab (i.e B).
     *
     * Example 2: for these tabs: X - Y - Z, moveTab(0,3) will
     * move tab X after tab Z so that the result is Y - Z - X.
     */
    void moveTab(int tab, int target);

    /**
     * Scrolls one tab back. Does nothing if the leftmost tab (rightmost tab
     * for right-to-left layouts) is already the first tab.
     *
     * \sa canScrollBack
     */
    void scrollBack();

    /**
     * Scrolls one tab forward. Does nothing if the rightmost tab (leftmost tab
     * for right-to-left layouts) is already the last tab.
     *
     * \sa canScrollForward
     */
    void scrollForward();

    /**
     * Scrolls to the first tab. Does nothing if the leftmost tab (rightmost tab
     * for right-to-left layouts) is already the first tab.
     *
     * \sa canScrollBack
     */
    void scrollFirst();

    /**
     * Scrolls to the last tab. Does nothing if the rightmost tab (leftmost tab
     * for right-to-left layouts) is already the last tab.
     *
     * \sa canScrollForward
     */
    void scrollLast();

    /**
     * Sets active tab.
     */
    void setActiveTab(const QString &text);

    /**
     * Removes all tabs.
     */
    void clear();

    QSize sizeHint() const override;

Q_SIGNALS:

    /**
     * Emitted if the active tab changed.
     */
    void tabChanged(const QString &_text);

    /**
     * This signal is emitted whenever a tab is dragged, moved and
     * released. This means that the user wants to move a tab into
     * another position (right before target).
     *
     * When the signal is emitted, the tabs are not reordered.
     * Therefore if you just ignore this signal, than no tab reorder
     * is possible. Call moveTab (from the slot connected to this signal)
     * to perform the actual tab reorder.
     */
    void tabMoved(unsigned tab, unsigned target);

    /**
     * This signal is emitted whenever the tab bar is right-clicked.
     * Typically it is used to popup a context menu.
     */
    void contextMenu(const QPoint &pos);

    /**
     * This signal is emitted whenever the tab bar is double-clicked.
     */
    void doubleClicked();

protected Q_SLOTS:
    void autoScrollBack();
    void autoScrollForward();

protected:
    void paintEvent(QPaintEvent *ev) override;
    void resizeEvent(QResizeEvent *ev) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;
    void mouseDoubleClickEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void wheelEvent(QWheelEvent *e) override;

private:
    TabBarPrivate *const d;

    // don't allow copy or assignment
    TabBar(const TabBar &);
    TabBar &operator=(const TabBar &);
};

} // namespace Sheets
} // namespace Calligra

#endif // TABBAR_H
