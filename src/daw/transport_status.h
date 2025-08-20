#pragma once
#include "../processor.h"
#include <JuceHeader.h>

namespace track {
class TransportStatusComponent : public juce::Component {
  public:
    TransportStatusComponent();
    ~TransportStatusComponent();

    void paint(juce::Graphics &g) override;

    AudioPluginAudioProcessor *processorRef = nullptr;


    // visual studio is an absolute truckload of shit
    juce::Font getRobotoMonoThin() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::RobotoMonoThin_ttf, BinaryData::RobotoMonoThin_ttfSize);

        return Font(typeface);
    }
};
} // namespace track
