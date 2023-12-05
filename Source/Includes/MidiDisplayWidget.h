//
// Created by Behzad Haki on 8/3/2023.
//

#pragma once
#include <utility>

#include "GuiParameters.h"
#include "Configs_Parser.h"

using namespace std;

class InputMidiPianoRollComponent : public juce::Component,
                                    public juce::FileDragAndDropTarget
{
public:

    explicit InputMidiPianoRollComponent()
    {
        Initialize();

    }


    explicit InputMidiPianoRollComponent(
        LockFreeQueue<juce::MidiFile, 4>* MidiQue_,
        const juce::MidiMessageSequence& NMP2GUI_IncomingMessageSequence_)
    {
        Initialize();

        MidiQue = MidiQue_;
        if (MidiQue_->getNumberOfWrites() > 0)
        {
            DraggedMidi = MidiQue_->getLatestDataWithoutMovingFIFOHeads();
            // convert all timings to ticks
            if (DraggedMidi.getNumTracks() > 0)
            {
                auto track = DraggedMidi.getTrack(0);
                if (track != nullptr)
                {
                    for (int i = 0; i < track->getNumEvents(); ++i)
                    {
                        auto event = track->getEventPointer(i);
                        if (event != nullptr)
                        {
                            auto msg = event->message;
                            msg.setTimeStamp(msg.getTimeStamp() * 960);
                            event->message = msg;
                        }
                    }
                }
                full_repaint = true;
            }
        }

        juce::MidiFile mf;
        mf.setTicksPerQuarterNote(960);
        mf.addTrack(NMP2GUI_IncomingMessageSequence_);
        IncomingMidi = mf;
        // convert all timings to ticks
        if (IncomingMidi.getNumTracks() > 0)
        {
            auto track = IncomingMidi.getTrack(0);
            if (track != nullptr)
            {
                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto event = track->getEventPointer(i);
                    if (event != nullptr)
                    {
                        auto msg = event->message;
                        msg.setTimeStamp(msg.getTimeStamp() * 960);
                        event->message = msg;
                    }
                }
            }
            full_repaint = true;
        }
    }

    // generates and displays a random midi file (for testing purposes only)
    // called from PluginEditor.cpp during initialization
    void generateRandomMidiFile(int numNotes)
    {
        DraggedMidi = juce::MidiFile();
        int ticksPerQuarterNote = 960;
        DraggedMidi.setTicksPerQuarterNote(ticksPerQuarterNote);

        juce::MidiMessageSequence sequence;

        for (int i = 0; i < numNotes; ++i)
        {
            int noteNumber = juce::Random::getSystemRandom().nextInt(
                {21, 108});
            int velocity = juce::Random::getSystemRandom().nextInt(
                {20, 127});
            double startTime = juce::Random::getSystemRandom().nextDouble() * 10.0;
            double noteDuration = juce::Random::getSystemRandom().nextDouble() *
                                      2.0 + 0.5f; // duration in quarter notes

            sequence.addEvent(juce::MidiMessage::noteOn(
                                  1, noteNumber, juce::uint8(velocity)),
                              startTime * ticksPerQuarterNote);
            sequence.addEvent(juce::MidiMessage::noteOff(
                                  1, noteNumber),
                              (startTime + noteDuration) *
                                  ticksPerQuarterNote);
        }

        DraggedMidi.addTrack(sequence);

        repaint();
    }

    double getLength() {
        int ticksPerQuarterNote = 960;
        double len = 0;

        if (IncomingMidi.getNumTracks() > 0)
        {
            auto track = IncomingMidi.getTrack(0);
            if (track != nullptr)
            {
                len = std::max(track->getEndTime() + 4 * ticksPerQuarterNote,
                               playhead_pos + 4 * ticksPerQuarterNote);
            }
        }

        if (DraggedMidi.getNumTracks() > 0)
        {
            auto track = DraggedMidi.getTrack(0);
            if (track != nullptr)
            {
                auto len2 = std::max(track->getEndTime() + 4 * ticksPerQuarterNote,
                               playhead_pos + 4 * ticksPerQuarterNote);
                len = std::max(len, len2);
            }
        }

        return len;
    }

    void setLength(double length) {
        if (length != disp_length) {
            disp_length = length;
            full_repaint = true;
            repaint();
        }
    }

    // should manually call repaint() after calling this function
    // incoming note with time in midi ticks with 960 ticks per quarter note
    void displayMidiMessageSequence(const juce::MidiMessageSequence& incoming_seq) {
        // add incoming_note to IncomingMidi
        IncomingMidi = juce::MidiFile();
        IncomingMidi.setTicksPerQuarterNote(960);
        IncomingMidi.addTrack(incoming_seq);
        notePositions.clear();
        uniquePitches.clear();

        // convert all timings to ticks
        if (IncomingMidi.getNumTracks() > 0)
        {
            auto track = IncomingMidi.getTrack(0);
            if (track != nullptr)
            {
                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto event = track->getEventPointer(i);
                    if (event != nullptr)
                    {
                        auto msg = event->message;
                        msg.setTimeStamp(msg.getTimeStamp() * 960);
                        event->message = msg;
                    }
                }
            }
        }

        full_repaint = true;
    }

    // should manually call repaint() after calling this function
    void displayMidiMessageSequence(double playhead_pos_quarter_notes) {
        //        DraggedMidi = juce::MidiFile();
        int ticksPerQuarterNote = 960;
        playhead_pos = playhead_pos_quarter_notes * ticksPerQuarterNote;
        full_repaint = false;
    }

    // draws midi notes on the component (piano roll)
    // maps the midi notes to the component's width and height
    // pitch values are mapped to the y axis, and time values are mapped to the x axis
    // velocity values are mapped to the alpha value of the note color
    void paint(juce::Graphics& g) override
    {
        g.fillAll(backgroundColour);

        // Label in the top-right corner
        juce::String incomingLabel = "Incoming MIDI";
        g.setColour(juce::Colours::black);
        g.setFont(14.0f); // Font size
        g.drawText(incomingLabel, getWidth() - 120, 0, 120, 20, juce::Justification::right);

        calculateNotePositions();


        drawNotes(g, *DraggedMidi.getTrack(0));
        drawNotes(g, *IncomingMidi.getTrack(0));

        // Draw playhead
        if (playhead_pos >= 0)
        {
            g.setColour(juce::Colours::red);
            auto x = playhead_pos / disp_length * (float)getWidth();
            g.drawLine(x, 0, x,
                       (float) getHeight(), 2);
        }

        full_repaint = false;
    }

    // call back function for drag and drop out of the component
    // should drag from component to file explorer or DAW if allowed
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (IncomingMidi.getNumTracks() <= 0 && DraggedMidi.getNumTracks() <= 0)
        {
            return;
        }

        juce::MidiMessageSequence displayedSequence{};

        // add content of DraggedMidi and IncomingMidi to diplayMidi
        if (DraggedMidi.getNumTracks() > 0)
        {
            auto track = DraggedMidi.getTrack(0);
            if (track != nullptr)
            {
                displayedSequence.addSequence(*track, 0, 0, track->getEndTime());

            }
        }
        if (IncomingMidi.getNumTracks() > 0)
        {
            auto track = IncomingMidi.getTrack(0);
            if (track != nullptr)
            {
                displayedSequence.addSequence(*track, 0, 0, track->getEndTime());
            }
        }

        juce::MidiFile diplayMidi = juce::MidiFile();
        diplayMidi.setTicksPerQuarterNote(960);
        diplayMidi.addTrack(displayedSequence);

        juce::String fileName = juce::Uuid().toString() + ".mid";  // Random filename

        juce::File outf = juce::File::getSpecialLocation(
                              juce::File::SpecialLocationType::tempDirectory
                              ).getChildFile(fileName);
        juce::TemporaryFile tempf(outf);
        {
            auto p_os = std::make_unique<juce::FileOutputStream>(
                tempf.getFile());
            if (p_os->openedOk()) {
                diplayMidi.writeTo(*p_os, 0);
            }
        }

        bool succeeded = tempf.overwriteTargetFileWithTemporary();
        if (succeeded) {
            juce::DragAndDropContainer::performExternalDragDropOfFiles(
                {outf.getFullPathName()}, false);
        }
    }

    // call back function for drag and drop into the component
    // returns true if the file was successfully loaded
    // called from filesDropped() in PluginEditor.cpp
    // keep in mind that a file can be dropped anywhere in the parent component
    // (i.e. PluginEditor.cpp) and not just on this component
    // *NOTE* As soon as loaded, all timings are converted to quarter notes instead of ticks
    bool loadMidiFile(const juce::File& file)
    {
        if (!UIObjects::MidiInVisualizer::allowToDragInMidi) { return false; }

        auto stream = file.createInputStream();
        if (stream != nullptr)
        {
            if (DraggedMidi.readFrom(*stream))
            {
                full_repaint = true;
                // Successfully read the MIDI file, so repaint the component

                // get the start time of the first note
                int TPQN = DraggedMidi.getTimeFormat();
                if (TPQN > 0)
                {
                    // Create a copy of the MIDI file for conversion to quarter notes
                    MidiFile quarterNoteMidiFile = DraggedMidi;

                    // Iterate through all notes in track 0 of the original file
                    auto track = DraggedMidi.getTrack(0);
                    if (track != nullptr)
                    {
                        // Iterate through all notes in track 0 of the copy for conversion
                        auto quarterNoteTrack = quarterNoteMidiFile.getTrack(0);
                        if (quarterNoteTrack != nullptr)
                        {
                            for (int i = 0; i < quarterNoteTrack->getNumEvents(); ++i)
                            {
                                auto event = quarterNoteTrack->getEventPointer(i);
                                event->message.setTimeStamp(
                                    event->message.getTimeStamp() / TPQN);
                            }
                        }
                    }

                    // Push the quarter note version to the queue
                    MidiQue->push(quarterNoteMidiFile);


                    IncomingMidi.clear();
                    IncomingMidi.setTicksPerQuarterNote(960);
                    IncomingMidi.addTrack(juce::MidiMessageSequence());

                    repaint();

                    return true;
                } else {
                    // Negative value indicates frames per second (SMPTE)
                    cout << "SMPTE Format midi files are not supported at this time." << endl;
                }
            }
        }



        return false;
    }


    bool isInterestedInFileDrag (const juce::StringArray& files) override
    {
        return true;
    }

    void filesDropped (const juce::StringArray& files, int /*x*/, int /*y*/) override
    {
        for (auto& file : files)
        {
            if (file.endsWith(".mid") || file.endsWith(".midi"))
            {
                juce::File midiFileToLoad(file);
                if (loadMidiFile(midiFileToLoad))
                {
                    repaint();
                    break;
                }
            }
        }
    }

private:
    void Initialize()
    {
        // Load an empty MidiFile to start
        DraggedMidi = juce::MidiFile();
        DraggedMidi.setTicksPerQuarterNote(960);

        IncomingMidi = juce::MidiFile();
        IncomingMidi.setTicksPerQuarterNote(960);

        // Enable drag and drop
        //        setInterceptsMouseClicks(false, true);
        setInterceptsMouseClicks(
            true, true);
    }

    juce::MidiFile DraggedMidi;
    juce::MidiFile IncomingMidi;
    juce::Colour backgroundColour{juce::Colours::whitesmoke};
    juce::Colour DraggedNoteColour = juce::Colours::skyblue;
    juce::Colour IncomingNoteColour = juce::Colours::darkolivegreen;
    LockFreeQueue<juce::MidiFile, 4>* MidiQue{};

    double playhead_pos{-1};
    double disp_length{8};

    std::set<int> uniquePitches;
    std::map<int, std::vector<std::tuple<float, float, float, juce::Colour>>> notePositions; // Maps note numbers to positions and velocity and color
    bool full_repaint {false};

    void calculateNotePositions()
    {
        uniquePitches.clear();
        notePositions.clear();

        auto processTrack = [&](const juce::MidiMessageSequence& track, const juce::Colour& color) {
            for (int i = 0; i < track.getNumEvents(); ++i)
            {
                auto event = track.getEventPointer(i);
                if (event != nullptr && event->message.isNoteOn())
                {
                    int noteNumber = event->message.getNoteNumber();
                    uniquePitches.insert(noteNumber);

                    float x = float(event->message.getTimeStamp() / disp_length) *
                              (float)getWidth();

                    float length = 0;
                    for (int j = i + 1; j < track.getNumEvents(); ++j)
                    {
                        auto offEvent = track.getEventPointer(j);
                        if (offEvent != nullptr && offEvent->message.isNoteOff() &&
                            offEvent->message.getNoteNumber() == noteNumber)
                        {
                            double noteLength = offEvent->message.getTimeStamp() -
                                                event->message.getTimeStamp();
                            length = float(noteLength / disp_length) *
                                     (float)getWidth();
                            break;
                        }
                    }

                    float velocity = event->message.getFloatVelocity();

                    notePositions[noteNumber].emplace_back(x, length, velocity, color);
                }
            }
        };

        if (DraggedMidi.getNumTracks() > 0 && UIObjects::MidiInVisualizer::allowToDragInMidi)
        {
            processTrack(*DraggedMidi.getTrack(0), DraggedNoteColour);
        }

        if (IncomingMidi.getNumTracks() > 0 && UIObjects::MidiInVisualizer::visualizeIncomingMidiFromHost)
        {
            processTrack(*IncomingMidi.getTrack(0), IncomingNoteColour);
        }
    }

    void drawNotes(juce::Graphics& g, const juce::MidiMessageSequence& midi)
    {
        int numRows = (int) uniquePitches.size();
        float rowHeight = (float)getHeight() / (float)numRows;
        g.setFont(juce::Font(10.0f));

        for (const auto& pitch : uniquePitches)
        {
            float y = float(numRows - 1 - std::distance(uniquePitches.begin(), uniquePitches.find(pitch))) * rowHeight;

            for (const auto& [x, length, velocity, color_] : notePositions[pitch])
            {
                // Set color according to the velocity
                auto color = color_.withAlpha(velocity);
                g.setColour(color);
                g.fillRect(juce::Rectangle<float>(x, y, length, rowHeight));

                // Draw border around the note
                g.setColour(juce::Colours::black);
                g.drawRect(juce::Rectangle<float>(x, y, length, rowHeight), 1);
            }
        }
    }
};

class OutputMidiPianoRollComponent : public juce::Component
{
public:

    explicit OutputMidiPianoRollComponent()
    {
        Initialize();

    }

    explicit OutputMidiPianoRollComponent(LockFreeQueue<juce::MidiFile, 4>* MidiQue_)
    {
        Initialize();

        MidiQue = MidiQue_;
        if (MidiQue_->getNumberOfWrites() > 0) {
            midiFile = MidiQue_->getLatestDataWithoutMovingFIFOHeads();
        }

    }

    // generates and displays a random midi file (for testing purposes only)
    // called from PluginEditor.cpp during initialization
    void generateRandomMidiFile(int numNotes)
    {
        midiFile = juce::MidiFile();
        int ticksPerQuarterNote = 960;
        midiFile.setTicksPerQuarterNote(ticksPerQuarterNote);

        juce::MidiMessageSequence sequence;

        for (int i = 0; i < numNotes; ++i)
        {
            int noteNumber = juce::Random::getSystemRandom().nextInt(
                {21, 108});
            int velocity = juce::Random::getSystemRandom().nextInt(
                {20, 127});
            double startTime = juce::Random::getSystemRandom().nextDouble() * 10.0;
            double noteDuration = juce::Random::getSystemRandom().nextDouble() *
                                      2.0 + 0.5f; // duration in quarter notes

            sequence.addEvent(juce::MidiMessage::noteOn(
                                  1, noteNumber, juce::uint8(velocity)),
                              startTime * ticksPerQuarterNote);
            sequence.addEvent(juce::MidiMessage::noteOff(
                                  1, noteNumber),
                              (startTime + noteDuration) *
                                  ticksPerQuarterNote);
        }

        midiFile.addTrack(sequence);

        repaint();
    }

    // should manually call repaint() after calling this function
    void displayMidiMessageSequence(const juce::MidiMessageSequence& sequence_,
                                    PlaybackPolicies playbackPolicy_,
                                    double fs, double qpm) {

        midiFile = juce::MidiFile();
        int ticksPerQuarterNote = 960;
        midiFile.setTicksPerQuarterNote(ticksPerQuarterNote);  \
        juce::MidiMessageSequence sequence;

        // all timings need to be converted to ticks
        for (auto m: sequence_) {
            auto msg = m->message;
            if (playbackPolicy_.IsTimeUnitIsAudioSamples()) {
                auto time_in_quartern = msg.getTimeStamp() / fs / 60.0 * qpm;
                // convert to ticks
                msg.setTimeStamp(time_in_quartern * ticksPerQuarterNote);
            } else if (playbackPolicy_.IsTimeUnitIsPPQ()) {
                // convert to ticks
                msg.setTimeStamp(msg.getTimeStamp() * ticksPerQuarterNote);
            } else if (playbackPolicy_.IsTimeUnitIsSeconds()) {
                auto time_in_quartern = msg.getTimeStamp() / 60.0 * qpm;
                // convert to ticks
                msg.setTimeStamp(time_in_quartern * ticksPerQuarterNote);
            }
            sequence.addEvent(msg);
        }
        midiFile.addTrack(sequence);

        midiDataChanged = true;

        //        repaint();
    }

    // should manually call repaint() after calling this function
    void displayLoopedMidiMessageSequence(const juce::MidiMessageSequence& sequence_,
                                          PlaybackPolicies playbackPolicy_,
                                          double fs, double qpm,
                                          double LoopStartPPQ_, double LoopEndPPQ_) {

        midiFile = juce::MidiFile();
        int ticksPerQuarterNote = 960;
        midiFile.setTicksPerQuarterNote(ticksPerQuarterNote);  \
        juce::MidiMessageSequence sequence;

        LoopStartPPQ = LoopStartPPQ_;
        LoopEndPPQ = LoopEndPPQ_;

        // all timings need to be converted to ticks
        for (auto m: sequence_) {
            auto msg = m->message;
            if (playbackPolicy_.IsTimeUnitIsAudioSamples()) {
                auto time_in_quartern = msg.getTimeStamp() / fs / 60.0 * qpm;
                // convert to ticks
                msg.setTimeStamp(time_in_quartern * ticksPerQuarterNote);
            } else if (playbackPolicy_.IsTimeUnitIsPPQ()) {
                // convert to ticks
                msg.setTimeStamp(msg.getTimeStamp() * ticksPerQuarterNote);
            } else if (playbackPolicy_.IsTimeUnitIsSeconds()) {
                auto time_in_quartern = msg.getTimeStamp() / 60.0 * qpm;
                // convert to ticks
                msg.setTimeStamp(time_in_quartern * ticksPerQuarterNote);
            }
            sequence.addEvent(msg);
        }
        midiFile.addTrack(sequence);

        midiDataChanged = true;

        //        repaint();
    }

    // should manually call repaint() after calling this function
    void displayMidiMessageSequence(double playhead_pos_quarter_notes) {
        int ticksPerQuarterNote = 960;
        playhead_pos = playhead_pos_quarter_notes * ticksPerQuarterNote;
        midiDataChanged = false;

        //        repaint();
    }

    double getLength() {
        int ticksPerQuarterNote = 960;
        if (midiFile.getNumTracks() > 0)
        {
            auto track = midiFile.getTrack(0);
            if (track != nullptr)
            {
                return std::max(track->getEndTime() + 4 * ticksPerQuarterNote,
                                playhead_pos + 4 * ticksPerQuarterNote);
            }
        }
        return 0;
    }

    double getMidiLength() {
        // int ticksPerQuarterNote = 960;
        if (midiFile.getNumTracks() > 0)
        {
            auto track = midiFile.getTrack(0);
            if (track != nullptr)
            {
                return track->getEndTime();
            }
        }
        return 0;
    }

    void setLength(double length) {
        if (length != disp_length) {
            disp_length = length;
            midiDataChanged = true;
            repaint();
        }
    }

    // draws midi notes on the component (piano roll)
    // maps the midi notes to the component's width and height
    // pitch values are mapped to the y axis, and time values are mapped to the x axis
    // velocity values are mapped to the alpha value of the note color
    void paint(juce::Graphics& g) override
    {
        g.fillAll(backgroundColour);

        // Label in the top-right corner
        juce::String inputLabel = "Outgoing MIDI";
        g.setColour(juce::Colours::white);
        g.setFont(14.0f); // Font size
        g.drawText(inputLabel, getWidth() - 100, 0, 100, 20, juce::Justification::right);

        // Draw loop start and end brackets
        if (LoopStartPPQ >= 0 && LoopEndPPQ > 0)
        {
            // Draw loop area as a transparent rectangle
            g.setColour(juce::Colours::brown.withAlpha(0.4f));
            auto xStart = LoopStartPPQ / disp_length * (float) getWidth() * 960;
            auto xEnd = LoopEndPPQ / disp_length * (float) getWidth() * 960;
            g.fillRect((int) xStart, 0, int(xEnd - xStart), getHeight());
        }

        if (midiDataChanged) // You need to define and update this flag
        {
            calculateNotePositions();
        }

        drawNotes(g, noteColour);

        // Draw playhead
        if (playhead_pos >= 0)
        {
            g.setColour(juce::Colours::red);
            auto x = playhead_pos / disp_length * (float) getWidth();
            g.drawLine(x, 0, x, getHeight(), 2);
        }
    }


    // call back function for drag and drop out of the component
    // should drag from component to file explorer or DAW if allowed
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (UIObjects::GeneratedContentVisualizer::allowToDragOutAsMidi)
        {
            if (midiFile.getNumTracks() <= 0)
            {
                return;
            }

            juce::String fileName = juce::Uuid().toString() + ".mid";  // Random filename

            juce::File outf = juce::File::getSpecialLocation(
                                  juce::File::SpecialLocationType::tempDirectory
                                  ).getChildFile(fileName);
            juce::TemporaryFile tempf(outf);
            {
                auto p_os = std::make_unique<juce::FileOutputStream>(
                    tempf.getFile());
                if (p_os->openedOk()) {
                    midiFile.writeTo(*p_os, 0);
                }
            }

            bool succeeded = tempf.overwriteTargetFileWithTemporary();
            if (succeeded) {
                juce::DragAndDropContainer::performExternalDragDropOfFiles(
                    {outf.getFullPathName()}, false);
            }
        }
    }

private:
    void Initialize()
    {
        // Load an empty MidiFile to start
        midiFile = juce::MidiFile();
        midiFile.setTicksPerQuarterNote(960);


        // Enable drag and drop
        //        setInterceptsMouseClicks(false, true);
        setInterceptsMouseClicks(
            true, true);
    }

    juce::MidiFile midiFile;
    juce::Colour backgroundColour{juce::Colours::black};
    juce::Colour noteColour = juce::Colours::white;
     LockFreeQueue<juce::MidiFile, 4>* MidiQue{};
    double playhead_pos{-1};
    double disp_length{8};
    double LoopStartPPQ{-1};
    double LoopEndPPQ{-1};

    std::set<int> uniquePitches;
    std::map<int, std::vector<std::tuple<float, float, float>>> notePositions; // Maps note numbers to positions and velocity
    bool midiDataChanged{false};

    void calculateNotePositions()
    {
        uniquePitches.clear();
        notePositions.clear();

        if (midiFile.getNumTracks() > 0)
        {
            auto track = midiFile.getTrack(0);
            if (track != nullptr)
            {
                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto event = track->getEventPointer(i);
                    if (event != nullptr && event->message.isNoteOn())
                    {
                        int noteNumber = event->message.getNoteNumber();
                        uniquePitches.insert(noteNumber);

                        float x = float(event->message.getTimeStamp() / disp_length) *
                                  (float)getWidth();

                        float length = 0;
                        for (int j = i + 1; j < track->getNumEvents(); ++j)
                        {
                            auto offEvent = track->getEventPointer(j);
                            if (offEvent != nullptr && offEvent->message.isNoteOff() &&
                                offEvent->message.getNoteNumber() == noteNumber)
                            {
                                double noteLength = offEvent->message.getTimeStamp() -
                                                    event->message.getTimeStamp();
                                length = float(noteLength / disp_length) *
                                         (float)getWidth();
                                break;
                            }
                        }

                        float velocity = event->message.getFloatVelocity(); // Add velocity
                        notePositions[noteNumber].push_back(std::make_tuple(x, length, velocity));
                    }
                }
            }
        }
    }

    void drawNotes(juce::Graphics& g, const juce::Colour& noteColour_)
    {
        int numRows = (int) uniquePitches.size();
        float rowHeight = (float)getHeight() / (float)numRows;
        g.setFont(juce::Font(10.0f));

        for (const auto& pitch : uniquePitches)
        {
            float y = float(numRows - 1 - std::distance(uniquePitches.begin(), uniquePitches.find(pitch))) * rowHeight;

            for (const auto& pos : notePositions[pitch])
            {
                float x = std::get<0>(pos);
                float length = std::get<1>(pos);
                float velocity = std::get<2>(pos); // Get velocity

                // Set color according to the velocity
                auto color = noteColour_.withAlpha(velocity); // Set alpha to velocity
                g.setColour(color);
                g.fillRect(juce::Rectangle<float>(x, y, length, rowHeight));

                // Draw border around the note
                g.setColour(juce::Colours::black);
                g.drawRect(juce::Rectangle<float>(x, y, length, rowHeight), 1);
            }
        }
    }
};
/// --------------------- MIDI IO VISUALIZER ---------------------





class PlayheadVisualizer : public juce::Component {
public:
    bool show_playhead{true};
    juce::Colour playheadColour = juce::Colours::red;
    float playhead_pos{0};
    float disp_length{8};

    void paint(juce::Graphics& g) override
    {
        if (show_playhead) {
            g.setColour(playheadColour);
            // Draw playhead
            if (playhead_pos >= 0)
            {
                auto x = playhead_pos / disp_length * (float)getWidth();
                g.drawLine(
                    x, 0, x, (float) getHeight(), 2);
            }
        }
    }

    void resized() override
    {
        repaint();
    }

    void setLength(float length) {
        if (length != disp_length) {
            disp_length = length;
            repaint();
        }
    }

    void setPlayheadPos(float pos) {
        if (pos != playhead_pos) {
            playhead_pos = pos;
            repaint();
        }
    }

};

class PianoKeysComponent : public juce::Component
{
public:

    void paint(Graphics &g) override {

        g.fillAll(backgroundColour);

        // ---------------------
        // divide the component into 12 rows
        // Starting from C0 (MIDI note 12)
        // Draw a horizontal rectangle for each row with the corresponding note name
        // and color the row if it is a sharp note
        // ---------------------
        int numRows = 12;
        float rowHeight = (float)getHeight() / (float)numRows;
        g.setFont(juce::Font(getHeight() / 10.0f)); // Font size
        vector<juce::String> noteNames = {"C", "", "D", "", "E", "F",
                                          "", "G", "", "A", "", "B"};

        for (int i = 0; i < numRows; ++i)
        {
            float y = float(numRows - 1 - i) * rowHeight;
            g.setColour(juce::Colours::grey);
            g.drawRect(juce::Rectangle<float>(0, y, (float)getWidth(), rowHeight), .1);

            // Draw note name
            juce::String noteName = noteNames[i];
            g.drawText(noteName, 0, y, (float)getWidth(), rowHeight, juce::Justification::centredLeft);

            // Color the row if it is a sharp note
            if (noteName == "")
            {
                g.setColour(sharpNoteColour);
                g.fillRect(juce::Rectangle<float>(0, y, getWidth(), rowHeight));
            }
        }

    }

    PianoKeysComponent(juce::Colour backgroundColour_,
                       juce::Colour sharpNoteColour_)
    {
        backgroundColour = backgroundColour_;
        sharpNoteColour = sharpNoteColour_;
    }

private:
    juce::Colour backgroundColour;
    juce::Colour sharpNoteColour;
};


class NoteComponent : public juce::Component {
public:
    int noteNumber{};
    int pitch_class{}; // 0-11
    juce::String noteInfo;
    float start_time{};
    float duration{-1}; // if neg, then note is still on (i.e. no note off event)
    float velocity{};
    juce::Colour color{juce::Colours::black};
    juce::Label* display_label{};
    bool isHangingNoteOn{false};
    bool isHangingNoteOff{false};

    NoteComponent() = default;

    // creates a note component with a specified duration
    // NOTE: Velocity must be between 0 and 1
    // If you want to display the note name, pass in a pointer to a label
    void completeNote(int noteNumber_,
                  float start_time_,
                  float velocity_,
                  float duration_,
                  juce::Label* display_label_ = nullptr) {
        isHangingNoteOff = false;
        isHangingNoteOn = false;
        noteNumber = noteNumber_;
        start_time = start_time_;
        duration = duration_;
        velocity = velocity_;
        display_label = display_label_;
        generate_info_text();
        repaint();
    }

    void hangingNoteOn(int noteNumber_,
                  float start_time_,
                  float velocity_,
                  juce::Label* display_label_ = nullptr) {
        isHangingNoteOn = true;
        isHangingNoteOff = false;
        noteNumber = noteNumber_;
        start_time = start_time_;
        duration = -1;
        velocity = velocity_;
        display_label = display_label_;
        generate_info_text();
        repaint();
    }

    void hangingNoteOff(int noteNumber_,
                  float start_time_,
                  float velocity_,
                  juce::Label* display_label_ = nullptr) {
        isHangingNoteOff = true;
        isHangingNoteOn = false;
        noteNumber = noteNumber_;
        start_time = start_time_;
        duration = -1;
        velocity = velocity_;
        display_label = display_label_;
        generate_info_text();
        repaint();
    }

    void paint(juce::Graphics& g) override {
        // Set the colour with alpha based on velocity for all shapes
        g.setColour(color.withAlpha(velocity));

        if (!isHangingNoteOn && !isHangingNoteOff) {
            // Standard note: Draw a rectangle
            g.fillRect(getLocalBounds());
            // Draw a black border around the rectangle
            g.setColour(juce::Colours::black);
            g.drawRect(getLocalBounds().toFloat(), 1);
        } else {
            // Prepare to draw an arrow for note-on or note-off
            juce::Path arrowPath;
            auto bounds = getLocalBounds().toFloat();

            if (isHangingNoteOn) {
                // Note-on (right-pointing arrow)
                arrowPath.startNewSubPath(bounds.getX(), bounds.getY());
                arrowPath.lineTo(bounds.getRight(), bounds.getCentreY());
                arrowPath.lineTo(bounds.getX(), bounds.getBottom());
                arrowPath.closeSubPath();
            } else if (isHangingNoteOff) {
                // Note-off (left-pointing arrow)
                arrowPath.startNewSubPath(bounds.getRight(), bounds.getY());
                arrowPath.lineTo(bounds.getX(), bounds.getCentreY());
                arrowPath.lineTo(bounds.getRight(), bounds.getBottom());
                arrowPath.closeSubPath();
            }

            // Fill the arrow with the same colour and alpha as the standard note
            g.fillPath(arrowPath);

            // Optionally, draw a black border around the arrow if needed
             g.setColour(juce::Colours::black);
             g.strokePath(arrowPath, juce::PathStrokeType(1.0f));
        }
    }

    // Mouse enter and exit events
    void mouseEnter(const juce::MouseEvent& event) override {
        if (display_label != nullptr) {
            display_label->setText(noteInfo, juce::dontSendNotification);
        }
    }

    void mouseExit(const juce::MouseEvent& event) override {
        if (display_label != nullptr) {
            display_label->setText("", juce::dontSendNotification);
        }
    }

private:
    void generate_info_text() {
        pitch_class = noteNumber % 12;
        int octave = noteNumber / 12 - 1;
        std::map<int, juce::String> pitch_class_to_name = {
            {0, "C"}, {1, "C#"}, {2, "D"},
            {3, "D#"}, {4, "E"}, {5, "F"},
            {6, "F#"}, {7, "G"}, {8, "G#"},
            {9, "A"}, {10, "A#"}, {11, "B"}};

        noteInfo = pitch_class_to_name[pitch_class] + juce::String(octave);
        noteInfo += " (" + juce::String(noteNumber) + ")";
        noteInfo += " | t: " + juce::String(start_time);
        noteInfo += " | Vel: " + juce::String(velocity);
        if (!isHangingNoteOff && !isHangingNoteOn) {
            noteInfo += " | Dur: " + juce::String(duration);
        } else if (isHangingNoteOn) {
            noteInfo += " | Note On";
        } else if (isHangingNoteOff) {
            noteInfo += " | Note Off";
        } else {
            noteInfo += " | Unknown";
        }
    }

};

class PianoRollComponent : public juce::Component, public juce::FileDragAndDropTarget {
public:
    bool AllowToDragInMidi{true};
    bool newFileDraggedIn{false};
    bool AllowToDragOutAsMidi{true};
    bool show_playhead{true};
    juce::String label{"Midi Display"};
    juce::Colour backgroundColour{juce::Colours::whitesmoke};
    juce::Colour playheadColour = juce::Colours::red;
    juce::Label* noteInfoLabel;

    juce::Colour sharpNoteColour = juce::Colours::lightgrey;

    PlaybackSequence displayedSequence{};
    float SequenceDuration{8};

    bool NewDroppedSequenceAccessed{false};

    PianoRollComponent() {
        setWantsKeyboardFocus(true);
    }

    // clear the sequence
    void clear_noteComponents() {
        noteComponents.clear();
        resized();
    }


    void add_complete_noteComponent(int noteNumber_, float start_time_,
                                    float velocity_, float duration_ = -1,
                           juce::Label* display_label_ = nullptr) {

        auto noteComponent = std::make_unique<NoteComponent>();
        noteComponent->completeNote(
            noteNumber_, start_time_, velocity_, duration_, display_label_);
        noteComponents.push_back(std::move(noteComponent));
        addAndMakeVisible(noteComponents.back().get());
        setSequenceDuration();
        resized();

    }

    void add_hanging_noteOnComponent(int noteNumber_, float start_time_, float velocity_,
                           juce::Label* display_label_ = nullptr) {
        auto noteComponent = std::make_unique<NoteComponent>();
        noteComponent->hangingNoteOn(noteNumber_, start_time_, velocity_, display_label_);
        noteComponents.push_back(std::move(noteComponent));
        addAndMakeVisible(noteComponents.back().get());
        resized();
    }

    void add_hanging_noteOffComponent(int noteNumber_, float start_time_, float velocity_,
                           juce::Label* display_label_ = nullptr) {
        auto noteComponent = std::make_unique<NoteComponent>();
        noteComponent->hangingNoteOff(noteNumber_, start_time_, velocity_, display_label_);
        noteComponents.push_back(std::move(noteComponent));
        addAndMakeVisible(noteComponents.back().get());
        resized();
    }


    // paints the sequence
    void paint(juce::Graphics& g) override {

        // ---------------------
        // Draw background (12 notes
        // ---------------------
        g.fillAll(backgroundColour);

        // ---------------------
        // divide the component into 12 rows
        // Starting from C0 (MIDI note 12)
        // Draw a horizontal rectangle for each row with the corresponding note name
        // and color the row if it is a sharp note
        // ---------------------
        int numRows = 12;
        float rowHeight = (float)getHeight() / (float)numRows;
        g.setFont(juce::Font(getHeight() / 10.0f)); // Font size
        vector<juce::String> noteNames = {"C", "", "D", "", "E", "F",
                                          "", "G", "", "A", "", "B"};

        for (int i = 0; i < numRows; ++i)
        {
            float y = float(numRows - 1 - i) * rowHeight;
            g.setColour(juce::Colours::lightgrey);
            g.drawRect(juce::Rectangle<float>(0, y, (float)getWidth(), rowHeight), .1);

            // Draw note name
            juce::String noteName = noteNames[i];

            // Color the row if it is a sharp note
            if (noteName == "")
            {
                g.setColour(sharpNoteColour);
                g.fillRect(juce::Rectangle<float>(0, y, getWidth(), rowHeight));
            }
        }

    }

    // resizes the note components
    void resized() override {
        for (auto& noteComponent : noteComponents) {
            float x = noteComponent->start_time / SequenceDuration * (float)getWidth();
            float length = 0;
            float height = 1.0f / 12.0f * (float)getHeight();
            if (noteComponent->duration > 0) {
                length = noteComponent->duration / SequenceDuration * (float)getWidth();
                length = max(length, 1.0f);
            } else {
                // will draw a small circle if the note has no note off event
                length = height;
            }
            float y = (12 - noteComponent->pitch_class - 1) / 12.0f * (float)getHeight();

            noteComponent->setBounds(x, y, length, height);
        }

    }

    std::vector<std::unique_ptr<NoteComponent>>& get_noteComponents() {
        return noteComponents;
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (AllowToDragOutAsMidi)
        {
            // populate the midi file with the notes from the piano roll
            auto mfile = juce::MidiFile();
            float ticksPerQuarterNote = 960;
            mfile.setTicksPerQuarterNote((int) ticksPerQuarterNote);  \
            juce::MidiMessageSequence sequence;

            for (auto& noteComponent : noteComponents)
            {
                if (noteComponent->isHangingNoteOff || noteComponent->isHangingNoteOn) {
                    continue;
                } else {
                    auto noteNumber = noteComponent->noteNumber;
                    auto start_time = noteComponent->start_time;
                    auto duration = noteComponent->duration;
                    auto velocity = noteComponent->velocity;

                    sequence.addEvent(
                        juce::MidiMessage::noteOn(1, noteNumber, juce::uint8(velocity*127)),
                        start_time * ticksPerQuarterNote);
                    sequence.addEvent(juce::MidiMessage::noteOff(1, noteNumber),
                                      (start_time + duration) * ticksPerQuarterNote);
                }

            }


            mfile.addTrack(sequence);

            // save the midi file to a temporary file
            juce::String fileName = juce::Uuid().toString() + ".mid";  // Random filename

            juce::File outf (juce::File::getSpecialLocation(
                                  juce::File::SpecialLocationType::tempDirectory
                                  ).getChildFile(fileName));

            juce::TemporaryFile tempf(outf);
            {
                auto p_os = std::make_unique<juce::FileOutputStream>(
                    tempf.getFile());
                if (p_os->openedOk()) {
                    mfile.writeTo(*p_os, 0);
                }
            }

            bool succeeded = tempf.overwriteTargetFileWithTemporary();
            if (succeeded) {
                juce::DragAndDropContainer::performExternalDragDropOfFiles(
                    {outf.getFullPathName()}, false);
            }
        }
    }

    bool isInterestedInFileDrag (const juce::StringArray& files) override
    {
        // Check if the files are of a type you're interested in
        // Return true to indicate interest
        return true;
    }

    void filesDropped (const juce::StringArray& files, int x, int y) override
    {
        auto file = juce::File(files[0]);

        // check if midi and exists
        if (files[0].endsWith(".mid") && file.exists())
        {
            // load the midi file
            loadMidiFile(file);
        }

    }

    void fileDragEnter (const juce::StringArray& files, int x, int y) override {

    }

    // call back function for drag and drop into the component
    // returns true if the file was successfully loaded
    // called from filesDropped() in PluginEditor.cpp
    // keep in mind that a file can be dropped anywhere in the parent component
    // (i.e. PluginEditor.cpp) and not just on this component
    // *NOTE* As soon as loaded, all timings are converted to quarter notes instead of ticks
    bool loadMidiFile(const juce::File& file)
    {
        if (!AllowToDragInMidi) { return false; }

        auto stream = file.createInputStream();


        if (stream != nullptr)
        {
            juce::MidiFile loadedMFile;
            if (loadedMFile.readFrom(*stream))
            {
                // MidiMessageSequence
                juce::MidiMessageSequence quarter_note_sequence;

                // get the start time of the first note
                int TPQN = loadedMFile.getTimeFormat();
                if (TPQN > 0)
                {
                    // Iterate through all notes in track 0 of the original file
                    auto track = loadedMFile.getTrack(0);
                    if (track != nullptr)
                    {
                        // Iterate through all notes in track 0 of the copy for conversion
                        auto quarterNoteTrack = loadedMFile.getTrack(0);
                        if (quarterNoteTrack != nullptr)
                        {
                            for (int i = 0; i < quarterNoteTrack->getNumEvents(); ++i)
                            {
                                auto event = quarterNoteTrack->getEventPointer(i);
                                event->message.setTimeStamp(
                                    event->message.getTimeStamp() / TPQN);
                                // add to quarter_note_sequence
                                quarter_note_sequence.addEvent(event->message);
                            }
                        }
                    }

                    quarter_note_sequence.updateMatchedPairs();

                    // iterate through all note ons and find corresponding note offs
                    // and add them to the displayed sequence

                    noteComponents.clear();

                    for (int i = 0; i < quarter_note_sequence.getNumEvents(); ++i)
                    {
                        auto event = quarter_note_sequence.getEventPointer(i);
                        if (event->message.isNoteOn())
                        {
                            int noteNumber = event->message.getNoteNumber();
                            float start_time = event->message.getTimeStamp();
                            float duration = 0;
                            float velocity = event->message.getFloatVelocity();

                            // find the corresponding note off
                            for (int j = i + 1; j < quarter_note_sequence.getNumEvents(); ++j)
                            {
                                auto offEvent = quarter_note_sequence.getEventPointer(j);
                                if (offEvent->message.isNoteOff() &&
                                    offEvent->message.getNoteNumber() == noteNumber &&
                                    offEvent->message.getTimeStamp() >= event->message.getTimeStamp())
                                {
                                    double noteLength = offEvent->message.getTimeStamp() -
                                                        event->message.getTimeStamp();
                                    duration = float(noteLength);
                                    break;
                                }
                            }

                            add_complete_noteComponent(
                                noteNumber, start_time,
                                velocity, duration, noteInfoLabel);
                        }
                    }

                    repaint();

                    return true;

                } else {
                    // Negative value indicates frames per second (SMPTE)
                    cout << "SMPTE Format midi files are not supported at this time." << endl;
                }

                if (pianoRollData != nullptr) {
                    auto track = loadedMFile.getTrack(0);
                    pianoRollData->setSequence(*track, true);

                }
            }
        }



        return false;
    }

    void setPianoRollData(PianoRollData* pianoRollData_) {
        pianoRollData = pianoRollData_;
    }

private:
    std::vector<std::unique_ptr<NoteComponent>> noteComponents;
    PianoRollData* pianoRollData{nullptr};
    void setSequenceDuration() {
        // find the last note off event
        float max_time = 0;
        for (auto& noteComponent : noteComponents) {
            if (noteComponent->start_time + noteComponent->duration > max_time) {
                max_time = noteComponent->start_time + noteComponent->duration;
            }
        }
        // make sure the max time is multiple of 4
        float remainder = fmod(max_time, 4);
        if (remainder > 0) {
            max_time += 4 - remainder;
        }
        SequenceDuration = max_time;
        resized();
    }
};

class MidiVisualizer: public juce::Component, juce::Timer {

public:
    /*bool AllowToDragInMidi{true};
    bool AllowToDragOutAsMidi{true};*/

    MidiVisualizer(bool needsPlayhead_,
                   string paramID_) {
        setInterceptsMouseClicks(false, true);
        paramID = std::move(paramID_);
        std::transform(paramID.begin(), paramID.end(), paramID.begin(), ::toupper);
        needsPlayhead = needsPlayhead_;
        noteInfoLabel.setFont(10);
        pianoRollComponent.noteInfoLabel = &noteInfoLabel;
        addAndMakeVisible(playheadVisualizer);
        addAndMakeVisible(pianoRollComponent);
        addAndMakeVisible(pianoKeysComponent);
        addAndMakeVisible(noteInfoLabel);

        // 4 on the floor kick snare pattern
        pianoRollComponent.add_complete_noteComponent(
            36, 0.1, 0.1f,
            0.1f, &noteInfoLabel);
        pianoRollComponent.add_complete_noteComponent(
            36, 1, 0.3f,
            0.1f, &noteInfoLabel);
        pianoRollComponent.add_complete_noteComponent(
            36, 2, 0.5f,
            0.1f, &noteInfoLabel);
        pianoRollComponent.add_complete_noteComponent(
            36, 3, 0.6f,
            0.1f, &noteInfoLabel);
        pianoRollComponent.add_complete_noteComponent(
            38, 1, 0.8f,
            0.1f, &noteInfoLabel);
        pianoRollComponent.add_complete_noteComponent(
            38, 3, 1.0f,
            0.1f, &noteInfoLabel);

        // 1 hihat without note off
        pianoRollComponent.add_hanging_noteOnComponent(
            42, 0, 0.1f, &noteInfoLabel);
        // 1 tom without note on
        pianoRollComponent.add_hanging_noteOffComponent(
            43, 0.1, 0.0f, &noteInfoLabel);

        // start timer
        startTimerHz(10);
    }

    void resized() override {
        // 10% of the height for the playhead
        // 90% of the height for the piano roll
        auto area = getLocalBounds();

        noteInfoLabel.setBounds(area.removeFromBottom(area.getHeight() * 0.1));

        auto key_area = area.removeFromLeft(20);


        if (needsPlayhead) {
            auto phead_height = area.getHeight() * 0.1;
            pianoRollComponent.setBounds(
                area.removeFromTop(area.getHeight() * 0.9));
            playheadVisualizer.setBounds(
                area.removeFromTop(phead_height));

            key_area.removeFromBottom(phead_height);
            pianoKeysComponent.setBounds(key_area);
        } else {
            pianoRollComponent.setBounds(area);
            pianoKeysComponent.setBounds(key_area);
        }

    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::black);
    }

    void enableDragInMidi(bool enable) {
        pianoRollComponent.AllowToDragInMidi = enable;
    }

    void enableDragOutAsMidi(bool enable) {
        pianoRollComponent.AllowToDragOutAsMidi = enable;
    }

    void setpianoRollData(PianoRollData* pianoRollData_) {
        pianoRollComponent.setPianoRollData(pianoRollData_);
        pianoRollData = pianoRollData_;
    }

    // timer callback
    void timerCallback() override
    {
        if (pianoRollData != nullptr)
        {
            if (pianoRollData->shouldRepaint()
                && !pianoRollData->userDroppedNewSequence())
            {
                pianoRollComponent.clear_noteComponents();

                auto sequence = pianoRollData->getCurrentSequence();
                sequence.sort();
                sequence.updateMatchedPairs();

                auto CompleteNotes = vector<pair<MidiMessage, MidiMessage>>();

                for (int i = 0; i < sequence.getNumEvents(); ++i)
                {
                    auto event = sequence.getEventPointer(i);
                    if (event->message.isNoteOn())
                    {
                        if (event->noteOffObject != nullptr)
                        {
                            pianoRollComponent.add_complete_noteComponent(
                                event->message.getNoteNumber(),
                                event->message.getTimeStamp(),
                                event->message.getFloatVelocity(),
                                event->noteOffObject->message.getTimeStamp() -
                                event->message.getTimeStamp(),
                                &noteInfoLabel);

                            CompleteNotes.push_back(
                                pair<MidiMessage, MidiMessage>(
                                    event->message, event->noteOffObject->message));
                        }
                        else
                        {
                            pianoRollComponent.add_hanging_noteOnComponent(
                                event->message.getNoteNumber(),
                                event->message.getTimeStamp(),
                                event->message.getFloatVelocity(),
                                &noteInfoLabel);
                        }

                    }

                    // find note offs that don't have a corresponding note on
                    else if (event->message.isNoteOff())
                    {
                        // check if message already in CompleteNotes
                        bool found = false;
                        for (auto& pair : CompleteNotes)
                        {
                            if (pair.second.getNoteNumber() == event->message.getNoteNumber()
                                && pair.second.getChannel() == event->message.getChannel() &&
                                pair.second.getTimeStamp() == event->message.getTimeStamp())
                            {
                                found = true;
                                break;
                            }
                        }

                        if (!found)
                        {
                            pianoRollComponent.add_hanging_noteOffComponent(
                                event->message.getNoteNumber(),
                                event->message.getTimeStamp(),
                                event->message.getFloatVelocity(),
                                &noteInfoLabel);
                        }

                    }

                }

            }

        }

    }

    std::string getParamID() {

        return paramID;
    }

private:
    juce::MidiFile midiFile;

    PlayheadVisualizer playheadVisualizer;
    PianoRollComponent pianoRollComponent;
    PianoKeysComponent pianoKeysComponent{
        juce::Colours::whitesmoke,
        juce::Colours::grey};
    juce::Label noteInfoLabel; // displays the note name when the mouse hovers over a note
    bool needsPlayhead{false};
    PianoRollData* pianoRollData{nullptr};
    std::string paramID;

};

