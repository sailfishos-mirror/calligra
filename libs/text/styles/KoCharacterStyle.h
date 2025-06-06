/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2006-2009 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2007 Sebastian Sauer <mail@dipe.org>
 * SPDX-FileCopyrightText: 2007 Pierre Ducroquet <pinaraf@gmail.com>
 * SPDX-FileCopyrightText: 2008 Thorsten Zachmann <zachmann@kde.org>
 * SPDX-FileCopyrightText: 2008 Girish Ramakrishnan <girish@forwardbias.in>
 * SPDX-FileCopyrightText: 2008 Pierre Stirnweiss <pierre.stirnweiss_calligra@gadz.org>
 * SPDX-FileCopyrightText: 2011 Stuart Dickson <stuart@furkinfantasic.net>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#ifndef KOCHARACTERSTYLE_H
#define KOCHARACTERSTYLE_H

#include <KoXmlReaderForward.h>

#include "kotext_export.h"
#include <QObject>
#include <QTextCharFormat>

class QTextBlock;
class KoShapeLoadingContext;
class KoShadowStyle;
class KoGenStyle;
class QString;
class QChar;
class QVariant;

/**
 * A container for all properties for a character style.
 * A character style represents all character properties for a set of characters.
 * Each character in the document will have a character style, most of the time
 * shared with all the characters next to it that have the same style (see
 * QTextFragment).
 * In a document the instances of QTextCharFormat which are based on a
 * KoCharacterStyle have a property StyleId with an integer as value which
 * equals styleId() of that style.
 * @see KoStyleManager
 */
class KOTEXT_EXPORT KoCharacterStyle : public QObject
{
    Q_OBJECT
public:
    /// types of style
    enum Type {
        CharacterStyle,
        ParagraphStyle
    };

    /// list of character style properties we can store in a QTextCharFormat
    enum Property {
        StyleId = QTextFormat::UserProperty + 1, ///< The id stored in the charFormat to link the text to this style.
        HasHyphenation,
        StrikeOutStyle,
        StrikeOutType,
        StrikeOutColor,
        StrikeOutWidth,
        StrikeOutWeight,
        StrikeOutMode,
        StrikeOutText,
        OverlineStyle,
        OverlineType,
        OverlineColor,
        OverlineWidth,
        OverlineWeight,
        OverlineMode,
        UnderlineStyle,
        UnderlineType,
        UnderlineWidth,
        UnderlineWeight,
        UnderlineMode,
        Language,
        Country,
        FontCharset,
        TextRotationAngle,
        TextRotationScale,
        TextScale,
        InlineRdf, ///< KoTextInlineRdf pointer
        TextShadow,
        FontRelief,
        TextEmphasizeStyle,
        TextEmphasizePosition,
        TextCombine, ///< TextCombineType
        TextCombineStartChar, ///< QChar
        TextCombineEndChar, ///< QChar
        HyphenationPushCharCount, ///< int
        HyphenationRemainCharCount, ///< int
        FontLetterSpacing, ///< qreal, not the same format as the FontLetterSpacing in QTextFormat
        PercentageFontSize, // font-size can be in % and this stores that value
        AdditionalFontSize, // font-size-rel can specify an addition to the parent value
        UseWindowFontColor, // boolean, same as odf
        Blink,
        AnchorType, // valid only if QTextCharFormat::isAnchor() is true
        InlineInstanceId = 577297549, // Internal: Reserved for KoInlineTextObjectManager
        ChangeTrackerId = 577297550, // Internal: Reserved for ChangeTracker
        FontYStretch = 577297551 // Internal: Ratio between Linux font pt size and Windows font height
    };

    /// List of possible combine mode
    enum TextCombineType {
        NoTextCombine,
        TextCombineLetters,
        TextCombineLines
    };

    /// list of possible line type : no line, single line, double line
    enum LineType {
        NoLineType,
        SingleLine,
        DoubleLine
    };

    /// List of possible font relief : none, embossed, engraved
    enum ReliefType {
        NoRelief,
        Embossed,
        Engraved
    };

    enum EmphasisStyle {
        NoEmphasis,
        AccentEmphasis,
        CircleEmphasis,
        DiscEmphasis,
        DotEmphasis
    };

    enum EmphasisPosition {
        EmphasisAbove,
        EmphasisBelow
    };

    /// list of possible line style.
    enum LineStyle {
        NoLineStyle = Qt::NoPen,
        SolidLine = Qt::SolidLine,
        DottedLine = Qt::DotLine,
        DashLine = Qt::DashLine,
        DotDashLine = Qt::DashDotLine,
        DotDotDashLine = Qt::DashDotDotLine,
        LongDashLine,
        WaveLine
    };

    enum LineWeight {
        AutoLineWeight,
        NormalLineWeight,
        BoldLineWeight,
        ThinLineWeight,
        DashLineWeight, // ## ??what the heck does this mean??
        MediumLineWeight,
        ThickLineWeight,
        PercentLineWeight,
        LengthLineWeight
    };

    /// list of possible line modes.
    enum LineMode {
        NoLineMode,
        ContinuousLineMode,
        SkipWhiteSpaceLineMode
    };

    enum RotationScale {
        Fixed,
        LineHeight
    };

    enum AnchorTypes {
        Bookmark,
        Anchor // corresponds to text:a ODF element
    };

    /**
     * Constructor. Initializes with standard size/font properties.
     * @param parent the parent object for memory management purposes.
     */
    explicit KoCharacterStyle(QObject *parent = nullptr);
    /// Copy constructor
    explicit KoCharacterStyle(const QTextCharFormat &format, QObject *parent = nullptr);
    /// Destructor
    ~KoCharacterStyle() override;

    /// returns the type of style
    virtual Type styleType() const;

    /// set the default style this one inherits its unset properties from if no parent style.
    void setDefaultStyle(KoCharacterStyle *parent);

    /// set the parent style this one inherits its unset properties from.
    void setParentStyle(KoCharacterStyle *parent);

    /// return the parent style
    KoCharacterStyle *parentStyle() const;

    /// return the effective font for this style
    QFont font() const;

    /// See similar named method on QTextCharFormat
    void setFontFamily(const QString &family);
    /// See similar named method on QTextCharFormat
    QString fontFamily() const;
    /// See similar named method on QTextCharFormat
    void setFontPointSize(qreal size);
    /// remove the font point size attribute
    void clearFontPointSize();
    /// See similar named method on QTextCharFormat
    qreal fontPointSize() const;
    /// See similar named method on QTextCharFormat
    void setFontWeight(QFont::Weight weight);
    /// See similar named method on QTextCharFormat
    QFont::Weight fontWeight() const;
    /// See similar named method on QTextCharFormat
    void setFontItalic(bool italic);
    /// See similar named method on QTextCharFormat
    bool fontItalic() const;
    /*
    /// See similar named method on QTextCharFormat
    void setFontOverline(bool overline);
    /// See similar named method on QTextCharFormat
    bool fontOverline() const;
    */
    /// See similar named method on QTextCharFormat
    void setFontFixedPitch(bool fixedPitch);
    /// See similar named method on QTextCharFormat
    bool fontFixedPitch() const;
    /// See similar named method on QTextCharFormat
    void setVerticalAlignment(QTextCharFormat::VerticalAlignment alignment);
    /// See similar named method on QTextCharFormat
    QTextCharFormat::VerticalAlignment verticalAlignment() const;
    /// See similar named method on QTextCharFormat
    void setTextOutline(const QPen &pen);
    /// See similar named method on QTextCharFormat
    QPen textOutline() const;
    /// See similar named method on QTextCharFormat
    void setFontLetterSpacing(qreal spacing);
    /// See similar named method on QTextCharFormat
    qreal fontLetterSpacing() const;
    /// See similar named method on QTextCharFormat
    void setFontWordSpacing(qreal spacing);
    /// See similar named method on QTextCharFormat
    qreal fontWordSpacing() const;
    /// Set the text capitalization
    void setFontCapitalization(QFont::Capitalization capitalization);
    /// Return how the text should be capitalized
    QFont::Capitalization fontCapitalization() const;
    /// Set font Y stretch
    void setFontYStretch(qreal stretch);
    /// Return font Y stretch (value relevant for MS compatibility)
    qreal fontYStretch() const;

    /// See similar named method on QTextCharFormat
    void setFontStyleHint(QFont::StyleHint styleHint);
    /// See similar named method on QTextCharFormat
    QFont::StyleHint fontStyleHint() const;
    /// See similar named method on QTextCharFormat
    void setFontKerning(bool enable);
    /// See similar named method on QTextCharFormat
    bool fontKerning() const;

    /// See similar named method on QTextCharFormat
    void setBackground(const QBrush &brush);
    /// See similar named method on QTextCharFormat
    QBrush background() const;
    /// See similar named method on QTextCharFormat
    void clearBackground();

    /// See similar named method on QTextCharFormat
    void setForeground(const QBrush &brush);
    /// See similar named method on QTextCharFormat
    QBrush foreground() const;
    /// See similar named method on QTextCharFormat
    void clearForeground();

    /// Set the boolean of using window font color (see odf spec)
    void setFontAutoColor(bool use);

    /// Apply a font strike out style to this KoCharacterStyle
    void setStrikeOutStyle(LineStyle style);
    /// Get the current font strike out style of this KoCharacterStyle
    LineStyle strikeOutStyle() const;
    /// Apply a font strike out width to this KoCharacterStyle
    void setStrikeOutWidth(LineWeight weight, qreal width);
    /// Get the current font strike out width of this KoCharacterStyle
    void strikeOutWidth(LineWeight &weight, qreal &width) const;
    /// Apply a font strike out color to this KoCharacterStyle
    void setStrikeOutColor(const QColor &color);
    /// Get the current font strike out color of this KoCharacterStyle
    QColor strikeOutColor() const;
    /// Apply a font strike out color to this KoCharacterStyle
    void setStrikeOutType(LineType lineType);
    /// Get the current font strike out color of this KoCharacterStyle
    LineType strikeOutType() const;
    /// Apply a strike out mode of this KoCharacterStyle
    void setStrikeOutMode(LineMode lineMode);
    /// Get the current strike out mode of this KoCharacterStyle
    LineMode strikeOutMode() const;
    /// Apply a strike out text of this KoCharacterStyle
    void setStrikeOutText(const QString &text);
    /// Get the current strike out text of this KoCharacterStyle
    QString strikeOutText() const;

    /// Apply a font overline style to this KoCharacterStyle
    void setOverlineStyle(LineStyle style);
    /// Get the current font overline style of this KoCharacterStyle
    LineStyle overlineStyle() const;
    /// Apply a font overline width to this KoCharacterStyle
    void setOverlineWidth(LineWeight weight, qreal width);
    /// Get the current font overline width of this KoCharacterStyle
    void overlineWidth(LineWeight &weight, qreal &width) const;
    /// Apply a font overline color to this KoCharacterStyle
    void setOverlineColor(const QColor &color);
    /// Get the current font overline color of this KoCharacterStyle
    QColor overlineColor() const;
    /// Apply a font overline color to this KoCharacterStyle
    void setOverlineType(LineType lineType);
    /// Get the current font overline color of this KoCharacterStyle
    LineType overlineType() const;
    /// Apply a overline mode to this KoCharacterStyle
    void setOverlineMode(LineMode mode);
    /// Get the current overline mode of this KoCharacterStyle
    LineMode overlineMode() const;

    /// Apply a font underline style to this KoCharacterStyle
    void setUnderlineStyle(LineStyle style);
    /// Get the current font underline style of this KoCharacterStyle
    LineStyle underlineStyle() const;
    /// Apply a font underline width to this KoCharacterStyle
    void setUnderlineWidth(LineWeight weight, qreal width);
    /// Get the current font underline width of this KoCharacterStyle
    void underlineWidth(LineWeight &weight, qreal &width) const;
    /// Apply a font underline color to this KoCharacterStyle
    void setUnderlineColor(const QColor &color);
    /// Get the current font underline color of this KoCharacterStyle
    QColor underlineColor() const;
    /// Apply a font underline color to this KoCharacterStyle
    void setUnderlineType(LineType lineType);
    /// Get the current font underline color of this KoCharacterStyle
    LineType underlineType() const;
    /// Apply a underline mode to this KoCharacterStyle
    void setUnderlineMode(LineMode mode);
    /// Get the current underline mode of this KoCharacterStyle
    LineMode underlineMode() const;

    /// Apply text rotation angle to this KoCharacterStyle
    void setTextRotationAngle(qreal angle);
    /// Get the current text rotation angle of this KoCharacterStyle
    qreal textRotationAngle() const;
    /**
     *  RotationScale pecifies whether for rotated text the width of the text
     *  should be scaled to fit into the current line height or the width of the text
     *  should remain fixed, therefore changing the current line height
     */
    void setTextRotationScale(RotationScale scale);
    /// Get the current text rotation scale of this KoCharacterStyle
    RotationScale textRotationScale() const;
    /// Apply text scale to this KoCharacterStyle
    void setTextScale(int scale);
    /// Get the current text scale of this KoCharacterStyle
    int textScale() const;

    KoShadowStyle textShadow() const;
    void setTextShadow(const KoShadowStyle &shadow);

    TextCombineType textCombine() const;
    void setTextCombine(TextCombineType type);

    QChar textCombineStartChar() const;
    void setTextCombineStartChar(const QChar &character);

    QChar textCombineEndChar() const;
    void setTextCombineEndChar(const QChar &character);

    ReliefType fontRelief() const;
    void setFontRelief(ReliefType relief);

    EmphasisStyle textEmphasizeStyle() const;
    void setTextEmphasizeStyle(EmphasisStyle emphasis);

    EmphasisPosition textEmphasizePosition() const;
    void setTextEmphasizePosition(EmphasisPosition position);

    /// Set the country
    void setCountry(const QString &country);
    /// Set the language
    void setLanguage(const QString &language);
    /// Get the country
    QString country() const;
    /// Get the language
    QString language() const;

    bool blinking() const;
    void setBlinking(bool blink);

    void setHasHyphenation(bool on);
    bool hasHyphenation() const;

    void setHyphenationPushCharCount(int count);
    int hyphenationPushCharCount() const;

    void setHyphenationRemainCharCount(int count);
    int hyphenationRemainCharCount() const;

    void setPercentageFontSize(qreal percent);
    qreal percentageFontSize() const;

    void setAdditionalFontSize(qreal percent);
    qreal additionalFontSize() const;

    /// set the anchor type, valid only if QTextCharFormat::isAnchor() is true
    void setAnchorType(AnchorTypes anchorType);
    /// returns the anchor type, valid only if QTextCharFormat::isAnchor() is true
    AnchorTypes anchorType() const;

    void copyProperties(const KoCharacterStyle *style);
    void copyProperties(const QTextCharFormat &format);

    KoCharacterStyle *clone(QObject *parent = nullptr) const;

    /// return the name of the style.
    QString name() const;

    /// set a user-visible name on the style.
    void setName(const QString &name);

    /// each style has a unique ID (non persistent) given out by the styleManager
    int styleId() const;

    /// each style has a unique ID (non persistent) given out by the styleManager
    void setStyleId(int id);

    void unapplyStyle(QTextBlock &block) const;
    void unapplyStyle(QTextCharFormat &format) const;

    /**
     * Apply this style to a blockFormat by copying all properties from this
     * style to the target char format.
     */
    void applyStyle(QTextCharFormat &format, bool emitSignal = true) const;
    /**
     * Apply this style to the textBlock by copying all properties from this
     * style to the target block formats.
     */
    void applyStyle(QTextBlock &block) const;
    /**
     * Reset any styles and apply this style on the whole selection.
     */
    void applyStyle(QTextCursor *selection) const;

    /// This should be called after all charFormat properties are merged.
    void ensureMinimalProperties(QTextCharFormat &format) const;

    /**
     * Load the style form the element
     *
     * @param context the odf loading context
     * @param element the element containing the
     * @param loadParents true = use the stylestack, false = use just the element
     */
    void loadOdf(const KoXmlElement *element, KoShapeLoadingContext &context, bool loadParents = false);

    /// return true if this style has a non-default value set for the Property
    bool hasProperty(int key) const;

    bool compareCharacterProperties(const KoCharacterStyle &other) const;

    bool operator==(const KoCharacterStyle &other) const;

    bool operator!=(const KoCharacterStyle &other) const;

    /**
     * Removes properties from this style that have the same value in other style.
     */
    void removeDuplicates(const KoCharacterStyle &other);

    /**
     * Removes properties from this style that have the same value in other format.
     */
    void removeDuplicates(const QTextCharFormat &other_format);

    /**
     * Create an autostyle out of the format and baseFormat
     * @param format the format that is converted to an autostyle.
     * @param baseFormat the format (typically a blockCharFormat) that is the basis of the format,
     * but not itself part of the character style inheritance.
     * @return pointer to autostyle that has this as parent style
     */
    KoCharacterStyle *autoStyle(const QTextCharFormat &format, QTextCharFormat blockCharFormat) const;

    void saveOdf(KoGenStyle &style) const;

    /**
     * Returns true if this style has no properties set. Else, returns false.
     */
    bool isEmpty() const;

    /** Returns true if the style is in use.
     */
    bool isApplied() const;

    /**
     * Return the value of key as represented on this style.
     * You should consider using the direct accessors for individual properties instead.
     * @param key the Property to request.
     * @returns a QVariant which holds the property value.
     */
    QVariant value(int key) const;

    /**
     * Remove the hardcoded defaults from this style (SansSerif, 12 points, black).
     * @internal - this method is a bit of an ugly workaround to make it easier to
     * use KoTextLoader for loading richtext in Calligra Sheets, normally styles with
     * no font etc. set are not something you should want.
     */
    void removeHardCodedDefaults();

    void remove(int key);

Q_SIGNALS:
    void nameChanged(const QString &newName);
    void styleApplied(const KoCharacterStyle *) const;

protected:
    /**
     * Load the text properties from the \a KoStyleStack style stack using the
     * OpenDocument format.
     */
    void loadOdfProperties(KoShapeLoadingContext &context);

private:
    class Private;
    Private *const d;
};

Q_DECLARE_METATYPE(KoCharacterStyle *)
Q_DECLARE_METATYPE(const KoCharacterStyle *)
Q_DECLARE_METATYPE(QSharedPointer<KoCharacterStyle>)

#endif
