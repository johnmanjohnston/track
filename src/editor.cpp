#include "editor.h"
#include "daw/defs.h"
#include "daw/timeline.h"
#include "daw/track.h"
#include "processor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p),
      masterSliderAttachment(*p.masterGain, masterSlider) {

    juce::ignoreUnused(processorRef);

    // sizing
    setResizable(true, true);
    setResizeLimits(540, 360, 1280, 720);
    setSize(1280, 720);

    // startTimer(120);

    // clipComponent.setBounds(200, 200, 200, 200);
    // addAndMakeVisible(clipComponent);

    // TimelineViewport holds TimelineComponent
    timelineComponent->viewport = &timelineViewport;
    timelineComponent->processorRef = &processorRef;
    timelineViewport.setViewedComponent(timelineComponent.get(), true);
    timelineViewport.setBounds(track::UI_TRACK_WIDTH, 55,
                               getWidth() - track::UI_TRACK_WIDTH, 665);
    addAndMakeVisible(timelineViewport);

    // tracks
    tracklist.setBounds(0, 60, track::UI_TRACK_WIDTH, 2000);
    // tracklist.addAndMakeVisible(_trackComponent);
    // _trackComponent.setBounds(10, 10, 100, 100);
    tracklist.processor = (void *)&processorRef;
    tracklist.createTrackComponents();

    trackViewport.setViewedComponent(&tracklist, false);
    trackViewport.setBounds(0, 60, track::UI_TRACK_WIDTH, 655);
    trackViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(trackViewport);

    trackViewport.timelineViewport = &timelineViewport;
    timelineViewport.trackViewport = &trackViewport;
    timelineViewport.tracklist = &tracklist;

    setLookAndFeel(&lnf);

    addAndMakeVisible(masterSlider);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {
    setLookAndFeel(nullptr);
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xFF3D3D3D));

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
    // like 6/8; that's now a problem for future me
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

    // === TOP BAR ===
    int topBarHeight = 50;

    g.setFont(lnf.getRobotoMonoThin());

    // draw top bar bg
    g.setColour(juce::Colour(0xFFCBCBCB));
    g.fillRect(0, 0, getWidth(), topBarHeight);

    // draw "track" logo
    g.setColour(juce::Colour(0xFF727272));
    g.setFont(24.f);
    g.drawFittedText("track", juce::Rectangle<int>(34, 1, 100, 50),
                     juce::Justification::left, 1);

    // draw time info bg
    g.setColour(juce::Colour(0xFF1B1E2D));
    int timeInfoBgWidth = 350;
    int timeInfoBgHeight = 40;

    juce::Rectangle<int> timeInfoRectangle =
        juce::Rectangle<int>((getWidth() / 2) - (timeInfoBgWidth / 2),
                             (topBarHeight / 2) - (timeInfoBgHeight / 2),
                             timeInfoBgWidth, timeInfoBgHeight);
    g.fillRoundedRectangle(timeInfoRectangle.toFloat(), 4.f);

    // draw text for time info
    // bar.beat.division
    g.setColour(juce::Colour(0xFFCDD8E4));
    juce::String tempoInformationToDisplay = juce::String(bar);
    tempoInformationToDisplay.append(".", 1);
    tempoInformationToDisplay.append(juce::String(beat), 3);
    tempoInformationToDisplay.append(".", 1);
    tempoInformationToDisplay.append(juce::String(division), 3);

    juce::Rectangle<int> timeInfoTextRectangle = timeInfoRectangle;
    timeInfoTextRectangle.setWidth(timeInfoTextRectangle.getWidth() - 10);
    timeInfoTextRectangle.setX(timeInfoTextRectangle.getX() + 10);
    g.drawFittedText(tempoInformationToDisplay, timeInfoTextRectangle,
                     juce::Justification::left, 1);
}

void AudioPluginAudioProcessorEditor::resized() {
    masterSlider.setBounds((getWidth() / 3) * 2, 2, 250, 70);
}
