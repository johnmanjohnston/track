#pragma once
#include <JuceHeader.h>

namespace track {
class clip {
  public:
    juce::String path;
    float fadeIn = 0.f;
    float fadeOut = 0.f;
};

class ClipComponent : public juce::Component {
  public:
    ClipComponent();
    ~ClipComponent();

    void paint(juce::Graphics &g) override;
};

class track {
  public:
    bool s = false;
    bool m = false;

    // future john, have fun trying to implement hosting audio plugins :skull:

    std::vector<clip> clips;
};
} // namespace track
