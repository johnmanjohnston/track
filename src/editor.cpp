#include "editor.h"
#include "daw/timeline.h"
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

    // clipComponent.setBounds(200, 200, 200, 200);
    // addAndMakeVisible(clipComponent);

    // TODO: don't use new here
    timelineViewport.setViewedComponent(new track::TimelineComponent, true);
    timelineViewport.setBounds(170, 50 + 8, 1090, 650 + 8);
    addAndMakeVisible(timelineViewport);
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

    // TODO: timing calculations don't work properly for some time signatures
    // like 6/8; that's now a prolem for future me
    int ppq = -1;
    int timeInSeconds = -1;
    int tempo = -1;

    int bar = -1;
    int beat = -1;
    int division = -1;
    juce::AudioPlayHead::TimeSignature timeSignature;

    double sampleRate = processorRef.getSampleRate();
    double curSample = -1;
    double curSecond = -1;

    // check if host DAW supplies tempo to make sure that the host DAW actually
    // exists, because getPlayHead() doesn't return nullptr sometimes for some
    // reason, leading to segfault
    if (processorRef.getPlayHead() != nullptr &&
        processorRef.getPlayHead()->getPosition()->getBpm().hasValue()) {
        tmp = juce::String(
            *processorRef.getPlayHead()->getPosition()->getTimeInSamples());

        // yoink data from host DAW
        curSample =
            *processorRef.getPlayHead()->getPosition()->getTimeInSamples();
        curSecond =
            *processorRef.getPlayHead()->getPosition()->getTimeInSeconds();
        tempo = *processorRef.getPlayHead()->getPosition()->getBpm();
        timeInSeconds =
            *processorRef.getPlayHead()->getPosition()->getTimeInSeconds();
        timeSignature =
            *processorRef.getPlayHead()->getPosition()->getTimeSignature();
        ppq = *processorRef.getPlayHead()->getPosition()->getPpqPosition();

        // bar, beat, division calculations
        // https://music.stackexchange.com/questions/109729/how-to-figure-out-the-length-time-in-ms-of-a-bar-from-bpm-and-time-signature
        // 4 * N / D = length of bar in quarter notes (@Bavi_H's answer)
        int barLength = 4 * timeSignature.numerator / timeSignature.denominator;
        bar = static_cast<int>(ppq / barLength) + 1;
        beat = (ppq % timeSignature.numerator) + 1;

        // calculate 8th note divisions from sample counts
        float eigthNoteLengthInSeconds = (60.f * 1.f / tempo) * .5f;
        float beatDuration = 60.f / tempo;

        float mul = 4.f / timeSignature.denominator;
        float effectiveBeatDuration = beatDuration * mul;

        float elapsedTime = curSample / sampleRate;

        int totalDivisions = ((elapsedTime / effectiveBeatDuration) * 2);
        int divisionPerBar = timeSignature.numerator * 2;
        division = (totalDivisions % divisionPerBar) + 1;
    }

    g.drawFittedText(tmp, getLocalBounds(), juce::Justification::centred, 1);

    g.setFont(24.f);
    g.drawFittedText("track", juce::Rectangle<int>(34, 8, 100, 50),
                     juce::Justification::left, 1);

    // bar.beat.division
    juce::String tempoInformationToDisplay = juce::String(bar);
    tempoInformationToDisplay.append(".", 1);
    tempoInformationToDisplay.append(juce::String(beat), 3);
    tempoInformationToDisplay.append(".", 1);
    tempoInformationToDisplay.append(juce::String(division), 3);

    g.drawFittedText(tempoInformationToDisplay,
                     juce::Rectangle<int>(300, 8, 100, 50),
                     juce::Justification::left, 1);
}

void AudioPluginAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
