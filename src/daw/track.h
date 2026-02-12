#pragma once
#include "BinaryData.h"
#include "subwindow.h"
#include <JuceHeader.h>

namespace track {
class clip {
  public:
    juce::String path;
    juce::String name = "untitled clip";

    int startPositionSample = -1; // absolute sample position within host
    bool active = true;

    float gain = 1.f;

    int trimLeft = 0;
    int trimRight = 0;

    juce::AudioBuffer<float> buffer;
    void updateBuffer();
};

struct clipCoordinate {
    std::vector<int> route;
    int pluginIndex = -1;
};

class ClipComponent : public juce::Component, public juce::ChangeListener {
  public:
    ClipComponent(clip *c, int clipHash);
    ~ClipComponent();

    juce::AudioThumbnailCache thumbnailCache;
    juce::AudioThumbnail thumbnail;
    juce::AudioFormatManager afm;
    void changeListenerCallback(ChangeBroadcaster *source) override;

    clip *correspondingClip = nullptr;

    void paint(juce::Graphics &g) override;
    bool stale = true;
    int nodeDisplayIndex = -1;

    void copyClip();

    // moving clips
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;

    void mouseEnter(const juce::MouseEvent &) override { repaint(); }
    void mouseExit(const juce::MouseEvent &) override {
        drawTrimHandles = 0;
        repaint();
    }
    void mouseMove(const juce::MouseEvent &event) override;

    bool keyStateChanged(bool isKeyDown) override;
    void focusLost(juce::Component::FocusChangeType cause) override {
        DBG("focus lost");
        repaint();
    };

    bool isBeingDragged = false;
    int startDragX = -1;
    int startDragStartPositionSample = -1;
    int startTrimLeftPositionSample = -1;
    int startTrimRightPositionSample = -1;
    int mouseClickX = -1;
    int trimMode = 0; // -1 = left, 0 = none, 1 = right

    int drawTrimHandles = 0; // -1 for left, 1 for right, 0 for neither
    bool reachedLeft = false;

    int finalEndPos = -1;

    // this function would ideally not be in this class but whatever
    juce::Font getInterSemiBold() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptSemiBold_ttf,
            BinaryData::Inter_18ptSemiBold_ttfSize);

        return Font(typeface);
    }

    juce::Label clipNameLabel;

    bool coolColors = false;
};

class ActionClipModified : public juce::UndoableAction {
  public:
    void *p = nullptr;
    std::vector<int> route;
    int clipIndex;

    // int oldStartSample;
    // int newStartSample;

    clip *getClip();
    clip newClip;
    clip oldClip;

    // ui
    // FIXME: if you're storing pointers to UI components, they might be
    // invalidated if:
    // - you move a clip
    // - you close the editor
    // - open the editor again
    // - try to undo the moving of that clip
    // so make sure to check that and fix it you moron
    void *tc = nullptr;

    ActionClipModified(void *processor, std::vector<int> nodeRoute,
                       int indexOfClip, clip c);
    ~ActionClipModified();

    bool perform() override;
    bool undo() override;
    void markClipComponentStale();
    void updateGUI(); // GUI sounds cooler than UI here, idk man // y
};

class ClipPropertiesWindow : public track::Subwindow {
  public:
    ClipPropertiesWindow();
    ~ClipPropertiesWindow();

    void paint(juce::Graphics &g) override;
    void resized() override;

    void init();

    juce::Label nameLabel;
    juce::Slider gainSlider;

    std::vector<int> route;
    int clipIndex = -1;
    void *p = nullptr;
    track::clip *getClip();

    // this function would ideally not be in this class but whatever
    juce::Font getInterSemiBold() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptSemiBold_ttf,
            BinaryData::Inter_18ptSemiBold_ttfSize);

        return Font(typeface);
    }

    juce::String oldName = "";
    float gainAtDragStart = -1.f;
};

struct relayParam;
class subplugin {
  public:
    subplugin();
    ~subplugin();

    bool initializePlugin(juce::String path);

    std::unique_ptr<juce::AudioPluginInstance> plugin;
    std::vector<relayParam> relayParams;

    void relayParamsToPlugin();

    void *processor = nullptr;
    bool bypassed = false;
    float dryWetMix = 1.f;

    void process(juce::AudioBuffer<float> &buffer);
};

class audioNode {
  public:
    bool s = false;
    bool m = false;
    float gain = 1.f;
    float pan = 0.f;

    juce::String trackName = "Untitled Node";
    int stain = -1;

    // future john, have fun trying to implement hosting audio plugins :skull:
    // haha screw you past john you old sack of dirt
    std::vector<std::unique_ptr<subplugin>> plugins;
    bool addPlugin(juce::String path);
    void removePlugin(int index);
    void preparePlugins();

    void process(int numSamples, int currentSample);
    juce::AudioBuffer<float> buffer;

    void *processor = nullptr;

    bool isTrack = true;
    std::vector<clip> clips;
    std::vector<audioNode> childNodes;

    int latency = -1;
    int getLatencySamples();
    int getTotalLatencySamples();
};

class TrackComponent : public juce::Component {
  public:
    // TrackComponent(track *t);
    TrackComponent(int trackIndex);
    ~TrackComponent();

    void paint(juce::Graphics &g) override;
    void resized() override;
    void initializSliders();

    void mouseDown(const juce::MouseEvent &event) override;
    void mouseUp(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;

    void sendFocusToTimeline();
    bool keyStateChanged(bool isKeyDown) override;
    void focusLost(juce::Component::FocusChangeType /*cause*/) override {
        repaint();
    }

    void copyNodeToClipboard();

    // instances of TrackComponent are responsible for only handling the UI for
    // an indiviaul track (only the left section which shows track name, volume
    // controls, mute/soloing control). The actual CLIPS of audio will be
    // handles in the TimelineComponent class
    // track *correspondingTrack = nullptr;
    audioNode *getCorrespondingTrack();
    void *processor = nullptr;
    int siblingIndex = -1;
    std::vector<int> route;
    int displayIndex = -1;

    juce::TextButton muteBtn;
    juce::TextButton soloBtn;
    juce::TextButton fxBtn; // like in REAPER
    juce::Slider gainSlider;
    juce::Slider panSlider;

    float panValueAtStartDrag = -1.f;
    float gainValueAtStartDrag = -1.f;

    juce::Font getAudioNodeLabelFont() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptRegular_ttf,
            BinaryData::Inter_18ptRegular_ttfSize);

        return Font(typeface);
    }

    juce::Font
    getInterSemiBoldFromThisFunctionBecauseOtherwiseThisWillNotBuildOnWindowsForSomeStupidReason() {
        static auto typeface = Typeface::createSystemTypefaceFor(
            BinaryData::Inter_18ptSemiBold_ttf,
            BinaryData::Inter_18ptSemiBold_ttfSize);

        return Font(typeface);
    }

    juce::Label trackNameLabel;

    bool coolColors = false;
};

class InsertIndicator : public juce::Component {
  public:
    InsertIndicator() {}
    ~InsertIndicator() {}
    void paint(juce::Graphics &g) { g.fillAll(juce::Colours::silver); }
};

class ActionCreateNode : public juce::UndoableAction {
  public:
    // audioNode *parent;
    bool isTrack = true;
    std::vector<int> parentRoute;

    void *tl = nullptr;
    void *p = nullptr;

    ActionCreateNode(std::vector<int> pRoute, bool isATrack, void *tlist,
                     void *processor);
    ~ActionCreateNode();

    bool perform() override;
    bool undo() override;
    void updateGUI(); // y
};

class ActionDeleteNode : public juce::UndoableAction {
  public:
    std::vector<int> route;
    void *p = nullptr;
    void *tl = nullptr;
    void *tc = nullptr;

    ActionDeleteNode(std::vector<int> nodeRoute, void *processor,
                     void *tracklist, void *timelineComponent);
    ~ActionDeleteNode();

    audioNode nodeCopy;

    bool perform() override;
    bool undo() override;
    void updateGUI(); // y
};

class ActionPasteNode : public juce::UndoableAction {
  public:
    std::vector<int> parentRoute;
    std::vector<int> pastedNodeRoute;
    void *p = nullptr;
    void *e = nullptr;

    ActionPasteNode(std::vector<int> pRoute, audioNode *node, void *processor,
                    void *editor);
    ~ActionPasteNode();

    audioNode *nodeToPaste = nullptr;

    bool perform() override;
    bool undo() override;
    void updateGUI(); // y
};

struct TrivialNodeData {
    juce::String trackName;
    float pan;
    float gain;
    bool s;
    bool m;
};
class ActionModifyTrivialNodeData : public juce::UndoableAction {
  public:
    std::vector<int> route;
    // audioNode oldState;
    // audioNode newState;

    ActionModifyTrivialNodeData(std::vector<int> nodeRoute,
                                TrivialNodeData oldData,
                                TrivialNodeData newData, void *processor,
                                void *editor);
    ~ActionModifyTrivialNodeData();

    TrivialNodeData oldState;
    TrivialNodeData newState;

    void *p = nullptr;
    void *e = nullptr;

    bool perform() override;
    bool undo() override;
    void updateGUI(); // y
};

class ActionReorderNode : public juce::UndoableAction {
  public:
    std::vector<int> r1;
    std::vector<int> r2;

    ActionReorderNode(std::vector<int> route1, std::vector<int> route2,
                      void *processor, void *tracklist,
                      void *timelineComponent);
    ~ActionReorderNode();

    void *p = nullptr;
    void *tl = nullptr;
    void *tc = nullptr;

    bool perform() override;
    bool undo() override;
    void updateGUI(); // y
};

class ActionUngroup : public juce::UndoableAction {
  public:
    ActionUngroup(std::vector<int> nodeRoute, void *processor, void *tracklist);
    ~ActionUngroup();

    std::vector<int> route;
    std::vector<int> trackRouteAfterUngroup;

    void *p = nullptr;
    void *tl = nullptr;

    audioNode nodeCopy;

    bool perform() override;
    bool undo() override;
    void updateGUI(); // y
};

class ActionMoveNodeToGroup : public juce::UndoableAction {
  public:
    std::vector<int> nodeToMoveRoute;
    std::vector<int> groupRoute;

    std::vector<int> routeAfterMoving;
    std::vector<int> groupRouteAfterMoving;

    void *p = nullptr;
    void *tl = nullptr;
    void *tc = nullptr;

    ActionMoveNodeToGroup(std::vector<int> toMove, std::vector<int> group,
                          void *processor, void *tracklist,
                          void *timelineComponent);
    ~ActionMoveNodeToGroup();

    bool perform() override;
    bool undo() override;
    void updateGUI(); // y

    void updateOnlyTracklist();
    std::vector<int> getStainedRoute(int staincode);
    std::vector<int> traverseStain(int staincode, audioNode *parentNode,
                                   std::vector<int> r, int depth);
};

class Tracklist : public juce::Component {
  public:
    Tracklist();
    ~Tracklist();

    std::vector<std::unique_ptr<TrackComponent>> trackComponents;
    int findChildren(audioNode *parentNode, std::vector<int> route,
                     int foundItems, int depth);
    juce::String getPrettyVector(std::vector<int> v) {
        juce::String retval = "[";
        for (int x : v) {
            retval.append(juce::String(x), 2);
            retval.append(",", 2);
        }
        retval.append("]", 1);
        return retval;
    }
    void createTrackComponents();
    void setDisplayIndexes();
    void setTrackComponentBounds();
    void updateExistingTrackComponents();

    void clearStains();

    InsertIndicator insertIndicator;
    void updateInsertIndicator(int index);
    /*int insertIndicatorIndex = -1; // when trying to reorder nodes, which
       index
                                   // should we highlight to show user?
*/

    void mouseDown(const juce::MouseEvent &event) override;

    void copyNode(audioNode *dest, audioNode *src);
    void addNewNode(bool isTrack = true,
                    std::vector<int> parentRoute = std::vector<int>());
    void deleteTrack(std::vector<int> route);
    void recursivelyDeleteNodePlugins(audioNode *node);
    bool isDescendant(audioNode *parent, audioNode *possibleChild,
                      bool directDescandant);
    void moveNodeToGroup(track::TrackComponent *caller, int targetIndex);
    void deepCopyGroupInsideGroup(audioNode *childNode, audioNode *parentNode);

    void *processor = nullptr;
    void *timelineComponent = nullptr;

    juce::TextButton newTrackBtn;
    juce::TextButton newGroupBtn;
    juce::TextButton unsoloAllBtn;
    juce::TextButton unmuteAllBtn;
};

class TrackViewport : public juce::Viewport {
  public:
    TrackViewport();
    ~TrackViewport();

    void scrollBarMoved(juce::ScrollBar *bar, double newRangeStart) override;

    void *timelineViewport = nullptr;
};
} // namespace track
