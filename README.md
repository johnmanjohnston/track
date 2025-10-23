# track
audio plugin which acts as a mini DAW, as a workaround to track count limits in major DAWs

## About
**track is a free plugin developed as a workaround to track count limits in DAWs** (like Ableton Live Lite's 8 track limit)


To start using track, add the plugin to any track within your host DAW—you now have a mini DAW in a plugin.
You can then create tracks to play audio clips, as well as hosting other plugins with delay compensation.

## Features
- **unlimited stereo audio tracks**, which can be nested using groups
- **drag and drop audio clips** (with basic clip manipulation features like splitting and trimming)
- **scanning and hosting plugins** for individual tracks and/or groups, with **delay compensation**
- **automation medium** from host DAW to plugins hosted inside of track

## Usage
TODO

## Installation
### Latest Release (v0.0.1)
Linux (compiled on Arch): TODO

Windows (64-bit): TODO

macOS: TODO

### Older Versions
for older builds, from the Releases section, download the appropriate file for your platform

## Building
### Downloading Source and Dependencies
```bash
# clone this repo and cd into it
$ git clone https://github.com/johnmanjohnston/track.git
$ cd track

# clone JUCE
$ git clone https://github.com/juce-framework/JUCE.git
```

```bash
# generate build files with CMake
$ cmake .
```

You should then see the required files to build for your platform

<details>
    <summary>
        <b>Temporary hack for track to run properly on Linux</b>
    </summary>

<br>

inside `JUCE/modules/juce_audio_processors/format_types/juce_VST3PluginFormat.cpp`, add the following code anywhere inside the `VST3PluginWindow` struct

```cpp
#if JUCE_LINUX
    void handleCommandMessage(int commandId) override {
        if (commandId == 420) {
            embeddedComponent.updateEmbeddedBounds();
        }
    }
#endif

```
</details>

### Compiling
#### Linux-based
run Make
```bash
$ make
```

#### Windows (Visual Studio)
- open `track.sln`
- Under Build, click Build Solution (or just do Ctrl+Shift+B)

### Running
**If you built a debug build**, move `track_artefacts/Debug/VST3/track.vst3` to the appropriate plugins folder for your DAW

otherwise, move `track_artefacts/Release/VST3/track.vst3` to the appropriate plugins folder for your DAW

then add track to any track in your DAW

