#include "lookandfeel.h"
#include "BinaryData.h"
#include "juce_gui_basics/juce_gui_basics.h"

const juce::Font track::ui::CustomLookAndFeel::getRobotoMonoThin() {
    static auto typeface = Typeface::createSystemTypefaceFor(
        BinaryData::RobotoMonoThin_ttf, BinaryData::RobotoMonoThin_ttfSize);

    return Font(typeface);
}

const juce::Font track::ui::CustomLookAndFeel::getInterRegular() {
    static auto typeface = Typeface::createSystemTypefaceFor(
        BinaryData::Inter_18ptRegular_ttf,
        BinaryData::Inter_18ptRegular_ttfSize);

    return Font(typeface);
}

const juce::Font track::ui::CustomLookAndFeel::getInterSemiBold() {
    static auto typeface = Typeface::createSystemTypefaceFor(
        BinaryData::Inter_18ptSemiBold_ttf,
        BinaryData::Inter_18ptSemiBold_ttfSize);

    return Font(typeface);
}

void track::ui::CustomLookAndFeel::drawRotarySlider(
    Graphics &g, int x, int y, int width, int height, float sliderPos,
    const float rotaryStartAngle, const float rotaryEndAngle, Slider &slider) {
    auto outline = slider.findColour(Slider::rotarySliderOutlineColourId);
    auto fill = slider.findColour(Slider::rotarySliderFillColourId);

    auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(10);

    auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle =
        rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    // auto lineW = jmin(8.0f, radius * 0.5f);
    auto lineW = 2;
    auto arcRadius = radius - lineW * 0.5f;

    // background circle
    g.setColour(juce::Colour(0xFF'818181));
    g.fillEllipse(
        bounds.getX() + 2, bounds.getY() + 2, (arcRadius * 1.7f) + 1.f,
        arcRadius * 1.7f); // probably not the best way to position the cirlce
                           // but i couldn't be bothered to implement a better
                           // way. this is now a problem for you, future john

    Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(),
                                arcRadius, arcRadius, 0.0f, rotaryStartAngle,
                                rotaryEndAngle, true);

    g.setColour(outline);
    g.strokePath(backgroundArc, PathStrokeType(lineW, PathStrokeType::curved,
                                               PathStrokeType::square));

    if (slider.isEnabled()) {
        Path valueArc;
        float pi = juce::MathConstants<float>::pi;
        valueArc.addCentredArc(bounds.getCentreX(), bounds.getCentreY(),
                               arcRadius, arcRadius, 0.0f, 2 * pi, toAngle,
                               true);

        g.setColour(fill);
        g.strokePath(valueArc, PathStrokeType(lineW, PathStrokeType::curved,
                                              PathStrokeType::square));
    }

    auto thumbWidth = lineW * 1.0f;
    Point<float> thumbPoint(
        bounds.getCentreX() +
            arcRadius * std::cos(toAngle - MathConstants<float>::halfPi),
        bounds.getCentreY() +
            arcRadius * std::sin(toAngle - MathConstants<float>::halfPi));

    g.setColour(slider.findColour(Slider::thumbColourId));
    g.fillEllipse(
        Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));

    // draw line from center to the thumb
    g.setColour(findColour(Slider::backgroundColourId));
    Path centerToThumbLine;

    juce::Line<float> line;
    line.setStart(bounds.getCentre());
    line.setEnd(thumbPoint);
    line = line.withShortenedStart(4.f);
    line = line.withShortenedEnd(.4f);

    centerToThumbLine.startNewSubPath(line.getStart());
    centerToThumbLine.lineTo(line.getEnd());
    centerToThumbLine.closeSubPath();
    g.strokePath(centerToThumbLine, PathStrokeType(2.f));
}

void track::ui::CustomLookAndFeel::drawButtonBackground(
    Graphics &g, Button &b, const Colour &backgroundColour,
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {

    // fill
    auto c = findColour(0x1004011);

    if (b.getButtonText().toLowerCase() == "config") {
        c = juce::Colour(0xFF'B5B5B5);
    }

    if (shouldDrawButtonAsHighlighted) {
        g.setColour(c.brighter(0.1f));
    } else {
        g.setColour(c);
    }

    g.fillRect(b.getLocalBounds());

    // don't draw border for certain buttons by checking its content. i
    // cannot think of another way that does not involve making my own button
    // class/making multiple lookandfeels outline
    if (b.getButtonText().toLowerCase() == "editor" ||
        b.getButtonText().toLowerCase() == "remove" ||
        b.getButtonText().toLowerCase() == "bypass" ||

        b.getButtonText().toLowerCase() == "config")
        return;

    // draw border
    g.setColour(c.darker(0.4f));
    g.drawRect(b.getLocalBounds(), 2);
}

// overriding this whole function just to change color of "config" button.
void track::ui::CustomLookAndFeel::drawButtonText(
    Graphics &g, TextButton &button, bool /*shouldDrawButtonAsHighlighted*/,
    bool /*shouldDrawButtonAsDown*/) {

    Font font(getTextButtonFont(button, button.getHeight()));
    g.setFont(font);
    g.setColour(button
                    .findColour(button.getToggleState()
                                    ? TextButton::textColourOnId
                                    : TextButton::textColourOffId)
                    .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));

    if (button.getButtonText().toLowerCase() == "config") {
        g.setColour(juce::Colours::black.withAlpha(0.5f));
    }

    const int yIndent = jmin(4, button.proportionOfHeight(0.3f));
    const int cornerSize = jmin(button.getHeight(), button.getWidth()) / 2;

    const int fontHeight = roundToInt(font.getHeight() * 0.6f);
    const int leftIndent =
        jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
    const int rightIndent = jmin(
        fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
    const int textWidth = button.getWidth() - leftIndent - rightIndent;

    if (textWidth > 0)
        g.drawFittedText(button.getButtonText(), leftIndent, yIndent, textWidth,
                         button.getHeight() - yIndent * 2,
                         Justification::centred, 2);
}

void track::ui::CustomLookAndFeel::drawLinearSlider(
    Graphics &g, int x, int y, int width, int height, float sliderPos,
    float minSliderPos, float maxSliderPos, const Slider::SliderStyle style,
    Slider &slider) {

    auto isTwoVal = (style == Slider::SliderStyle::TwoValueVertical ||
                     style == Slider::SliderStyle::TwoValueHorizontal);
    auto isThreeVal = (style == Slider::SliderStyle::ThreeValueVertical ||
                       style == Slider::SliderStyle::ThreeValueHorizontal);

    auto trackWidth = jmin(6.0f, slider.isHorizontal() ? (float)height * 0.25f
                                                       : (float)width * 0.25f);

    Point<float> startPoint(
        slider.isHorizontal() ? (float)x : (float)x + (float)width * 0.5f,
        slider.isHorizontal() ? (float)y + (float)height * 0.5f
                              : (float)(height + y));

    Point<float> endPoint(slider.isHorizontal() ? (float)(width + x)
                                                : startPoint.x,
                          slider.isHorizontal() ? startPoint.y : (float)y);

    Path backgroundTrack;
    backgroundTrack.startNewSubPath(startPoint);
    backgroundTrack.lineTo(endPoint);

    g.setColour(Colour(0xFF474748)); // fill color
    g.strokePath(backgroundTrack, {trackWidth + 5.f, PathStrokeType::curved,
                                   PathStrokeType::rounded});

    g.setColour(Colour(0xFF313330)); // outline color
    g.strokePath(backgroundTrack,
                 {trackWidth + 1.0f + 5.f, PathStrokeType::curved,
                  PathStrokeType::rounded});

    Point<float> minPoint, maxPoint, thumbPoint;

    if (isTwoVal || isThreeVal) {
        minPoint = {slider.isHorizontal() ? minSliderPos : (float)width * 0.5f,
                    slider.isHorizontal() ? (float)height * 0.5f
                                          : minSliderPos};

        if (isThreeVal)
            thumbPoint = {
                slider.isHorizontal() ? sliderPos : (float)width * 0.5f,
                slider.isHorizontal() ? (float)height * 0.5f : sliderPos};

        maxPoint = {slider.isHorizontal() ? maxSliderPos : (float)width * 0.5f,
                    slider.isHorizontal() ? (float)height * 0.5f
                                          : maxSliderPos};
    } else {
        auto kx = slider.isHorizontal() ? sliderPos
                                        : ((float)x + (float)width * 0.5f);
        auto ky = slider.isHorizontal() ? ((float)y + (float)height * 0.5f)
                                        : sliderPos;

        minPoint = startPoint;
        maxPoint = {kx, ky};
    }

    auto thumbWidth = getSliderThumbRadius(slider) * 1.4f;

    if (!isTwoVal) {
        // draw thumb fill
        g.setColour(juce::Colour(0x8F8F8F).withAlpha(.8f));
        g.fillEllipse(Rectangle<float>(static_cast<float>(thumbWidth),
                                       static_cast<float>(thumbWidth))
                          .withCentre(isThreeVal ? thumbPoint : maxPoint));
        // draw thumb outline
        g.setColour(juce::Colour(0xD9D9D9).withAlpha(1.f));
        g.drawEllipse(Rectangle<float>(static_cast<float>(thumbWidth),
                                       static_cast<float>(thumbWidth))
                          .withCentre(isThreeVal ? thumbPoint : maxPoint),
                      1.f);
    }

    if (isTwoVal || isThreeVal) {
        auto sr =
            jmin(trackWidth,
                 (slider.isHorizontal() ? (float)height : (float)width) * 0.4f);
        auto pointerColour = slider.findColour(Slider::thumbColourId);

        if (slider.isHorizontal()) {
            drawPointer(
                g, minSliderPos - sr,
                jmax(0.0f, (float)y + (float)height * 0.5f - trackWidth * 2.0f),
                trackWidth * 2.0f, pointerColour, 2);

            drawPointer(g, maxSliderPos - trackWidth,
                        jmin((float)(y + height) - trackWidth * 2.0f,
                             (float)y + (float)height * 0.5f),
                        trackWidth * 2.0f, pointerColour, 4);
        } else {
            drawPointer(
                g,
                jmax(0.0f, (float)x + (float)width * 0.5f - trackWidth * 2.0f),
                minSliderPos - trackWidth, trackWidth * 2.0f, pointerColour, 1);

            drawPointer(g,
                        jmin((float)(x + width) - trackWidth * 2.0f,
                             (float)x + (float)width * 0.5f),
                        maxSliderPos - sr, trackWidth * 2.0f, pointerColour, 3);
        }
    }
}

void track::ui::CustomLookAndFeel::drawPopupMenuBackground(Graphics &g,
                                                           int width,
                                                           int height) {
    g.setColour(findColour(PopupMenu::backgroundColourId));
    g.fillAll();

    g.setColour(findColour(PopupMenu::textColourId).withAlpha(.2f));
    g.drawRect(juce::Rectangle<int>(0, 0, width, height), 1);
}

void track::ui::CustomLookAndFeel::drawLabel(Graphics &g, Label &label) {
    g.fillAll(label.findColour(Label::backgroundColourId));

    auto textArea =
        getLabelBorderSize(label).subtractedFrom(label.getLocalBounds());

    if (!label.isBeingEdited()) {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        const Font font(getLabelFont(label));

        g.setColour(
            label.findColour(Label::textColourId).withMultipliedAlpha(alpha));
        g.setFont(font);

        g.drawText(label.getText(), textArea, label.getJustificationType(),
                   true);
        /*
        g.drawFittedText(
            label.getText(), textArea, label.getJustificationType(),
            jmax(1, (int)((float)textArea.getHeight() / font.getHeight())),
            label.getMinimumHorizontalScale());
            */

        g.setColour(label.findColour(Label::outlineColourId)
                        .withMultipliedAlpha(alpha));
    } else if (label.isEnabled()) {
        g.setColour(label.findColour(Label::outlineColourId));
    }

    // text editor moves by 1px and is annoying, check if y is odd/even to
    // make sure you move only y value onle once
    if (label.isBeingEdited()) {
        TextEditor *te = label.getCurrentTextEditor();
        te->setFont(label.getFont());

        // ABSOLUTE CINEMA.
        juce::Rectangle<int> bounds = te->getBounds();
        if (bounds.getY() % 2 == 0)
            bounds.setY(bounds.getY() - 1);

        te->setBounds(bounds);
    }

    // g.drawRect(label.getLocalBounds());
}

Font track::ui::CustomLookAndFeel::getPopupMenuFont() {
    return getInterRegular().withHeight(18.f).withExtraKerningFactor(-.03f);
}

Font track::ui::CustomLookAndFeel::getTextButtonFont(TextButton &button,
                                                     int buttonHeight) {
    if (button.getButtonText().toLowerCase() == "editor" ||
        button.getButtonText().toLowerCase() == "remove" ||
        button.getButtonText().toLowerCase() == "bypass" ||

        button.getButtonText().toLowerCase() == "config") {
        return getInterSemiBold()
            .withHeight((float)buttonHeight / 1.6f)
            .italicised()
            .boldened()
            .withExtraKerningFactor(-.03f);
    }

    // is length is <= 2 then it's one of those "icon" buttons like mute button
    // or FX chain button
    if (button.getButtonText().length() <= 2)
        return getInterSemiBold().withHeight((float)buttonHeight / 1.6f);

    return getInterSemiBold().withHeight((float)buttonHeight / 2.f);
}
