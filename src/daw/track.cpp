// TODO: this file
//

#include "track.h"
track::ClipComponent::ClipComponent() : juce::Component() {}
track::ClipComponent::~ClipComponent() {}

void track::ClipComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::red);
    g.fillAll();

    // DBG("ClipComponent::paint() called");
}
