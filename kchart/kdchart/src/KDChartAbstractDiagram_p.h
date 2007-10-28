/****************************************************************************
 ** Copyright (C) 2007 Klaralvdalens Datakonsult AB.  All rights reserved.
 **
 ** This file is part of the KD Chart library.
 **
 ** This file may be distributed and/or modified under the terms of the
 ** GNU General Public License version 2 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.GPL included in the
 ** packaging of this file.
 **
 ** Licensees holding valid commercial KD Chart licenses may use this file in
 ** accordance with the KD Chart Commercial License Agreement provided with
 ** the Software.
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** See http://www.kdab.net/kdchart for
 **   information about KDChart Commercial License Agreements.
 **
 ** Contact info@kdab.net if any conditions of this
 ** licensing are not clear to you.
 **
 **********************************************************************/

#ifndef KDCHARTABSTRACTDIAGRAM_P_H
#define KDCHARTABSTRACTDIAGRAM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the KD Chart API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "KDChartAbstractCoordinatePlane.h"
#include "KDChartDataValueAttributes.h"
#include "KDChartRelativePosition.h"
#include "KDChartPosition.h"
#include "KDChartPainterSaver_p.h"
#include "KDChartPaintContext.h"
#include "Scenery/ReverseMapper.h"

#include <QPoint>
#include <QPointer>
#include <QFont>
#include <QFontMetrics>
#include <QPaintDevice>
#include <QModelIndex>

#include <KDABLibFakes>


namespace KDChart {
    class AttributesModel;

    class DataValueTextInfo {
    public:
        DataValueTextInfo(){}
        DataValueTextInfo( const QModelIndex& _index, const QPointF& _pos, const QPointF& _markerPos, double _value )
            :index( _index ), pos( _pos ), markerPos( _markerPos ), value( _value )
        {}
        DataValueTextInfo( const DataValueTextInfo& other )
            :index( other.index ), pos( other.pos ), markerPos( other.markerPos ), value( other.value ) {}
        QModelIndex index;
        QPointF pos;
        QPointF markerPos;
        double value;
    };

    typedef QVector<DataValueTextInfo> DataValueTextInfoList;
    typedef QVectorIterator<DataValueTextInfo> DataValueTextInfoListIterator;


/**
 * \internal
 */
    class KDChart::AbstractDiagram::Private
    {
        friend class AbstractDiagram;
    public:
        explicit Private();
        virtual ~Private();

        Private( const Private& rhs );

        void setAttributesModel( AttributesModel* );

        bool usesExternalAttributesModel()const;

        void appendDataValueTextInfoToList(
            AbstractDiagram * diagram,
            DataValueTextInfoList & list,
            const QModelIndex & index,
            const PositionPoints& points,
            const Position& autoPositionPositive,
            const Position& autoPositionNegative,
            const qreal value )
        {
            const DataValueAttributes attrs( diagram->dataValueAttributes( index ) );
            if( attrs.isVisible() ) {
                const bool bValueIsPositive = (value >= 0.0);
                RelativePosition relPos( attrs.position( bValueIsPositive ) );
                relPos.setReferencePoints( points );
                if( relPos.referencePosition().isUnknown() )
                    relPos.setReferencePosition( bValueIsPositive ? autoPositionPositive : autoPositionNegative );

                const QPointF referencePoint = relPos.referencePoint();
                if( diagram->coordinatePlane()->isVisiblePoint( referencePoint ) ){
                    const qreal fontHeight = cachedFontMetrics( attrs.textAttributes().font(), diagram )->height();

                    // Note: When printing data value texts the font height is used as reference size for both,
                    //       horizontal and vertical padding, if the respective padding's Measure is using
                    //       automatic reference area detection.
                    QSizeF relativeMeasureSize( fontHeight, fontHeight );

                    // Store the anchor point, that's already shifted according to horiz./vert. padding:
                    list.append( DataValueTextInfo(
                                     index,
                                     relPos.calculatedPoint( relativeMeasureSize ),
                                     referencePoint,
                                     value ) );
                }
            }
        }

        const QFontMetrics * cachedFontMetrics( const QFont& font, QPaintDevice * paintDevice)
        {
            if( (font != mCachedFont) || (paintDevice != mCachedPaintDevice) )
                mCachedFontMetrics = QFontMetrics( font, paintDevice );
            return & mCachedFontMetrics;
        }
        const QFontMetrics cachedFontMetrics() const
        {
            return mCachedFontMetrics;
        }

        void paintDataValueTextsAndMarkers( AbstractDiagram* diag, PaintContext* ctx,
                                            const DataValueTextInfoList & list, bool paintMarkers )
        {
            PainterSaver painterSaver( ctx->painter() );
            ctx->painter()->setClipping( false );
            if( paintMarkers )
            {
                DataValueTextInfoListIterator it( list );
                while ( it.hasNext() ) {
                    const DataValueTextInfo& info = it.next();
                    diag->paintMarker( ctx->painter(), info.index, info.markerPos );
                }
            }
            DataValueTextInfoListIterator it( list );
            while ( it.hasNext() ) {
                const DataValueTextInfo& info = it.next();
                diag->paintDataValueText( ctx->painter(), info.index, info.pos, info.value );
            }
        }

        void paintDataValueText( QPainter* painter,
                                 const QModelIndex& index,
                                 const QPointF& pos,
                                 QString text )
        {
            Q_UNUSED( index );
            // paint one data series
            const DataValueAttributes a( qVariantValue<DataValueAttributes>(
                                             attributesModel->modelData( KDChart::DataValueLabelAttributesRole ) ) );
            if ( !a.isVisible() ) return;

            // handle prefix and suffix
            if ( !a.prefix().isNull() )
                text.prepend( a.prefix() );

            if ( !a.suffix().isNull() )
                text.append( a.suffix() );

            const TextAttributes ta( a.textAttributes() );
            if ( ta.isVisible() ) {

                QPointF pt( pos );
                // adjust the text start point position, if alignment is not Bottom/Left
                const RelativePosition relPos( a.position( true ) );
                const Qt::Alignment alignBottomLeft = Qt::AlignBottom | Qt::AlignLeft;
                const QFont calculatedFont( ta.calculatedFont( plane, KDChartEnums::MeasureOrientationMinimum ) );
                //qDebug() << "calculatedFont's point size:" << calculatedFont.pointSizeF();
                if( (relPos.alignment() & alignBottomLeft) != alignBottomLeft ){
                    const QRectF boundRect(
                        cachedFontMetrics( calculatedFont, painter->device() )->boundingRect( text ) );
                    if( relPos.alignment() & Qt::AlignRight )
                        pt.rx() -= boundRect.width();
                    else if( relPos.alignment() & Qt::AlignHCenter )
                        pt.rx() -= 0.5 * boundRect.width();

                    if( relPos.alignment() & Qt::AlignTop )
                        pt.ry() += boundRect.height();
                    else if( relPos.alignment() & Qt::AlignVCenter )
                        pt.ry() += 0.5 * boundRect.height();
                }

                // FIXME draw the non-text bits, background, etc

                if ( a.showRepetitiveDataLabels() || pos.x() <= lastX || lastRoundedValue != text ) {
                    lastRoundedValue = text;
                    lastX = pos.x();

                    PainterSaver painterSaver( painter );
                    painter->setPen( ta.pen() );
                    painter->setFont( calculatedFont );
                    painter->translate( pt );
                    painter->rotate( ta.rotation() );
                    painter->drawText( QPointF(0, 0), text );
                }
            }
        }

        virtual QModelIndex indexAt( const QPoint& point ) const
        {
            QModelIndexList l = indexesAt( point );
            if ( !l.isEmpty() )
                return l.first();
            else
                return QModelIndex();
        }

        QModelIndexList indexesAt(  const QPoint& point ) const
        {
            return reverseMapper.indexesAt(  point ); // which could be empty
        }

        QModelIndexList indexesIn( const QRect& rect ) const
        {
            return reverseMapper.indexesIn( rect );
        }

    protected:
        void init();
        void init( AbstractCoordinatePlane* plane );
        QPointer<AbstractCoordinatePlane> plane;
        mutable QModelIndex attributesModelRootIndex;
        QPointer<AttributesModel> attributesModel;
        bool allowOverlappingDataValueTexts;
        bool antiAliasing;
        bool percent;
        int datasetDimension;
        mutable QPair<QPointF,QPointF> databoundaries;
        mutable bool databoundariesDirty;
        ReverseMapper reverseMapper;

        QMap< Qt::Orientation, QString > unitSuffix;
        QMap< Qt::Orientation, QString > unitPrefix;
        QMap< int, QMap< Qt::Orientation, QString > > unitSuffixMap;
        QMap< int, QMap< Qt::Orientation, QString > > unitPrefixMap;

    private:
        QString lastRoundedValue;
        qreal lastX;
        QFontMetrics   mCachedFontMetrics;
        QFont          mCachedFont;
        QPaintDevice * mCachedPaintDevice;
    };

    inline AbstractDiagram::AbstractDiagram( Private * p ) : _d( p )
    {
        init();
    }
    inline AbstractDiagram::AbstractDiagram(
        Private * p, QWidget* parent, AbstractCoordinatePlane* plane )
        : QAbstractItemView( parent ), _d( p )
    {
        _d->init( plane );
        init();
    }


    class LineAttributesInfo {
        public :
        LineAttributesInfo() {}
        LineAttributesInfo( const QModelIndex _index, const QPointF& _value, const QPointF& _nextValue )
            :index( _index ), value ( _value ), nextValue ( _nextValue )  {}

        QModelIndex index;
        QPointF value;
        QPointF nextValue;
    };

    typedef QVector<LineAttributesInfo> LineAttributesInfoList;
    typedef QVectorIterator<LineAttributesInfo> LineAttributesInfoListIterator;

}
#endif /* KDCHARTDIAGRAM_P_H */
