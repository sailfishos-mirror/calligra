/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2006-2008 Thorsten Zachmann <zachmann@kde.org>
   SPDX-FileCopyrightText: 2006, 2008 C. Boemann <cbo@boemann.dk>
   SPDX-FileCopyrightText: 2006-2010 Thomas Zander <zander@kde.org>
   SPDX-FileCopyrightText: 2007-2009, 2011 Jan Hambrecht <jaham@gmx.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KOSHAPE_H
#define KOSHAPE_H

#include "KoConnectionPoint.h"
#include "KoFlake.h"

#include <QMetaType>
#include <QSet>
#include <QSharedPointer>

#include <KoShapeBackground.h>
#include <KoXmlReaderForward.h>

// #include <KoSnapData.h>

#include "flake_export.h"

class QPainter;
class QRectF;
class QPainterPath;
class QTransform;

class KoShapeContainer;
class KoShapeStrokeModel;
class KoShapeUserData;
class KoViewConverter;
class KoShapeApplicationData;
class KoShapeSavingContext;
class KoShapeLoadingContext;
class KoGenStyle;
class KoShapeShadow;
class KoEventAction;
class KoShapePrivate;
class KoFilterEffectStack;
class KoSnapData;
class KoClipPath;
class KoShapePaintingContext;
class KoShapeAnchor;
class KoBorder;
struct KoInsets;

/**
 *
 * Base class for all flake shapes. Shapes extend this class
 * to allow themselves to be manipulated. This class just represents
 * a graphical shape in the document and can be manipulated by some default
 * tools in this library.
 *
 * Due to the limited responsibility of this class, the extending object
 * can have any data backend and is responsible for painting itself.
 *
 * We strongly suggest that any extending class will use a Model View
 * Controller (MVC) design where the View part is all in this class, as well
 * as the one that inherits from this one.  This allows the data that rests
 * in the model to be reused in different parts of the document. For example
 * by having two flake objects that show that same data. Or each showing a section of it.
 *
 * The KoShape data is completely in postscript-points (pt) (see KoUnit
 * for conversion methods to and from points).
 * This image will explain the real-world use of the shape and its options.
 * <img src="../flake_shape_coords.png" align=center><br>
 *  The Rotation center can be returned with absolutePosition()
 *
 * <p>Flake objects can be created in three ways:
 * <ul>
 *   <li>a simple new KoDerivedFlake(),
 *   <li>through an associated tool,
 *   <li>through a factory
 * </ul>
 *
 * <h1>Shape interaction notifications</h1>
 * We had several notification methods that allow your shape to be notified of changes in other
 * shapes positions or rotation etc.
 * <ol><li>The most general is KoShape::shapeChanged().<br>
 * a virtual method that you can use to check various changed to your shape made by tools or otherwise.</li>
 * <li>for shape hierarchies the parent may receive a notification when a child was modified.
 *  This is done though KoShapeContainerModel::childChanged()</li>
 * <li>any shape that is at a similar position as another shape there is collision detection.
 * You can register your shape to be sensitive to any changes like moving or whatever to
 * <b>other</b> shapes that intersect yours.
 * Such changes will then be notified to your shape using the method from (1) You should call
 * KoShape::setCollisionDetection(bool) to enable this.
 * </ol>
 */
class FLAKE_EXPORT KoShape
{
public:
    /// Used by shapeChanged() to select which change was made
    enum ChangeType {
        PositionChanged, ///< used after a setPosition()
        RotationChanged, ///< used after a setRotation()
        ScaleChanged, ///< used after a scale()
        ShearChanged, ///< used after a shear()
        SizeChanged, ///< used after a setSize()
        GenericMatrixChange, ///< used after the matrix was changed without knowing which property explicitly changed
        ParentChanged, ///< used after a setParent()
        CollisionDetected, ///< used when another shape moved in our boundingrect
        Deleted, ///< the shape was deleted
        StrokeChanged, ///< the shapes stroke has changed
        BackgroundChanged, ///< the shapes background has changed
        ShadowChanged, ///< the shapes shadow has changed
        BorderChanged, ///< the shapes border has changed
        ParameterChanged, ///< the shapes parameter has changed (KoParameterShape only)
        ContentChanged, ///< the content of the shape changed e.g. a new image inside a pixmap/text change inside a textshape
        TextRunAroundChanged, ///< used after a setTextRunAroundSide()
        ChildChanged, ///< a child of a container was changed/removed. This is propagated to all parents
        ConnectionPointChanged, ///< a connection point has changed
        ClipPathChanged, ///< the shapes clip path has changed
        ControlPointChanged, ///< a control point has changed
        BeginResize, ///< used during resizing
        EndResize ///< used during resizing
    };

    /// The behavior text should do when intersecting this shape.
    enum TextRunAroundSide {
        BiggestRunAroundSide, ///< Run other text around the side that has the most space
        LeftRunAroundSide, ///< Run other text around the left side of the frame
        RightRunAroundSide, ///< Run other text around the right side of the frame
        EnoughRunAroundSide, ///< Run other text dynamically around both sides of the shape, provided there is sufficient space left
        BothRunAroundSide, ///< Run other text around both sides of the shape
        NoRunAround, ///< The text will be completely avoiding the frame by keeping the horizontal space that this frame occupies blank.
        RunThrough ///< The text will completely ignore the frame and layout as if it was not there
    };

    /// The behavior text should do when intersecting this shape.
    enum TextRunAroundContour {
        ContourBox, /// Run other text around a bounding rect of the outline
        ContourFull, ///< Run other text around also on the inside
        ContourOutside ///< Run other text around only on the outside
    };

    /**
     * TODO
     */
    enum RunThroughLevel {
        Background,
        Foreground
    };

    /// Fine grained control of allowed user interactions
    enum AllowedInteraction {
        MoveAllowed = 1, ///< Moving the shape is allowed
        ResizeAllowed = 2, ///< Resizing the shape is allowed
        ShearingAllowed = 4, ///< Sharing the shape is allowed
        RotationAllowed = 8, ///< Rotating the shape is allowed
        SelectionAllowed = 16, ///< Selecting the shape is allowed
        ContentChangeAllowed = 32, ///< Editing the content is allowed
        DeletionAllowed = 64 ///< Deleting the shape is allowed
    };
    Q_DECLARE_FLAGS(AllowedInteractions, AllowedInteraction)
    Q_FLAGS(AllowedInteractions)

    /**
     * @brief Constructor
     */
    KoShape();

    /**
     * @brief Destructor
     */
    virtual ~KoShape();

    /**
     * @brief Paint the shape
     * The class extending this one is responsible for painting itself.  Since we do not
     * assume the shape is square the paint must also clear its background if it will draw
     * something transparent on top.
     * This can be done with a method like:
     * <code>
       painter.fillRect(converter.normalToView(QRectF(QPointF(0.0,0.0), size())), background());</code>
     * Or equivalent for non-square objects.
     * Do note that a shape's top-left is always at coordinate 0,0. Even if the shape itself is rotated
     * or translated.
     * @param painter used for painting the shape
     * @param converter to convert between internal and view coordinates.
     * @see applyConversion()
     * @param paintcontext the painting context.
     */
    virtual void paint(QPainter &painter, const KoViewConverter &converter, KoShapePaintingContext &paintcontext) = 0;

    /**
     * @brief Paint the shape's border
     * This is a helper function that could be called from the paint() method of all shapes.
     * @param painter used for painting the shape
     * @param converter to convert between internal and view coordinates.
     * @see applyConversion()
     */
    virtual void paintBorder(QPainter &painter, const KoViewConverter &converter);

    /**
     * Load a shape from odf
     *
     * @param context the KoShapeLoadingContext used for loading
     * @param element element which represents the shape in odf
     *
     * @return false if loading failed
     */
    virtual bool loadOdf(const KoXmlElement &element, KoShapeLoadingContext &context) = 0;

    /**
     * @brief store the shape data as ODF XML.
     * This is the method that will be called when saving a shape as a described in
     * OpenDocument 9.2 Drawing Shapes.
     * @see saveOdfAttributes
     */
    virtual void saveOdf(KoShapeSavingContext &context) const = 0;

    /**
     * This method can be used while saving the shape as ODF to add the data
     * stored on this shape to the current element.
     *
     * @param context the context for the current save.
     * @param attributes a number of OdfAttribute items to state which attributes to save.
     * @see saveOdf
     */
    void saveOdfAttributes(KoShapeSavingContext &context, int attributes) const;

    /**
     * This method can be used while saving the shape as Odf to add common child elements
     *
     * The office:event-listeners and draw:glue-point are saved.
     * @param context the context for the current save.
     */
    void saveOdfCommonChildElements(KoShapeSavingContext &context) const;

    /**
     * This method can be used to save contour data from the clipPath()
     *
     * The draw:contour-polygon or draw:contour-path elements are saved.
     * @param context the context for the current save.
     * @param originalSize the original size of the unscaled image.
     */
    void saveOdfClipContour(KoShapeSavingContext &context, const QSizeF &originalSize) const;

    /**
     * @brief Scale the shape using the zero-point which is the top-left corner.
     * @see position()
     *
     * @param sx scale in x direction
     * @param sy scale in y direction
     */
    void scale(qreal sx, qreal sy);

    /**
     * @brief Rotate the shape (relative)
     *
     * The shape will be rotated from the current rotation using the center of the shape using the size()
     *
     * @param angle change the angle of rotation increasing it with 'angle' degrees
     */
    void rotate(qreal angle);

    /**
     * Return the current rotation in degrees.
     * It returns NaN if the shape has a shearing or scaling transformation applied.
     */
    qreal rotation() const;

    /**
     * @brief Shear the shape
     * The shape will be sheared using the zero-point which is the top-left corner.
     * @see position()
     *
     * @param sx shear in x direction
     * @param sy shear in y direction
     */
    void shear(qreal sx, qreal sy);

    /**
     * @brief Resize the shape
     *
     * @param size the new size of the shape.  This is different from scaling as
     * scaling is a so called secondary operation which is comparable to zooming in
     * instead of changing the size of the basic shape.
     * Easiest example of this difference is that using this method will not distort the
     * size of pattern-fills and strokes.
     */
    virtual void setSize(const QSizeF &size);

    /**
     * @brief Get the size of the shape in pt.
     *
     * The size is in shape coordinates.
     *
     * @return the size of the shape as set by setSize()
     */
    virtual QSizeF size() const;

    /**
     * @brief Set the position of the shape in pt
     *
     * @param position the new position of the shape
     */
    virtual void setPosition(const QPointF &position);

    /**
     * @brief Get the position of the shape in pt
     *
     * @return the position of the shape
     */
    QPointF position() const;

    /**
     * @brief Check if the shape is hit on position
     * @param position the position where the user clicked.
     * @return true when it hits.
     */
    virtual bool hitTest(const QPointF &position) const;

    /**
     * @brief Get the bounding box of the shape
     *
     * This includes the line width and the shadow of the shape
     *
     * @return the bounding box of the shape
     */
    virtual QRectF boundingRect() const;

    /**
     * @brief Add a connector point to the shape
     *
     * A connector is a place on the shape that allows a graphical connection to be made
     * using a line, for example.
     *
     * @param point the connection point to add
     * @return the id of the new connection point
     */
    int addConnectionPoint(const KoConnectionPoint &point);

    /**
     * Sets data of connection point with specified id.
     *
     * The position of the connector is restricted to the bounding rectangle of the shape.
     * When setting a default connection point, the new position is ignored, as these
     * are fixed at their default position.
     * The function will insert a new connection point if the specified id was not used
     * before.
     *
     * @param connectionPointId the id of the connection point to set
     * @param point the connection point data
     * @return false if specified connection point id is invalid, else true
     */
    bool setConnectionPoint(int connectionPointId, const KoConnectionPoint &point);

    /// Checks if a connection point with the specified id exists
    bool hasConnectionPoint(int connectionPointId) const;

    /// Returns connection point with specified connection point id
    KoConnectionPoint connectionPoint(int connectionPointId) const;

    /**
     * Return a list of the connection points that have been added to this shape.
     * All the points are relative to the shape position, see absolutePosition().
     * @return a list of the connectors that have been added to this shape.
     */
    KoConnectionPoints connectionPoints() const;

    /// Removes connection point with specified id
    void removeConnectionPoint(int connectionPointId);

    /// Removes all connection points
    void clearConnectionPoints();

    /**
     * Add a event action
     */
    void addEventAction(KoEventAction *action);

    /**
     * Remove a event action
     */
    void removeEventAction(KoEventAction *action);

    /**
     * Get all event actions
     */
    QSet<KoEventAction *> eventActions() const;

    /**
     * Return the side text should flow around this shape. This implements the ODF style:wrap
     * attribute that specifies how text is displayed around a frame or graphic object.
     */
    TextRunAroundSide textRunAroundSide() const;

    /**
     * Set the side text should flow around this shape.
     * @param side the requested side
     * @param runThrough run through the foreground or background or...
     */
    void setTextRunAroundSide(TextRunAroundSide side, RunThroughLevel runThrough = Background);

    /**
     * The space between this shape's left edge and text that runs around this shape.
     * @return the space around this shape to keep free from text
     */
    qreal textRunAroundDistanceLeft() const;

    /**
     * Set the space between this shape's left edge and the text that run around this shape.
     * @param distance the space around this shape to keep free from text
     */
    void setTextRunAroundDistanceLeft(qreal distance);

    /**
     * The space between this shape's top edge and text that runs around this shape.
     * @return the space around this shape to keep free from text
     */
    qreal textRunAroundDistanceTop() const;

    /**
     * Set the space between this shape's top edge and the text that run around this shape.
     * @param distance the space around this shape to keep free from text
     */
    void setTextRunAroundDistanceTop(qreal distance);

    /**
     * The space between this shape's right edge and text that runs around this shape.
     * @return the space around this shape to keep free from text
     */
    qreal textRunAroundDistanceRight() const;

    /**
     * Set the space between this shape's right edge and the text that run around this shape.
     * @param distance the space around this shape to keep free from text
     */
    void setTextRunAroundDistanceRight(qreal distance);

    /**
     * The space between this shape's bottom edge and text that runs around this shape.
     * @return the space around this shape to keep free from text
     */
    qreal textRunAroundDistanceBottom() const;

    /**
     * Set the space between this shape's bottom edge and the text that run around this shape.
     * @param distance the space around this shape to keep free from text
     */
    void setTextRunAroundDistanceBottom(qreal distance);

    /**
     * Return the threshold above which text should flow around this shape.
     * The text will not flow around the shape on a side unless the space available on that side
     * is above this threshold. Only used when the text run around side is EnoughRunAroundSide.
     * @return threshold the threshold
     */
    qreal textRunAroundThreshold() const;

    /**
     * Set the threshold above which text should flow around this shape.
     * The text will not flow around the shape on a side unless the space available on that side
     * is above this threshold. Only used when the text run around side is EnoughRunAroundSide.
     * @param threshold the new threshold
     */
    void setTextRunAroundThreshold(qreal threshold);

    /**
     * Return the how tight text run around is done around this shape.
     * @return the contour
     */
    TextRunAroundContour textRunAroundContour() const;

    /**
     * Set how tight text run around is done around this shape.
     * @param contour the new contour
     */
    void setTextRunAroundContour(TextRunAroundContour contour);

    /**
     * Set the KoShapeAnchor
     */
    void setAnchor(KoShapeAnchor *anchor);

    /**
     * Return the KoShapeAnchor, or 0
     */
    KoShapeAnchor *anchor() const;

    /**
     * Set the minimum height of the shape.
     * Currently it's not respected but only for informational purpose
     * @param height the minimum height of the frame.
     */
    void setMinimumHeight(qreal height);

    /**
     * Return the minimum height of the shape.
     * @return the minimum height of the shape. Default is 0.0.
     */
    qreal minimumHeight() const;

    /**
     * Set the background of the shape.
     * A shape background can be a plain color, a gradient, a pattern, be fully transparent
     * or have a complex fill.
     * Setting such a background will allow the shape to be filled and will be able to tell
     * if it is transparent or not.
     * @param background the new shape background.
     */
    void setBackground(QSharedPointer<KoShapeBackground> background);

    /**
     * return the brush used to paint te background of this shape with.
     * A QBrush can have a plain color, be fully transparent or have a complex fill.
     * setting such a brush will allow the shape to fill itself using that brush and
     * will be able to tell if its transparent or not.
     * @return the background-brush
     */
    QSharedPointer<KoShapeBackground> background() const;

    /**
     * Returns true if there is some transparency, false if the shape is fully opaque.
     * The default implementation will just return if the background has some transparency,
     * you should override it and always return true if your shape is not square.
     * @return if the shape is (partly) transparent.
     */
    virtual bool hasTransparency() const;

    /**
     * Sets shape level transparency.
     * @param transparency the new shape level transparency
     */
    void setTransparency(qreal transparency);

    /**
     * Returns the shape level transparency.
     * @param recursive when true takes the parents transparency into account
     */
    qreal transparency(bool recursive = false) const;

    /**
     * Retrieve the z-coordinate of this shape.
     * The zIndex property is used to determine which shape lies on top of other objects.
     * An shape with a higher z-order is on top, and can obscure another shape.
     * @return the z-index of this shape.
     * @see setZIndex()
     */
    int zIndex() const;

    /**
     * Set the z-coordinate of this shape.
     * The zIndex property is used to determine which shape lies on top of other objects.
     * An shape with a higher z-order is on top, and can obscure, another shape.
     * <p>Just like two objects having the same x or y coordinate will make them 'touch',
     * so will two objects with the same z-index touch on the z plane.  In layering the
     * shape this, however, can cause a little confusion as one always has to be on top.
     * The layering if two overlapping objects have the same index is implementation dependent
     * and probably depends on the order in which they are added to the shape manager.
     * @param zIndex the new z-index;
     */
    void setZIndex(int zIndex);

    /**
     * Retrieve the run through property of this shape.
     * The run through property is used to determine if the shape is behind, inside or before text.
     * @return the run through of this shape.
     */
    int runThrough();

    /**
     * Set the run through property of this shape.
     * The run through property is used to determine if the shape is behind, inside or before text.
     * @param runThrough the new run through;
     */
    virtual void setRunThrough(short int runThrough);

    /**
     * Changes the Shape to be visible or invisible.
     * Being visible means being painted, as well as being used for
     *   things like guidelines or searches.
     * @param on when true; set the shape to be visible.
     * @see setGeometryProtected(), setContentProtected(), setSelectable()
     */
    void setVisible(bool on);
    /**
     * Returns current visibility state of this shape.
     * Being visible means being painted, as well as being used for
     *   things like guidelines or searches.
     * @param recursive when true, checks visibility recursively
     * @return current visibility state of this shape.
     * @see isGeometryProtected(), isContentProtected(), isSelectable()
     */
    bool isVisible(bool recursive = false) const;

    /**
     * Changes the shape to be printable or not. The default is true.
     *
     * If a Shape's print flag is true, the shape will be printed. If
     * false, the shape will not be printed. If a shape is not visible (@see isVisible),
     * it isPrinted will return false, too.
     */
    void setPrintable(bool on);

    /**
     * Returns the current printable state of this shape.
     *
     * A shape can be visible but not printable, not printable and not visible
     * or visible and printable, but not invisible and still printable.
     *
     * @return current printable state of this shape.
     */
    bool isPrintable() const;

    /**
     * Makes it possible for the user to select this shape.
     * This parameter defaults to true.
     * @param selectable when true; set the shape to be selectable by the user.
     * @see setGeometryProtected(), setContentProtected(), setVisible()
     */
    void setSelectable(bool selectable);

    /**
     * Returns if this shape can be selected by the user.
     * @return true only when the object is selectable.
     * @see isGeometryProtected(), isContentProtected(), isVisible()
     */
    bool isSelectable() const;

    /**
     * Tells the shape to have its position/rotation and size protected from user-changes.
     * The geometry being protected means the user can not change shape or position of the
     * shape. This includes any matrix operation such as rotation.
     * @param on when true; set the shape to have its geometry protected.
     * @see setContentProtected(), setSelectable(), setVisible()
     */
    void setGeometryProtected(bool on);

    /**
     * Returns current geometry protection state of this shape.
     * The geometry being protected means the user can not change shape or position of the
     * shape. This includes any matrix operation such as rotation.
     * @return current geometry protection state of this shape.
     * @see isContentProtected(), isSelectable(), isVisible()
     */
    bool isGeometryProtected() const;

    /**
     * Marks the shape to have its content protected against editing.
     * Content protection is a hint for tools to disallow the user editing the content.
     * @param protect when true set the shapes content to be protected from user modification.
     * @see setGeometryProtected(), setSelectable(), setVisible()
     */
    void setContentProtected(bool protect);

    /**
     * Returns current content protection state of this shape.
     * Content protection is a hint for tools to disallow the user editing the content.
     * @return current content protection state of this shape.
     * @see isGeometryProtected(), isSelectable(), isVisible()
     */
    bool isContentProtected() const;

    /**
     * Returns the parent, or 0 if there is no parent.
     * @return the parent, or 0 if there is no parent.
     */
    KoShapeContainer *parent() const;

    /**
     * Set the parent of this shape.
     * @param parent the new parent of this shape. Can be 0 if the shape has no parent anymore.
     */
    void setParent(KoShapeContainer *parent);

    /**
     * Request a repaint to be queued.
     * The repaint will be of the entire Shape, including its selection handles should this
     * shape be selected.
     * <p>This method will return immediately and only request a repaint. Successive calls
     * will be merged into an appropriate repaint action.
     */
    virtual void update() const;

    /**
     * Request a repaint to be queued.
     * The repaint will be restricted to the parameters rectangle, which is expected to be
     * in points (the internal coordinates system of KoShape) and it is expected to be
     * normalized.
     * <p>This method will return immediately and only request a repaint. Successive calls
     * will be merged into an appropriate repaint action.
     * @param rect the rectangle (in pt) to queue for repaint.
     */
    virtual void update(const QRectF &rect) const;

    /// Used by compareShapeZIndex() to order shapes
    enum ChildZOrderPolicy {
        ChildZDefault,
        ChildZParentChild = ChildZDefault, ///< normal parent/child ordering
        ChildZPassThrough ///< children are considered equal to this shape
    };

    /**
     * Returns if during compareShapeZIndex() how this shape portrays the values
     * of its children. The default behaviour is to let this shape's z values take
     * the place of its children values, so you get a parent/child relationship.
     * The children are naturally still ordered relatively to their z values
     *
     * But for special cases (like Calligra's TextShape) it can be overloaded to return
     * ChildZPassThrough which means the children keep their own z values
     * @returns the z order policy of this shape
     */
    virtual ChildZOrderPolicy childZOrderPolicy();

    /**
     * This is a method used to sort a list using the STL sorting methods.
     * @param s1 the first shape
     * @param s2 the second shape
     */
    static bool compareShapeZIndex(KoShape *s1, KoShape *s2);

    /**
     * returns the outline of the shape in the form of a path.
     * The outline returned will always be relative to the position() of the shape, so
     * moving the shape will not alter the result.  The outline is used to draw the stroke
     * on, for example.
     * @returns the outline of the shape in the form of a path.
     */
    virtual QPainterPath outline() const;

    /**
     * returns the outline of the shape in the form of a rect.
     * The outlineRect returned will always be relative to the position() of the shape, so
     * moving the shape will not alter the result.  The outline is used to calculate
     * the boundingRect.
     * @returns the outline of the shape in the form of a rect.
     */
    virtual QRectF outlineRect() const;

    /**
     * returns the outline of the shape in the form of a path for the use of painting a shadow.
     *
     * Normally this would be the same as outline() if there is a fill (background) set on the
     * shape and empty if not.  However, a shape could reimplement this to return an outline
     * even if no fill is defined. A typical example of this would be the picture shape
     * which has a picture but almost never a background.
     *
     * @returns the outline of the shape in the form of a path.
     */
    virtual QPainterPath shadowOutline() const;

    /**
     * Returns the currently set stroke, or 0 if there is no stroke.
     * @return the currently set stroke, or 0 if there is no stroke.
     */
    KoShapeStrokeModel *stroke() const;

    /**
     * Set a new stroke, removing the old one.
     * @param stroke the new stroke, or 0 if there should be no stroke.
     */
    void setStroke(KoShapeStrokeModel *stroke);

    /**
     * Return the insets of the stroke.
     * Convenience method for KoShapeStrokeModel::strokeInsets()
     */
    KoInsets strokeInsets() const;

    /// Sets the new shadow, removing the old one
    void setShadow(KoShapeShadow *shadow);

    /// Returns the currently set shadow or 0 if there is no shadow set
    KoShapeShadow *shadow() const;

    /// Sets the new border, removing the old one.
    void setBorder(KoBorder *border);

    /// Returns the currently set border or 0 if there is no border set
    KoBorder *border() const;

    /// Sets a new clip path, removing the old one
    void setClipPath(KoClipPath *clipPath);

    /// Returns the currently set clip path or 0 if there is no clip path set
    KoClipPath *clipPath() const;

    /**
     * Setting the shape to keep its aspect-ratio has the effect that user-scaling will
     * keep the width/height ratio intact so as not to distort shapes that rely on that
     * ratio.
     * @param keepAspect the new value
     */
    void setKeepAspectRatio(bool keepAspect);

    /**
     * Setting the shape to keep its aspect-ratio has the effect that user-scaling will
     * keep the width/height ratio intact so as not to distort shapes that rely on that
     * ratio.
     * @return whether to keep aspect ratio of this shape
     */
    bool keepAspectRatio() const;

    /**
     * Return the position of this shape regardless of rotation/skew/scaling and regardless of
     * this shape having a parent (being in a group) or not.<br>
     * @param anchor The place on the (unaltered) shape that you want the position of.
     * @return the point that is the absolute, centered position of this shape.
     */
    QPointF absolutePosition(KoFlake::Position anchor = KoFlake::CenteredPosition) const;

    /**
     * Move this shape to an absolute position where the end location will be the same
     * regardless of the shape's rotation/skew/scaling and regardless of this shape having
     * a parent (being in a group) or not.<br>
     * The newPosition is going to be the center of the shape.
     * This has the convenient effect that: <pre>
    shape-&gt;setAbsolutePosition(QPointF(0,0));
    shape-&gt;rotate(45);</pre>
        Will result in the same visual position of the shape as the opposite:<pre>
    shape-&gt;rotate(45);
    shape-&gt;setAbsolutePosition(QPointF(0,0));</pre>
     * @param newPosition the new absolute center of the shape.
     * @param anchor The place on the (unaltered) shape that you set the position of.
     */
    void setAbsolutePosition(const QPointF &newPosition, KoFlake::Position anchor = KoFlake::CenteredPosition);

    /**
     * Set a data object on the shape to be used by an application.
     * This is specifically useful when a shape is created in a plugin and that data from that
     * shape should be accessible outside the plugin.
     * @param userData the new user data, or 0 to delete the current one.
     */
    void setUserData(KoShapeUserData *userData);
    /**
     * Return the current userData.
     */
    KoShapeUserData *userData() const;

    /**
     * Set a data object on the shape to be used by an application.
     * This is specifically useful when an application wants to have data that is per shape
     * and should be deleted when the shape is destructed.
     * @param applicationData the new application data, or 0 to delete the current one.
     */
    void setApplicationData(KoShapeApplicationData *applicationData);

    /**
     * Return the current applicationData.
     */
    KoShapeApplicationData *applicationData() const;

    /**
     * Return the Id of this shape, identifying the type of shape by the id of the factory.
     * @see KoShapeFactoryBase::shapeId()
     * @return the id of the shape-type
     */
    QString shapeId() const;

    /**
     * Set the Id of this shape.  A shapeFactory is expected to set the Id at creation
     * so applications can find out what kind of shape this is.
     * @see KoShapeFactoryBase::shapeId()
     * @param id the ID from the factory that created this shape
     */
    void setShapeId(const QString &id);

    /**
     * Create a matrix that describes all the transformations done on this shape.
     *
     * The absolute transformation is the combined transformation of this shape
     * and all its parents and grandparents.
     *
     * @param converter if not null, this method uses the converter to mark the right
     *        offsets in the current view.
     */
    QTransform absoluteTransformation(const KoViewConverter *converter) const;

    /**
     * Applies a transformation to this shape.
     *
     * The transformation given is relative to the global coordinate system, i.e. the document.
     * This is a convenience function to apply a global transformation to this shape.
     * @see applyTransformation
     *
     * @param matrix the transformation matrix to apply
     */
    void applyAbsoluteTransformation(const QTransform &matrix);

    /**
     * Sets a new transformation matrix describing the local transformations on this shape.
     * @param matrix the new transformation matrix
     */
    void setTransformation(const QTransform &matrix);

    /// Returns the shapes local transformation matrix
    QTransform transformation() const;

    /**
     * Applies a transformation to this shape.
     *
     * The transformation given is relative to the shape coordinate system.
     *
     * @param matrix the transformation matrix to apply
     */
    void applyTransformation(const QTransform &matrix);

    /**
     * Copy all the settings from the parameter shape and apply them to this shape.
     * Settings like the position and rotation to visible and locked.  The parent
     * is a notable exclusion.
     * @param shape the shape to use as original
     */
    void copySettings(const KoShape *shape);

    /**
     * Convenience method that allows people implementing paint() to use the shape
     * internal coordinate system directly to paint itself instead of considering the
     * views zoom.
     * @param painter the painter to alter the zoom level of.
     * @param converter the converter for the current views zoom.
     */
    static void applyConversion(QPainter &painter, const KoViewConverter &converter);

    /**
     * @brief Transforms point from shape coordinates to document coordinates
     * @param point in shape coordinates
     * @return point in document coordinates
     */
    QPointF shapeToDocument(const QPointF &point) const;

    /**
     * @brief Transforms rect from shape coordinates to document coordinates
     * @param rect in shape coordinates
     * @return rect in document coordinates
     */
    QRectF shapeToDocument(const QRectF &rect) const;

    /**
     * @brief Transforms point from document coordinates to shape coordinates
     * @param point in document coordinates
     * @return point in shape coordinates
     */
    QPointF documentToShape(const QPointF &point) const;

    /**
     * @brief Transform rect from document coordinates to shape coordinates
     * @param rect in document coordinates
     * @return rect in shape coordinates
     */
    QRectF documentToShape(const QRectF &rect) const;

    /**
     * Returns the name of the shape.
     * @return the shapes name
     */
    QString name() const;

    /**
     * Sets the name of the shape.
     * @param name the new shape name
     */
    void setName(const QString &name);

    /**
     * Update the position of the shape in the tree of the KoShapeManager.
     */
    void notifyChanged();

    /**
     * A shape can be in a state that it is doing processing data like loading or text layout.
     * In this case it can be shown on screen probably partially but it should really not be printed
     * until it is fully done processing.
     * Warning! This method can be blocking for a long time
     * @param converter    The converter to convert between internal and view coordinates.
     * @param asynchronous If set to true the processing will can take place in a different thread and the
     *                     function will not block until the shape is finished.
     *                     In case of printing Flake will call this method from a non-main thread and only
     *                     start printing it when the in case of printing method returned.
     *                     If set to false the processing needs to be done synchronously and will
     *                     block until the result is finished.
     */
    virtual void waitUntilReady(const KoViewConverter &converter, bool asynchronous = true) const;

    /// checks recursively if the shape or one of its parents is not visible or locked
    bool isEditable() const;

    /**
     * Adds a shape which depends on this shape.
     * Making a shape dependent on this one means it will get shapeChanged() called
     * on each update of this shape.
     *
     * If this shape already depends on the given shape, establishing the
     * dependency is refused to prevent circular dependencies.
     *
     * @param shape the shape which depends on this shape
     * @return true if dependency could be established, otherwise false
     * @see removeDependee(), hasDependee()
     */
    bool addDependee(KoShape *shape);

    /**
     * Removes as shape depending on this shape.
     * @see addDependee(), hasDependee()
     */
    void removeDependee(KoShape *shape);

    /// Returns if the given shape is dependent on this shape
    bool hasDependee(KoShape *shape) const;

    /// Returns list of shapes depending on this shape
    QList<KoShape *> dependees() const;

    /// Returns additional snap data the shape wants to have snapping to
    virtual KoSnapData snapData() const;

    /**
     * Set additional attribute
     *
     * This can be used to attach additional attributes to a shape for attributes
     * that are application specific like presentation:placeholder
     *
     * @param name The name of the attribute in the following form prefix:tag e.g. presentation:placeholder
     * @param value The value of the attribute
     */
    void setAdditionalAttribute(const QString &name, const QString &value);

    /**
     * Remove additional attribute
     *
     * @param name The name of the attribute in the following form prefix:tag e.g. presentation:placeholder
     */
    void removeAdditionalAttribute(const QString &name);

    /**
     * Check if additional attribute is set
     *
     * @param name The name of the attribute in the following form prefix:tag e.g. presentation:placeholder
     *
     * @return true if there is a attribute with prefix:tag set, false otherwise
     */
    bool hasAdditionalAttribute(const QString &name) const;

    /**
     * Get additional attribute
     *
     * @param name The name of the attribute in the following form prefix:tag e.g. presentation:placeholder
     *
     * @return The value of the attribute if it exists or a null string if not found.
     */
    QString additionalAttribute(const QString &name) const;

    void setAdditionalStyleAttribute(const char *name, const QString &value);

    void removeAdditionalStyleAttribute(const char *name);

    QString additionalStyleAttribute(const QByteArray &name) const;

    QMap<QByteArray, QString> additionalStyleAttributes() const;

    /**
     * Returns the filter effect stack of the shape
     *
     * @return the list of filter effects applied on the shape when rendering.
     */
    KoFilterEffectStack *filterEffectStack() const;

    /// Sets the new filter effect stack, removing the old one
    void setFilterEffectStack(KoFilterEffectStack *filterEffectStack);

    /**
     * Set the property collision detection.
     * Setting this to true will result in calls to shapeChanged() with the CollisionDetected
     * parameter whenever either this or another shape is moved/rotated etc and intersects this shape.
     * @param detect if true detect collisions.
     */
    void setCollisionDetection(bool detect);

    /**
     * get the property collision detection.
     * @returns true if collision detection is on.
     */
    bool collisionDetection();

    /**
     * Return the tool delegates for this shape.
     * In Flake a shape being selected will cause the tool manager to make available all tools that
     * can edit the selected shapes.  In some cases selecting one shape should allow the tool to
     * edit a related shape be available too.  The tool delegates allows this to happen by taking
     * all the shapes in the set into account on tool selection.
     * Notice that if the set is non-empty 'this' shape is no longer looked at. You can choose
     * to add itself to the set too.
     */
    QSet<KoShape *> toolDelegates() const;

    /**
     * Set the tool delegates.
     * @param delegates the new delegates.
     * @see toolDelegates()
     */
    void setToolDelegates(const QSet<KoShape *> &delegates);

    /**
     * Return the hyperlink for this shape.
     */
    QString hyperLink() const;

    /**
     * Set hyperlink for this shape.
     * @param hyperLink name.
     */
    void setHyperLink(const QString &hyperLink);

    /**
     * Makes it possible for the user to delete this shape.
     * This parameter defaults to true.
     * @param deletable when true; set the shape to be deletable by the user.
     * @see isDeletable(), setGeometryProtected(), setContentProtected(), setSelectable()
     */
    void setDeletable(bool deletable);

    /**
     * Returns if this shape can be deleted by the user.
     * @return true only when this shape is deletable.
     * @see setDeletable(), isGeometryProtected(), isContentProtected(), isSelectable()
     */
    bool isDeletable() const;

    /// Sets the AllowedInteraction @p flag to @p value
    void setAllowedInteraction(AllowedInteraction flag, bool value);
    /// @return the AllowedInteraction state for @p flag.
    /// Convenience method that just calls allowedInteractions(recursive).testFlag(flag).
    /// @see allowedInteractions()
    bool allowedInteraction(AllowedInteraction flag, bool recursive = true) const;
    /// Sets the interactions the user is allowed to perform on this shape to @p interactions
    void setAllowedInteractions(AllowedInteractions interactions);
    /// @return the interactions the user is allowed to perform on this shape.
    /// If @p recursive is false, the shapes flags are returned as is. Use this e.g. for ui.
    /// If @p recursive is true:
    ///     If the shape is not visible, no interactions are allowed.
    ///     If there is a parent, the parent is checked.
    ///
    /// @see KoShapeContainer::allowedInteractions()
    AllowedInteractions allowedInteractions(bool recursive = true) const;

    /**
     * \internal
     * Returns the private object for use within the flake lib
     */
    KoShapePrivate *priv();

protected:
    /// constructor
    KoShape(KoShapePrivate &);

    /* ** loading saving helper methods */
    /// attributes from ODF 1.1 chapter 9.2.15 Common Drawing Shape Attributes
    enum OdfAttribute {
        OdfTransformation = 1, ///< Store transformation information
        OdfSize = 2, ///< Store size information
        OdfPosition = 8, ///< Store position
        OdfAdditionalAttributes = 4, ///< Store additional attributes of the shape
        OdfCommonChildElements = 16, ///< Event actions and connection points
        OdfLayer = 64, ///< Store layer name
        OdfStyle = 128, ///< Store the style
        OdfId = 256, ///< Store the unique ID
        OdfName = 512, ///< Store the name of the shape
        OdfZIndex = 1024, ///< Store the z-index
        OdfViewbox = 2048, ///< Store the viewbox

        /// A mask for all mandatory attributes
        OdfMandatories = OdfLayer | OdfStyle | OdfId | OdfName | OdfZIndex,
        /// A mask for geometry attributes
        OdfGeometry = OdfPosition | OdfSize,
        /// A mask for all the attributes
        OdfAllAttributes = OdfTransformation | OdfGeometry | OdfAdditionalAttributes | OdfMandatories | OdfCommonChildElements
    };

    /**
     * This method is used during loading of the shape to load common attributes
     *
     * @param context the KoShapeLoadingContext used for loading
     * @param element element which represents the shape in odf
     * @param attributes a number of OdfAttribute items to state which attributes to load.
     */
    bool loadOdfAttributes(const KoXmlElement &element, KoShapeLoadingContext &context, int attributes);

    /**
     * Parses the transformation attribute from the given string
     * @param transform the transform attribute string
     * @param context the loading context
     * @return the resulting transformation matrix
     */
    QTransform parseOdfTransform(const QString &transform, KoShapeLoadingContext &context);

    /**
     * @brief Saves the style used for the shape
     *
     * This method fills the given style object with the stroke and
     * background properties and then adds the style to the context.
     *
     * @param style the style object to fill
     * @param context used for saving
     * @return the name of the style
     * @see saveOdf
     */
    virtual QString saveStyle(KoGenStyle &style, KoShapeSavingContext &context) const;

    /**
     * Loads the stroke and fill style from the given element.
     *
     * @param element the xml element to  load the style from
     * @param context the loading context used for loading
     */
    virtual void loadStyle(const KoXmlElement &element, KoShapeLoadingContext &context);

    /// Loads the stroke style
    KoShapeStrokeModel *loadOdfStroke(const KoXmlElement &element, KoShapeLoadingContext &context) const;

    /// Loads the fill style
    QSharedPointer<KoShapeBackground> loadOdfFill(KoShapeLoadingContext &context) const;

    /// Loads the connection points
    void loadOdfGluePoints(const KoXmlElement &element, KoShapeLoadingContext &context);

    /// Loads the clip contour
    void loadOdfClipContour(const KoXmlElement &element, KoShapeLoadingContext &context, const QSizeF &scaleFactor);

    /* ** end loading saving */

    /**
     * A hook that allows inheriting classes to do something after a KoShape property changed
     * This is called whenever the shape, position rotation or scale properties were altered.
     * @param type an indicator which type was changed.
     * @param shape the shape.
     */
    virtual void shapeChanged(ChangeType type, KoShape *shape = nullptr);

    /// return the current matrix that contains the rotation/scale/position of this shape
    QTransform transform() const;

    KoShapePrivate *d_ptr;

private:
    Q_DECLARE_PRIVATE(KoShape)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KoShape::AllowedInteractions)

#endif
