#include "editor.h"
#include "juce_graphics/juce_graphics.h"
#include "processor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p) {
    juce::ignoreUnused(processorRef);

    // sizing
    setResizable(true, true);
    setResizeLimits(540, 360, 1280, 720);
    setSize(1280, 720);

    startTimer(20);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g) {
    // (Our component is opaque, so we must completely fill the background with
    // a solid colour)
    g.fillAll(
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);

    juce::String tmp;

    /*
    if (isPlaying)
        tmp = "is playing";
    else
        tmp = "NOT playing";
    */

    int ppq = -1;
    int timeInSeconds = -1;
    int tempo = -1;
    int bar = -1;
    juce::AudioPlayHead::TimeSignature timeSignature;

    if (processorRef.getPlayHead() != nullptr) {
        tmp = juce::String(
            *processorRef.getPlayHead()->getPosition()->getTimeInSamples());

        // bar = *processorRef.getPlayHead()->getPosition()->getBarCount();
        tempo = *processorRef.getPlayHead()->getPosition()->getBpm();
        timeInSeconds =
            *processorRef.getPlayHead()->getPosition()->getTimeInSeconds();
        timeSignature =
            *processorRef.getPlayHead()->getPosition()->getTimeSignature();
        ppq = *processorRef.getPlayHead()->getPosition()->getPpqPosition();

        // https://music.stackexchange.com/questions/109729/how-to-figure-out-the-length-time-in-ms-of-a-bar-from-bpm-and-time-signature
        // 4 * N / D = length of bar in quarter notes (@Bavi_H's answer)
        int barLength = 4 * timeSignature.numerator / timeSignature.denominator;
        bar = static_cast<int>(ppq / barLength);
    }

    g.drawFittedText(tmp, getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(24.f);
    g.drawFittedText("track", juce::Rectangle<int>(34, 8, 100, 50),
                     juce::Justification::left, 1);

    g.drawFittedText(juce::String(bar), juce::Rectangle<int>(300, 8, 100, 50),
                     juce::Justification::left, 1);
}

void AudioPluginAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
