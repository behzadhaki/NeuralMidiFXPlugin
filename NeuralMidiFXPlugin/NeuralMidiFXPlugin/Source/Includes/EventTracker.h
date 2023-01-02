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
    void add_note_on(int channel, int number, int velocity, double time) {
        auto m = juce::MidiMessage::noteOn(channel, number, velocity / 127.0f);
        TrackedEvents->addEvent(m, time);
        TrackedEvents->sort();
    }

    void add_note_off(int channel, int number, int velocity, double time) {
        auto m = juce::MidiMessage::noteOff(channel, number, velocity / 127.0f);
        TrackedEvents->addEvent(m, time);
        TrackedEvents->sort();
    }

    void add_cc(int channel, int cc_number, int value, double time) {
        auto m = juce::MidiMessage::controllerEvent(channel, cc_number, value);
        TrackedEvents->addEvent(m, time);
        TrackedEvents->sort();
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
        return *TrackedEvents;
    }

    /*
     * Returns a pointer to the tracked tempos (modifying these will modify the contents of the tracker)
     * If you want to modify the contents of the tracker, use the add_* methods or
     * get a pointer to the sequence using get_tracked_tempos_mutable() method
     */
    juce::MidiMessageSequence get_tracked_tempos_non_mutable() {
        return *TrackedTempos;
    }

    /*
     * Returns a pointer to the tracked time signatures (modifying these will modify the contents of the tracker)
     * If you want to modify the contents of the tracker, use the add_* methods or
     * get a pointer to the sequence using get_tracked_time_signatures_mutable() method
     */
    juce::MidiMessageSequence get_tracked_time_signatures_non_mutable() {
        return *TrackedTimeSignatures;
    }

    /* Returns a pointer to the tracked events (modifying these will modify the contents of the tracker) */
    juce::MidiMessageSequence* get_tracked_events_mutable() {
        return TrackedEvents.get();
    }

    /* Returns a pointer to the tracked tempos (modifying these will modify the contents of the tracker) */
    juce::MidiMessageSequence* get_tracked_tempos_mutable() {
        return TrackedTempos.get();
    }

    /* Returns a pointer to the tracked time signatures (modifying these will modify the contents of the tracker) */
    juce::MidiMessageSequence* get_tracked_time_signatures_mutable() {
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
                return make_tuple(time, tempo, numerator, denominator);
            }
        }
    }

    int get_num_events() {
        return TrackedEvents->getNumEvents();
    }

    // ======================================================================================
    // ==== Methods for clearing the tracker
    // --------------------------------------------------------------------------------------
    void clear() {
        TrackedEvents->clear();
        TrackedTempos->clear();
        TrackedTimeSignatures->clear();
    }

    void clear_starting_at(double time, bool delete_matching_note_up) {
        for (int i = 0; i < TrackedEvents->getNumEvents(); i++) {
            if (TrackedEvents->getEventPointer(i)->message.getTimeStamp() >= time) {
                TrackedEvents->deleteEvent(i, delete_matching_note_up);
            }
        }
        for (int i = 0; i < TrackedTempos->getNumEvents(); i++) {
            if (TrackedTempos->getEventPointer(i)->message.getTimeStamp() >= time) {
                TrackedTempos->deleteEvent(i, delete_matching_note_up);
                TrackedTimeSignatures->deleteEvent(i, delete_matching_note_up);
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

    void print_messages_in_buffer(){
        for (int i = 0; i < TrackedEvents->getNumEvents(); i++) {
            auto msg = TrackedEvents->getEventPointer(i)->message;
            DBG(msg.getDescription() << "time: " << msg.getTimeStamp());
        }
    }

private:

    shared_ptr<juce::MidiMessageSequence> TrackedEvents;
    shared_ptr<juce::MidiMessageSequence> TrackedTempos;
    shared_ptr<juce::MidiMessageSequence> TrackedTimeSignatures;

};


struct ITP_MultiTime_EventTracker {
    EventTracker buffer_absolutetime_in_ppq {};
    EventTracker buffer_relativetime_in_ppq {};
    EventTracker buffer_absolutetime_in_seconds {};
    EventTracker buffer_relativetime_in_seconds {};

    explicit ITP_MultiTime_EventTracker(bool should_remove_after_access):_should_remove_after_access(
            should_remove_after_access) {}

    /***
     * Return events available with a specified time setting.
     * The returned events are then removed from the EventTrackers.
     * Use this method to access and process newly available info
     * @param use_abolute_time
     * @param durations_in_ppq
     * @param ignore_zero_duration_notes
     * @return
     */

    // make sure all notes off point to the correct note on
    void updateMatchedPairs() {
        buffer_absolutetime_in_ppq.get_tracked_events_mutable()->updateMatchedPairs();
        buffer_relativetime_in_ppq.get_tracked_events_mutable()->updateMatchedPairs();
        buffer_absolutetime_in_seconds.get_tracked_events_mutable()->updateMatchedPairs();
        buffer_relativetime_in_seconds.get_tracked_events_mutable()->updateMatchedPairs();
    }

    MultiTimedStructure<vector<pair<juce::MidiMessage, double>>> get_notes_with_duration() {

        // make sure all notes off point to the correct note on
        updateMatchedPairs();

        // print_messages_in_buffer();

        // output container
        MultiTimedStructure<vector<pair<juce::MidiMessage, double>>> events_with_duration;

        // track events already prepared to return
        vector<int> indices_to_remove;

        MultiTimedStructure<juce::MidiMessageSequence*> tracked_events_ptrs {};
        tracked_events_ptrs.absolute_time_in_ppq = buffer_absolutetime_in_ppq.get_tracked_events_mutable();
        tracked_events_ptrs.relative_time_in_ppq = buffer_relativetime_in_ppq.get_tracked_events_mutable();
        tracked_events_ptrs.absolute_time_in_seconds = buffer_absolutetime_in_seconds.get_tracked_events_mutable();
        tracked_events_ptrs.relative_time_in_seconds = buffer_relativetime_in_seconds.get_tracked_events_mutable();

        for (int i = 0; i < tracked_events_ptrs.absolute_time_in_ppq->getNumEvents(); i++) {
            // get ith event in all time settings
            MultiTimedStructure<juce::MidiMessage> event {};
            event.absolute_time_in_ppq = tracked_events_ptrs.absolute_time_in_ppq->getEventPointer(i)->message;
            event.relative_time_in_ppq = tracked_events_ptrs.relative_time_in_ppq->getEventPointer(i)->message;
            event.absolute_time_in_seconds = tracked_events_ptrs.absolute_time_in_seconds->getEventPointer(i)->message;
            event.relative_time_in_seconds = tracked_events_ptrs.relative_time_in_seconds->getEventPointer(i)->message;

            // prepare durations
            MultiTimedStructure<double> duration {};
            duration.absolute_time_in_ppq = 0.0f;
            duration.relative_time_in_ppq = 0.0f;
            duration.absolute_time_in_seconds = 0.0f;
            duration.relative_time_in_seconds = 0.0f;

            // check if i is not in indices_to_delete (i.e. a note off event with a MATCHING note on event)
            if (std::find(indices_to_remove.begin(), indices_to_remove.end(), i) ==
                indices_to_remove.end()) {
                if (event.absolute_time_in_ppq.isNoteOn()) {

                    auto matching_event_index = tracked_events_ptrs.absolute_time_in_ppq->getIndexOfMatchingKeyUp(i);
                    // add note on to vector
                    if (matching_event_index != -1) {
                        MultiTimedStructure<double> corresponding_note_off {};

                        corresponding_note_off.absolute_time_in_ppq =
                                tracked_events_ptrs.absolute_time_in_ppq->getEventPointer(
                                        matching_event_index)->message.getTimeStamp();
                        corresponding_note_off.relative_time_in_ppq =
                                tracked_events_ptrs.relative_time_in_ppq->getEventPointer(
                                        matching_event_index)->message.getTimeStamp();
                        corresponding_note_off.absolute_time_in_seconds =
                                tracked_events_ptrs.absolute_time_in_seconds->getEventPointer(
                                        matching_event_index)->message.getTimeStamp();
                        corresponding_note_off.relative_time_in_seconds =
                                tracked_events_ptrs.relative_time_in_seconds->getEventPointer(
                                        matching_event_index)->message.getTimeStamp();

                        duration.absolute_time_in_ppq = corresponding_note_off.absolute_time_in_ppq-
                                                        event.absolute_time_in_ppq.getTimeStamp();
                        duration.relative_time_in_ppq = corresponding_note_off.relative_time_in_ppq-
                                                        event.relative_time_in_ppq.getTimeStamp();
                        duration.absolute_time_in_seconds = corresponding_note_off.absolute_time_in_seconds-
                                                            event.absolute_time_in_seconds.getTimeStamp();
                        duration.relative_time_in_seconds = corresponding_note_off.relative_time_in_seconds-
                                                            event.relative_time_in_seconds.getTimeStamp();
                    }

                    if (duration.absolute_time_in_ppq > 0.0f) {
                        events_with_duration.absolute_time_in_ppq.emplace_back(
                                make_pair(event.absolute_time_in_ppq, duration.absolute_time_in_ppq));
                        events_with_duration.relative_time_in_ppq.emplace_back(
                                make_pair(event.relative_time_in_ppq, duration.relative_time_in_ppq));
                        events_with_duration.absolute_time_in_seconds.emplace_back(
                                make_pair(event.absolute_time_in_seconds, duration.absolute_time_in_seconds));
                        events_with_duration.relative_time_in_seconds.emplace_back(
                                make_pair(event.relative_time_in_seconds, duration.relative_time_in_seconds));

                        // only note ons with corresponding note offs will be deleted if needed
                        if (duration.absolute_time_in_ppq > 0.0f) {
                            indices_to_remove.push_back(i);
                        }

                        // add note off to indices_to_delete if it exists and deletion needed
                        if (matching_event_index != -1) {
                            indices_to_remove.push_back(matching_event_index);
                        }

                    }
                }
            }
        }

        if (_should_remove_after_access) {
            // delete events from buffer
            for (auto index: indices_to_remove) {
                buffer_absolutetime_in_ppq.get_tracked_events_mutable()->deleteEvent(index, false);
                buffer_relativetime_in_ppq.get_tracked_events_mutable()->deleteEvent(index, false);
                buffer_absolutetime_in_seconds.get_tracked_events_mutable()->deleteEvent(index, false);
                buffer_relativetime_in_seconds.get_tracked_events_mutable()->deleteEvent(index, false);
            }
        }

        return events_with_duration;
    }

    MultiTimedStructure<vector<pair<juce::MidiMessage, double>>> get_note_midi_messages() {
// make sure all notes off point to the correct note on
        updateMatchedPairs();

        // print_messages_in_buffer();

        // output container
        MultiTimedStructure<vector<pair<juce::MidiMessage, double>>> events_available;

        // track events already prepared to return
        vector<int> indices_to_remove;

        MultiTimedStructure<juce::MidiMessageSequence*> tracked_events_ptrs {};
        tracked_events_ptrs.absolute_time_in_ppq = buffer_absolutetime_in_ppq.get_tracked_events_mutable();
        tracked_events_ptrs.relative_time_in_ppq = buffer_relativetime_in_ppq.get_tracked_events_mutable();
        tracked_events_ptrs.absolute_time_in_seconds = buffer_absolutetime_in_seconds.get_tracked_events_mutable();
        tracked_events_ptrs.relative_time_in_seconds = buffer_relativetime_in_seconds.get_tracked_events_mutable();

        for (int i = 0; i < tracked_events_ptrs.absolute_time_in_ppq->getNumEvents(); i++) {
            // get ith event in all time settings
            MultiTimedStructure<juce::MidiMessage> event {};
            event.absolute_time_in_ppq = tracked_events_ptrs.absolute_time_in_ppq->getEventPointer(i)->message;
            event.relative_time_in_ppq = tracked_events_ptrs.relative_time_in_ppq->getEventPointer(i)->message;
            event.absolute_time_in_seconds = tracked_events_ptrs.absolute_time_in_seconds->getEventPointer(i)->message;
            event.relative_time_in_seconds = tracked_events_ptrs.relative_time_in_seconds->getEventPointer(i)->message;

            // check if i is not in indices_to_delete (i.e. a note off event with a MATCHING note on event)
            if (std::find(indices_to_remove.begin(), indices_to_remove.end(), i) ==
                indices_to_remove.end()) {
                if (event.absolute_time_in_ppq.isNoteOn() or event.absolute_time_in_ppq.isNoteOff()) {
                    events_available.absolute_time_in_ppq.emplace_back(
                            make_pair(event.absolute_time_in_ppq, 0.0f));
                    events_available.relative_time_in_ppq.emplace_back(
                            make_pair(event.relative_time_in_ppq, 0.0f));
                    events_available.absolute_time_in_seconds.emplace_back(
                            make_pair(event.absolute_time_in_seconds, 0.0f));
                    events_available.relative_time_in_seconds.emplace_back(
                            make_pair(event.relative_time_in_seconds, 0.0f));
                    indices_to_remove.push_back(i);
                }
            }
        }

        if (_should_remove_after_access) {
            // delete events from buffer
            for (auto index: indices_to_remove) {
                buffer_absolutetime_in_ppq.get_tracked_events_mutable()->deleteEvent(index, false);
                buffer_relativetime_in_ppq.get_tracked_events_mutable()->deleteEvent(index, false);
                buffer_absolutetime_in_seconds.get_tracked_events_mutable()->deleteEvent(index, false);
                buffer_relativetime_in_seconds.get_tracked_events_mutable()->deleteEvent(index, false);
            }
        }

        return events_available;
    }

    vector<juce::MidiMessage> get_CCs(bool use_abolute_time, bool durations_in_ppq){
        updateMatchedPairs();
        // print_messages_in_buffer();

        vector<juce::MidiMessage> events_with_duration;
        vector<int> indices_already_accessed;
        juce::MidiMessageSequence *tracked_events_ptr;

        // make sure all notes off point to the correct note on

        if (use_abolute_time) {
            if (durations_in_ppq) {
                tracked_events_ptr = buffer_absolutetime_in_ppq.get_tracked_events_mutable();
            } else {
                tracked_events_ptr = buffer_absolutetime_in_seconds.get_tracked_events_mutable();
            }
        } else {
            if (durations_in_ppq) {
                tracked_events_ptr = buffer_relativetime_in_ppq.get_tracked_events_mutable();
            } else {
                tracked_events_ptr = buffer_relativetime_in_seconds.get_tracked_events_mutable();
            }
        }

        for (int i = 0; i < tracked_events_ptr->getNumEvents(); i++) {
            auto event = tracked_events_ptr->getEventPointer(i)->message;
            if (event.isController()){
                events_with_duration.push_back(event);
            }
        }

        if (_should_remove_after_access) {
            // delete events from buffer
            for (auto index: indices_already_accessed) {
                buffer_absolutetime_in_ppq.get_tracked_events_mutable()->deleteEvent(index, false);
                buffer_relativetime_in_ppq.get_tracked_events_mutable()->deleteEvent(index, false);
                buffer_absolutetime_in_seconds.get_tracked_events_mutable()->deleteEvent(index, false);
                buffer_relativetime_in_seconds.get_tracked_events_mutable()->deleteEvent(index, false);
            }
        }

        return events_with_duration;
    }

    void print_messages_in_buffer(){
        DBG("++++++ buffer_absolutetime_in_ppq ++++++");
        buffer_absolutetime_in_ppq.print_messages_in_buffer();
        DBG("----------------------------------------");
        DBG("++++++ buffer_relativetime_in_ppq ++++++");
        buffer_relativetime_in_ppq.print_messages_in_buffer();
        DBG("----------------------------------------");
        DBG("++++++ buffer_absolutetime_in_seconds ++++++");
        buffer_absolutetime_in_seconds.print_messages_in_buffer();
        DBG("----------------------------------------");
        DBG("++++++ buffer_relativetime_in_seconds ++++++");
        buffer_relativetime_in_seconds.print_messages_in_buffer();
        DBG("----------------------------------------");
    }

    vector<tuple<double, double, int, int>> get_tempo_time_signatures(bool use_abolute_time, bool time_unit_in_ppq) {
        vector<tuple<double, double, int, int>> tempo_time_signatures;
        juce::MidiMessageSequence *tracked_tempos_ptr;
        juce::MidiMessageSequence *tracked_time_signatures_ptr;

        if (use_abolute_time) {
            if (time_unit_in_ppq) {
                tracked_tempos_ptr = buffer_absolutetime_in_ppq.get_tracked_tempos_mutable();
                tracked_time_signatures_ptr = buffer_absolutetime_in_ppq.get_tracked_time_signatures_mutable();
            } else {
                tracked_tempos_ptr = buffer_absolutetime_in_seconds.get_tracked_tempos_mutable();
                tracked_time_signatures_ptr = buffer_absolutetime_in_seconds.get_tracked_time_signatures_mutable();
            }
        } else {
            if (time_unit_in_ppq) {
                tracked_tempos_ptr = buffer_relativetime_in_ppq.get_tracked_tempos_mutable();
                tracked_time_signatures_ptr = buffer_relativetime_in_ppq.get_tracked_time_signatures_mutable();
            } else {
                tracked_tempos_ptr = buffer_relativetime_in_seconds.get_tracked_tempos_mutable();
                tracked_time_signatures_ptr = buffer_relativetime_in_seconds.get_tracked_time_signatures_mutable();
            }
        }

        for (int i = 0; i < tracked_tempos_ptr->getNumEvents(); i++) {
            auto tempo = tracked_tempos_ptr->getEventPointer(i)->message;
            auto qpm = 60.0f / tempo.getTempoSecondsPerQuarterNote();
            auto time_signature = tracked_time_signatures_ptr->getEventPointer(i)->message;
            int numerator, denominator;
            time_signature.getTimeSignatureInfo(numerator, denominator);

            tempo_time_signatures.emplace_back(tempo.getTimeStamp(),
                                               qpm,
                                               numerator,
                                               numerator);
        }

        if (_should_remove_after_access) {
            // delete events from buffer
            tracked_tempos_ptr->clear();
            tracked_time_signatures_ptr->clear();
        }
        return tempo_time_signatures;
    }

    // returns total number of NoteOn, NoteOff, CC messages
    // (ignores tempo and time signature in the count)
    int getNumEvents() {
        return buffer_absolutetime_in_ppq.get_num_events();
    }

    void clear() {
        buffer_absolutetime_in_ppq.clear();
        buffer_relativetime_in_ppq.clear();
        buffer_absolutetime_in_seconds.clear();
        buffer_relativetime_in_seconds.clear();
    }

    void clear_starting_at(double absolute_time_ppq, double absolute_time_sec,
                           double relative_time_ppq, double relative_time_sec,
                           bool delete_matching_note_up=true){
        buffer_absolutetime_in_ppq.clear_starting_at(absolute_time_ppq, delete_matching_note_up);
        buffer_relativetime_in_ppq.clear_starting_at(relative_time_ppq, delete_matching_note_up);
        buffer_absolutetime_in_seconds.clear_starting_at(absolute_time_sec, delete_matching_note_up);
        buffer_relativetime_in_seconds.clear_starting_at(relative_time_sec, delete_matching_note_up);
    }

    void addNoteOn(int channel, int number, int velocity,
                   double absolute_time_ppq, double absolute_time_sec,
                   double relative_time_ppq, double relative_time_sec){
        buffer_absolutetime_in_ppq.add_note_on(channel, number, velocity, absolute_time_ppq);
        buffer_relativetime_in_ppq.add_note_on(channel, number, velocity, relative_time_ppq);
        buffer_absolutetime_in_seconds.add_note_on(channel, number, velocity, absolute_time_sec);
        buffer_relativetime_in_seconds.add_note_on(channel, number, velocity, relative_time_sec);
    }

    void addNoteOff(int channel, int number, int velocity,
                    double absolute_time_ppq, double absolute_time_sec,
                    double relative_time_ppq, double relative_time_sec){
        buffer_absolutetime_in_ppq.add_note_off(channel, number, velocity, absolute_time_ppq);
        buffer_relativetime_in_ppq.add_note_off(channel, number, velocity, relative_time_ppq);
        buffer_absolutetime_in_seconds.add_note_off(channel, number, velocity, absolute_time_sec);
        buffer_relativetime_in_seconds.add_note_off(channel, number, velocity, relative_time_sec);
    }

    void addCC(int channel, int cc_number, int value,
               double absolute_time_ppq, double absolute_time_sec,
               double relative_time_ppq, double relative_time_sec){
        buffer_absolutetime_in_ppq.add_cc(channel, cc_number, value, absolute_time_ppq);
        buffer_relativetime_in_ppq.add_cc(channel, cc_number, value, relative_time_ppq);
        buffer_absolutetime_in_seconds.add_cc(channel, cc_number, value, absolute_time_sec);
        buffer_relativetime_in_seconds.add_cc(channel, cc_number, value, relative_time_sec);
    }

    void addTempoAndTimeSignature(double qpm, int numerator, int denominator,
                                  double absolute_time_ppq, double absolute_time_sec,
                                  double relative_time_ppq, double relative_time_sec){
        buffer_absolutetime_in_ppq.tempo_or_time_signature_change(qpm, numerator, denominator, absolute_time_ppq);
        buffer_relativetime_in_ppq.tempo_or_time_signature_change(qpm, numerator, denominator, relative_time_ppq);
        buffer_absolutetime_in_seconds.tempo_or_time_signature_change(qpm, numerator, denominator, absolute_time_sec);
        buffer_relativetime_in_seconds.tempo_or_time_signature_change(qpm, numerator, denominator, relative_time_sec);
    }


private:
    bool _should_remove_after_access;

};