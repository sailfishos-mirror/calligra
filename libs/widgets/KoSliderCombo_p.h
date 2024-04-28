/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 C. Boemann <cbo@boemann.dk>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef KoSliderCombo_p_h
#define KoSliderCombo_p_h

#include "KoSliderCombo.h"

#include <QTimer>
#include <QApplication>
#include <QSize>
#include <QSlider>
#include <QStyle>
#include <QStylePainter>
#include <QStyleOptionSlider>
#include <QLineEdit>
#include <QValidator>
#include <QHBoxLayout>
#include <QFrame>
#include <QMenu>
#include <QMouseEvent>
#include <QDoubleSpinBox>

#include <KLocalizedString>
#include <WidgetsDebug.h>

class KoSliderComboContainer : public QMenu
{
Q_OBJECT
public:
    KoSliderComboContainer(KoSliderCombo *parent) : QMenu(parent ), m_parent(parent) {}
    ~KoSliderComboContainer() override = default;
protected:
    void mousePressEvent(QMouseEvent *e) override;
private:
    KoSliderCombo *m_parent;
};

class Q_DECL_HIDDEN KoSliderCombo::KoSliderComboPrivate {
public:
    KoSliderCombo *thePublic;
    QValidator *m_validator;
    QTimer m_timer;
    KoSliderComboContainer *container;
    QSlider *slider;
    QStyle::StateFlag arrowState;
    qreal minimum;
    qreal maximum;
    int decimals;
    bool firstShowOfSlider;

    void showPopup();
    void hidePopup();

    void sliderValueChanged(int value);
    void sliderReleased();
    void lineEditFinished();
};

#endif
