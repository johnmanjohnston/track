namespace track {
extern int UI_ZOOM_MULTIPLIER;
constexpr int UI_MINIMUM_ZOOM_MULTIPLIER = 10;
constexpr int UI_MAXIMUM_ZOOM_MULTIPLIER = 160;

extern int UI_TRACK_HEIGHT;
constexpr int UI_MINIMUM_TRACK_HEIGHT = 16;
constexpr int UI_MAXIMUM_TRACK_HEIGHT = 120;

constexpr int UI_TRACK_HEIGHT_COLLAPSE_BREAKPOINT = 25;

constexpr int UI_TRACK_VERTICAL_MARGIN = 0;
constexpr int UI_TRACK_VERTICAL_OFFSET = 19;
constexpr int UI_TRACK_WIDTH = 300;
constexpr int UI_TRACK_INDEX_WIDTH = 22;
constexpr int UI_TRACK_DEPTH_INCREMENTS = 8;

constexpr int UI_PLUGIN_NODE_WIDTH = 250;
constexpr int UI_PLUGIN_NODE_MARGIN = 8;

constexpr int UI_TOPBAR_HEIGHT = 50;

constexpr int UI_SUBWINDOW_TITLEBAR_HEIGHT = 24;
constexpr int UI_SUBWINDOW_TITLEBAR_MARGIN = 5;

extern int BPM; // set in playhead
extern int SNAP_DIVISION;

constexpr int TRIM_REGION_WIDTH = 8;

// set in prepareToPlay()
extern double SAMPLE_RATE;
extern int SAMPLES_PER_BLOCK;
extern int MAX_LATENT_SAMPLES;

// must align with internal JUCE modifications
#define COMMAND_UPDATE_VST3_EMBEDDED_BOUNDS 420
} // namespace track
