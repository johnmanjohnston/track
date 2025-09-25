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

        setColour(ComboBox::textColourId, juce::Colour(0xFF'E5E5E5));
        setColour(ComboBox::backgroundColourId, juce::Colour(0xFF'1F1F1F));
        setColour(ComboBox::outlineColourId, juce::Colour(0xFF'3F3F3F));

        setColour(PopupMenu::textColourId, juce::Colour(0xFF'E5E5E5));
        setColour(PopupMenu::backgroundColourId, juce::Colour(0xFF'1F1F1F));
        setColour(PopupMenu::highlightedBackgroundColourId,
                  juce::Colour(0xFF'454545));

        setColour(Label::outlineWhenEditingColourId,
                  juce::Colours::transparentWhite);
    }

    static const juce::Font getInterRegularScaledForPlatforms();
    static const juce::Font getRobotoMonoThin();
    static const juce::Font getInterRegular();
    static const juce::Font getInterSemiBold();

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

    void drawButtonText(Graphics &g, TextButton &button,
                        bool /*shouldDrawButtonAsHighlighted*/,
                        bool /*shouldDrawButtonAsDown*/) override;

    void drawComboBox(Graphics &g, int width, int height, bool, int, int, int,
                      int, ComboBox &box) override;

    void drawPopupMenuBackground(Graphics &g, int width, int height) override;

    void drawLabel(Graphics &g, Label &label) override;

    Font getPopupMenuFont() override;

    Font getComboBoxFont(ComboBox &) override;

    Font getTextButtonFont(TextButton &button, int buttonHeight) override;
};
} // namespace track::ui
