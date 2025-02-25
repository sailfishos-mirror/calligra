/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2001, 2002, 2003 Daniel Naber <daniel.naber@t-online.de>
 * SPDX-FileCopyrightText: 2007 Fredy Yanardi <fyanardi@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef THESAURUS_H
#define THESAURUS_H

#include <KoTextEditingPlugin.h>

#include <QUrl>

class QLineEdit;
class QPushButton;
class KHistoryComboBox;
class KProcess;
class KoDialog;

class QToolButton;
class QTextDocument;
class QTabWidget;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QComboBox;
class QTextBrowser;
class QTextDocument;

class Thesaurus : public KoTextEditingPlugin
{
    Q_OBJECT

public:
    Thesaurus();
    ~Thesaurus() override;

    void finishedWord(QTextDocument *document, int cursorPosition) override;
    void finishedParagraph(QTextDocument *document, int cursorPosition) override;
    void startingSimpleEdit(QTextDocument *document, int cursorPosition) override;

    void checkSection(QTextDocument *document, int startPosition, int endPosition) override;

private Q_SLOTS:
    void process();
    void dialogClosed();

    void slotChangeLanguage();

    void slotFindTerm();
    void slotFindTerm(const QString &term, bool add_to_history = true);
    void slotFindTermFromList(QListWidgetItem *item);
    void slotFindTermFromUrl(const QUrl &url);

    void slotGotoHistory(int index);

    void slotSetReplaceTermSyn(QListWidgetItem *item);
    void slotSetReplaceTermHypo(QListWidgetItem *item);
    void slotSetReplaceTermHyper(QListWidgetItem *item);

    void slotBack();
    void slotForward();

private:
    void findTermThesaurus(const QString &term);
    void findTermWordnet(const QString &term);
    QString formatLine(const QString &line) const;

    void setCaption();
    void updateNavButtons();

    enum Mode {
        grep,
        other
    };

    bool m_standAlone;
    int m_historyPos;
    int m_startPosition;
    Mode m_mode;

    KProcess *m_thesProc;
    KProcess *m_wnProc;
    KoDialog *m_dialog;
    KHistoryComboBox *m_edit;
    QPushButton *m_search;
    QLineEdit *m_replaceLineEdit;

    QString m_word;
    QString m_noMatch;
    QString m_dataFile;
    QToolButton *m_back;
    QToolButton *m_forward;
    QTabWidget *m_tabWidget;
    QLabel *m_replaceLabel;
    QTextDocument *m_document;

    // Thesaurus:
    QListWidget *m_synListWidget;
    QListWidget *m_hyperListWidget;
    QListWidget *m_hypoListWidget;

    // WordNet:
    QTextBrowser *m_resultTextBrowser;
    QComboBox *m_wnComboBox;
};

#endif
