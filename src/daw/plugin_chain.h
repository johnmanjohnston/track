#include <JuceHeader.h>

namespace track {
class PluginChainComponent : public juce::Component {
  public:
    PluginChainComponent();
    ~PluginChainComponent();

    void paint(juce::Graphics &g) override;
};
} // namespace track
