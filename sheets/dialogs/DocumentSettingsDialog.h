/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2002-2004 Ariya Hidayat <ariya@kde.org>
             SPDX-FileCopyrightText: 2002-2003 Norbert Andres <nandres@web.de>
             SPDX-FileCopyrightText: 2000-2003 Laurent Montel <montel@kde.org>
             SPDX-FileCopyrightText: 2002 John Dailey <dailey@vt.edu>
             SPDX-FileCopyrightText: 2002 Philipp Mueller <philipp.mueller@gmx.de>
             SPDX-FileCopyrightText: 2001-2002 David Faure <faure@kde.org>
             SPDX-FileCopyrightText: 2001 Werner Trobin <trobin@kde.org>
             SPDX-FileCopyrightText: 2000 Bernd Johannes Wuebben <wuebben@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CALLIGRA_SHEETS_DOCUMENT_SETTINGS_DIALOG
#define CALLIGRA_SHEETS_DOCUMENT_SETTINGS_DIALOG

#include <QGroupBox>
#include <QLabel>

#include <kpagedialog.h>

class KoVBox;
class KLocale;
class QCheckBox;
class QSpinBox;
class QComboBox;
class QPushButton;

namespace Sonnet
{
}

namespace Calligra
{
namespace Sheets
{
class Selection;
class CalculationSettings;

/**
 * \ingroup UI
 * Dialog page to show the calculation settings used by the document.
 */
class calcSettings :  public QObject
{
    Q_OBJECT
public:
    calcSettings(Selection* selection, KoVBox *box);
    void apply();
protected:
    CalculationSettings *m_cs;
    QCheckBox *m_caseSensitiveCheckbox, *m_precisionAsShownCheckbox, *m_searchCriteriaMustApplyToWholeCellCheckbox, *m_automaticFindLabelsCheckbox;
    QComboBox *m_matchModeCombobox;
    QSpinBox *m_nullYearEdit;
};

/**
 * \ingroup UI
 * Dialog page to show the current locale used by the document.
 */
class parameterLocale :  public QObject
{
    Q_OBJECT
public:
    parameterLocale(Selection* selection, KoVBox *box);
    void apply();
public Q_SLOTS:
    void updateDefaultSystemConfig();
protected:
    /**
     * Updates the GUI widgets to match the specified locale.
     */
    void updateToMatchLocale(KLocale* locale);

    QLabel *m_shortDate, *m_time, *m_money, *m_date, *m_language, *m_number;
    QPushButton *m_updateButton;
    Selection* m_selection;
    bool m_bUpdateLocale;
};


/**
 * \ingroup UI
 * Dialog to alter document settings.
 */
class DocumentSettingsDialog : public KPageDialog
{
    Q_OBJECT
public:
    explicit DocumentSettingsDialog(Selection* selection, QWidget* parent);
    ~DocumentSettingsDialog() override;

public Q_SLOTS:
    void slotApply();
    void slotDefault();
    void slotReset();

private:
    class Private;
    Private * const d;
};

} // namespace Sheets
} // namespace Calligra

#endif // CALLIGRA_SHEETS_DOCUMENT_SETTINGS_DIALOG
