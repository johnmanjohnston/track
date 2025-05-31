# track
audio plugin which acts as a mini DAW, as a workaround to track count limits in major DAWs 

## About
TODO

## Installation
From the Releases section, download the appropriate file for your operating system

## Building
### Downloading Source and Dependencies
```bash
# clone this repo and cd into it
$ git clone https://github.com/johnmanjohnston/track.git
$ cd track

# clone JUCE
$ git clone https://github.com/juce-framework/JUCE.git
```

```
# generate build files with CMake
$ cmake .
```

You should then see the required files to build for your platform

### Temporary hack for track to run properly on Linux
inside JUCE/modules/juce_audio_processors/format_types/juce_VST3PluginFormat.cpp, add the following code anywhere inside the `VST3PluginWindow` struct
```cpp
#if JUCE_LINUX
    void handleCommandMessage(int commandId) override {
        if (commandId == 420) {
            embeddedComponent.updateEmbeddedBounds();
        }
    }
#endif
```

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
move `track_artefacts/DEBUG/VST3/track.vst3` to the appropriate plugins folder for your DAW, then add the plugin to a track in your DAW
