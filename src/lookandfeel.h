#pragma once
#include <JuceHeader.h>

using namespace juce;

namespace track::ui {
class CustomLookAndFeel : public juce::LookAndFeel_V4 {
  public:
    static const juce::Font getRobotoMonoThin();

    void drawLinearSlider(Graphics &g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos,
                          float maxSliderPos, const Slider::SliderStyle style,
                          Slider &slider) override;
};
}
