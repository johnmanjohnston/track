#include "playhead.h"
#include "defs.h"
#include "utility.h"

track::PlayheadComponent::PlayheadComponent() : juce::Component() {
    setBounds(10, 10, 10, 10);
    setOpaque(false);
}
track::PlayheadComponent::~PlayheadComponent() {}

void track::PlayheadComponent::paint(juce::Graphics &g) {
    if (getBounds().getX() < UI_TRACK_WIDTH)
        return;

    juce::Rectangle<int> lineBounds = getLocalBounds()
                                          .withSizeKeepingCentre(3, getHeight())
                                          .withTrimmedTop(2);

    g.setColour(juce::Colour(0xAA'FFFFFF));
    g.fillRect(lineBounds);

    g.setColour(juce::Colours::black);
    g.drawRect(lineBounds);

    track::utility::customDrawGlassPointer(g, 0.f, 2.f, w - 1,
                                           juce::Colours::white, 0.5f, 2);
}

void track::PlayheadComponent::updateBounds() {
    /* okay idfk this randomly crashed and luckily i was running under
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
                  tv->getViewPositionX() - (w / 2.f),
              UI_TOPBAR_HEIGHT + 7, w, 720 - UI_TOPBAR_HEIGHT - 5 - 8);
    repaint();
}
