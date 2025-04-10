#include "plugin_chain.h"

track::PluginChainComponent::PluginChainComponent() : juce::Component() {
    // setSize(500, 500);
}

track::PluginChainComponent::~PluginChainComponent() {}

void track::PluginChainComponent::paint(juce::Graphics &g) {
    int margin = 4;

    // bg
    g.fillAll(juce::Colours::black);

    // titlebar
    juce::Rectangle<int> titlebarBounds = getLocalBounds();
    titlebarBounds.setHeight(titlebarBounds.getHeight() / 9);
    g.setColour(juce::Colours::green);
    g.fillRect(titlebarBounds);

    g.setColour(juce::Colours::white);
    juce::String x = juce::String(getCorrespondingTrack()->clips.size());
    g.drawText(getCorrespondingTrack()->trackName + x, titlebarBounds,
               juce::Justification::left);

    // border
    g.setColour(juce::Colours::red);
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
