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

    /* FIXME: okay idfk this randomly crashed and luckily i was running under
       gdb so here's some info for you, future john:
logs:
        pure virtual method called
        terminate called without an active exception
        Thread 1 "track" received signal SIGABRT, Aborted
relevant backtrace section:

#7  0x00005555556726fe in track::PlayheadComponent::updateBounds
        (this=0x5555588421c0) at
/home/johnston/nerd/track/src/daw/playhead.cpp:24

#8 0x000055555563d5cc in
AudioPluginAudioProcessorEditor::timerCallback (this=0x555558840b70) at
/home/johnston/nerd/track/src/editor.h:111
        */

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
