# track
A mini DAW in a plugin. You can add tracks, drag audio clips, and host other plugins.

An audio plugin developed as a workaround to track count limits in "lower tier versions" of DAWs (like Ableton Live Lite's 8 track limit).

<figure>
    <img width="1920" height="1024" alt="image" src="https://github.com/user-attachments/assets/5f31f22c-6bf8-4a7d-a0dc-e1ca157066d7" />
    <p align="center">track running within Ableton Live Lite</p>
</figure>

## Features
- **unlimited stereo audio tracks**, which can be organized using groups
- **drag and drop audio clips** (with basic clip manipulation features like splitting and trimming)
- **hosting plugins** for individual tracks or groups, with **latency compensation**
- **automation passthrough** from host DAW to plugins hosted inside *track*
- **dry/wet control added to every hosted plugin** (even if the hosted plugin doesn't provide it by default)

## Usage
TODO


## Installation
All releases are provided as VST3
### Latest Release (v0.0.1)
Linux: TODO

Windows: TODO

macOS: TODO

### Older Versions
For older builds, from the Releases section, download the appropriate build for your platform.

## Building
### Downloading Source and Dependencies
```bash
# clone this repo and cd into it
git clone https://github.com/johnmanjohnston/track/
cd track

# clone JUCE
git clone https://github.com/juce-framework/JUCE/
```

```bash
# generate build files with CMake
cmake .
```

You should then see the required files to build for your platform (Makefile for Linux, Visual Studio solution for Windows).

<details>
    <summary>
        <b>Linux-specific patch for plugin editors to function properly inside <i>track</i></b>
    </summary>

<br>

On Linux, plugin editors hosted inside of *track* cannot be dragged around; this JUCE patch fixes that.

Inside `JUCE/modules/juce_audio_processors/format_types/juce_VST3PluginFormat.cpp`, add the following code anywhere inside the `VST3PluginWindow` struct:

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
#### Linux
```bash
# run make
make
```

#### Windows (Visual Studio)
- open `track.sln`
- Build > Build Solution (or do `Ctrl+Shift+B`)

### Running
If you built a **debug** build:
    `track_artefacts/Debug/VST3/track.vst3`
    
If you built a **release** build:
    `track_artefacts/Release/VST3/track.vst3`

Move the file to your DAW's VST3 plugin folder.
Then, scan and add *track* to any track in your DAW.

## License
*track* is licensed under the AGPLv3, see [LICENSE](https://github.com/johnmanjohnston/track/blob/main/LICENSE) for details
