#include "automation_relay.h"

float ::track::relayParam::getValueUsingPercentage(float min, float max) {
    float v = juce::jlimit(0.f, 100.f, this->value);
    float retval = -1.f;

    float mul = v / 100.f;
    return min + mul * (max - min);

    return retval;
}
