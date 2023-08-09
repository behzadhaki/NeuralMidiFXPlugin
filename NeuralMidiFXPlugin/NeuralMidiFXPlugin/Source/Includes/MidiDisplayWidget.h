//
// Created by Behzad Haki on 8/3/2023.
//

#pragma once
#include "GuiParameters.h"


using namespace std;


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
                                    double fs, double qpm,
                                    double playhead_pos_quarter_notes) {
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
        playhead_pos = playhead_pos_quarter_notes * ticksPerQuarterNote;
        midiFile.addTrack(sequence);

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

    void setLength(double length) {
        disp_length = length;
        repaint();
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

        std::set<int> uniquePitches;

        if (midiFile.getNumTracks() > 0)
        {
            auto track = midiFile.getTrack(0);
            if (track != nullptr)
            {
                // Find unique pitches in the track
                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto event = track->getEventPointer(i);
                    if (event != nullptr && event->message.isNoteOn())
                    {
                        uniquePitches.insert(event->message.getNoteNumber());
                    }
                }

                int numRows = uniquePitches.size(); // Number of unique pitch values
                float rowHeight = (float)getHeight() / (float)numRows;

                // Set font size for the labels
                g.setFont(juce::Font(10.0f)); // 10.0f is the font size, you can adjust as needed

                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto event = track->getEventPointer(i);
                    if (event != nullptr && event->message.isNoteOn())
                    {
                        int noteNumber = event->message.getNoteNumber();

                        float y = float(numRows - 1 - std::distance(uniquePitches.begin(), uniquePitches.find(noteNumber))) * rowHeight;
                        float x = float(event->message.getTimeStamp() / disp_length) *
                                  (float)getWidth();

                        // Add the corresponding "note off" event
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

                        // Set color according to the velocity
                        float velocity = event->message.getFloatVelocity();

                        auto color = noteColour.withAlpha(velocity);
                        g.setColour(color);
                        g.fillRect(juce::Rectangle<float>(
                            x, y, length, rowHeight));

                        // Draw border around the note
                        g.setColour(juce::Colours::black);
                        g.drawRect(
                            juce::Rectangle<float>(
                                x, y, length, rowHeight
                                ), 1);

                        // Draw label for the pitch class and octave
                        juce::String noteLabel = MidiMessage::getMidiNoteName(
                            noteNumber, true,
                            true, 3); // 3 is the default middle C octave
                        g.setColour(juce::Colours::black);
                        g.drawText(noteLabel, x, y, length, rowHeight,
                                   juce::Justification::centred, true);
                    }
                }
            }
        }

        // Draw playhead
        if (playhead_pos >= 0)
        {
            g.setColour(juce::Colours::red);
            auto x = playhead_pos / disp_length *
                     (float)getWidth();
            g.drawLine(x, 0, x, getHeight(), 2);
        }
    }

    // call back function for drag and drop out of the component
    // should drag from component to file explorer or DAW if allowed
    void mouseDrag(const juce::MouseEvent& event) override
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

};


class InputMidiPianoRollComponent : public juce::Component
{
public:

    explicit InputMidiPianoRollComponent()
    {
        Initialize();

    }

    explicit InputMidiPianoRollComponent(LockFreeQueue<juce::MidiFile, 4>* MidiQue_)
    {
        Initialize();

        MidiQue = MidiQue_;
        if (MidiQue_->getNumberOfWrites() > 0) {
            DraggedMidi = MidiQue_->getLatestDataWithoutMovingFIFOHeads();
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
        if (diplayMidi.getNumTracks() > 0)
        {
            auto track = diplayMidi.getTrack(0);
            if (track != nullptr)
            {
                return std::max(track->getEndTime() + 4 * ticksPerQuarterNote,
                                playhead_pos + 4 * ticksPerQuarterNote);
            }
        }
        return 0;
    }

    void setLength(double length) {
        disp_length = length;
        repaint();
    }

    void displayMidiMessageSequence(PlaybackPolicies playbackPolicy_,
                                    double fs, double qpm,
                                    double playhead_pos_quarter_notes) {
//        DraggedMidi = juce::MidiFile();
        int ticksPerQuarterNote = 960;
//        DraggedMidi.setTicksPerQuarterNote(ticksPerQuarterNote);  \
//        juce::MidiMessageSequence sequence;
//
//        // all timings need to be converted to ticks
//        for (auto m: sequence_) {
//            auto msg = m->message;
//            if (playbackPolicy_.IsTimeUnitIsAudioSamples()) {
//                auto time_in_quartern = msg.getTimeStamp() / fs / 60.0 * qpm;
//                // convert to ticks
//                msg.setTimeStamp(time_in_quartern * ticksPerQuarterNote);
//            } else if (playbackPolicy_.IsTimeUnitIsPPQ()) {
//                // convert to ticks
//                msg.setTimeStamp(msg.getTimeStamp() * ticksPerQuarterNote);
//            } else if (playbackPolicy_.IsTimeUnitIsSeconds()) {
//                auto time_in_quartern = msg.getTimeStamp() / 60.0 * qpm;
//                // convert to ticks
//                msg.setTimeStamp(time_in_quartern * ticksPerQuarterNote);
//            }
//            sequence.addEvent(msg);
//        }
        playhead_pos = playhead_pos_quarter_notes * ticksPerQuarterNote;
//        DraggedMidi.addTrack(sequence);

        repaint();
    }

    void clearIncomingMidi()
    {
        IncomingMidi = juce::MidiFile();
        repaint();
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

        int ticksPerQuarterNote = 960;

        std::set<int> uniquePitches;


        if (DraggedMidi.getNumTracks() > 0)
        {
            auto track = DraggedMidi.getTrack(0);
            if (track != nullptr)
            {
                // Find unique pitches in the track
                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto event = track->getEventPointer(i);
                    if (event != nullptr && event->message.isNoteOn())
                    {
                        uniquePitches.insert(event->message.getNoteNumber());
                    }
                }

                int numRows = uniquePitches.size(); // Number of unique pitch values
                float rowHeight = (float)getHeight() / (float)numRows;


                // Set font size for the labels
                g.setFont(juce::Font(10.0f)); // 10.0f is the font size, you can adjust as needed

                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto event = track->getEventPointer(i);
                    if (event != nullptr && event->message.isNoteOn())
                    {
                        int noteNumber = event->message.getNoteNumber();

                        float y = float(numRows - 1 - std::distance(uniquePitches.begin(), uniquePitches.find(noteNumber))) * rowHeight;
                        float x = float(event->message.getTimeStamp() / disp_length) *
                                  (float)getWidth();

                        // Add the corresponding "note off" event
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

                        // Set color according to the velocity
                        float velocity = event->message.getFloatVelocity();

                        auto color = DraggedNoteColour.withAlpha(velocity);
                        g.setColour(color);
                        g.fillRect(juce::Rectangle<float>(
                            x, y, length, rowHeight));

                        // Draw border around the note
                        g.setColour(juce::Colours::black);
                        g.drawRect(
                            juce::Rectangle<float>(
                                x, y, length, rowHeight
                                ), 1);

                        // Draw label for the pitch class and octave
                        juce::String noteLabel = MidiMessage::getMidiNoteName(
                            noteNumber, true,
                            true, 3); // 3 is the default middle C octave
                        g.setColour(juce::Colours::black);
                        g.drawText(noteLabel, x, y, length, rowHeight,
                                   juce::Justification::centred, true);
                    }
                }
            }
        }

        if (IncomingMidi.getNumTracks() > 0)
        {
            auto track = IncomingMidi.getTrack(0);
            if (track != nullptr)
            {
                // Find unique pitches in the track
                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto event = track->getEventPointer(i);
                    if (event != nullptr && event->message.isNoteOn())
                    {
                        uniquePitches.insert(event->message.getNoteNumber());
                    }
                }

                int numRows = uniquePitches.size(); // Number of unique pitch values
                float rowHeight = (float)getHeight() / (float)numRows;


                // Set font size for the labels
                g.setFont(juce::Font(10.0f)); // 10.0f is the font size, you can adjust as needed

                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto event = track->getEventPointer(i);
                    if (event != nullptr && event->message.isNoteOn())
                    {
                        int noteNumber = event->message.getNoteNumber();

                        float y = float(numRows - 1 - std::distance(uniquePitches.begin(), uniquePitches.find(noteNumber))) * rowHeight;
                        float x = float(event->message.getTimeStamp() / disp_length) *
                                  (float)getWidth();

                        // Add the corresponding "note off" event
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

                        // Set color according to the velocity
                        float velocity = event->message.getFloatVelocity();

                        auto color = IncomingNoteColour.withAlpha(velocity);
                        g.setColour(color);
                        g.fillRect(juce::Rectangle<float>(
                            x, y, length, rowHeight));

                        // Draw border around the note
                        g.setColour(juce::Colours::black);
                        g.drawRect(
                            juce::Rectangle<float>(
                                x, y, length, rowHeight
                                ), 1);

                        // Draw label for the pitch class and octave
                        juce::String noteLabel = MidiMessage::getMidiNoteName(
                            noteNumber, true,
                            true, 3); // 3 is the default middle C octave
                        g.setColour(juce::Colours::black);
                        g.drawText(noteLabel, x, y, length, rowHeight,
                                   juce::Justification::centred, true);
                    }
                }
            }
        }

        // Draw playhead
        if (playhead_pos >= 0)
        {
            g.setColour(juce::Colours::red);
            auto x = playhead_pos / disp_length *
                     (float)getWidth();
            g.drawLine(x, 0, x, getHeight(), 2);
        }
    }

    // call back function for drag and drop out of the component
    // should drag from component to file explorer or DAW if allowed
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (diplayMidi.getNumTracks() <= 0)
        {
            return ;
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
        auto stream = file.createInputStream();
        if (stream != nullptr)
        {
            if (DraggedMidi.readFrom(*stream))
            {
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

                    int ticksPerQuarterNote = 960;
                    diplayMidi.clear();
                    diplayMidi.setTicksPerQuarterNote(ticksPerQuarterNote);
                    // add content of DraggedMidi and IncomingMidi to diplayMidi
                    if (DraggedMidi.getNumTracks() > 0)
                    {
                        auto track = DraggedMidi.getTrack(0);
                        if (track != nullptr)
                        {
                            diplayMidi.addTrack(*track);
                        }
                    }
                    if (IncomingMidi.getNumTracks() > 0)
                    {
                        auto track = IncomingMidi.getTrack(0);
                        if (track != nullptr)
                        {
                            diplayMidi.addTrack(*track);
                        }
                    }

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

        diplayMidi = juce::MidiFile();
        diplayMidi.setTicksPerQuarterNote(960);
        // Enable drag and drop
        //        setInterceptsMouseClicks(false, true);
        setInterceptsMouseClicks(
            true, true);
    }

    juce::MidiFile DraggedMidi;
    juce::MidiFile IncomingMidi;
    juce::MidiFile diplayMidi;
    juce::Colour backgroundColour{juce::Colours::whitesmoke};
    juce::Colour DraggedNoteColour = juce::Colours::skyblue;
    juce::Colour IncomingNoteColour = juce::Colours::darkolivegreen;
    LockFreeQueue<juce::MidiFile, 4>* MidiQue{};
    double playhead_pos{-1};
    double disp_length{8};

};
