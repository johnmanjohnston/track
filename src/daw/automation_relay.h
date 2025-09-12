#pragma once
#include <JuceHeader.h>

namespace track {
class relayParam {
  public:
    relayParam() {}
    ~relayParam() {}

    juce::String pluginsParamID;
    int outputParamID = -1;

    float value = -1.f;

    float getValueUsingPercentage(float min, float max);
};
} // namespace track
