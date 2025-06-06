#include "playhead.h"
#include "defs.h"

track::PlayheadComponent::PlayheadComponent() : juce::Component() {
    setBounds(10, 10, 10, 10);
}
track::PlayheadComponent::~PlayheadComponent() {}

void track::PlayheadComponent::paint(juce::Graphics &g) {
    if (getBounds().getX() < UI_TRACK_WIDTH)
        return;

    g.fillAll(juce::Colour(0xFF'E9FFFF));
    g.setColour(juce::Colours::black);
    g.drawRect(getLocalBounds());
}

void track::PlayheadComponent::updateBounds() {
    // DBG("playhead updatebounds() called");

    // assign current sample number, or resort to a fallback value
    int curSample = 44100;
    if (processor->getPlayHead() != nullptr &&
        processor->getPlayHead()->getPosition()->getBpm().hasValue()) {
        curSample =
            *processor->getPlayHead()->getPosition()->getTimeInSamples();

        BPM = *processor->getPlayHead()->getPosition()->getBpm();
    } else
        BPM = 120;

    setBounds((curSample / SAMPLE_RATE * UI_ZOOM_MULTIPLIER) + UI_TRACK_WIDTH -
                  tv->getViewPositionX(),
              UI_TOPBAR_HEIGHT + 5, 3, 720 - UI_TOPBAR_HEIGHT - 5 - 8);
    repaint();
}
