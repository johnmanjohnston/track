#pragma once
#include <vector>

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

constexpr int UI_VISUAL_FEEDBACK_FLASH_DURATION_MS = 60;

extern int BPM; // set in playhead
extern int SNAP_DIVISION;
extern bool AUTO_GRID;

constexpr int TRIM_REGION_WIDTH = 16;

// set in prepareToPlay()
extern double SAMPLE_RATE;
extern int SAMPLES_PER_BLOCK;
extern int MAX_LATENT_SAMPLES;

struct uiinstruction {
    int command = -1;
    void *metadata = nullptr;
    std::vector<int> r;
};

#define UI_INSTRUCTION_UPDATE_CORE 0x01
#define UI_INSTRUCTION_MARK_CC_STALE 0x02
#define UI_INSTRUCTION_UPDATE_STALE_TIMELINE 0x03
#define UI_INSTRUCTION_INIT_CPWS 0x04
#define UI_INSTRUCTION_RECREATE_PCC 0x05
#define UI_INSTRUCTION_RECREATE_ALL_PNCS 0x06
#define UI_INSTRUCTION_RECREATE_RELAY_NODES 0x07
#define UI_INSTRUCTION_UPDATE_CLIP_COMPONENTS 0x08
#define UI_INSTRUCTION_CLEAR_CLIP_COMPONENTS 0x09

// must align with internal JUCE modifications
#define COMMAND_UPDATE_VST3_EMBEDDED_BOUNDS 420

#define STAIN_MOVENODETOGROUP_NODE 0x1
#define STAIN_MOVENODETOGROUP_GROUP 0x2
} // namespace track
