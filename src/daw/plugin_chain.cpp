#include "plugin_chain.h"

track::PluginChainComponent::PluginChainComponent() : juce::Component() {
    // setSize(500, 500);
}

track::PluginChainComponent::~PluginChainComponent() {}

void track::PluginChainComponent::paint(juce::Graphics &g) {
    int margin = 3;

    // bg
    g.fillAll(juce::Colour(0xFF'282828));

    // titlebar
    juce::Rectangle<int> titlebarBounds = getLocalBounds();
    titlebarBounds.setHeight(titlebarBounds.getHeight() / 7);
    titlebarBounds.reduce(margin, 0);
    g.setColour(juce::Colours::green);
    // g.fillRect(titlebarBounds);

    g.setFont(this->getInterBoldItalic().withHeight(19.f));
    g.setColour(juce::Colour(0xFF'A7A7A7)); // track name text color
    juce::String x = juce::String(getCorrespondingTrack()->clips.size());
    g.drawText(getCorrespondingTrack()->trackName + x,
               titlebarBounds.withLeft(50).withTop(2),
               juce::Justification::left);

    // fx logo
    /*
    int fxLogoDivider = 7;
    int fxLogoImageHeight = 100 / fxLogoDivider;
    int fxLogoImageWidth = 200 / fxLogoDivider;

    juce::Image fxLogo = juce::ImageCache::getFromMemory(
        BinaryData::FX_png, BinaryData::FX_pngSize);
    g.drawImageWithin(fxLogo, 6, 6, fxLogoImageWidth, fxLogoImageHeight,
                      juce::RectanglePlacement::yMid, false);
    */

    // outline
    juce::Rectangle<int> fxLogoBounds =
        juce::Rectangle<int>(10, 2, 30, titlebarBounds.getHeight());

    juce::Path textPath;
    juce::GlyphArrangement glyphs;
    glyphs.addFittedText(this->getInterBoldItalic().withHeight(22.f), "FX",
                         fxLogoBounds.getX(), fxLogoBounds.getY(),
                         fxLogoBounds.getWidth(), fxLogoBounds.getHeight() - 1,
                         juce::Justification::left, 2);
    glyphs.createPath(textPath);

    g.setColour(juce::Colour(0xFF'6C6C6C)); // "FX" text outline color
    juce::PathStrokeType strokeType(2.f);
    g.strokePath(textPath, strokeType);
    g.setColour(juce::Colours::black);
    g.fillPath(textPath);

    // gradient text fill
    // g.setFont(24.f);
    g.setFont(this->getInterBoldItalic().withHeight(22.f));
    juce::ColourGradient gradient = juce::ColourGradient::vertical(
        juce::Colours::white, .3f, juce::Colours::black,
        titlebarBounds.getHeight());
    g.setGradientFill(gradient);
    g.drawText("FX", fxLogoBounds, juce::Justification::left, false);

    // border
    g.setColour(juce::Colour(0xFF'4A4A4A));
    // g.drawHorizontalLine(titlebarBounds.getHeight(), 0, getWidth());
    g.fillRect(0, titlebarBounds.getHeight(), getWidth(), 2);
    g.drawRect(getLocalBounds(), 2);
}

void track::PluginChainComponent::mouseDrag(const juce::MouseEvent &event) {
    if (juce::ModifierKeys::currentModifiers.isAltDown()) {
        juce::Rectangle<int> newBounds = this->dragStartBounds;
        newBounds.setX(newBounds.getX() + event.getDistanceFromDragStartX());
        newBounds.setY(newBounds.getY() + event.getDistanceFromDragStartY());
        setBounds(newBounds);
    }
}

void track::PluginChainComponent::mouseDown(const juce::MouseEvent &event) {
    if (juce::ModifierKeys::currentModifiers.isAltDown())
        dragStartBounds = getBounds();
}

track::track *track::PluginChainComponent::getCorrespondingTrack() {
    return &processor->tracks[(size_t)trackIndex];
}
