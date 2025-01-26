#include "./timeline.h"

track::TimelineComponent::TimelineComponent() : juce::Component(){};
track::TimelineComponent::~TimelineComponent(){};

void track::TimelineComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::black);
}
