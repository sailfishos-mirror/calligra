/* This file is part of the KDE project
   Copyright (C)  2001 Montel Laurent <lmontel@mandrakesoft.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __kwconfig__
#define __kwconfig__

#include <kdialogbase.h>

namespace KFormula {
    class ConfigurePage;
}

class KWView;
class QCheckBox;
class KIntNumInput;
class KDoubleNumInput;
class KSpellConfig;
class KConfig;
class QComboBox;
class KIntNumInput;
class KCommand;
class KWDocument;
class ConfigureSpellPage : public QObject
{
    Q_OBJECT
public:
    ConfigureSpellPage( KWView *_view, QVBox *box, char *name = 0 );
    void apply();
    void slotDefault();
public slots:
    void slotClearIgnoreAllHistory();
private:
    KWView* m_pView;
    KSpellConfig *_spellConfig;
    QCheckBox *_dontCheckUpperWord;
    QCheckBox *_dontCheckTitleCase;
    QCheckBox *cbBackgroundSpellCheck;
    QPushButton *clearIgnoreAllHistory;
    KConfig* config;
};

class ConfigureInterfacePage : public QObject
{
    Q_OBJECT
public:
    ConfigureInterfacePage( KWView *_view, QVBox *box, char *name = 0 );
    void apply();
    void slotDefault();
private:
    KWView* m_pView;
    KDoubleNumInput* gridX,*gridY;
    KDoubleNumInput* indent;
    KIntNumInput* recentFiles;
    QCheckBox *showStatusBar, *showScrollBar, *pgUpDownMovesCaret;
    int oldNbRecentFiles;
    KConfig* config;
    KIntNumInput *m_nbPagePerRow;
};

class ConfigureMiscPage : public QObject
{
    Q_OBJECT
public:
    ConfigureMiscPage( KWView *_view, QVBox *box, char *name = 0 );
    KCommand* apply();
    void slotDefault();
private:
    KWView* m_pView;
    KConfig* config;
    QComboBox *m_unit;
    int m_oldUnit;
    KIntNumInput* m_undoRedoLimit;
    int m_oldNbRedo;
    bool m_oldFormattingEndParag, m_oldFormattingSpace, m_oldFormattingTabs, m_oldFormattingBreak;
    QCheckBox* m_displayLink, *m_displayComment, *m_underlineLink, *m_displayFieldCode;
    QCheckBox* m_cbViewFormattingEndParag, *m_cbViewFormattingSpace;
    QCheckBox* m_cbViewFormattingTabs, *m_cbViewFormattingBreak;
};

class ConfigureDefaultDocPage : public QObject
{
    Q_OBJECT
public:
    ConfigureDefaultDocPage( KWView *_view, QVBox *box, char *name = 0 );
    ~ConfigureDefaultDocPage();
    KCommand* apply();
    void slotDefault();
public slots:
    void selectNewDefaultFont();
private:
    KWView* m_pView;
    KConfig* config;
    QFont *font;
    QLabel *fontName;

    KDoubleNumInput* columnSpacing;

    KIntNumInput* autoSave;
    int oldAutoSaveValue;
    double m_oldTabStopWidth;

    int m_oldStartingPage;
    KIntNumInput* m_variableNumberOffset;
    KDoubleNumInput *m_tabStopWidth;
    QCheckBox *m_cursorInProtectedArea;
};

class KWConfig : public KDialogBase
{
    Q_OBJECT
public:
    enum { KW_KSPELL=1,KP_INTERFACE=2,KP_MISC=4, KP_DOCUMENT=8, KP_FORMULA=16};
    KWConfig( KWView* parent );
    void openPage(int flags);
public slots:
    void slotApply();
    void slotDefault();
private:
    ConfigureSpellPage *m_spellPage;
    ConfigureInterfacePage *m_interfacePage;
    ConfigureMiscPage *m_miscPage;
    ConfigureDefaultDocPage *m_defaultDocPage;
    KFormula::ConfigurePage *m_formulaPage;
    KWDocument *m_doc;
};



#endif
