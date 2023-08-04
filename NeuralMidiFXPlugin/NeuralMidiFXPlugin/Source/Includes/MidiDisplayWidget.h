//
// Created by Behzad Haki on 8/3/2023.
//

#pragma once
#include "GuiParameters.h"


using namespace std;


class MidiPianoRollComponent : public juce::Component
{
public:
    MidiPianoRollComponent()
    {
        // Load an empty MidiFile to start
        midiFile = juce::MidiFile();
        midiFile.setTicksPerQuarterNote(960);


        // Enable drag and drop
//        setInterceptsMouseClicks(false, true);
        setInterceptsMouseClicks(true, true);

    }

    ~MidiPianoRollComponent() {}

    void generateRandomMidiFile(int numNotes)
    {
        midiFile = juce::MidiFile();
        int ticksPerQuarterNote = 960;
        midiFile.setTicksPerQuarterNote(ticksPerQuarterNote);  // This line was missing

        juce::MidiMessageSequence sequence;

        for (int i = 0; i < numNotes; ++i)
        {
            int noteNumber = juce::Random::getSystemRandom().nextInt({21, 108});
            int velocity = juce::Random::getSystemRandom().nextInt({20, 127});
            double startTime = juce::Random::getSystemRandom().nextDouble() * 10.0;
            double noteDuration = juce::Random::getSystemRandom().nextDouble() * 2.0 + 0.5f; // duration in quarter notes

            sequence.addEvent(juce::MidiMessage::noteOn(1, noteNumber, juce::uint8(velocity)), startTime * ticksPerQuarterNote);
            sequence.addEvent(juce::MidiMessage::noteOff(1, noteNumber), (startTime + noteDuration) * ticksPerQuarterNote);
        }

        midiFile.addTrack(sequence);

        repaint();
    }


    void paint(juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::whitesmoke);

        if (midiFile.getNumTracks() > 0)
        {
            auto track = midiFile.getTrack(0);
            if (track != nullptr)
            {
                double midiFileLength = track->getEndTime();
                int numRows = 24;  // number of pitch values
                float rowHeight = getHeight() / numRows;

                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto event = track->getEventPointer(i);
                    if (event != nullptr && event->message.isNoteOn())
                    {
                        int noteNumber = event->message.getNoteNumber() % 24;

                        float y = (23 - noteNumber) * rowHeight;  // flip the y axis
                        float x = (event->message.getTimeStamp() / midiFileLength) * getWidth();

                        // find the corresponding "note off" event
                        float length = 0;
                        for (int j = i + 1; j < track->getNumEvents(); ++j)
                        {
                            auto offEvent = track->getEventPointer(j);
                            if (offEvent != nullptr && offEvent->message.isNoteOff() && offEvent->message.getNoteNumber() % 24 == noteNumber)
                            {
                                double noteLength = offEvent->message.getTimeStamp() - event->message.getTimeStamp();
                                length = (noteLength / midiFileLength) * getWidth();
                                break;
                            }
                        }

                        // set color according to the velocity
                        float velocity = event->message.getFloatVelocity();
                        juce::Colour noteColour = juce::Colours::skyblue.withAlpha(velocity);

                        g.setColour(noteColour);
                        g.fillRect(juce::Rectangle<float>(x, y, length, rowHeight));
                    }
                }
            }
        }
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {

        juce::File outf = juce::File::getSpecialLocation(juce::File::SpecialLocationType::tempDirectory).getChildFile("test.mid");
        juce::TemporaryFile tempf(outf);
        {
            auto p_os = std::make_unique<juce::FileOutputStream>(tempf.getFile());
            if (p_os->openedOk()) {
                midiFile.writeTo(*p_os, 0);
            }
        }

        bool succeeded = tempf.overwriteTargetFileWithTemporary();
        if (succeeded) {
            juce::DragAndDropContainer::performExternalDragDropOfFiles({outf.getFullPathName()}, false);
        }
    }

    bool loadMidiFile(const juce::File& file)
    {
        auto stream = file.createInputStream();
        if (stream != nullptr)
        {
            if (midiFile.readFrom(*stream))
            {
                // Successfully read the MIDI file, so repaint the component
                repaint();
                return true;
            }
        }

        return false;
    }

private:
    juce::MidiFile midiFile;
};

