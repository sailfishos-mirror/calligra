#ifndef __koffice_filter_dialog_h__
#define __koffice_filter_dialog_h__

#ifndef USE_QFD

#include <qwidget.h>

/**
 * This is an abstract base class for filter config dialogs. Derive your 
 * dialog class from this one. Create a dialog in the CTOR and destroy it in
 * the DTOR. The KOffice part will search for the dialog (via the trader and
 * service stuff) and display it if the user selects the appropriate extension.
 * The user changes the state (e.g. checks a checkbox,...) and selects the
 * file. After the user clicked 'Ok' the state() method will be called.
 * In this method you have to create a QString and return it. This string
 * will be passed to the CTOR of the filter. Of course the format of this
 * string is up to you, but using XML (QDom/QXML) seems to be a good
 * choice...
 */
class KoFilterDialog : public QWidget {

    Q_OBJECT

public:
    KoFilterDialog(QWidget *parent=0L, QString name=QString::null);
    virtual ~KoFilterDialog() {}
    virtual const QString state() = 0;
};
#endif
#endif
