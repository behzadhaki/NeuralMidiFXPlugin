//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/CustomStructsAndLockFreeQueue.h"
#include "../settings.h"
#include <utility>

inline int qpm_to_microseconds_per_quarter(double qpm)
{
    return  int(60000000.0 / qpm);
}

inline int microseconds_per_quarter_to_qpm(double microseconds_per_quarter)
{
    return int(60000000.0 / microseconds_per_quarter);
}

class EventTracker {
public:
    // Constructor
    EventTracker() {
        TrackedEvents = make_shared<juce::MidiMessageSequence>();
        TrackedTempos = make_shared<juce::MidiMessageSequence>();
        TrackedTimeSignatures = make_shared<juce::MidiMessageSequence>();
    }

    // ======================================================================================
    // ==== Methods for adding (aka registering) events to the tracker
    // --------------------------------------------------------------------------------------
    void add_note_on(int channel, int number, float velocity, double time) {
        auto m = juce::MidiMessage::noteOn(channel, number, velocity);
        TrackedEvents->addEvent(m, time);
    }

    void add_note_off(int channel, int number, float velocity, double time) {
        auto m = juce::MidiMessage::noteOff(channel, number, velocity);
        TrackedEvents->addEvent(m, time);
    }

    void add_cc(int channel, int cc_number, int value, double time) {
        auto m = juce::MidiMessage::controllerEvent(channel, cc_number, value);
        TrackedEvents->addEvent(m, time);
    }

    void tempo_or_time_signature_change(double qpm, int numerator, int denominator, double time) {
        auto tmp = juce::MidiMessage::tempoMetaEvent(qpm_to_microseconds_per_quarter(qpm));
        TrackedTempos->addEvent(tmp, time);
        TrackedTempos->sort();

        auto ts = juce::MidiMessage::timeSignatureMetaEvent(numerator, denominator);
        TrackedTimeSignatures->addEvent(ts, time);
        TrackedTimeSignatures->sort();
    }

    // ======================================================================================

    // ======================================================================================
    // ==== Getter Utilities
    // --------------------------------------------------------------------------------------
    /*
     * Returns the tracked events (modifying these won't modify the contents of the tracker)
     * If you want to modify the contents of the tracker, use the add_* methods or
     * get a pointer to the sequence using get_tracked_events_mutable() method
     */
    juce::MidiMessageSequence get_tracked_events_non_mutable() {
        TrackedEvents->sort();
        TrackedEvents->updateMatchedPairs();
        return *TrackedEvents;
    }

    /*
     * Returns a pointer to the tracked tempos (modifying these will modify the contents of the tracker)
     * If you want to modify the contents of the tracker, use the add_* methods or
     * get a pointer to the sequence using get_tracked_tempos_mutable() method
     */
    juce::MidiMessageSequence get_tracked_tempos_non_mutable() {
        TrackedTempos->sort();
        TrackedTempos->updateMatchedPairs();
        return *TrackedTempos;
    }

    /*
     * Returns a pointer to the tracked time signatures (modifying these will modify the contents of the tracker)
     * If you want to modify the contents of the tracker, use the add_* methods or
     * get a pointer to the sequence using get_tracked_time_signatures_mutable() method
     */
    juce::MidiMessageSequence get_tracked_time_signatures_non_mutable() {
        TrackedTimeSignatures->sort();
        TrackedTimeSignatures->updateMatchedPairs();
        return *TrackedTimeSignatures;
    }

    /* Returns a pointer to the tracked events (modifying these will modify the contents of the tracker) */
    juce::MidiMessageSequence* get_tracked_events_mutable() {
        TrackedEvents->sort();
        TrackedEvents->updateMatchedPairs();
        return TrackedEvents.get();
    }

    /* Returns a pointer to the tracked tempos (modifying these will modify the contents of the tracker) */
    juce::MidiMessageSequence* get_tracked_tempos_mutable() {
        TrackedTempos->sort();
        TrackedTempos->updateMatchedPairs();
        return TrackedTempos.get();
    }

    /* Returns a pointer to the tracked time signatures (modifying these will modify the contents of the tracker) */
    juce::MidiMessageSequence* get_tracked_time_signatures_mutable() {
        TrackedTimeSignatures->sort();
        TrackedTimeSignatures->updateMatchedPairs();
        return TrackedTimeSignatures.get();
    }

    /* Returns the tempo, time signature corresponding to the given time
     * Ouput is a tuple of (time_stamp of closest tempo/ts, tempo, numerator, denominator)
     */
    tuple<double, double, int, int> get_tempo_and_time_signature_at_time(double time) {
        // get the correct index for the tempo and time signature by checking the closest timestamp before time
        for (int i = 0; i < TrackedTempos->getNumEvents(); i++) {
            if (TrackedTempos->getEventPointer(i)->message.getTimeStamp() > time) {
                // we have found the correct index
                auto tempo = microseconds_per_quarter_to_qpm(TrackedTempos->getEventPointer(i - 1)->message.getTimeStamp());
                int numerator, denominator;
                TrackedTimeSignatures->getEventPointer(i - 1)->message.getTimeSignatureInfo(numerator, denominator);
                return make_tuple(tempo, time, numerator, denominator);
            }
        }
    }

    // ======================================================================================
    // ==== Methods for clearing the tracker
    // --------------------------------------------------------------------------------------
    void clear() {
        TrackedEvents->clear();
        TrackedTempos->clear();
        TrackedTimeSignatures->clear();
    }

    void clear_starting_at(double time) {
        for (int i = 0; i < TrackedEvents->getNumEvents(); i++) {
            if (TrackedEvents->getEventPointer(i)->message.getTimeStamp() >= time) {
                TrackedEvents->deleteEvent(i, true);
            }
        }
        for (int i = 0; i < TrackedTempos->getNumEvents(); i++) {
            if (TrackedTempos->getEventPointer(i)->message.getTimeStamp() >= time) {
                TrackedTempos->deleteEvent(i, true);
                TrackedTimeSignatures->deleteEvent(i, true);
            }
        }
    }

    // ======================================================================================
    // ==== Midi Utilities
    // --------------------------------------------------------------------------------------

    juce::MidiFile get_trackedEvents_as_midiFile(int ticksPerQuarterNote=480) {

        juce::MidiFile midiFile;
        midiFile.setTicksPerQuarterNote (ticksPerQuarterNote);
        auto sequence = *TrackedEvents;
        sequence.addSequence(*TrackedTempos, 0, 0, 1e10);
        sequence.addSequence(*TrackedTimeSignatures, 0, 0, 1e10);
        midiFile.addTrack(sequence);

        return midiFile;
    }

    void save_trackedEvents_as_midiFile(const string& path, int ticksPerQuarterNote=480){
        juce::MidiFile midiFile = get_trackedEvents_as_midiFile(ticksPerQuarterNote);

        // make sure path ends with .mid or .midi
        string path_ = path;
        if (path_.find(".mid") == string::npos && path_.find(".midi") == string::npos){
            path_ += ".mid";
        }

        // create directories if they don't exist
        juce::File file(path_);
        file.createDirectory();

        // open output stream
        juce::FileOutputStream outputStream(file);

        // save midi file
        midiFile.writeTo(outputStream);
    }
    // ======================================================================================

private:

    shared_ptr<juce::MidiMessageSequence> TrackedEvents;
    shared_ptr<juce::MidiMessageSequence> TrackedTempos;
    shared_ptr<juce::MidiMessageSequence> TrackedTimeSignatures;

};