# track
an audio plugin developed as a workaround to track count limits in "lower tier versions" of DAWs (like Ableton Live Lite's 8 track limit)

To start using *track*, add the plugin to any track within your host DAW—you now have a mini DAW in a plugin.
You can then create tracks to play audio clips, as well as host other plugins.

## Features
- **unlimited stereo audio tracks**, which can be organized using groups
- **drag and drop audio clips** (with basic clip manipulation features like splitting and trimming)
- **hosting plugins** for individual tracks and/or groups, with **delay compensation**
- **automation passthrough** from host DAW to plugins hosted within *track*
- **lightweight audio engine** with minimal overhead
- **simple**, intuitive user interface

## Usage
TODO

## Installation
### Latest Release (v0.0.1)
Linux (compiled on Arch): TODO

Windows (64-bit): TODO

macOS: TODO

### Older Versions
For older builds, from the Releases section, download the appropriate file for your platform.

## Building
### Downloading Source and Dependencies
```bash
# clone this repo and cd into it
$ git clone https://github.com/johnmanjohnston/track/
$ cd track

# clone JUCE
$ git clone https://github.com/juce-framework/JUCE/
```

```bash
# generate build files with CMake
$ cmake .
```

You should then see the required files to build for your platform (Makefile for Linux, Visual Studio solution for Windows).

<details>
    <summary>
        <b>Temporary hack for <i>track</i> to run properly on Linux</b>
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
run `make`
```bash
$ make
```

#### Windows (Visual Studio)
- open `track.sln`
- Build -> Build Solution (or do Ctrl+Shift+B)

### Running
**If you built a debug build**, move `track_artefacts/Debug/VST3/track.vst3` to the appropriate plugins folder for your DAW.

Otherwise, move `track_artefacts/Release/VST3/track.vst3` to the appropriate plugins folder for your DAW.

Then, scan and add *track* to any track in your DAW.

## License
*track* is licensed under AGPLv3, see [LICENSE](https://github.com/johnmanjohnston/track/blob/main/LICENSE) for details
