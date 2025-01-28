#include "./timeline.h"

track::TimelineComponent::TimelineComponent() : juce::Component() {
    setSize(4000, 500);
};
track::TimelineComponent::~TimelineComponent(){};

track::TimelineViewport::TimelineViewport() : juce::Viewport() {}
track::TimelineViewport::~TimelineViewport() {}

void track::TimelineComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::black);

    g.setColour(juce::Colours::white);
    for (int i = 0; i < 10; ++i) {
        juce::Rectangle<int> newBounds = getLocalBounds();
        newBounds.setX(newBounds.getX() + (i * (zoomLevel * 100)));
        newBounds.setY(newBounds.getY() - (newBounds.getHeight() / 2.1f));

        g.drawText(juce::String(i), newBounds, juce::Justification::left,
                   false);
    }
}
