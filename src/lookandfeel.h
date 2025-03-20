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
    }

    static const juce::Font getRobotoMonoThin();

    void drawLinearSlider(Graphics &g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos,
                          float maxSliderPos, const Slider::SliderStyle style,
                          Slider &slider) override;

    void drawButtonBackground(Graphics &g, Button &b,
                              const Colour &backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
};
} // namespace track::ui
