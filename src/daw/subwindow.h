#pragma once
#include <JuceHeader.h>

namespace track {

class CloseButton : public juce::Component {
  public:
    CloseButton();
    ~CloseButton();

    juce::Font font;
    void paint(juce::Graphics &g) override;
    void mouseUp(const juce::MouseEvent &event) override;

    bool isHoveredOver = false;
    void mouseEnter(const juce::MouseEvent &event) override;
    void mouseExit(const juce::MouseEvent &event) override;
    juce::Colour normalColor = juce::Colour(0xFF'585858);
    juce::Colour hoveredColor = juce::Colour(0xFF'808080);

    bool behaveLikeANormalCloseButton = true;
};

class Subwindow : public juce::Component {
  public:
    Subwindow() : juce::Component() {
        closeBtn.font = getInterBoldItalic();
        addAndMakeVisible(closeBtn);
    }
    ~Subwindow() {}

    CloseButton closeBtn;

    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    juce::Rectangle<int> dragStartBounds;
    juce::Rectangle<int> getTitleBarBounds();

    void resized() override;
    void paint(juce::Graphics &g) override;

    static const juce::Font getInterBoldItalic() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptBoldItalic_ttf,
            BinaryData::Inter_18ptBoldItalic_ttfSize);

        return typeface;
    }
};
} // namespace track
