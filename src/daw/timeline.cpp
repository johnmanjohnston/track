#include "timeline.h"
#include "clipboard.h"
#include "defs.h"
#include "juce_data_structures/juce_data_structures.h"
#include "track.h"
#include <cmath>

track::BarNumbersComponent::BarNumbersComponent() : juce::Component() {
    setInterceptsMouseClicks(false, false);
}

track::BarNumbersComponent::~BarNumbersComponent() {}
void track::BarNumbersComponent::paint(juce::Graphics &g) {
    // DBG("timelineComponent paint called");
    g.fillAll(juce::Colour(0xDD'2E2E2E));

    // bar markers
    float secondsPerBeat = 60.f / BPM;
    float pxPerSecond = UI_ZOOM_MULTIPLIER;
    float pxPerBeat = secondsPerBeat * pxPerSecond;

    int beats = this->getWidth() / pxPerBeat;

    // TODO: make this work for other time signatures
    // assuming 4/4 time signature
    float pxPerBar = pxPerBeat * 4;
    int bars = beats * 4;

    // space out markers
    float minSpace = 30.f;
    int incrementAmount = std::ceil(minSpace / pxPerBar);
    incrementAmount = std::max(1, incrementAmount);

    g.setFont(getInterRegular().withHeight(16.f));
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.setY(bounds.getY() - (bounds.getHeight() / 2.f) + 9);

    for (int i = 0; i < bars; i += incrementAmount) {
        for (int k = 0; k < incrementAmount && i + k < bars; ++k) {
            float x = (i + k) * pxPerBar;

            if (k == 0) {
                // draw numbers
                bounds.setX((int)x + 4);
                g.setColour(juce::Colour(0xFF'929292).withAlpha(.8f));
                g.drawText(juce::String(i + 1), bounds,
                           juce::Justification::left, false);
            }
        }
    }
}

track::ActionAddClip::ActionAddClip(clip c, audioNode *node,
                                    void *timelineComponent)
    : juce::UndoableAction() {
    this->addedClip = c;
    this->track = node;
    this->tc = timelineComponent;
};
track::ActionAddClip::~ActionAddClip() {}

bool track::ActionAddClip::perform() {
    track->clips.push_back(addedClip);
    updateGUI();

    return true;
}

bool track::ActionAddClip::undo() {
    track->clips.erase(track->clips.begin() + (long)track->clips.size() - 1);
    updateGUI();

    return true;
}

void track::ActionAddClip::updateGUI() {
    TimelineComponent *timelineComponent = (TimelineComponent *)tc;
    timelineComponent->updateClipComponents();
}

track::TimelineComponent::TimelineComponent() : juce::Component() {
    setMouseClickGrabsKeyboardFocus(true);
    setWantsKeyboardFocus(true);
    addAndMakeVisible(barNumbers, 10);
};
track::TimelineComponent::~TimelineComponent(){};

track::TimelineViewport::TimelineViewport() : juce::Viewport() {}
track::TimelineViewport::~TimelineViewport() {}

void track::TimelineViewport::scrollBarMoved(juce::ScrollBar *bar,
                                             double newRangeStart) {
    if (bar->isVertical()) {
        // update tracklist scroll
        if (trackViewport != nullptr) {
            trackViewport->setViewPosition(0, newRangeStart);
        }

        setViewPosition(getViewPositionX(), newRangeStart);

        TimelineComponent *tc = (TimelineComponent *)getViewedComponent();
        tc->resized();
    } else {
        setViewPosition(newRangeStart, getViewPositionY());
    }
}

void track::TimelineViewport::mouseWheelMove(
    const juce::MouseEvent &ev,
    const juce::MouseWheelDetails &mouseWheelDetails) {

    if (juce::ModifierKeys::currentModifiers.isCtrlDown() ||
        juce::ModifierKeys::currentModifiers.isCommandDown()) {

        TimelineComponent *tc = (TimelineComponent *)getViewedComponent();
        // ev.x is mouse coordiantes in viweport
        // getViewPositionX() is x scroll left

        int mouseXCoordinates = ev.x + getViewPositionX();
        int oldZoom = UI_ZOOM_MULTIPLIER;

        int zoomIncrement = 8;

        if (mouseWheelDetails.deltaY < 0 &&
            UI_ZOOM_MULTIPLIER > UI_MINIMUM_ZOOM_MULTIPLIER) {
            UI_ZOOM_MULTIPLIER -= zoomIncrement;
        } else if (mouseWheelDetails.deltaY > 0 &&
                   UI_ZOOM_MULTIPLIER < UI_MAXIMUM_ZOOM_MULTIPLIER) {
            UI_ZOOM_MULTIPLIER += zoomIncrement;
        }

        UI_ZOOM_MULTIPLIER =
            juce::jlimit(UI_MINIMUM_ZOOM_MULTIPLIER, UI_MAXIMUM_ZOOM_MULTIPLIER,
                         UI_ZOOM_MULTIPLIER);
        DBG(UI_ZOOM_MULTIPLIER);

        tc->resizeTimelineComponent();

        float scale =
            (float)tc->getWidth() /
            (float)((float)tc->getWidth() * oldZoom / UI_ZOOM_MULTIPLIER);

        int newMouseXCoords = mouseXCoordinates * scale;
        int newX = newMouseXCoords - ev.x;

        setViewPosition(newX, getViewPositionY());

        repaint();
    }

    if (juce::ModifierKeys::currentModifiers.isAltDown()) {
        if (mouseWheelDetails.deltaY < 0 &&
            UI_TRACK_HEIGHT > UI_MINIMUM_TRACK_HEIGHT) {
            UI_TRACK_HEIGHT -= 5;
        } else if (mouseWheelDetails.deltaY > 0 &&
                   UI_TRACK_HEIGHT < UI_MAXIMUM_TRACK_HEIGHT) {
            UI_TRACK_HEIGHT += 5;
        }

        tracklist->setTrackComponentBounds();
        trackViewport->repaint();

        TimelineComponent *timelineComponent =
            (TimelineComponent *)getViewedComponent();
        int tcHeight =
            (tracklist->trackComponents.size() + 2) * (size_t)UI_TRACK_HEIGHT;
        timelineComponent->setSize(timelineComponent->getWidth(),
                                   juce::jmax(tcHeight, this->getHeight()));
        timelineComponent->resized();
        timelineComponent->repaint();

        if (timelineComponent->getHeight() <= this->getHeight()) {
            setScrollBarsShown(false, true);
        } else {
            setScrollBarsShown(true, true);
        }

        repaint();
    }

    if (ev.eventComponent == this)
        if (!useMouseWheelMoveIfNeeded(ev, mouseWheelDetails))
            Component::mouseWheelMove(ev, mouseWheelDetails);
}

bool track::TimelineComponent::keyStateChanged(bool isKeyDown) {
    DBG("key state changed for timeline component");

    if (isKeyDown) {
        // ctrl+z
        if (juce::KeyPress::isKeyCurrentlyDown(90)) {
            processorRef->undoManager.undo();
        }

        // ctrl+y
        else if (juce::KeyPress::isKeyCurrentlyDown(89)) {
            processorRef->undoManager.redo();
        }
    }

    return false;
}

void track::TimelineComponent::mouseDown(const juce::MouseEvent &event) {
    grabKeyboardFocus();

    if (event.mods.isRightButtonDown()) {
        DBG("TimelineComponent::mouseDown() for right mouse button down");
        juce::PopupMenu contextMenu;
        contextMenu.setLookAndFeel(&getLookAndFeel());

        // to select grid size for snapping to grid
        juce::PopupMenu gridMenu;
        juce::PopupMenu shiftUpMenu;
        juce::PopupMenu shiftDownMenu;

        // 2**5 = 32
        for (int i = 0; i <= 5; ++i) {
            int x = std::pow(2, i);

            // snap grid
            gridMenu.addItem("1/" + juce::String(x), true, SNAP_DIVISION == x,
                             [this, x] {
                                 SNAP_DIVISION = x;
                                 repaint();
                             });

            // shift up
            shiftUpMenu.addItem(juce::String(x) + " bars",
                                [this, x] { shiftClipByBars(x); });

            // shift down
            shiftDownMenu.addItem(juce::String(x) + " bars",
                                  [this, x] { shiftClipByBars(-x); });
        }

#define MENU_PASTE_CLIP 1

        contextMenu.addItem(MENU_PASTE_CLIP, "Paste clip",
                            clipboard::typecode == TYPECODE_CLIP);
        contextMenu.addSubMenu("Grid", gridMenu);

        contextMenu.addSeparator();

        contextMenu.addSubMenu("Shift all clips ahead ", shiftUpMenu);
        contextMenu.addSubMenu("Shift all clips back ", shiftDownMenu);

        contextMenu.showMenuAsync(juce::PopupMenu::Options(), [this, event](
                                                                  int result) {
            if (result == MENU_PASTE_CLIP) {
                DBG("paste clip");
                if (clipboard::typecode != TYPECODE_CLIP)
                    return;

                // create clip for processor
                clip *orginalClip = (clip *)clipboard::retrieveData();
                std::unique_ptr<clip> newClip(new clip());
                newClip->path = orginalClip->path;
                newClip->buffer = orginalClip->buffer;
                newClip->active = orginalClip->active;
                newClip->name = orginalClip->name;
                newClip->startPositionSample =
                    (event.getMouseDownX() * SAMPLE_RATE) / UI_ZOOM_MULTIPLIER;
                newClip->trimLeft = orginalClip->trimLeft;
                newClip->trimRight = orginalClip->trimRight;
                newClip->gain = orginalClip->gain;

                int y = event.getMouseDownY();
                int nodeDisplayIndex =
                    ((y + (UI_TRACK_HEIGHT / 2)) / UI_TRACK_HEIGHT) - 1;
                nodeDisplayIndex = juce::jlimit(
                    0, (int)viewport->tracklist->trackComponents.size() - 1,
                    nodeDisplayIndex);

                audioNode *node =
                    viewport->tracklist
                        ->trackComponents[(size_t)nodeDisplayIndex]
                        ->getCorrespondingTrack();
                if (node->isTrack) {
                    node->clips.push_back(*newClip);
                } else {
                    DBG("reject pasting clip into group");
                }

                updateClipComponents();
            }
        });
    }
}

void track::TimelineComponent::paint(juce::Graphics &g) {
    // DBG("timelineComponent paint called");
    g.fillAll(juce::Colour(0xFF2E2E2E));

    // bar markers
    float secondsPerBeat = 60.f / BPM;
    float pxPerSecond = UI_ZOOM_MULTIPLIER;
    float pxPerBeat = secondsPerBeat * pxPerSecond;

    int beats = this->getWidth() / pxPerBeat;
    int scrollValue = viewport->getVerticalScrollBar().getCurrentRangeStart();

    // TODO: make this work for other time signatures
    // assuming 4/4 time signature
    float pxPerBar = pxPerBeat * 4;
    int bars = beats * 4;

    // space out markers
    float minSpace = 30.f;
    int incrementAmount = std::ceil(minSpace / pxPerBar);
    incrementAmount = std::max(1, incrementAmount);

    g.setFont(getInterRegular().withHeight(16.f));
    juce::Rectangle<int> bounds = getLocalBounds();
    bounds.setY(bounds.getY() - (bounds.getHeight() / 2.f) + 10 + scrollValue);

    for (int i = 0; i < bars; i += incrementAmount) {
        for (int k = 0; k < incrementAmount && i + k < bars; ++k) {
            float x = (i + k) * pxPerBar;

            // draw vertical lines for bar numbers
            g.setColour(juce::Colour(0xFF'444444).withAlpha(.5f));
            g.fillRect((int)x, 0, 1, getHeight());

            // draw lines for grid snapping
            g.setColour(juce::Colour(0xFF'444444).withAlpha(.3f));
            for (int j = 0; j < SNAP_DIVISION; ++j) {
                int divX = x + ((pxPerBar / SNAP_DIVISION) * j);
                g.fillRect(divX, 0, 1, getHeight());
            }
        }
    }

    // horizontal divide line thingy
    g.setColour(juce::Colour(0xFF'1E1E1E).withAlpha(0.4f));
    g.drawRect(0, 20, getWidth(), 1);

    if (clipComponentsUpdated == false)
        updateClipComponents();
}

void track::TimelineComponent::resizeClipComponent(track::ClipComponent *clip) {
    clip->setBounds(clip->correspondingClip->startPositionSample / SAMPLE_RATE *
                        UI_ZOOM_MULTIPLIER,
                    UI_TRACK_VERTICAL_OFFSET +
                        (clip->nodeDisplayIndex * UI_TRACK_HEIGHT) +
                        (UI_TRACK_VERTICAL_MARGIN * clip->nodeDisplayIndex),
                    (clip->correspondingClip->buffer.getNumSamples() -
                     clip->correspondingClip->trimLeft -
                     clip->correspondingClip->trimRight) /
                        SAMPLE_RATE * UI_ZOOM_MULTIPLIER,
                    UI_TRACK_HEIGHT);

    // handle offline clips
    if (clip->correspondingClip->buffer.getNumSamples() == 0) {
        juce::Rectangle<int> offlineClipBounds = clip->getBounds();
        offlineClipBounds.setWidth(110);
        clip->setBounds(offlineClipBounds);
    }

    juce::Rectangle<int> clipLabelBounds = juce::Rectangle<int>(
        clip->getLocalBounds().getX(), clip->getLocalBounds().getY(), 300, 20);

    clip->clipNameLabel.setBounds(clipLabelBounds);

    if (UI_TRACK_HEIGHT <= UI_TRACK_HEIGHT_COLLAPSE_BREAKPOINT) {
        // clipLabelBounds.setY(clipLabelBounds.getHeight() / 2);
        clipLabelBounds.setY(0);
        clipLabelBounds.setHeight(UI_TRACK_HEIGHT);
        clip->clipNameLabel.setBounds(clipLabelBounds);
    }
}

void track::TimelineComponent::resized() {
    // draw clips
    for (auto &&clip : this->clipComponents) {
        resizeClipComponent(clip.get());
    }

    TimelineViewport *tv = findParentComponentOfClass<TimelineViewport>();
    barNumbers.setBounds(0, tv->getViewPositionY(), getWidth(), 16);
    barNumbers.toFront(false);
}

void track::TimelineComponent::updateClipComponents() {
    clipComponentsUpdated = true;

    clipComponents.clear();

    for (size_t i = 0; i < viewport->tracklist->trackComponents.size(); ++i) {
        audioNode *node =
            viewport->tracklist->trackComponents[i]->getCorrespondingTrack();

        if (!node->isTrack)
            continue;

        for (auto &c : node->clips) {
            this->clipComponents.push_back(std::make_unique<ClipComponent>(&c));
            addAndMakeVisible(*clipComponents.back());

            auto &cc = clipComponents.back();
            cc->nodeDisplayIndex = i;
        }
    }

    resized();
    resizeTimelineComponent();
}

void track::TimelineComponent::resizeTimelineComponent() {
    int largestEnd = -1;

    for (auto &tc : viewport->tracklist->trackComponents) {
        audioNode *node = tc->getCorrespondingTrack();

        for (clip &c : node->clips) {
            if ((c.buffer.getNumSamples() + c.startPositionSample) >
                largestEnd) {
                largestEnd = c.buffer.getNumSamples() + c.startPositionSample;
            }
        }
    }

    // DBG("largestEnd is " << largestEnd);
    jassert(UI_ZOOM_MULTIPLIER > 0);
    largestEnd /= (SAMPLE_RATE / UI_ZOOM_MULTIPLIER);
    largestEnd += 2000;
    // DBG("now, largestEnd is " << largestEnd);

    // this->setSize(juce::jmax(getWidth(), largestEnd), getHeight());
    this->setSize(largestEnd, getHeight());
}

void track::TimelineComponent::deleteClip(clip *c, int trackIndex) {
    int clipIndex = -1;

    audioNode *node = viewport->tracklist->trackComponents[(size_t)trackIndex]
                          ->getCorrespondingTrack();

    for (size_t i = 0; i < node->clips.size(); ++i) {
        if (c == &node->clips[i]) {
            clipIndex = i;
            break;
        }
    }

    node->clips.erase(node->clips.begin() + clipIndex);

    updateClipComponents();
}

void track::TimelineComponent::splitClip(clip *c, int splitSample,
                                         int nodeDisplayIndex) {
    audioNode *node =
        viewport->tracklist->trackComponents[(size_t)nodeDisplayIndex]
            ->getCorrespondingTrack();

    // handle split 1
    int clipIndex = -1;

    // find index of c1
    for (size_t i = 0; i < node->clips.size(); ++i) {
        if (c == &node->clips[i]) {
            clipIndex = i;
            break;
        }
    }
    jassert(clipIndex != -1);

    clip *c1 = &node->clips[(size_t)clipIndex];
    clip c2;
    c2.trimLeft = c1->trimLeft;
    c2.trimRight = c1->trimRight;

    int actualSplit = splitSample;
    c1->trimRight = c->buffer.getNumSamples() - actualSplit - c->trimLeft;

    // handle split 2
    c2.name = c->name;
    c2.path = c->path;
    c2.buffer = c->buffer;
    c2.startPositionSample = c->startPositionSample + actualSplit;
    c2.trimLeft += actualSplit;

    node->clips.push_back(c2);

    DBG("c1: left=" << c1->trimLeft << "; right=" << c1->trimRight);
    DBG("c2: left=" << c2.trimLeft << "; right=" << c2.trimRight);

    // trim left
    // c->trimLeft += splitSample;
    // c->startPositionSample += splitSample;

    updateClipComponents();
    repaint();
}

void track::TimelineComponent::shiftClipByBars(int bars) {
    DBG("shiftClipByBars() called with bars " << bars);

    int beatsPerBar = 4;
    double secondsPerBar = (60.0 / track::BPM) * beatsPerBar;
    int samplesPerBar = secondsPerBar * track::SAMPLE_RATE;

    Tracklist *tracklist = viewport->tracklist;

    for (auto &tc : tracklist->trackComponents) {
        if (tc->getCorrespondingTrack()->isTrack) {
            for (auto &clip : tc->getCorrespondingTrack()->clips) {
                clip.startPositionSample += samplesPerBar * bars;
            }
        }
    }

    updateClipComponents();
}

bool track::TimelineComponent::isInterestedInFileDrag(
    const juce::StringArray &files) {
    return true;
}

void track::TimelineComponent::handleClipResampling(int modalResult) {
    DBG("result is " << modalResult);
}

void track::TimelineComponent::filesDropped(const juce::StringArray &files,
                                            int x, int y) {
    if (viewport->tracklist->trackComponents.size() == 0)
        return;

    int nodeDisplayIndex = ((y + (UI_TRACK_HEIGHT / 2)) / UI_TRACK_HEIGHT) - 1;
    nodeDisplayIndex =
        juce::jlimit(0, (int)viewport->tracklist->trackComponents.size() - 1,
                     nodeDisplayIndex);

    DBG("track index is " << nodeDisplayIndex);

    // check if file is valid audio
    juce::File file(files[0]);
    juce::AudioFormatManager afm;
    afm.registerBasicFormats();

    for (int i = 0; i < afm.getNumKnownFormats(); ++i) {
        auto f = afm.getKnownFormat(i);
        DBG(f->getFormatName());
    }

    std::unique_ptr<juce::AudioFormatReader> reader(afm.createReaderFor(file));
    juce::String path = files[0];

    if (reader == nullptr) {
        juce::String filename = juce::File(path).getFileName();

        juce::NativeMessageBox::showMessageBoxAsync(
            juce::MessageBoxIconType::WarningIcon, "Invalid file",
            "Couldn't read data from \"" + filename +
                juce::String(
                    "\"\n\nTroubleshooting:\n- Verify "
                    "that you dragged the "
                    "right file\n- Verify that the file really is an "
                    "audio file\n- "
                    "Check if the file is corrupted (try playing it back in "
                    "a different "
                    "audio player program)\n- Verify that another program "
                    "isn't using the file"));

        DBG("INVALID AUDIO FILE DRAGGED");
        return;
    }

    DBG("drag x = " << x);
    DBG("UI_ZOOM_MULTIPLIER = " << UI_ZOOM_MULTIPLIER);

    int startSample = (x * SAMPLE_RATE) / UI_ZOOM_MULTIPLIER;

    // look for sample rate mismatch
    if (!juce::approximatelyEqual(track::SAMPLE_RATE, reader->sampleRate)) {
        juce::NativeMessageBox::showAsync(
            juce::MessageBoxOptions()
                .withIconType(juce::MessageBoxIconType::QuestionIcon)
                .withTitle("Sample rate mismatch")

                .withMessage("Host's sample rate is " +
                             juce::String(track::SAMPLE_RATE) +
                             "Hz but the audio file's sample rate is " +
                             juce::String(reader->sampleRate) + "Hz")

                .withButton("Resample file to " +
                            juce::String(track::SAMPLE_RATE) + "Hz")

                .withButton("Don't resample")

                .withButton("Cancel"),
            [this, path, startSample, nodeDisplayIndex](int result) {
                if (result == 0) {
                    // init reader
                    juce::AudioFormatManager afm;
                    afm.registerBasicFormats();
                    std::unique_ptr<juce::AudioFormatReader> reader(
                        afm.createReaderFor(path));

                    // read original buffer
                    juce::AudioBuffer<float> originalBuffer =
                        juce::AudioBuffer<float>((int)reader->numChannels,
                                                 reader->lengthInSamples);

                    reader->read(&originalBuffer, 0,
                                 originalBuffer.getNumSamples(), 0, true, true);

                    // le juice
                    double ratio = reader->sampleRate / track::SAMPLE_RATE;
                    juce::AudioBuffer<float> resampledBuffer =
                        juce::AudioBuffer<float>(
                            (int)reader->numChannels,
                            (int)((double)reader->lengthInSamples / ratio));

                    auto inputs = originalBuffer.getArrayOfReadPointers();
                    auto outputs = resampledBuffer.getArrayOfWritePointers();

                    DBG(reader->lengthInSamples
                        << " samples became " << resampledBuffer.getNumSamples()
                        << " samples with a ratio of " << ratio);

                    for (int ch = 0; ch < (int)reader->numChannels; ++ch) {
                        std::unique_ptr<juce::LagrangeInterpolator> resampler =
                            std::make_unique<juce::LagrangeInterpolator>();
                        resampler->reset();
                        resampler->process(ratio, inputs[ch], outputs[ch],
                                           resampledBuffer.getNumSamples());

                        DBG("resampled channel " << ch);
                    }

                    DBG("resampling finished");

                    // now write resampled file
                    juce::File originalFile = juce::File(path);
                    juce::File parentDir = originalFile.getParentDirectory();
                    juce::String originalFileName = originalFile.getFileName();

                    // because windows is stupid
                    char dirSep = '/';
#if JUCE_WINDOWS
                    dirSep = '\\';
#endif

                    // ex., /home/johnston/j35-44100hz.wav
                    juce::String resampledFilePath =
                        parentDir.getFullPathName() + dirSep +
                        originalFile.getFileNameWithoutExtension() + "-" +
                        juce::String(track::SAMPLE_RATE) + "hz" +
                        originalFile.getFileExtension();

                    juce::File resampledFile = juce::File(resampledFilePath);
                    resampledFile.create();

                    DBG("writing to " << resampledFilePath);

                    juce::WavAudioFormat wavFormat;
                    std::unique_ptr<juce::AudioFormatWriter> writer;

                    writer.reset(wavFormat.createWriterFor(
                        new juce::FileOutputStream(resampledFilePath),
                        track::SAMPLE_RATE, (unsigned int)reader->numChannels,
                        (int)reader->bitsPerSample, reader->metadataValues, 0));

                    jassert(writer != nullptr);

                    DBG("writing...");
                    writer->writeFromAudioSampleBuffer(
                        resampledBuffer, 0, resampledBuffer.getNumSamples());
                    writer->flush();
                    writer.reset();

                    DBG("written succesfully to " << resampledFilePath);

                    addNewClipToTimeline(resampledFilePath, startSample,
                                         nodeDisplayIndex);
                } else if (result == 1) {
                    addNewClipToTimeline(path, startSample, nodeDisplayIndex);
                }
            });
    } else {
        addNewClipToTimeline(path, startSample, nodeDisplayIndex);
    }
}

void track::TimelineComponent::addNewClipToTimeline(juce::String path,
                                                    int startSample,
                                                    int nodeDisplayIndex) {
    std::unique_ptr<clip> c(new clip());
    c->path = path;

    // TODO: see juce::File().getFileNameWithoutExtension() instead of doing all
    // this annoying string manipulation thing

    // remove directories leading up to the actual file name we want, and
    // strip file extension
    if (path.contains("/")) {
        c->name = path.fromLastOccurrenceOf(
            "/", false, true); // for REAL operating systems.
    } else {
        c->name = path.fromLastOccurrenceOf("\\", false, true);
    }
    c->name = c->name.upToLastOccurrenceOf(".", false, true);

    c->startPositionSample = startSample;
    c->updateBuffer();

    DBG("nodeDisplayIndex is " << nodeDisplayIndex);
    audioNode *node =
        viewport->tracklist->trackComponents[(size_t)nodeDisplayIndex]
            ->getCorrespondingTrack();

    if (!node->isTrack) {
        DBG("Rejecting file drop onto group");
    } else {

        ActionAddClip *action = new ActionAddClip(*c, node, this);

        // node->clips.push_back(*c);

        DBG("adding clip using action");
        processorRef->undoManager.perform(action);

        DBG("inserted clip into node");
    }

    updateClipComponents();
}
