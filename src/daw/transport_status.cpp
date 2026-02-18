#include "transport_status.h"
#include "juce_graphics/juce_graphics.h"

track::TransportStatusComponent::TransportStatusComponent()
    : juce::Component() {}

track::TransportStatusComponent::~TransportStatusComponent(){};

void track::TransportStatusComponent::paint(juce::Graphics &g) {
    // this whole function is absolute cinema

    // get time info
    // TODO: timing calculations don't work properly for some time signatures
    // like 6/8; that's now a problem for future me
    int ppq = -1;
    int tempo = -1;

    int bar = -1;
    int beat = -1;
    int division = -1;
    juce::AudioPlayHead::TimeSignature timeSignature;

    double sampleRate = processorRef->getSampleRate();
    double curSample = -1;

    // check if host DAW supplies tempo to make sure that the host DAW actually
    // exists, because getPlayHead() doesn't return nullptr sometimes for some
    // reason, leading to segfault
    if (processorRef->getPlayHead() != nullptr &&
        processorRef->getPlayHead()->getPosition()->getBpm().hasValue()) {
        // yoink data from host DAW
        curSample =
            *processorRef->getPlayHead()->getPosition()->getTimeInSamples();
        tempo = *processorRef->getPlayHead()->getPosition()->getBpm();
        timeSignature =
            *processorRef->getPlayHead()->getPosition()->getTimeSignature();
        ppq = *processorRef->getPlayHead()->getPosition()->getPpqPosition();

        // bar, beat, division calculations
        // https://music.stackexchange.com/questions/109729/how-to-figure-out-the-length-time-in-ms-of-a-bar-from-bpm-and-time-signature
        // 4 * N / D = length of bar in quarter notes (@Bavi_H's answer)
        int barLength = 4 * timeSignature.numerator / timeSignature.denominator;
        bar = static_cast<int>(ppq / barLength) + 1;
        beat = (ppq % timeSignature.numerator) + 1;

        // calculate 8th note divisions from sample counts
        float beatDuration = 60.f / tempo;

        float mul = 4.f / timeSignature.denominator;
        float effectiveBeatDuration = beatDuration * mul;

        float elapsedTime = curSample / sampleRate;

        int totalDivisions = ((elapsedTime / effectiveBeatDuration) * 2);
        int divisionPerBar = timeSignature.numerator * 2;
        division = (totalDivisions % divisionPerBar) + 1;
    }

    float cornerSize = 4.f;

    // fill bg color
    g.setColour(juce::Colour(0xFF1B1E2D));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerSize);

    juce::ColourGradient topGradient = juce::ColourGradient(
        juce::Colour(0xFF'414245), 0.f, 0.f, juce::Colour(0xFF'282A30), 0.f,
        (float)getHeight() / 2, false);
    g.setGradientFill(topGradient);
    g.fillRect(0, 0, getWidth(), getHeight() / 2);

    juce::ColourGradient bottomGradient = juce::ColourGradient(
        juce::Colour(0xFF'191B1F), 0.f, (float)getHeight() / 2,
        juce::Colour(0xFF'24262C), 0.f, getHeight(), false);
    g.setGradientFill(bottomGradient);
    g.fillRect(0, getHeight() / 2, getWidth(), getHeight() / 2);

    // outline
    g.setColour(juce::Colour(0xFF'1A1A18));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), cornerSize, 1.f);

    g.setColour(juce::Colour(0xFFCDD8E4)); // text color
    juce::Rectangle<int> timeInfoTextRectangle = getLocalBounds();
    timeInfoTextRectangle.reduce(14, 0);

    // draw text for time info
    // bar.beat.division
    juce::String timeInfoToDisplay = juce::String(bar);
    timeInfoToDisplay.append(".", 1);
    timeInfoToDisplay.append(juce::String(beat), 3);
    timeInfoToDisplay.append(".", 1);
    timeInfoToDisplay.append(juce::String(division), 3);

    juce::String tempoToDisplay;
    tempoToDisplay.append(juce::String(tempo), 4);

    juce::String timeSignatureInfoToDisplay;
    timeSignatureInfoToDisplay.append(juce::String(timeSignature.numerator), 3);
    timeSignatureInfoToDisplay.append("/", 1);
    timeSignatureInfoToDisplay.append(juce::String(timeSignature.numerator), 3);

    g.setColour(juce::Colour(0xFF'CDD8E4).withAlpha(0.7f));
    g.setFont(
        getRobotoMonoThin().withHeight(26.f).boldened().withExtraKerningFactor(
            -.06f));

    g.drawText(timeInfoToDisplay, timeInfoTextRectangle,
               juce::Justification::left, false);

    g.drawText(tempoToDisplay, timeInfoTextRectangle,
               juce::Justification::horizontallyCentred, false);

    g.drawText(timeSignatureInfoToDisplay, timeInfoTextRectangle,
               juce::Justification::right, false);
}
