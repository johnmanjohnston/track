#include "lookandfeel.h"

const juce::Font track::ui::CustomLookAndFeel::getRobotoMonoThin() {
    static auto typeface = Typeface::createSystemTypefaceFor(
        BinaryData::RobotoMonoThin_ttf, BinaryData::RobotoMonoThin_ttfSize);

    return Font(typeface);
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

    // if (slider.isBar())
    // drawLinearSliderOutline(g, x, y, width, height, style, slider);
}
