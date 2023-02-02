//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>

using namespace juce;

/*

  This class is used to Notify the Plugin that a new stream of generations is available.
  Moreover, it also contains information about the policy used for playback of generations
 
  PlaybackPolicy:
       1. relative to Now (register the current time, and play messages relative to that)
       2. relative to 0 (stream should be played relative to absolute 0 time)
       3. relative to playback start (stream should be played relative to the time when playback started)
 
  Time_unit:
    1. seconds
    2. ppq
    3. audio samples
 
  Overwrite Policy:
   1. delete all events in previous stream and use new stream
   2. delete all events after now
   3. Keep all previous events (that is generations can be played on top of each other)
*/


struct GenerationPlaybackPolicies {

    GenerationPlaybackPolicies() = default;

    void SetPaybackPolicy_RelativeToNow() { PlaybackPolicy = 1; }

    void SetPlaybackPolicy_RelativeToAbsoluteZero() { PlaybackPolicy = 2; }

    void SetPplaybackPolicy_RelativeToPlaybackStart() { PlaybackPolicy = 3; }

    void SetTimeUnitIsSeconds() { TimeUnit = 1; }

    void SetTimeUnitIsPPQ() { TimeUnit = 2; }

    void SetTimeUnitIsAudioSamples() { TimeUnit = 1; }

    void SetOverwritePolicy_DeleteAllEventsInPreviousStreamAndUseNewStream() { OverwritePolicy = 1; }
    void SetOverwritePolicy_DeleteAllEventsAfterNow() { OverwritePolicy = 2; }
    void SetOverwritePolicy_KeepAllPreviousEvents() { OverwritePolicy = 3; }

    // Checks if data is ready for transmission
    bool IsReadyForTransmission() const { return PlaybackPolicy != -1 && TimeUnit != -1 && OverwritePolicy != -1; }

    // ============================================================================================================

    bool IsPlaybackPolicy_RelativeToNow() const { return PlaybackPolicy == 1; }
    bool IsPlaybackPolicy_RelativeToAbsoluteZero() const { return PlaybackPolicy == 2; }
    bool IsPlaybackPolicy_RelativeToPlaybackStart() const { return PlaybackPolicy == 3; }

    bool IsTimeUnitIsSeconds() const { return TimeUnit == 1; }
    bool IsTimeUnitIsPPQ() const { return TimeUnit == 2; }
    bool IsTimeUnitIsAudioSamples() const { return TimeUnit == 3; }

    bool IsOverwritePolicy_DeleteAllEventsInPreviousStreamAndUseNewStream() const { return OverwritePolicy == 1; }
    bool IsOverwritePolicy_DeleteAllEventsAfterNow() const { return OverwritePolicy == 2; }
    bool IsOverwritePolicy_KeepAllPreviousEvents() const { return OverwritePolicy == 3; }

    // ============================================================================================================
    int getPlaybackPolicyType () const { return PlaybackPolicy; }
    int getTimeUnitType () const { return TimeUnit; }
    int getOverwritePolicyType () const { return OverwritePolicy; }

private:
    int PlaybackPolicy{-1};
    int TimeUnit{-1};
    int OverwritePolicy{-1};
};


/*

  Type:
   1. GenerationPlaybackPolicies --> notifies the plugin how to deal with the new stream of generations coming in next
   2. MidiMessageSequence --> contains the new stream of generations to be played
*/
struct GenerationEvent {
    GenerationPlaybackPolicies playbackPolicies {};
    juce::MidiMessageSequence messageSequence {};

    int type{-1};

    GenerationEvent() = default;

    explicit GenerationEvent(GenerationPlaybackPolicies nse) {
        type = 1;
        playbackPolicies = nse;
    }

    explicit GenerationEvent(juce::MidiMessageSequence ms) {
        type = 2;
        messageSequence = ms;
    }

    bool IsNewPlaybackPolicyEvent() const { return type == 1; }
    GenerationPlaybackPolicies getNewPlaybackPolicyEvent () const { return playbackPolicies; }

    bool IsNewMidiMessageSequence() const { return type == 2; }
    juce::MidiMessageSequence getNewMidiMessageSequence() const { return messageSequence; }
};