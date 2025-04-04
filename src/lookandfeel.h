#pragma once
#include <JuceHeader.h>

using namespace juce;

namespace track::ui {
class CustomLookAndFeel : public juce::LookAndFeel_V4 {
  public:
    CustomLookAndFeel() {
        // slider track
        setColour(0x1000400, juce::Colour(0xFF2A2A2A));

        // button colours
        // https://docs.juce.com/master/classDrawableButton.html
        //
        // button bg
        setColour(0x1004011, juce::Colour(0xFF'535353));

        // future john here. i forgot the enum names correspond to the color IDs
        setColour(Slider::rotarySliderFillColourId, juce::Colour(0xFF'67FF76));
        setColour(Slider::thumbColourId, juce::Colour(0xFF'67FF76));
        setColour(Slider::trackColourId, juce::Colour(0xFF'303030));
    }

    static const juce::Font getRobotoMonoThin();

    void drawLinearSlider(Graphics &g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos,
                          float maxSliderPos, const Slider::SliderStyle style,
                          Slider &slider) override;

    void drawRotarySlider(Graphics &, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, Slider &) override;

    void drawButtonBackground(Graphics &g, Button &b,
                              const Colour &backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
};
} // namespace track::ui
