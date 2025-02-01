#include "editor.h"
#include "daw/timeline.h"
#include "daw/track.h"
#include "processor.h"

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p), thumbnailCache(5),
      thumbnail(512, 
        audioFormatManager, thumbnailCache),
      _trackComponent(&processorRef.tracks[0]) {

    juce::ignoreUnused(processorRef);

    // sizing
    setResizable(true, true);
    setResizeLimits(540, 360, 1280, 720);
    setSize(1280, 720);

    startTimer(20);

    // clipComponent.setBounds(200, 200, 200, 200);
    // addAndMakeVisible(clipComponent);

    // TimelineViewport holds TimelineComponent
    track::TimelineComponent *x = new track::TimelineComponent;
    x->viewport = &timelineViewport;
    timelineViewport.setViewedComponent(x, true);
    timelineViewport.setBounds(170, 60, 1110 - 1, 650);
    addAndMakeVisible(timelineViewport);

    // tracks
    tracklist.setBounds(0, 60, 170 - 10, 2000);
    tracklist.addAndMakeVisible(_trackComponent);
    _trackComponent.setBounds(10, 10, 100, 100);

    trackViewport.setViewedComponent(&tracklist, false);
    trackViewport.setBounds(0, 60, 170, 650);
    addAndMakeVisible(trackViewport);

    trackViewport.timelineViewport = &timelineViewport;
    timelineViewport.trackViewport = &trackViewport;

    // draw audio thumbnail
    thumbnail.setSource(&processorRef.tracks[0].clips[0].buffer, processorRef.getSampleRate(), 1);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g) {
    // (Our component is opaque, so we must completely fill the background with
    // a solid colour)
    g.fillAll(
        getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);


    // draw waveform
    juce::Rectangle<int> thumbnailBounds(0, 0, 400,
                                         200);
    
    if (thumbnail.getNumChannels() != 0) {
         g.drawText("AUDIO THYUMBNAIL LOADED", 0, 0, 50, 10,
                    juce::Justification::left, true);

         thumbnail.drawChannels(g, thumbnailBounds, 0.0,
                                thumbnail.getTotalLength(), 1.f); 
    }

    else {
         g.drawText("no thyumbnail :(", 0, 0, 50, 10, juce::Justification::left,
                    true); 
    }


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
