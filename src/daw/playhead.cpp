#include "playhead.h"
#include "defs.h"

track::PlayheadComponent::PlayheadComponent() : juce::Component() {
    setBounds(10, 10, 10, 10);
}
track::PlayheadComponent::~PlayheadComponent() {}

void track::PlayheadComponent::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::orange);
}

void track::PlayheadComponent::updateBounds() {
    // DBG("playhead updatebounds() called");

    // assign current sample number, or resort to a fallback value
    int curSample = 44100;
    if (processor->getPlayHead() != nullptr &&
        processor->getPlayHead()->getPosition()->getBpm().hasValue()) {
        curSample =
            *processor->getPlayHead()->getPosition()->getTimeInSamples(); 
    }

    setBounds((curSample / 41000.0 * 32.0) + UI_TRACK_WIDTH -
                  tv->getViewPositionX(),
              UI_TOPBAR_HEIGHT, 2, 1080);
    repaint();
}
