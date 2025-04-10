#include "plugin_chain.h"

track::PluginChainComponent::PluginChainComponent() : juce::Component() {
    // setSize(500, 500);
}

track::PluginChainComponent::~PluginChainComponent() {}

void track::PluginChainComponent::paint(juce::Graphics &g) {
    int margin = 2;

    // bg
    g.fillAll(juce::Colour(0xFF'282828));

    // titlebar
    juce::Rectangle<int> titlebarBounds = getLocalBounds();
    titlebarBounds.setHeight(titlebarBounds.getHeight() / 9);
    titlebarBounds.reduce(margin, 0);
    g.setColour(juce::Colours::green);
    // g.fillRect(titlebarBounds);

    g.setColour(juce::Colours::white);
    juce::String x = juce::String(getCorrespondingTrack()->clips.size());
    g.drawText(getCorrespondingTrack()->trackName + x,
               titlebarBounds.withLeft(50), juce::Justification::left);

    // fx logo
    float fxLogoDivider = 3.f;
    juce::Image fxLogo = juce::ImageCache::getFromMemory(
        BinaryData::FX_png, BinaryData::FX_pngSize);
    g.drawImageWithin(fxLogo, 0, 0, 90.f / fxLogoDivider, 49.f / fxLogoDivider,
                      juce::RectanglePlacement::yMid, true);

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
