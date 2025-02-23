# track
audio plugin which acts as a mini DAW, as a workaround to track count limits in major DAWs 

## About
TODO

## Installation
TODO

## Building
Clone this git repository, `cd` into it, then clone the JUCE repository, then run CMake
```bash
git clone https://github.com/johnmanjohnston/track.git
cd track
git clone https://github.com/juce-framework/JUCE.git
cmake .
```

You should then see the required files to build for your platform

### Linux-based
run Make
```bash
make
```

### Windows (Visual Studio)
- open `track.sln`
- Under Build, click Build Solution (or just do Ctrl+Shift+B)
