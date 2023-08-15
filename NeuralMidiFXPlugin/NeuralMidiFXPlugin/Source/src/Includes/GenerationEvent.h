//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"

using namespace juce;

/*

  This class is used to Notify the Plugin that a new stream of generations is available.
  Moreover, it also contains information about the policy used for playback of generations
 
  PlaybackPolicy:
       1. relative to Now (register the current time, and play messages relative to that)
       2. relative to 0 (stream should be played relative to absolute 0 time)
       3. relative to playback start (stream should be played relative to the time when playback started)
 
  Time_unit:
    1. audio samples
    2. seconds
    3. ppq

 
  Overwrite Policy:
   1. delete all events in previous stream and use new stream
   2. delete all events after now
   3. Keep all previous events (that is generations can be played on top of each other)

   Additional Policies:
   1. Clear generations after pause/stop
   2. Repeat N times Assuming Generartions are T Time Units Long
*/


struct PlaybackPolicies {

    PlaybackPolicies() = default;

    void SetPaybackPolicy_RelativeToNow() { PlaybackPolicy = 1; }

    void SetPlaybackPolicy_RelativeToAbsoluteZero() { PlaybackPolicy = 2; }

    void SetPplaybackPolicy_RelativeToPlaybackStart() { PlaybackPolicy = 3; }

    void SetTimeUnitIsAudioSamples() { TimeUnit = 1; }

    void SetTimeUnitIsSeconds() { TimeUnit = 2; }

    void SetTimeUnitIsPPQ() { TimeUnit = 3; }

    // Deletes all previous events and uses the new stream
    // **ForceSendNoteOffsFirst** is used to kill all notes before starting the new stream
    // This is useful when the new stream is not a continuation of the previous stream
    // and there is a possibility of handing note ons from the previous stream
    void SetOverwritePolicy_DeleteAllEventsInPreviousStreamAndUseNewStream(bool ForceSendNoteOffsFirst_)
    {
        OverwritePolicy = 1;
        ForceSendNoteOffsFirst = ForceSendNoteOffsFirst_;
    }

    // Deletes all events after now (i.e. all previously received generations are kept,
    // (unless the timings of the events are after the timing at which this policy is set)
    void SetOverwritePolicy_DeleteAllEventsAfterNow(bool ForceSendNoteOffsFirst_)
    {
        OverwritePolicy = 2;
        ForceSendNoteOffsFirst = ForceSendNoteOffsFirst_;
    }

    // Keeps all previous events (i.e. generations can be played on top of each other)
    // **ForceSendNoteOffsFirst** is used to kill all notes before appending the new stream
    void SetOverwritePolicy_KeepAllPreviousEvents(bool ForceSendNoteOffsFirst_) {
        OverwritePolicy = 3;
        ForceSendNoteOffsFirst = ForceSendNoteOffsFirst_;
    }

    // previous generations are cleared after pause/stop so they can't be played again
    void SetClearGenerationsAfterPauseStop(bool enable) {
        ClearGenerationsAfterPauseStop = enable;
    }

    // allows looping of generations for a specific duration
    // the start of the loop is specified by the playback start policy
    //    i.e. RelativeToNow, RelativeToAbsoluteZero, RelativeToPlaybackStart
    // FOR ANY TIMING POLICY USED, THE LOOP DURATION MUST BE SPECIFIED IN QUARTER NOTES
    //
    void ActivateLooping(double LoopDurationInQuarterNotes) {
        LoopDuration = LoopDurationInQuarterNotes;
    }

    //
    void DisableLooping() {
        LoopDuration = -1;
    }

    double getLoopDuration() const {
        return LoopDuration;
    }

    // Checks if data is ready for transmission
    bool IsReadyForTransmission() const {
        assert (PlaybackPolicy != -1 && "PlaybackPolicy Not Set");
        assert (TimeUnit != -1 && "TimeUnit Not Set");
        assert (OverwritePolicy != -1 && "OverwritePolicy Not Set");

        return PlaybackPolicy != -1 && TimeUnit != -1 && OverwritePolicy != -1;
    }

    // ============================================================================================================
    bool IsPlaybackPolicy_RelativeToNow() const { return PlaybackPolicy == 1; }
    bool IsPlaybackPolicy_RelativeToAbsoluteZero() const { return PlaybackPolicy == 2; }
    bool IsPlaybackPolicy_RelativeToPlaybackStart() const { return PlaybackPolicy == 3; }

    bool IsTimeUnitIsAudioSamples() const { return TimeUnit == 1; }
    bool IsTimeUnitIsSeconds() const { return TimeUnit == 2; }
    bool IsTimeUnitIsPPQ() const { return TimeUnit == 3; }
    int getTimeUnitIndex() const { return TimeUnit; }

    bool IsOverwritePolicy_DeleteAllEventsInPreviousStreamAndUseNewStream() const { return OverwritePolicy == 1; }
    bool IsOverwritePolicy_DeleteAllEventsAfterNow() const { return OverwritePolicy == 2; }
    bool IsOverwritePolicy_KeepAllPreviousEvents() const { return OverwritePolicy == 3; }
    bool shouldForceSendNoteOffs() const { return ForceSendNoteOffsFirst; }

    bool getShouldClearGenerationsAfterPauseStop() const { return ClearGenerationsAfterPauseStop; }

    // ============================================================================================================
    int getPlaybackPolicyType () const { return PlaybackPolicy; }
    int getTimeUnitType () const { return TimeUnit; }
    int getOverwritePolicyType () const { return OverwritePolicy; }

private:
    int PlaybackPolicy{-1};
    int TimeUnit{-1};
    int OverwritePolicy{-1};
    bool ClearGenerationsAfterPauseStop{false};
    double LoopDuration{-1};
    bool ForceSendNoteOffsFirst{false};
};

struct noteOn_ge {
    int channel;
    int noteNumber;
    float velocity;
    double time;

    string getDescription() const {
        std::stringstream ss;
        ss << "Note On: " << "Channel: " << channel << " | Note Number: " << noteNumber << " | Velocity: " << velocity
           << " | Time: " << time;
        return ss.str();
    }
};

struct noteOff_ge {
    int channel;
    int noteNumber;
    float velocity;
    double time;

    string getDescription() const {
        std::stringstream ss;
        ss << "Note Off: " << "Channel: " << channel << " | Note Number: " << noteNumber << " | Velocity: " << velocity
           << " | Time: " << time;
        return ss.str();
    }
};

struct paired_note {
    noteOn_ge noteOn;
    double duration{0};
};

struct controller_ge {
    int channel;
    int controllerNumber;
    int controllerValue;
    double time;

    string getDescription() const {
        std::stringstream ss;
        ss << "Controller: " << "Channel: " << channel << " | Controller Number: " << controllerNumber << " | Controller Value: " << controllerValue
           << " | Time: " << time;
        return ss.str();
    }
};

struct PlaybackSequence {
    PlaybackSequence() = default;

    void clear() {
        messageSequence.clear();
    }

    void clearStartingAt(double time) {
        int ix{0};
        for (auto &event : messageSequence) {
            if (event->message.getTimeStamp() >= time) {
                messageSequence.deleteEvent(ix, false);
            }
            ix++;
        }
    }

    // adds a note on event to the sequence
    void addNoteOn(int channel, int noteNumber, float velocity, double time) {
        messageSequence.addEvent(MidiMessage::noteOn(channel, noteNumber, velocity), time);
    }

    // adds a note off event to the sequence
    void addNoteOff(int channel, int noteNumber, float velocity, double time) {
        messageSequence.addEvent(MidiMessage::noteOff(channel, noteNumber, velocity), time);
    }

    // adds a controller event to the sequence
    void addController(int channel, int controllerNumber, int controllerValue, double time) {
        messageSequence.addEvent(MidiMessage::controllerEvent(channel, controllerNumber, controllerValue), time);
    }

    // automatically creates note-on and note-off events for a note with a given duration
    void addNoteWithDuration(int channel, int noteNumber, float velocity, double time, double duration) {
        messageSequence.addEvent(MidiMessage::noteOn(channel, noteNumber, velocity), time);
        messageSequence.addEvent(MidiMessage::noteOff(channel, noteNumber, velocity), time + duration);
        messageSequence.updateMatchedPairs();
    }

    // get NoteOn Info
    std::vector<noteOn_ge> getNoteOnEvents()  {
        messageSequence.updateMatchedPairs();
        std::vector<noteOn_ge> noteOnEvents;
        for (auto &event : messageSequence) {
            if (event->message.isNoteOn()) {
                noteOn_ge noteOn;
                noteOn.channel = event->message.getChannel();
                noteOn.noteNumber = event->message.getNoteNumber();
                noteOn.velocity = event->message.getVelocity();
                noteOn.time = event->message.getTimeStamp();
                noteOnEvents.push_back(noteOn);
            }
        }
        return noteOnEvents;
    }

    // get NoteOff Info
    std::vector<noteOff_ge> getNoteOffEvents() {
        messageSequence.updateMatchedPairs();
        std::vector<noteOff_ge> noteOffEvents;
        for (auto &event : messageSequence) {
            if (event->message.isNoteOff()) {
                noteOff_ge noteOff;
                noteOff.channel = event->message.getChannel();
                noteOff.noteNumber = event->message.getNoteNumber();
                noteOff.velocity = event->message.getVelocity();
                noteOff.time = event->message.getTimeStamp();
                noteOffEvents.push_back(noteOff);
            }
        }
        return noteOffEvents;
    }

    std::vector<controller_ge> getControllerEvents() {
        messageSequence.updateMatchedPairs();
        std::vector<controller_ge> controllerEvents;
        for (auto &event : messageSequence) {
            if (event->message.isController()) {
                controller_ge controller;
                controller.channel = event->message.getChannel();
                controller.controllerNumber = event->message.getControllerNumber();
                controller.controllerValue = event->message.getControllerValue();
                controller.time = event->message.getTimeStamp();
                controllerEvents.push_back(controller);
            }
        }
        return controllerEvents;
    }

    std::vector<paired_note> getPairedNotes() {
        messageSequence.updateMatchedPairs();
        std::vector<paired_note> pairedNotes;
        int ix = 0;
        for (auto &event : messageSequence) {
            auto message = messageSequence.getEventPointer(ix)->message;
            if (message.isNoteOn()) {
                paired_note note;
                note.noteOn = noteOn_ge();
                note.noteOn.channel = message.getChannel();
                note.noteOn.noteNumber = message.getNoteNumber();
                note.noteOn.velocity = message.getVelocity();
                note.noteOn.time = message.getTimeStamp();
                auto noteOfftime = messageSequence.getTimeOfMatchingKeyUp(ix);
                note.duration =  noteOfftime - note.noteOn.time;
                pairedNotes.push_back(note);
            }
            ix++;
        }
        return pairedNotes;
    }

    juce::MidiMessageSequence getAsJuceMidMessageSequence() const {
        return messageSequence;
    }

private:
    juce::MidiMessageSequence messageSequence{};

};

/*

  Type:
   1. PlaybackPolicies --> notifies the plugin how to deal with the new stream of generations coming in next
   2. PlaybackSequence --> contains the new stream of generations to be played
*/
struct GenerationEvent {


    int type{-1};

    GenerationEvent() = default;

    explicit GenerationEvent(PlaybackPolicies nse) {
        timer.registerStartTime();
        type = 1;
        playbackPolicies = nse;
    }

    explicit GenerationEvent(PlaybackSequence ps) {
        timer.registerStartTime();
        type = 2;
        playbackSequence = ps;
    }

    bool IsNewPlaybackPolicyEvent() const { return type == 1; }
    PlaybackPolicies getNewPlaybackPolicyEvent () const { return playbackPolicies; }

    bool IsNewPlaybackSequence() const { return type == 2; }
    PlaybackSequence getNewPlaybackSequence() const { return playbackSequence; }

    juce::MidiMessageSequence getAsJuceMidMessageSequence() const {
        return playbackSequence.getAsJuceMidMessageSequence();
    }
private:
    PlaybackPolicies playbackPolicies {};
    PlaybackSequence playbackSequence {};

    // uses chrono::system_clock to time events (for debugging only)
    // don't use this for anything else than debugging.
    // used to keep track of when the object was created and when it was accessed
    chrono_timer timer;
};