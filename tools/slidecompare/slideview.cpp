#include "slideview.h"
#include "slideloader.h"
#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtGui/QGraphicsItem>
#include <QtGui/QImage>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPixmapCache>
#include <QtGui/QScrollBar>
#include <QtGui/QTransform>
#include <QtGui/QWheelEvent>
#include <QtOpenGL/QGLWidget>
#include <cmath>

class GraphicsSlideItem : public QGraphicsItem {
public:
#ifdef QT46
    class Cache {
    private:
        QPixmapCache::Key key;
    public:
        bool find(QPixmap& pixmap) const {
            return QPixmapCache::find(key, &pixmap);
        }
        void clear() const { QPixmapCache::remove(key); }
        void add(const QPixmap& pixmap) {
            clear();
            key = QPixmapCache::insert(pixmap);
        }
    };
#else
    class Cache {
    private:
        const QString key;
    public:
        Cache() :key(QString("%1 GraphicsSlideItem").arg((qlonglong)this)) {}
        bool find(QPixmap& pixmap) const {
            return QPixmapCache::find(key, pixmap);
        }
        void clear() const { QPixmapCache::remove(key); }
        void add(const QPixmap& pixmap) {
            clear();
            QPixmapCache::insert(key, pixmap);
        }
    };
#endif
    Cache cache;
    QRectF rect;
    QPainterPath path;
    SlideLoader* const loader;
    int position;
    int slideVersion;

    explicit GraphicsSlideItem(int pos, SlideLoader* l,
             QGraphicsItem * parent = 0)
            :QGraphicsItem(parent), loader(l), position(pos) {
        slideVersion = -1;
    }
    void setPosition(int pos) {
        if (position != pos) {
            position = pos;
            slideVersion = -1;
        }
        if (loader->slideVersion(position) != slideVersion) {
            update(rect);
        }
    } 
    void paint(QPainter* painter, const QStyleOptionGraphicsItem * /*option*/,
               QWidget * /*widget*/ = 0) {
        QPixmap pixmap;
        if (slideVersion != loader->slideVersion(position)
                || !cache.find(pixmap)) {
            slideVersion = loader->slideVersion(position);
            pixmap = loader->loadSlide(position, rect.size().toSize());
            cache.add(pixmap);
        }
        if (pixmap.isNull()) {
            painter->fillRect(rect, Qt::gray);
        } else {
            painter->drawPixmap(rect.topLeft(), pixmap);
        }
    }
    QRectF boundingRect() const {
        return rect;
    }
    QPainterPath opaqueArea() const {
        return path;
    }
    void setBoundingRect(const QRectF& newrect) {
        if (rect != newrect) {
            if (rect.size() != newrect.size()) {
                cache.clear();
            }
            prepareGeometryChange();
            rect = newrect;
            path = QPainterPath();
            path.addRect(rect);
        }
    }
};

SlideView::SlideView(SlideLoader* l, QWidget* parent) :QWidget(parent),
        loader(l), zoomfactor(0.25), sendingChange(false)
{
    // use opengl canvas
    view.setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
    // listen to resize and wheel events
    view.viewport()->installEventFilter(this);

    connect(loader, SIGNAL(slidesChanged()), this, SLOT(slotUpdateSlides()));
    connect(view.verticalScrollBar(), SIGNAL(valueChanged(int)),
        this, SLOT(slotViewChanged()));
    connect(view.horizontalScrollBar(), SIGNAL(valueChanged(int)),
        this, SLOT(slotViewChanged()));

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(&view);
    layout->addWidget(&progressBar);
    setLayout(layout);

    progressBar.setVisible(false);
    scene.setBackgroundBrush(Qt::Dense4Pattern);
    view.setScene(&scene);

    slotUpdateSlides();
}

void SlideView::slotUpdateSlides() {
    int numberOfSlides = loader->numberOfSlides();
    QList<QGraphicsItem*> items = scene.items();
    if (items.size() > numberOfSlides) {
        // remove surplus items
        for (int i = numberOfSlides; i < items.size(); ++i) {
            scene.removeItem(items[i]);
        }
    } else if (numberOfSlides > items.size()) {
        // add new items
        for (int i=items.size(); i<numberOfSlides; ++i) {
            GraphicsSlideItem* item = new GraphicsSlideItem(i, loader);
            item->setVisible(true);
            scene.addItem(item);
        }
    }
    items = scene.items();
    for (int i=0; i<items.size(); ++i) {
        static_cast<GraphicsSlideItem*>(items[i])->setPosition(i);
    }
    layout();
}
void SlideView::layout() {
    const qreal spacing = 2;
    QSizeF slidesize = loader->slideSize();
    int slidesPerRow = (zoomfactor > 1) ?1 :1/zoomfactor;

    if (zoomfactor <= 1) {
        view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);;
    } else {
        view.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
    qreal viewwidth = view.viewport()->width();

    qreal x = 0;
    qreal y = 0;
    qreal screenslidewidth = zoomfactor * viewwidth;

    qreal w, h, viewscale;
    if (screenslidewidth > slidesize.width()) {
        viewscale = screenslidewidth / slidesize.width();
        w = slidesize.width();
        h = slidesize.height();
    } else {
        viewscale = 1;
        qreal slidescale = screenslidewidth / slidesize.width();
        w = slidesize.width() * slidescale;
        h = slidesize.height() * slidescale;
    }
    QTransform transform;
    transform.scale(viewscale, viewscale);
    view.setTransform(transform);

    const QList<QGraphicsItem *> items = scene.items();
    qreal dx = w + spacing;
    qreal dy = h + spacing;
    for (int i=0; i < items.size(); ++i) {
        GraphicsSlideItem* item = static_cast<GraphicsSlideItem*>(items[i]);
        item->setBoundingRect(QRectF(x, y, w, h));
        if ((i+1) % slidesPerRow) {
            x += dx;
        } else {
            x = 0;
            y += dy;
        }
    }
    if (x == 0) {
        scene.setSceneRect(0, 0, slidesPerRow*dx, y);
    } else {
        scene.setSceneRect(0, 0, slidesPerRow*dx, y + dy);
    }
}
bool SlideView::eventFilter(QObject * /*obj*/, QEvent *event) {
    if (event->type() == QEvent::Wheel) {
        const QWheelEvent* e = static_cast<QWheelEvent*>(event);
        if (e->modifiers() == Qt::ControlModifier) {
            zoomfactor *= pow(1.1, e->delta()/120.0);
            layout();
            slotViewChanged();
            return true;
        }
    } else if (event->type() == QEvent::Resize) {
        layout();
        slotViewChanged();
    }
    return false;
}
void
SlideView::setView(qreal zoomfactor, int h, int v) {
    if (sendingChange) return;
    this->zoomfactor = zoomfactor;
    layout();
    view.horizontalScrollBar()->setValue(h);
    view.verticalScrollBar()->setValue(v);
}
void SlideView::slotViewChanged() {
    sendingChange = true;
    int h = view.horizontalScrollBar()->value();
    int v = view.verticalScrollBar()->value();
    emit viewChanged(zoomfactor, h, v);
    sendingChange = false;
}
