#pragma once
#include "juce_gui_basics/juce_gui_basics.h"
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
    juce::Colour normalColor = juce::Colour(0xFF'585858).brighter();
    juce::Colour hoveredColor = juce::Colour(0xFF'808080).brighter();

    std::function<void()> onClose;
};

class Subwindow : public juce::Component {
  public:
    Subwindow() : juce::Component() {
        closeBtn.font = getInterBoldItalic();
        addAndMakeVisible(closeBtn);
    }
    ~Subwindow() {}

    CloseButton closeBtn;

    bool permitMovement = false;

    void mouseUp(const juce::MouseEvent &event) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    juce::Rectangle<int> dragStartBounds;
    juce::Rectangle<int> getTitleBarBounds();

    void resized() override;
    void paint(juce::Graphics &g) override;

    juce::Font getTitleBarFont() {
        return getInterBoldItalic().withHeight(19.f);
    }

    static const juce::Font getInterBoldItalic() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptBoldItalic_ttf,
            BinaryData::Inter_18ptBoldItalic_ttfSize);

        return typeface;
    }
};

class SubwindowChildFocusGrabber : public juce::Component {
  public:
    SubwindowChildFocusGrabber() : juce::Component() {}
    ~SubwindowChildFocusGrabber() {}

    void bringOverOtherSubwindows() {
        Subwindow *sw = findParentComponentOfClass<Subwindow>();
        sw->toFront(true);
        repaint();
    }

    void mouseDown(const juce::MouseEvent & /*event*/) override {
        bringOverOtherSubwindows();
    }

    void focusGained(juce::Component::FocusChangeType /*cause*/) override {
        bringOverOtherSubwindows();
    }

    void focusLost(juce::Component::FocusChangeType /*cause*/) override {
        bringOverOtherSubwindows();
    }
};
} // namespace track
