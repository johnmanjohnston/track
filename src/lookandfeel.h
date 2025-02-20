#pragma once
#include <JuceHeader.h>

namespace track::ui {
class CustomLookAndFeel : public juce::LookAndFeel_V4 {
  public:
    static const juce::Font getRobotoMonoThin();
};
}
