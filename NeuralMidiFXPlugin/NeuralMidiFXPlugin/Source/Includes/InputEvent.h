//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/GuiParameters.h"
#include "../Includes/chrono_timer.h"
#include "../DeploymentSettings/ThreadsAndQueuesAndInputEvents.h"
#include <utility>


using namespace juce;


struct time_ {
    time_(): seconds(0.0), samples(0), ppq(0.0) {}

    explicit time_(int64_t samples_, double seconds_, double ppq_) :
    seconds(seconds_), samples(samples_), ppq(ppq_) {}

    double inSeconds() const { return seconds; }
    int64_t inSamples() const { return samples; }
    double inQuarterNotes() const { return ppq; }

    // unitType == 1 --> samples
    // unitType == 2 --> seconds
    // unitType == 3 --> quarter notes
    double getTimeWithUnitType(int unitType) const {
        switch (unitType) {
            case 1:
                return inSamples();
            case 2:
                return inSeconds();
            case 3:
                return inQuarterNotes();
            default:
                assert("Invalid unit type");
        }
    }
    time_ operator-(const time_ &e) const {
        return time_(this->inSamples() - e.inSamples(),
                     this->inSeconds() - e.inSeconds(),
                     this->inQuarterNotes() - e.inQuarterNotes());
    }

    bool operator==(const time_ &e) const {
        return (this->inSeconds() == e.inSeconds()) &&
               (this->inSamples() == e.inSamples()) &&
               (this->inQuarterNotes() == e.inQuarterNotes());
    }

    bool operator!=(const time_ &e) const {
        return !(*this == e);
    }

    bool operator<(const time_ &e) const {
        return (this->inSeconds() < e.inSeconds()) &&
               (this->inSamples() < e.inSamples()) &&
               (this->inQuarterNotes() < e.inQuarterNotes());
    }

    bool operator>(const time_ &e) const {
        return (this->inSeconds() > e.inSeconds()) &&
               (this->inSamples() > e.inSamples()) &&
               (this->inQuarterNotes() > e.inQuarterNotes());
    }

    bool operator<=(const time_ &e) const {
        return (this->inSeconds() <= e.inSeconds()) &&
               (this->inSamples() <= e.inSamples()) &&
               (this->inQuarterNotes() <= e.inQuarterNotes());
    }

    bool operator>=(const time_ &e) const {
        return (this->inSeconds() >= e.inSeconds()) &&
               (this->inSamples() >= e.inSamples()) &&
               (this->inQuarterNotes() >= e.inQuarterNotes());
    }

private:
    double seconds{};
    int64_t samples{};
    double ppq{};
};


/*
 * This structure holds the metadata for a given buffer received from the host.
 * All necessary information is embedded in this structure:
 *
 *  - qpm: tempo in quarter notes per minute
 *  - numerator: numerator of the time signature
 *  - denominator: denominator of the time signature
 *
 *  - isPlaying: is the host playing or not
 *  - isRecording: is the host recording or not
 *
 *  - time_in_samples: starting point of the buffer in audio samples
 *  - time_in_seconds: starting point of the buffer in seconds
 *  - time_in_ppq: starting point of the buffer in quarter notes
 *
 *  - playhead_force_moved_forward: has the playhead has been moved forcibly ahead of the next expected buffer
 *  - playhead_force_moved_backward: has the playhead has been moved forcibly behind the next expected buffer
 *
 *  - isLooping: is the host looping or not
 *  - loop_start_in_ppq: starting point of the loop in quarter notes
 *  - loop_end_in_ppq: ending point of the loop in quarter notes
 *
 *  - bar_count: number of bars in the buffer
 *  - ppq_position_of_last_bar_start: quarter note position of the last bar corresponding to the existing position
 *
 *  - sample_rate: sample rate of the host
 *  - buffer_size: buffer size of the host in number of samples
 *
 *
 *
 * You can have access to these information at different frequencies,
 *      depending on the usecase intended. The frequency of accessing
 *      this information is determined by the settings in settings.h.
 *      Examples:
 *      1. If you need these information at every buffer, then set
 *
 *      >> constexpr bool SendEventAtBeginningOfNewBuffers_FLAG{true};
 *      >> constexpr bool SendEventForNewBufferIfMetadataChanged_FLAG{false};
 *
 *      2. Alternatively, you may want to access these information,
 *      only when the metadata changes. In this case:
 *
 *      >> constexpr bool SendEventAtBeginningOfNewBuffers_FLAG{true};
 *      >> constexpr bool SendEventForNewBufferIfMetadataChanged_FLAG{false};
 *
 *      3. If you need these information at every bar, then:
 *
 *      >> constexpr bool SendNewBarEvents_FLAG{true};
 *
 *      4. If you need these information at every specific time shift periods,
 *      you can specify the time shift as a ratio of a quarter note. For instance,
 *      if you want to access these information at every 8th note, then:
 *
 *      >>  constexpr bool SendTimeShiftEvents_FLAG{false};
 *      >>  constexpr double delta_TimeShiftEventRatioOfQuarterNote{0.5};
 *
 *      5. If you only need this information, whenever a new midi message
 *      (note on/off or cc) is received, you don't need to set any of the above,
 *      since the metadata is embedded in any midi Event. Just remember to not
 *      filter out the midi events.
 *
 *      >>  constexpr bool FilterNoteOnEvents_FLAG{false};
 *      && /or
 *      >>  constexpr bool FilterNoteOffEvents_FLAG{false};
 *      && /or
 *      >>  constexpr bool FilterCCEvents_FLAG{false};
 *
 */
struct BufferMetaData {
    double qpm{-1};
    double numerator{-1};
    double denominator{-1};

    bool isPlaying{false};
    bool isRecording{false};

    int64_t time_in_samples{-1};
    double time_in_seconds{-1};
    double time_in_ppq{-1};

    bool playhead_force_moved_forward{false};
    bool playhead_force_moved_backward{false};

    bool isLooping{false};
    double loop_start_in_ppq{-1};
    double loop_end_in_ppq{-1};

    int64_t bar_count{-1};
    double ppq_position_of_last_bar_start{-1};

    double sample_rate{-1};
    int64_t buffer_size_in_samples{-1};

    BufferMetaData() = default;

    BufferMetaData(juce::Optional<juce::AudioPlayHead::PositionInfo> Pinfo,
                   double sample_rate_, int64_t buffer_size_in_samples_) {
        if (Pinfo) {
            qpm = Pinfo->getBpm() ? *Pinfo->getBpm() : -1;
            auto ts = Pinfo->getTimeSignature();
            numerator = ts ? ts->numerator : -1;
            denominator = ts ? ts->denominator : -1;
            isPlaying = Pinfo->getIsPlaying();
            isRecording = Pinfo->getIsRecording();
            time_in_samples = Pinfo->getTimeInSamples() ? *Pinfo->getTimeInSamples() : -1;
            time_in_seconds = Pinfo->getTimeInSeconds() ? *Pinfo->getTimeInSeconds() : -1;
            time_in_ppq = Pinfo->getPpqPosition() ? *Pinfo->getPpqPosition() : -1;
            isLooping = Pinfo->getIsLooping();
            auto loopPoints = Pinfo->getLoopPoints();
            loop_start_in_ppq = loopPoints ? loopPoints->ppqStart : -1;
            loop_end_in_ppq = loopPoints ? loopPoints->ppqEnd : -1;
            bar_count = Pinfo->getBarCount() ? *Pinfo->getBarCount() : -1;
            ppq_position_of_last_bar_start = Pinfo->getPpqPositionOfLastBarStart() ?
                                             *Pinfo->getPpqPositionOfLastBarStart() : -1;
            sample_rate = sample_rate_;
            buffer_size_in_samples = buffer_size_in_samples_;
        }
    }

    bool operator==(const BufferMetaData &e) const {
        return (qpm == e.qpm) && (numerator == e.numerator) && (denominator == e.denominator) && 
               (isPlaying == e.isPlaying) && (isRecording == e.isRecording) && 
               (isLooping == e.isLooping) && 
               (loop_start_in_ppq == e.loop_start_in_ppq) && (loop_end_in_ppq == e.loop_end_in_ppq) && 
               (sample_rate == e.sample_rate) && 
               (buffer_size_in_samples == e.buffer_size_in_samples);
    }

    bool operator!=(const BufferMetaData &e) const {
        return !(*this == e);
    }

    void operator=(const BufferMetaData &e) {
        this->qpm = e.qpm;
        this->numerator = e.numerator;
        this->denominator = e.denominator;
        this->isPlaying = e.isPlaying;
        this->isRecording = e.isRecording;
        this->time_in_samples = e.time_in_samples;
        this->time_in_seconds = e.time_in_seconds;
        this->time_in_ppq = e.time_in_ppq;
        this->playhead_force_moved_forward = e.playhead_force_moved_forward;
        this->playhead_force_moved_backward = e.playhead_force_moved_backward;
        this->isLooping = e.isLooping;
        this->loop_start_in_ppq = e.loop_start_in_ppq;
        this->loop_end_in_ppq = e.loop_end_in_ppq;
        this->bar_count = e.bar_count;
        this->ppq_position_of_last_bar_start = e.ppq_position_of_last_bar_start;
        this->sample_rate = e.sample_rate;
        this->buffer_size_in_samples = e.buffer_size_in_samples;
    }
};

/*
 * All information received from the host are wrapped in this class.
 *
 * Each event, has the following fields:
 *
 * 1. bufferMetaData --> metadata of the buffer frame to which the event corresponds
 * 2. time_in_samples --> start time in samples of the event
 * 3. time_in_seconds --> start time in seconds of the event
 * 4. time_in_ppq --> start time in ppq of the event
 * 5. type:
 *
 *     -1: Playback Stopped Event
 *              the event holds information about the last frame
 *      1: First Buffer Event, Since Start
 *              the event holds information about the first frame
 *      2: New Buffer Event
 *              the event specifies that a new buffer just started
 *      3: New Bar Event
 *              specifies that a new bar just started
 *      4: New Time Shift Event
 *              specifies that the playback has progressed by a specific
 *              time amount specified in settings.h (for example, every 8th note)
 *
 *      10: MidiMessage within Buffer Event
 *              specifies that a midi message was received within the buffer
 *
 * A few notes about the availability of information:
 * - Playback Stopped Event: Always available
 * - First Buffer Event, Since Start: Always available
 * - New Buffer Event: Can be available at arrival of every new buffer, or
 *                  whenever the essential fields of the buffer (qpm, numerator,...)
 *                  are changed, flags to set:
 *
 *      >> constexpr bool SendEventAtBeginningOfNewBuffers_FLAG{true};
 *      >> constexpr bool SendEventForNewBufferIfMetadataChanged_FLAG{true};
 *
 *                  If first flag is false, this event wont be available at all.
 *
 * - New Bar Event: Can be available at arrival of every new bar, if the flag is set:
 *
 *      >> constexpr bool SendNewBarEvents_FLAG{true};
 *
 * - New Time Shift Event: Can be available at arrival of every new time shift, if the flag is set:
 *
 *      >> constexpr bool SendTimeShiftEvents_FLAG{true};
 *      >> constexpr double delta_TimeShiftEventRatioOfQuarterNote{0.5}; //8th note
 *
 * - MidiMessage within Buffer Event: always available as long as the filter flags are not set to true:
 *
 *      >> constexpr bool FilterNoteOnEvents_FLAG{false};
 *      >> constexpr bool FilterNoteOffEvents_FLAG{false};
 *      >> constexpr bool FilterCCEvents_FLAG{false};
 * -
 */

using chrono_time = std::chrono::time_point<std::chrono::system_clock>;

// any event from the host is wrapped in this class
class Event {
public:

    Event() = default;

    Event(juce::Optional<juce::AudioPlayHead::PositionInfo> Pinfo,
          double sample_rate_, int64_t buffer_size_in_samples_, bool isFirstFrame) {

        chrono_timed.registerStartTime();

        type = isFirstFrame ? 1 : 2;

        bufferMetaData = BufferMetaData(Pinfo, sample_rate_, buffer_size_in_samples_);

        time_in_samples = bufferMetaData.time_in_samples;
        time_in_seconds = bufferMetaData.time_in_seconds;
        time_in_ppq = bufferMetaData.time_in_ppq;
    }

    Event(juce::Optional<juce::AudioPlayHead::PositionInfo> Pinfo, double sample_rate_,
          int64_t buffer_size_in_samples_, juce::MidiMessage &message_) {

        chrono_timed.registerStartTime();

        type = 10;
        message = std::move(message_);
        message.getDescription();

        bufferMetaData = BufferMetaData(Pinfo, sample_rate_, buffer_size_in_samples_);

        if (Pinfo) {
            auto frame_start_time_in_samples = bufferMetaData.time_in_samples;
            auto message_start_in_samples = Pinfo->getTimeInSamples() ? int64_t(message_.getTimeStamp()) : -1;
            time_in_samples = frame_start_time_in_samples + message_start_in_samples;

            auto frame_start_time_in_seconds = bufferMetaData.time_in_seconds;
            auto message_start_in_seconds = Pinfo->getTimeInSeconds() ? n_samples_to_sec(
                    static_cast<double>(message_start_in_samples), sample_rate_) : -1;
            time_in_seconds = frame_start_time_in_seconds + message_start_in_seconds;

            auto frame_start_time_in_ppq = bufferMetaData.time_in_ppq;
            auto message_start_in_ppq = Pinfo->getPpqPosition() ? n_samples_to_ppq(
                    static_cast<double>(message_start_in_samples), bufferMetaData.qpm, sample_rate_) : -1;
            time_in_ppq = frame_start_time_in_ppq + message_start_in_ppq;
        }
    }

    std::optional<Event> checkIfNewBarHappensWithinBuffer() {
        auto ppq_start = bufferMetaData.time_in_ppq;
        auto ppq_end = bufferMetaData.time_in_ppq + n_samples_to_ppq(
                static_cast<double>(
                        bufferMetaData.buffer_size_in_samples), bufferMetaData.qpm, bufferMetaData.sample_rate);


        double bar_dur_in_ppq = bufferMetaData.numerator * 4.0 / bufferMetaData.denominator;

        // find first whole multiple of bar_dur_in_ppq that is greater than ppq_start
        double first_bar_start_in_ppq = ceil(ppq_start / bar_dur_in_ppq) * bar_dur_in_ppq;

        if (first_bar_start_in_ppq > ppq_end) {
            return std::nullopt;
        } else {
            auto new_event = *this;
            new_event.type = 3;
            // dtime from buffer_start_in_ppq
            auto dtime_ppq = first_bar_start_in_ppq - ppq_start;
            auto loc_ratio_in_buffer = dtime_ppq / (ppq_end - ppq_start);
            new_event.time_in_ppq = first_bar_start_in_ppq;
            // convert ppq to samples
            new_event.time_in_samples = int64_t(double(bufferMetaData.time_in_samples) * (1 + loc_ratio_in_buffer));
            new_event.time_in_seconds = bufferMetaData.time_in_seconds + dtime_ppq / 60.0 * bufferMetaData.qpm;
            return new_event;
        }
    }

    std::optional<Event> checkIfTimeShiftEventHappensWithinBuffer(double time_shift_in_ratio_of_quarter_note) {
        auto ppq_start = bufferMetaData.time_in_ppq;
        auto ppq_end = bufferMetaData.time_in_ppq + n_samples_to_ppq(
                static_cast<double>(
                        bufferMetaData.buffer_size_in_samples), bufferMetaData.qpm, bufferMetaData.sample_rate);

        double delta_time_shift_ppq = time_shift_in_ratio_of_quarter_note;

        // find first whole multiple of bar_dur_in_ppq that is greater than ppq_start
        double first_time_shift_start_in_ppq = ceil(ppq_start / delta_time_shift_ppq) * delta_time_shift_ppq;

        if (first_time_shift_start_in_ppq > ppq_end) {
            return std::nullopt;
        } else {
            auto new_event = *this;
            new_event.type = 4;
            // dtime from buffer_start_in_ppq
            auto dtime_ppq = first_time_shift_start_in_ppq - ppq_start;
            auto loc_ratio_in_buffer = dtime_ppq / (ppq_end - ppq_start);
            new_event.time_in_ppq = first_time_shift_start_in_ppq;
            // convert ppq to samples
            new_event.time_in_samples = int64_t(double(bufferMetaData.time_in_samples) * (1 + loc_ratio_in_buffer));
            new_event.time_in_seconds = bufferMetaData.time_in_seconds + dtime_ppq / 60.0 * bufferMetaData.qpm;
            return new_event;
        }

    }

    void setPlaybackStoppedEvent() {
        type = -1;
    }

    [[nodiscard]] int Type() const { return type; }

    bool isFirstBufferEvent() const { return type == 1; }

    bool isPlaybackStoppedEvent() const { return type == -1; }

    bool isNewBufferEvent() const { return type == 2; }

    bool isNewBarEvent() const { return type == 3; }

    bool isNewTimeShiftEvent() const { return type == 4; }

    bool isMidiMessageEvent() const { return type == 10; }

    bool isNoteOnEvent() const { return message.isNoteOn() && isMidiMessageEvent(); }

    bool isNoteOffEvent() const { return message.isNoteOff() && isMidiMessageEvent(); }

    bool isCCEvent() const { return message.isController() && isMidiMessageEvent(); }

    int getNoteNumber() const {
        assert ((isNoteOnEvent() || isNoteOffEvent()) && "Can only get note number for note on/off events");
        return message.getNoteNumber();
    }

    float getVelocity() const {
        assert ((isNoteOnEvent() || isNoteOffEvent()) && "Can only get velocity for note on/off events");
        return message.getFloatVelocity();
    }

    int getCCNumber() const {
        assert (isCCEvent() && "Can only get CC number for CC events");
        return message.getControllerNumber();
    }

    int getChannel() const {
        assert (isMidiMessageEvent() && "Can only get channel for midi events");
        return message.getChannel();
    }

    static double n_samples_to_ppq(double audioSamplePos, double qpm, double sample_rate) {
        auto tmp_ppq = audioSamplePos * qpm / (60 * sample_rate);
        return tmp_ppq;
    }

    static double n_samples_to_sec(double audioSamplePos, double sample_rate) {
        auto tmp_sec = audioSamplePos / sample_rate;
        return tmp_sec;
    }

    std::stringstream getDescriptionOfChangedFeatures(
            const Event& other, bool ignore_time_changes_for_new_buffer_events) const {
        std::stringstream ss;
        ss << "++ ";

        if (isFirstBufferEvent()) { ss << " | First Buffer"; }
        if (isPlaybackStoppedEvent()) { ss << " | Stop  Buffer"; }
        if (isNewBufferEvent()) { ss << " | New Buffer"; }
        if (isNewBarEvent()) { ss << " | New Bar"; }
        if (isNewTimeShiftEvent()) { ss << " | New Time Shift"; }

        if (isMidiMessageEvent()) { ss << " | message: " << message.getDescription(); }

        if (bufferMetaData.qpm != other.bufferMetaData.qpm) {
            ss << " | qpm: " << bufferMetaData.qpm;
        }
        if (bufferMetaData.numerator != other.bufferMetaData.numerator) {
            ss << " | num: " << bufferMetaData.numerator;
        }
        if (bufferMetaData.denominator != other.bufferMetaData.denominator) {
            ss << " | den: " << bufferMetaData.denominator;
        }

        if (bufferMetaData.isPlaying != other.bufferMetaData.isPlaying) {
            ss << " | isPly: " << bufferMetaData.isPlaying;
        }
        if (bufferMetaData.isRecording != other.bufferMetaData.isRecording) {
            ss << " | isRec: " << bufferMetaData.isRecording;
        }
        if (bufferMetaData.isLooping != other.bufferMetaData.isLooping) {
            ss << " | isLp: " << bufferMetaData.isLooping;
        }
        if (bufferMetaData.loop_start_in_ppq != other.bufferMetaData.loop_start_in_ppq) {
            ss << " | lp_str_ppq: " << bufferMetaData.loop_start_in_ppq;
        }
        if (bufferMetaData.loop_end_in_ppq != other.bufferMetaData.loop_end_in_ppq) {
            ss << " | lp_end_ppq: " << bufferMetaData.loop_end_in_ppq;
        }
        if (bufferMetaData.bar_count != other.bufferMetaData.bar_count) {
            ss << " | bar_ccnt: " << bufferMetaData.bar_count;
        }
        if (bufferMetaData.sample_rate != other.bufferMetaData.sample_rate) {
            ss << " | sr: " << bufferMetaData.sample_rate;
        }
        if (bufferMetaData.buffer_size_in_samples != other.bufferMetaData.buffer_size_in_samples) {
            ss << " | smpls_in_bfr: " << bufferMetaData.buffer_size_in_samples;
        }
        if (!(ignore_time_changes_for_new_buffer_events && isNewBufferEvent())) {
            if (time_in_samples != other.time_in_samples) {
                ss << " | time_in_samples: " << time_in_samples;
            }
            if (time_in_seconds != other.time_in_seconds) {
                ss << " | time_in_seconds: " << time_in_seconds;
            }
            if (time_in_ppq != other.time_in_ppq) {
                ss << " | time_in_ppq: " << time_in_ppq;
            }
        }

        if (chrono_timed.isValid()) {
            ss << *chrono_timed.getDescription(" | CreationToConsumption Delay: ");
        }

        if (ss.str().length() > 3) {
            return ss;
        } else {
            return {};
        }
    }

    std::stringstream getDescription() const {
        std::stringstream ss;
        ss << "++ ";
        if (isFirstBufferEvent()) { ss << " | First Buffer"; }
        if (isPlaybackStoppedEvent()) { ss << " | Stop  Buffer"; }
        if (isNewBufferEvent()) { ss << " |  New Buffer "; }
        if (isNewBarEvent()) { ss << " |  New  Bar   "; }
        if (isMidiMessageEvent()) { ss << " | message:    " << message.getDescription(); }

        ss << " | qpm: " << bufferMetaData.qpm;
        ss << " | num: " << bufferMetaData.numerator;
        ss << " | den: " << bufferMetaData.denominator;
        ss << " | isPly: " << bufferMetaData.isPlaying;
        ss << " | isRec: " << bufferMetaData.isRecording;
        ss << " | isLp: " << bufferMetaData.isLooping;
        ss << " | lp_str_ppq: " << bufferMetaData.loop_start_in_ppq;
        ss << " | lp_end_ppq: " << bufferMetaData.loop_end_in_ppq;
        ss << " | bar_ccnt: " << bufferMetaData.bar_count;
        ss << " | last_bar_ppq: " << bufferMetaData.ppq_position_of_last_bar_start;
        ss << " | sr: " << bufferMetaData.sample_rate;
        ss << " | smpls_in_bfr: " << bufferMetaData.buffer_size_in_samples;
        ss << " | time_in_samples: " << time_in_samples;
        ss << " | time_in_seconds: " << time_in_seconds;
        ss << " | time_in_ppq: " << time_in_ppq;

        if (chrono_timed.isValid()) {
            ss << *chrono_timed.getDescription(" | CreationToConsumption Delay: ");
        }

        if (ss.str().length() > 3) {
            return ss;
        } else {
            return {};
        }

    }

    // getter methods
    double qpm() const { return bufferMetaData.qpm; }
    double numerator() const { return bufferMetaData.numerator; }
    double denominator() const { return bufferMetaData.denominator; }
    bool isPlaying() const { return bufferMetaData.isPlaying; }
    bool isRecording() const { return bufferMetaData.isRecording; }

    time_ BufferStartTime() const {
        return time_(bufferMetaData.time_in_samples,
                     bufferMetaData.time_in_seconds,
                     bufferMetaData.time_in_ppq);
    }

    time_ Time() const {
        return time_(time_in_samples,
                     time_in_seconds,
                     time_in_ppq);
    }

    bool isLooping() const { return bufferMetaData.isLooping; }
    double loopStart() const { return bufferMetaData.loop_start_in_ppq; }
    double loopEnd() const { return bufferMetaData.loop_end_in_ppq; }

    int64_t barCount() const { return bufferMetaData.bar_count; }
    double lastBarPos() const { return bufferMetaData.ppq_position_of_last_bar_start; }

    time_ time_from(Event e) {
        return time_(time_in_samples - e.time_in_samples,
                     time_in_seconds - e.time_in_seconds,
                     time_in_ppq - e.time_in_ppq);
    }

    void setIsPlaying(bool isPlaying) { bufferMetaData.isPlaying = isPlaying; }

    BufferMetaData getBufferMetaData() const { return bufferMetaData; }

    void registerAccess() { chrono_timed.registerEndTime(); }

private:
    int type{0};

    juce::MidiMessage message{};
    BufferMetaData bufferMetaData;

    // The actual time of the event. If event is a midiMessage, this time stamp
    // can be different from the starting time stam of the buffer found in bufferMetaData.time_in_* fields
    int64_t time_in_samples{-1};
    double time_in_seconds{-1};
    double time_in_ppq{-1};

    // uses chrono::system_clock to time events (for debugging only)
    // don't use this for anything else than debugging.
    // used to keep track of when the object was created && when it was accessed
    chrono_timer chrono_timed;
};


// Events received from an imported midi file (via drag drop)
class MidiFileEvent {

    MidiFileEvent() = default;

    MidiFileEvent(juce::MidiMessage &message_, bool isFirstEvent_, bool isLastEvent_) {

        chrono_timed.registerStartTime();

        message = std::move(message_);
        message.getDescription();
        time_in_ppq = message.getTimeStamp();

        _isFirstEvent = isFirstEvent_;
        _isLastEvent = isLastEvent_;
    }

    bool isFirstMessage() const { return _isFirstEvent; }
    bool isLastMessage() const { return _isLastEvent; }

    bool isNoteOnEvent() const { return message.isNoteOn(); }

    bool isNoteOffEvent() const { return message.isNoteOff(); }

    bool isCCEvent() const { return message.isController(); }

    int getNoteNumber() const {
        assert ((isNoteOnEvent() || isNoteOffEvent()) && "Can only get note number for note on/off events");
        return message.getNoteNumber();
    }

    float getVelocity() const {
        assert ((isNoteOnEvent() || isNoteOffEvent()) && "Can only get velocity for note on/off events");
        return message.getFloatVelocity();
    }

    int getCCNumber() const {
        assert (isCCEvent() && "Can only get CC number for CC events");
        return message.getControllerNumber();
    }

    static double n_samples_to_ppq(double audioSamplePos, double qpm, double sample_rate) {
        auto tmp_ppq = audioSamplePos * qpm / (60 * sample_rate);
        return tmp_ppq;
    }

    static double n_samples_to_sec(double audioSamplePos, double sample_rate) {
        auto tmp_sec = audioSamplePos / sample_rate;
        return tmp_sec;
    }

    std::stringstream getDescription(double sample_rate, double qpm) const {
        auto time_in_samples = ppq_to_samples (sample_rate, qpm);
        auto time_in_seconds = ppq_to_seconds (qpm);
        std::stringstream ss;
        ss << "++ ";
        ss << " | message:    " << message.getDescription();

        ss << " | time_in_samples: " << time_in_samples;
        ss << " | time_in_seconds: " << time_in_seconds;
        ss << " | time_in_ppq: " << time_in_ppq;

        if (chrono_timed.isValid()) {
            ss << *chrono_timed.getDescription(" | CreationToConsumption Delay: ");
        }

        if (ss.str().length() > 1) {
            return ss;
        } else {
            return {};
        }

    }

    time_ Time(double sample_rate, double qpm) const {
        auto time_in_samples = ppq_to_samples (sample_rate, qpm);
        auto time_in_seconds = ppq_to_seconds (qpm);
        return time_(time_in_samples,
                     time_in_seconds,
                     time_in_ppq);
    }


    time_ time_from(const MidiFileEvent& e, double sample_rate, double qpm) {
        auto time_in_samples = ppq_to_samples (sample_rate, qpm);
        auto time_in_seconds = ppq_to_seconds (qpm);
        auto other_time_in_samples = e.ppq_to_samples (sample_rate, qpm);
        auto other_time_in_seconds = e.ppq_to_seconds (qpm);

        return time_(time_in_samples - other_time_in_samples,
                     time_in_seconds - other_time_in_seconds,
                     time_in_ppq - e.time_in_ppq);
    }

    void registerAccess() { chrono_timed.registerEndTime(); }

private:

    juce::MidiMessage message{};

    // The actual time of the event. If event is a midiMessage, this time stamp
    // can be different from the starting time stam of the buffer found in bufferMetaData.time_in_* fields
    double time_in_ppq{-1};

    long ppq_to_samples (double sample_rate, double qpm) const {
        if (time_in_ppq == -1) {
            return -1;
        } else {
            return long(time_in_ppq * 60.0 * sample_rate / qpm);
        }
    }

    double ppq_to_seconds (double qpm) const {
        if (time_in_ppq == -1) {
            return -1;
        } else {
            return time_in_ppq / qpm * 60.0;
        }
    }

    bool _isFirstEvent{false};
    bool _isLastEvent{false};

    // uses chrono::system_clock to time events (for debugging only)
    // don't use this for anything else than debugging.
    // used to keep track of when the object was created && when it was accessed
    chrono_timer chrono_timed;
};