//
// Created by Behzad Haki on 8/3/2023.
//

#pragma once
#include "GuiParameters.h"
#include "Configs_Parser.h"

using namespace std;

class InputMidiPianoRollComponent : public juce::Component
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
            g.drawLine(x, 0, x, getHeight(), 2);
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
                    DBG("SMPTE Format midi files are not supported at this time.");
                }
            }
        }



        return false;
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
        int numRows = uniquePitches.size();
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
        int ticksPerQuarterNote = 960;
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
            g.fillRect(xStart, 0, xEnd - xStart, getHeight());
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

    void drawNotes(juce::Graphics& g, const juce::Colour& noteColour)
    {
        int numRows = uniquePitches.size();
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
                auto color = noteColour.withAlpha(velocity); // Set alpha to velocity
                g.setColour(color);
                g.fillRect(juce::Rectangle<float>(x, y, length, rowHeight));

                // Draw border around the note
                g.setColour(juce::Colours::black);
                g.drawRect(juce::Rectangle<float>(x, y, length, rowHeight), 1);
            }
        }
    }
};

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

class PianoRollComponent : public juce::Component {
public:
    bool allowToDragInMidi{true};
    bool allowToDragOutAsMidi{true};
    bool show_playhead{true};
    juce::String label{"Midi Display"};
    juce::Colour backgroundColour{juce::Colours::whitesmoke};
    juce::Colour playheadColour = juce::Colours::red;

    juce::Colour sharpNoteColour = juce::Colours::lightgrey;

    PlaybackSequence displayedSequence{};
    float SequenceDuration{8};

    bool NewDroppedSequenceAccessed{false};

    // shows a sequence assuming the specified duration (as such, time unit agnostic)
    void show_sequence(PlaybackSequence& sequence, float duration) {
        displayedSequence = sequence;
        SequenceDuration = duration;
        NewDroppedSequenceAccessed = true;
        repaint();
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

};

class MidiVisualizer: public juce::Component {
public:
    MidiVisualizer(bool needsPlayhead_) {
        needsPlayhead = needsPlayhead_;
        addAndMakeVisible(playheadVisualizer);
        addAndMakeVisible(pianoRollComponent);
        addAndMakeVisible(pianoKeysComponent);
    }

    void resized() override {
        // 10% of the height for the playhead
        // 90% of the height for the piano roll
        auto area = getLocalBounds();
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
private:
    PlayheadVisualizer playheadVisualizer;
    PianoRollComponent pianoRollComponent;
    PianoKeysComponent pianoKeysComponent{
        juce::Colours::whitesmoke,
        juce::Colours::grey};
    bool needsPlayhead{false};
};