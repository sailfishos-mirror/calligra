/* This file is part of the KDE project
   Copyright (C) 2001, The Karbon Developers
*/

#ifndef __VCANVAS_H__
#define __VCANVAS_H__

#include <qscrollview.h>

class KarbonPart;
class KarbonView;

// The karbon canvas is a QScrollView.

class VCanvas : public QScrollView
{
	Q_OBJECT
public:
	VCanvas( KarbonView* view, KarbonPart* part );

	const double& zoomFactor() { return m_zoomFactor; }

protected:
	virtual void drawContents( QPainter* painter, int clipx, int clipy,
		int clipw, int cliph  );
	void drawDocument( QPainter* painter, const QRect& rect );

	virtual void resizeEvent( QResizeEvent* event );

private:
	KarbonPart* m_part;
	KarbonView* m_view;

	double m_zoomFactor;
};

#endif
