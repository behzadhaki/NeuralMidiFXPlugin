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
        // Set background color to grey

    }

    ~MidiPianoRollComponent() {}

    // Generate a random MidiFile
    void generateRandomMidiFile(int numNotes)
    {
        midiFile = juce::MidiFile();

        juce::MidiMessageSequence sequence;

        for (int i = 0; i < numNotes; ++i)
        {
            int noteNumber = juce::Random::getSystemRandom().nextInt({21, 108});
            int velocity = juce::Random::getSystemRandom().nextInt({20, 127});
            double startTime = juce::Random::getSystemRandom().nextDouble() * 10.0;
            double noteDuration = juce::Random::getSystemRandom().nextDouble() * 2.0 + 0.5f; // duration in quarter notes
            sequence.addEvent(juce::MidiMessage::noteOn(1, noteNumber, juce::uint8(velocity)), startTime);
            sequence.addEvent(juce::MidiMessage::noteOff(1, noteNumber), startTime + noteDuration);
        }

        midiFile.addTrack(sequence);

        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::whitesmoke);
        // Your own drawing code goes here
        if (midiFile.getNumTracks() > 0)
        {
            auto track = midiFile.getTrack(0);
            if (track != nullptr)
            {
                double midiFileLength = track->getEndTime();
                int numRows = 10;
                float rowHeight = getHeight() / numRows;
                int note_count = 0;
                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto event = track->getEventPointer(i);
                    if (event != nullptr && event->message.isNoteOn())
                    {
                        float y = (note_count % numRows) * rowHeight;
                        float x = (event->message.getTimeStamp() / midiFileLength) * getWidth();

                        // find the corresponding "note off" event
                        float length = 0;
                        for (int j = i + 1; j < track->getNumEvents(); ++j)
                        {
                            auto offEvent = track->getEventPointer(j);
                            if (offEvent != nullptr && offEvent->message.isNoteOff() && offEvent->message.getNoteNumber() == event->message.getNoteNumber())
                            {
                                double noteLength = offEvent->message.getTimeStamp() - event->message.getTimeStamp();
                                length = (noteLength / midiFileLength) * getWidth();
                                break;
                            }
                        }

                        // set color according to the velocity
                        float velocity = event->message.getFloatVelocity();
                        juce::Colour noteColour = juce::Colours::skyblue.withAlpha(velocity); // blue with opacity corresponding to velocity

                        g.setColour(noteColour);
                        g.fillRect(juce::Rectangle<float>(x, y, length, rowHeight));
                        note_count++;
                    }
                }
            }
        }
    }


private:
    juce::MidiFile midiFile;
};

