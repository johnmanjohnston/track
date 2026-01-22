#pragma once
#include "../editor.h"
#include "../processor.h"
#include "BinaryData.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include "subwindow.h"
#include "track.h"
#include <JuceHeader.h>

namespace track {

class pluginClipboardData {
  public:
    juce::String identifier = "unset path";
    juce::String data = "unset data";

    std::vector<relayParam> relayParams;
    bool bypassed = false;
    float dryWetMix = -1.f;
};

class pluginChainClipboardData {
  public:
    std::vector<pluginClipboardData> plugins;
};

class ActionAddPlugin : public juce::UndoableAction {
  public:
    std::vector<int> nodeRoute;
    juce::String pluginIdentifier = "unset identifier";
    void *p = nullptr;
    void *e = nullptr;

    ActionAddPlugin(std::vector<int> route, juce::String identifier,
                    void *processor, void *editor);
    ~ActionAddPlugin();

    bool validPlugin = false;

    bool perform() override;
    bool undo() override;
    void updateGUI();
};

class ActionRemovePlugin : public juce::UndoableAction {
  public:
    ActionRemovePlugin(pluginClipboardData data, std::vector<int> route,
                       int index, void *processor, void *editor);
    ~ActionRemovePlugin();

    pluginClipboardData subpluginData;
    std::vector<int> nodeRoute;
    void *p = nullptr;
    void *e = nullptr;
    int pluginIndex = -1;

    bool perform() override;
    bool undo() override;
    void updateGUI();

    std::vector<track::subplugin *> openedPlugins;
};

class ActionReorderPlugin : public juce::UndoableAction {
  public:
    ActionReorderPlugin(std::vector<int> nodeRoute, int sourceIndex,
                        int destinationIndex, void *processor, void *editor);
    ~ActionReorderPlugin();

    int srcIndex = -1;
    int destIndex = -1;
    std::vector<int> route;
    void *p = nullptr;
    void *e = nullptr;

    std::vector<track::subplugin *> openEditorsPlugins;
    std::vector<track::subplugin *> openRelayMenuPlugins;

    bool perform() override;
    bool undo() override;
    void updateGUI();
};

class ActionChangeTrivialPluginData : public juce::UndoableAction {
  public:
    ActionChangeTrivialPluginData(pluginClipboardData oldData,
                                  pluginClipboardData newData,
                                  std::vector<int> nodeRoute, int pluginIndex,
                                  void *processor, void *editor);
    ~ActionChangeTrivialPluginData();

    pluginClipboardData oldPluginData;
    pluginClipboardData newPluginData;
    std::vector<int> route;
    int index = -1;
    void *p = nullptr;
    void *e = nullptr;

    bool perform() override;
    bool undo() override;
    void updateGUI();
};

class PluginNodeComponent : public juce::Component {
  public:
    PluginNodeComponent();
    ~PluginNodeComponent();

    juce::TextButton openEditorBtn;
    juce::TextButton removePluginBtn;
    juce::TextButton bypassBtn;
    juce::TextButton automationButton;

    juce::Slider dryWetSlider;

    void setDryWetSliderValue();

    void copyPluginToClipboard();
    void removeThisPlugin();
    void toggleBypass();

    void paint(juce::Graphics &g) override;
    void resized() override;

    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;
    void mouseDown(const juce::MouseEvent &event) override;

    void focusGained(juce::Component::FocusChangeType /*cause*/) override {
        repaint();
    }

    void focusLost(juce::Component::FocusChangeType /*cause*/) override {
        repaint();
    }

    bool keyStateChanged(bool isKeyDown) override;

    int pluginIndex = -1;
    std::unique_ptr<track::subplugin> *getPlugin();
    bool getPluginBypassedStatus();

    float dryWetMixAtDragStart = -1.f;

    bool coolColors = false;
};

class PluginNodesWrapper : public juce::Component {
  public:
    PluginNodesWrapper();
    ~PluginNodesWrapper();

    InsertIndicator insertIndicator;

    void createPluginNodeComponents();
    PluginChainComponent *pcc = nullptr;

    std::vector<std::unique_ptr<PluginNodeComponent>> pluginNodeComponents;
    juce::KnownPluginList *knownPluginList = nullptr;

    void mouseDown(const juce::MouseEvent &event) override;

    juce::Rectangle<int> getBoundsForPluginNodeComponent(int index);

    juce::String rectToStr(juce::Rectangle<int> r) {
        juce::String retval = "x=" + juce::String(r.getX());
        retval += "; y=" + juce::String(r.getY());
        retval += "; width=" + juce::String(r.getWidth());
        retval += "; height=" + juce::String(r.getHeight());
        return retval;
    }
};

class PluginNodesViewport : public juce::Viewport {
  public:
    PluginNodesViewport();
    ~PluginNodesViewport();
};

class PluginChainComponent : public track::Subwindow {
  public:
    PluginChainComponent();
    ~PluginChainComponent();

    PluginNodesViewport nodesViewport;
    PluginNodesWrapper nodesWrapper;

    void paint(juce::Graphics &g) override;
    void resized() override;

    std::vector<int> route;
    AudioPluginAudioProcessor *processor = nullptr;
    audioNode *getCorrespondingTrack();

    void removePlugin(int pluginIndex);

    // track::InsertIndicator insertIndicator;
    void updateInsertIndicator(int index);
    void reorderPlugin(int srcIndex, int destIndex);
};

class PluginEditorWindow : public juce::Component, juce::Timer {
  public:
    PluginEditorWindow();
    ~PluginEditorWindow();

    CloseButton closeBtn;

    void timerCallback() override;

    void createEditor();
    void paint(juce::Graphics &g) override;
    void resized() override;

    void mouseUp(const juce::MouseEvent &event) override;
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    juce::Rectangle<int> dragStartBounds;

    std::vector<int> route;
    int pluginIndex = -1;

    // info to show on titlebar
    juce::String pluginName;
    juce::String pluginManufacturer;
    juce::String trackName;

    juce::AudioProcessorEditor *ape = nullptr;
    // std::unique_ptr<juce::AudioProcessorEditor> ape;

    AudioPluginAudioProcessor *processor = nullptr;
    audioNode *getCorrespondingTrack();
    std::unique_ptr<track::subplugin> *getPlugin();

    // this shouldn't belong in this class but whatever
    static const juce::Font getInterBoldItalic() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptBoldItalic_ttf,
            BinaryData::Inter_18ptBoldItalic_ttfSize);

        return typeface;
    }

    static const juce::Font getInterBold() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptRegular_ttf,
            BinaryData::Inter_18ptRegular_ttfSize);

        return typeface;
    }
};
} // namespace track
