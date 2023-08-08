//
// Created by Behzad Haki on 8/3/2023.
//

#pragma once
#include "GuiParameters.h"


using namespace std;

class MidiPianoRollComponent : public juce::Component
{
public:

    explicit MidiPianoRollComponent(bool isForGenerations)
    {
        Initialize();

        if (isForGenerations) {
            backgroundColour = juce::Colours::black;
            noteColour = juce::Colours::white;
            allow_drag_out = true;
        } else {
            backgroundColour = juce::Colours::whitesmoke;
            noteColour = juce::Colours::skyblue;
            allow_drag_in = true;
        }
    }

    explicit MidiPianoRollComponent(bool isForGenerations,
                                    LockFreeQueue<juce::MidiFile, 4>* MidiQue_)
    {
        Initialize();

        if (isForGenerations) {
            backgroundColour = juce::Colours::black;
            noteColour = juce::Colours::white;
            allow_drag_out = true;
        } else {
            backgroundColour = juce::Colours::whitesmoke;
            noteColour = juce::Colours::skyblue;
            allow_drag_in = true;
        }

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

        repaint();
    }

    // draws midi notes on the component (piano roll)
    // maps the midi notes to the component's width and height
    // pitch values are mapped to the y axis, and time values are mapped to the x axis
    // all pitch values are shifted into a 24 note range (C1 to C3)
    // velocity values are mapped to the alpha value of the note color
    void paint(juce::Graphics& g) override
    {
        g.fillAll (backgroundColour);

        double midiFileLength =  8;
        int ticksPerQuarterNote = 960;

        if (midiFile.getNumTracks() > 0)
        {
            auto track = midiFile.getTrack(0);
            if (track != nullptr)
            {
                midiFileLength = std::max(track->getEndTime()+4*ticksPerQuarterNote,
                                          playhead_pos+4*ticksPerQuarterNote);
                int numRows = 24;  // number of pitch values
                float rowHeight =  (float) getHeight() / (float) numRows;

                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto event = track->getEventPointer(i);
                    if (event != nullptr && event->message.isNoteOn())
                    {
                        int noteNumber = event->message.getNoteNumber() % 24;

                        float y = float(23 - noteNumber) * rowHeight;  // flip the y axis
                        float x = float(event->message.getTimeStamp() / midiFileLength) *
                                  (float) getWidth();

                        // find the corresponding "note off" event
                        float length = 0;
                        for (int j = i + 1; j < track->getNumEvents(); ++j)
                        {
                            auto offEvent = track->getEventPointer(j);
                            if (offEvent != nullptr && offEvent->message.isNoteOff() &&
                                offEvent->message.getNoteNumber() % 24 == noteNumber)
                            {
                                double noteLength = offEvent->message.getTimeStamp() -
                                                    event->message.getTimeStamp();
                                length = float(noteLength / midiFileLength) *
                                         (float) getWidth();
                                break;
                            }
                        }

                        // set color according to the velocity
                        float velocity = event->message.getFloatVelocity();

                        auto color = noteColour.withAlpha(velocity);
                        g.setColour(color);
                        g.fillRect(juce::Rectangle<float>(
                            x, y, length, rowHeight));
                    }
                }
            }
        }

        // draw playhead
        if (playhead_pos >= 0) {
            g.setColour(juce::Colours::red);
            auto x = playhead_pos / midiFileLength *
                     (float) getWidth();
            g.drawLine(x, 0, x, getHeight(), 2);
        }

    }


    // call back function for drag and drop out of the component
    // should drag from component to file explorer or DAW if allowed
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (allow_drag_out) {
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

    // call back function for drag and drop into the component
    // returns true if the file was successfully loaded
    // called from filesDropped() in PluginEditor.cpp
    // keep in mind that a file can be dropped anywhere in the parent component
    // (i.e. PluginEditor.cpp) and not just on this component
    // *NOTE* As soon as loaded, all timings are converted to quarter notes instead of ticks
    bool loadMidiFile(const juce::File& file)
    {
        if (!allow_drag_in) {
            return false;
        }

        auto stream = file.createInputStream();
        if (stream != nullptr)
        {
            if (midiFile.readFrom(*stream))
            {
                // Successfully read the MIDI file, so repaint the component

                // get the start time of the first note
                int TPQN = midiFile.getTimeFormat();
                if (TPQN > 0)
                {

                    // iterate through all notes in track 0
                    auto track = midiFile.getTrack(0);
                    if (track != nullptr)
                    {
                        for (int i = 0; i < track->getNumEvents(); ++i)
                        {
                            auto event = track->getEventPointer(i);
                            event->message.setTimeStamp(
                                event->message.getTimeStamp() / TPQN);
                        }
                    }
                }
                else
                {
                    // Negative value indicates frames per second (SMPTE)
                    DBG("SMPTE Format midi files are not supported at this time.");
                }

                MidiQue->push(midiFile);
                repaint();
                return true;
            }
        }

        return false;
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
    juce::Colour backgroundColour{juce::Colours::whitesmoke};
    juce::Colour noteColour = juce::Colours::skyblue;
    bool allow_drag_in{false};
    bool allow_drag_out{false};
    LockFreeQueue<juce::MidiFile, 4>* MidiQue{};
    double playhead_pos{-1};

};

