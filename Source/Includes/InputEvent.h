//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "GuiParameters.h"
#include "chrono_timer.h"
#include <utility>
#include <mutex>


using namespace juce;

struct time_ {
    time_(): seconds(0.0), samples(0), ppq(0.0) {}

    explicit time_(int64_t samples_, double seconds_, double ppq_) :
    seconds(seconds_), samples(samples_), ppq(ppq_) {}

    [[nodiscard]] double inSeconds() const { return seconds; }
    [[nodiscard]] int64_t inSamples() const { return samples; }
    [[nodiscard]]  double inQuarterNotes() const { return ppq; }

    // unitType == 1 --> samples
    // unitType == 2 --> seconds
    // unitType == 3 --> quarter notes
    [[nodiscard]] double getTimeWithUnitType(int unitType) const {
        switch (unitType) {
            case 1:
                return (double)inSamples();
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
 *      depending on the use case intended. The frequency of accessing
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
 *      since the metadata is embedded in any midi EventFromHost. Just remember to not
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

    [[maybe_unused]] bool playhead_force_moved_forward{false};
    [[maybe_unused]] bool playhead_force_moved_backward{false};

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

//    void operator=(const BufferMetaData &e) {
//        this->qpm = e.qpm;
//        this->numerator = e.numerator;
//        this->denominator = e.denominator;
//        this->isPlaying = e.isPlaying;
//        this->isRecording = e.isRecording;
//        this->time_in_samples = e.time_in_samples;
//        this->time_in_seconds = e.time_in_seconds;
//        this->time_in_ppq = e.time_in_ppq;
//        this->playhead_force_moved_forward = e.playhead_force_moved_forward;
//        this->playhead_force_moved_backward = e.playhead_force_moved_backward;
//        this->isLooping = e.isLooping;
//        this->loop_start_in_ppq = e.loop_start_in_ppq;
//        this->loop_end_in_ppq = e.loop_end_in_ppq;
//        this->bar_count = e.bar_count;
//        this->ppq_position_of_last_bar_start = e.ppq_position_of_last_bar_start;
//        this->sample_rate = e.sample_rate;
//        this->buffer_size_in_samples = e.buffer_size_in_samples;
//    }
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
 *     -1: Playback Stopped EventFromHost
 *              the event holds information about the last frame
 *      1: First Buffer EventFromHost, Since Start
 *              the event holds information about the first frame
 *      2: New Buffer EventFromHost
 *              the event specifies that a new buffer just started
 *      3: New Bar EventFromHost
 *              specifies that a new bar just started
 *      4: New Time Shift EventFromHost
 *              specifies that the playback has progressed by a specific
 *              time amount specified in settings.h (for example, every 8th note)
 *
 *      10: MidiMessage within Buffer EventFromHost
 *              specifies that a midi message was received within the buffer
 *
 * A few notes about the availability of information:
 * - Playback Stopped EventFromHost: Always available
 * - First Buffer EventFromHost, Since Start: Always available
 * - New Buffer EventFromHost: Can be available at arrival of every new buffer, or
 *                  whenever the essential fields of the buffer (qpm, numerator,...)
 *                  are changed, flags to set:
 *
 *      >> constexpr bool SendEventAtBeginningOfNewBuffers_FLAG{true};
 *      >> constexpr bool SendEventForNewBufferIfMetadataChanged_FLAG{true};
 *
 *                  If first flag is false, this event won't be available at all.
 *
 * - New Bar EventFromHost: Can be available at arrival of every new bar, if the flag is set:
 *
 *      >> constexpr bool SendNewBarEvents_FLAG{true};
 *
 * - New Time Shift EventFromHost: Can be available at arrival of every new time shift, if the flag is set:
 *
 *      >> constexpr bool SendTimeShiftEvents_FLAG{true};
 *      >> constexpr double delta_TimeShiftEventRatioOfQuarterNote{0.5}; //8th note
 *
 * - MidiMessage within Buffer EventFromHost: always available as long as the filter flags are not set to true:
 *
 *      >> constexpr bool FilterNoteOnEvents_FLAG{false};
 *      >> constexpr bool FilterNoteOffEvents_FLAG{false};
 *      >> constexpr bool FilterCCEvents_FLAG{false};
 * -
 */

// using chrono_time = std::chrono::time_point<std::chrono::system_clock>;

// any event from the host is wrapped in this class
class EventFromHost
{
public:
    EventFromHost() = default;

    EventFromHost(juce::Optional<juce::AudioPlayHead::PositionInfo> Pinfo,
          double sample_rate_, int64_t buffer_size_in_samples_, bool isFirstFrame) {

        chrono_timed.registerStartTime();

        type = isFirstFrame ? 1 : 2;

        bufferMetaData = BufferMetaData(Pinfo, sample_rate_, buffer_size_in_samples_);

        time_in_samples = bufferMetaData.time_in_samples;
        time_in_seconds = bufferMetaData.time_in_seconds;
        time_in_ppq = bufferMetaData.time_in_ppq;
    }

    EventFromHost(juce::Optional<juce::AudioPlayHead::PositionInfo> Pinfo, double sample_rate_,
          int64_t buffer_size_in_samples_, juce::MidiMessage &message_) {

        chrono_timed.registerStartTime();

        type = 10;
        message = std::move(message_);
        message.getDescription();

        bufferMetaData = BufferMetaData(Pinfo, sample_rate_, buffer_size_in_samples_);

        if (Pinfo) {
            auto frame_start_time_in_samples = bufferMetaData.time_in_samples;
            auto message_start_in_samples = Pinfo->getTimeInSamples() ? int64_t(message.getTimeStamp()) : -1;
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

    std::optional<EventFromHost> checkIfNewBarHappensWithinBuffer() {
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

    std::optional<EventFromHost> checkIfTimeShiftEventHappensWithinBuffer(double time_shift_in_ratio_of_quarter_note) {
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

    [[nodiscard]] bool isFirstBufferEvent() const { return type == 1; }

    [[nodiscard]] bool isPlaybackStoppedEvent() const { return type == -1; }

    [[nodiscard]] bool isNewBufferEvent() const { return type == 2; }

    [[nodiscard]] bool isNewBarEvent() const { return type == 3; }

    [[nodiscard]] bool isNewTimeShiftEvent() const { return type == 4; }

    [[nodiscard]] bool isMidiMessageEvent() const { return type == 10; }

    [[nodiscard]]  bool isNoteOnEvent() const { return message.isNoteOn() && isMidiMessageEvent(); }

    [[nodiscard]]  bool isNoteOffEvent() const { return message.isNoteOff() && isMidiMessageEvent(); }

    [[nodiscard]] bool isCCEvent() const { return message.isController() && isMidiMessageEvent(); }

    [[nodiscard]] int getNoteNumber() const {
        assert ((isNoteOnEvent() || isNoteOffEvent()) && "Can only get note number for note on/off events");
        return message.getNoteNumber();
    }

    [[nodiscard]] float getVelocity() const {
        assert ((isNoteOnEvent() || isNoteOffEvent()) && "Can only get velocity for note on/off events");
        return message.getFloatVelocity();
    }

    [[maybe_unused]] [[nodiscard]] int getCCNumber() const {
        assert (isCCEvent() && "Can only get CC number for CC events");
        return message.getControllerNumber();
    }

    [[maybe_unused]] [[nodiscard]] float getCCValue() const {
        assert (isCCEvent() && "Can only get CC value for CC events");
        return (float) message.getControllerValue();
    }

    [[nodiscard]] int getChannel() const {
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

    [[nodiscard]] std::stringstream getDescriptionOfChangedFeatures(
            const EventFromHost& other, bool ignore_time_changes_for_new_buffer_events) const {
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
            ss << " | bar_cnt: " << bufferMetaData.bar_count;
        }
        if (bufferMetaData.sample_rate != other.bufferMetaData.sample_rate) {
            ss << " | sr: " << bufferMetaData.sample_rate;
        }
        if (bufferMetaData.buffer_size_in_samples != other.bufferMetaData.buffer_size_in_samples) {
            ss << " | samples_in_bfr: " << bufferMetaData.buffer_size_in_samples;
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

    [[nodiscard]]  std::stringstream getDescription() const {
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
        ss << " | bar_cnt: " << bufferMetaData.bar_count;
        ss << " | last_bar_ppq: " << bufferMetaData.ppq_position_of_last_bar_start;
        ss << " | sr: " << bufferMetaData.sample_rate;
        ss << " | samples_in_bfr: " << bufferMetaData.buffer_size_in_samples;
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
    [[maybe_unused]][[nodiscard]] double qpm() const { return bufferMetaData.qpm; }
    [[maybe_unused]][[nodiscard]] double numerator() const { return bufferMetaData.numerator; }
    [[maybe_unused]][[nodiscard]]  double denominator() const { return bufferMetaData.denominator; }
    [[maybe_unused]][[nodiscard]] bool isPlaying() const { return bufferMetaData.isPlaying; }
    [[maybe_unused]][[nodiscard]] bool isRecording() const { return bufferMetaData.isRecording; }

    [[nodiscard]] time_ BufferStartTime() const {
        return time_(bufferMetaData.time_in_samples,
                     bufferMetaData.time_in_seconds,
                     bufferMetaData.time_in_ppq);
    }

    [[nodiscard]]  time_ Time() const {
        return time_(time_in_samples,
                     time_in_seconds,
                     time_in_ppq);
    }

    [[maybe_unused]][[nodiscard]] bool isLooping() const { return bufferMetaData.isLooping; }
    [[maybe_unused]][[nodiscard]] double loopStart() const {
        return bufferMetaData.loop_start_in_ppq;
    }
    [[maybe_unused]][[nodiscard]] double loopEnd() const { return bufferMetaData.loop_end_in_ppq; }

    [[maybe_unused]][[nodiscard]] int64_t barCount() const { return bufferMetaData.bar_count; }
    [[maybe_unused]][[nodiscard]] time_ lastBarPos() const
    {
        auto l_bar_ppq = bufferMetaData.ppq_position_of_last_bar_start;
        auto buffer_start = BufferStartTime();
        auto l_bar_sec =
            buffer_start.inSeconds()
            + (l_bar_ppq - buffer_start.inQuarterNotes()) / 60.0 * bufferMetaData.qpm;
        auto l_bar_samples = double(buffer_start.inSamples())
                             + (l_bar_ppq - buffer_start.inQuarterNotes())
                                   / bufferMetaData.qpm * 60.0
                                   * bufferMetaData.sample_rate;
        return time_((std::int64_t) l_bar_samples, l_bar_sec, l_bar_ppq);
    }

    [[maybe_unused]] [[nodiscard]] time_ time_from(const EventFromHost& e) const {
        return time_(time_in_samples - e.time_in_samples,
                     time_in_seconds - e.time_in_seconds,
                     time_in_ppq - e.time_in_ppq);
    }

    void setIsPlaying(bool isPlaying) { bufferMetaData.isPlaying = isPlaying; }

    [[maybe_unused]] [[nodiscard]] BufferMetaData getBufferMetaData() const { return bufferMetaData; }

    void registerAccess() { chrono_timed.registerEndTime(); }

private:
    int type{0};

    juce::MidiMessage message{};
    BufferMetaData bufferMetaData;

    // The actual time of the event. If event is a midiMessage, this time stamp
    // can be different from the starting time stamp of the buffer found in bufferMetaData.time_in_* fields
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

public:
    MidiFileEvent() = default;

    MidiFileEvent(juce::MidiMessage &message_, bool isFirstEvent_, bool isLastEvent_) {

        chrono_timed.registerStartTime();

        message = std::move(message_);
        message.getDescription();
        time_in_ppq = message.getTimeStamp();

        _isFirstEvent = isFirstEvent_;
        _isLastEvent = isLastEvent_;
    }

    [[nodiscard]] bool isFirstMessage() const { return _isFirstEvent; }
    [[nodiscard]] bool isLastMessage() const { return _isLastEvent; }

    [[nodiscard]]  bool isNoteOnEvent() const { return message.isNoteOn(); }

    [[nodiscard]] bool isNoteOffEvent() const { return message.isNoteOff(); }

    [[nodiscard]] bool isCCEvent() const { return message.isController(); }

    [[nodiscard]] int getNoteNumber() const {
        assert ((isNoteOnEvent() || isNoteOffEvent()) && "Can only get note number for note on/off events");
        return message.getNoteNumber();
    }

    [[nodiscard]] float getVelocity() const {
        assert ((isNoteOnEvent() || isNoteOffEvent()) && "Can only get velocity for note on/off events");
        return message.getFloatVelocity();
    }

    [[maybe_unused]] [[nodiscard]] int getCCNumber() const {
        assert (isCCEvent() && "Can only get CC number for CC events");
        return message.getControllerNumber();
    }

    [[maybe_unused]] [[nodiscard]] float getCCValue() const {
        assert (isCCEvent() && "Can only get CC value for CC events");
        return (float) message.getControllerValue();
    }

    [[maybe_unused]] [[nodiscard]] static double n_samples_to_ppq(double audioSamplePos, double qpm, double sample_rate) {
        auto tmp_ppq = audioSamplePos * qpm / (60 * sample_rate);
        return tmp_ppq;
    }

    [[maybe_unused]] [[nodiscard]] static double n_samples_to_sec(double audioSamplePos, double sample_rate) {
        auto tmp_sec = audioSamplePos / sample_rate;
        return tmp_sec;
    }

    // if no sample rate is provided, use Time() method instead
    // it's your responsibility to keep track of the sample rate and qpm
    // (provided as EventFromHost in the **deploy** method of the InputTensorPreparator -DPL-
    // thread)
    [[maybe_unused]] [[nodiscard]] std::stringstream getDescription(double sample_rate, double qpm) const {
        auto time_in_samples = ppq_to_samples (sample_rate, qpm);
        auto time_in_seconds = ppq_to_seconds (qpm);
        std::stringstream ss;
        ss << "++ ";
        ss << " | message:    " << message.getDescription();

        ss << " | time_in_ppq: " << time_in_ppq;
        ss << " | time_in_samples: " << time_in_samples;
        ss << " | time_in_seconds: " << time_in_seconds;
        if (isFirstMessage()) { ss << " | First Message"; }
        if (isLastMessage()) { ss << " | Last Message"; }
        if (chrono_timed.isValid()) {
            ss << *chrono_timed.getDescription(" | CreationToConsumption Delay: ");
        }

        if (ss.str().length() > 1) {
            return ss;
        } else {
            return {};
        }
    }

    [[maybe_unused]] [[nodiscard]] std::stringstream getDescription() const{
        std::stringstream ss;
        ss << "++ ";
        ss << " | message:    " << message.getDescription();

        ss << " | time_in_ppq: " << time_in_ppq;

        if (isFirstMessage()) { ss << " | First Message"; }
        if (isLastMessage()) { ss << " | Last Message"; }
        if (chrono_timed.isValid()) {
            ss << *chrono_timed.getDescription(" | CreationToConsumption Delay: ");
        }

        if (ss.str().length() > 1) {
            return ss;
        } else {
            return {};
        }
    }

    // if no sample rate is provided, use Time() method instead
    // it's your responsibility to keep track of the sample rate and qpm
    // (provided as EventFromHost in the **deploy** method of the InputTensorPreparator -DPL-
    // thread)
    [[nodiscard]] time_ Time(double sample_rate, double qpm) const {
        auto time_in_samples = ppq_to_samples (sample_rate, qpm);
        auto time_in_seconds = ppq_to_seconds (qpm);
        return time_(time_in_samples,
                     time_in_seconds,
                     time_in_ppq);
    }


    [[nodiscard]] double Time() const {

        return time_in_ppq;
    }

    [[maybe_unused]]  [[nodiscard]] time_ time_from(const MidiFileEvent& e, double sample_rate, double qpm) {
        auto time_in_samples = ppq_to_samples (sample_rate, qpm);
        auto time_in_seconds = ppq_to_seconds (qpm);
        auto other_time_in_samples = e.ppq_to_samples (sample_rate, qpm);
        auto other_time_in_seconds = e.ppq_to_seconds (qpm);

        return time_(time_in_samples - other_time_in_samples,
                     time_in_seconds - other_time_in_seconds,
                     time_in_ppq - e.time_in_ppq);
    }

    [[maybe_unused]] [[nodiscard]] double time_from(const MidiFileEvent& e) const {


        return time_in_ppq - e.time_in_ppq;
    }

    [[maybe_unused]] void registerAccess() { chrono_timed.registerEndTime(); }

private:

    juce::MidiMessage message{};

    // The actual time of the event. If event is a midiMessage, this time stamp
    // can be different from the starting time stamp of the buffer found
    // in bufferMetaData.time_in_* fields
    double time_in_ppq{-1};

    [[nodiscard]] long ppq_to_samples (double sample_rate, double qpm) const {
        if (time_in_ppq == -1) {
            return -1;
        } else {
            return long(time_in_ppq * 60.0 * sample_rate / qpm);
        }
    }

    [[nodiscard]] double ppq_to_seconds (double qpm) const {
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


struct RealTimePlaybackInfo {
private:
    BufferMetaData bufferMetaData{};
    std::mutex mutex;

public:
    void setValues(BufferMetaData bufferMetaData_) {
        if (mutex.try_lock()) {
            bufferMetaData = bufferMetaData_;
            mutex.unlock(); // Don't forget to unlock after use
        } else {
            // Mutex was already locked; handle this case as needed
        }
    }

    BufferMetaData get() {
        std::lock_guard<std::mutex> lock(mutex);
        return bufferMetaData;
    }
};


struct CrossThreadPianoRollData
{
    CrossThreadPianoRollData() = default;

    // copy constructor
    CrossThreadPianoRollData(const CrossThreadPianoRollData& other) {
        std::lock_guard<std::mutex> lock(mutex);
        displayedSequence = other.displayedSequence;
        should_repaint = other.should_repaint;
        user_dropped_new_sequence = other.user_dropped_new_sequence;
    }

    // copy assignment operator
    CrossThreadPianoRollData& operator=(const CrossThreadPianoRollData& other) {
        std::lock_guard<std::mutex> lock(mutex);
        displayedSequence = other.displayedSequence;
        should_repaint = other.should_repaint;
        user_dropped_new_sequence = other.user_dropped_new_sequence;
        return *this;
    }

    // call this to empty out the content of the piano roll
    void clear() {
        std::lock_guard<std::mutex> lock(mutex);
        displayedSequence.clear();
        should_repaint = true;
        user_dropped_new_sequence = false;
    }

    // call this to set the content of the piano roll
    // isDraggedIn is true if the user dragged in a new sequence
    // if you call this from DPL thread, set this to false
    void setSequence(const juce::MidiMessageSequence& sequence, bool isDraggedIn = false) {
        std::lock_guard<std::mutex> lock(mutex);
        displayedSequence.clear();
        should_repaint = true;
        user_dropped_new_sequence = false;
        displayedSequence = sequence;
        should_repaint = true;
        user_dropped_new_sequence = isDraggedIn;
    }

    void addNoteOn(int channel, int noteNumber, float velocity, double time) {
        std::lock_guard<std::mutex> lock(mutex);
        channel = channel % 16 + 1; // make sure channel is between 1 and 16
        velocity = velocity > 1 ? 1 : velocity; // make sure velocity is between 0 and 1
        velocity = velocity < 0 ? 0 : velocity;
        displayedSequence.addEvent(
            juce::MidiMessage::noteOn(channel, noteNumber, velocity),
            time);
        user_dropped_new_sequence = false;
        should_repaint = true;
    }

    void addNoteOff(int channel, int noteNumber, double time) {
        std::lock_guard<std::mutex> lock(mutex);
        channel = channel % 16 + 1; // make sure channel is between 1 and 16
        displayedSequence.addEvent(
            juce::MidiMessage::noteOff(channel, noteNumber),
            time);
        user_dropped_new_sequence = false;
        should_repaint = true;
    }

    void addNoteWithDuration(
        int channel, int noteNumber, float velocity, double time, double duration)	{
        std::lock_guard<std::mutex> lock(mutex);
        channel = channel % 16 + 1; // make sure channel is between 1 and 16
        velocity = velocity > 1 ? 1 : velocity; // make sure velocity is between 0 and 1
        velocity = velocity < 0 ? 0 : velocity;
        displayedSequence.addEvent(
            juce::MidiMessage::noteOn(channel, noteNumber, velocity),
            time);
        displayedSequence.addEvent(
            juce::MidiMessage::noteOff(channel, noteNumber),
            time + duration);
        user_dropped_new_sequence = false;
        should_repaint = true;
    }

    vector<MidiFileEvent> getMidiFileEvents() {
            std::lock_guard<std::mutex> lock(mutex);
            vector<MidiFileEvent> events;
            for (int i = 0; i < displayedSequence.getNumEvents(); ++i) {
                auto e = displayedSequence.getEventPointer(i);
                auto& m = e->message;
                events.emplace_back(
                    m, i == 0, i == displayedSequence.getNumEvents() - 1);
            }
            should_repaint = false;
            user_dropped_new_sequence = false;
            return events;
    }

    juce::MidiMessageSequence getCurrentSequence() {
        std::lock_guard<std::mutex> lock(mutex);
        user_dropped_new_sequence = false;
        return displayedSequence;
    }

    bool shouldRepaint() {
        std::lock_guard<std::mutex> lock(mutex);
        return should_repaint;
    }

    bool userDroppedNewSequence() {
        std::lock_guard<std::mutex> lock(mutex);
        return user_dropped_new_sequence;
    }

private:
    std::mutex mutex;
    juce::MidiMessageSequence displayedSequence;
    bool user_dropped_new_sequence{false};
    bool should_repaint{false};
};

struct MidiVisualizersData
{
    explicit MidiVisualizersData(const std::vector<std::string>& param_ids) {
        for (auto& param_id: param_ids) {
            (pianoRolls)[label2ParamID(param_id)] = CrossThreadPianoRollData();
        }
    }

    // do not use this method in DPL thread!!
    [[maybe_unused]] void setVisualizers(std::map<std::string, CrossThreadPianoRollData> pianoRolls_) {
        std::lock_guard<std::mutex> lock(mutex);
        pianoRolls = std::move(pianoRolls_);
    }

    // do not use this method in DPL thread!!
    CrossThreadPianoRollData* getVisualizerResources(const std::string& param_id) {
        std::lock_guard<std::mutex> lock(mutex);

        if (!is_valid_param_id(param_id)) {
            return nullptr;
        } else {
            return &(pianoRolls)[label2ParamID(param_id)];
        }
    }

    // get the list of visualizer ids on which the user dropped a new sequence
    std::vector<std::string> get_visualizer_ids_with_user_dropped_new_sequences() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<std::string> visualizers_with_new_sequences;
        for (auto& [key, value] : pianoRolls) {
            if (value.userDroppedNewSequence()) {
                visualizers_with_new_sequences.push_back(key);
            }
        }
        return visualizers_with_new_sequences;
    }

    // returns the midi file events for the visualizer
    [[maybe_unused]] std::optional<vector<MidiFileEvent>> get_visualizer_data(const std::string& param_id) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!is_valid_param_id(param_id)) {
            return std::nullopt;
        } else {
            auto mf_events = (pianoRolls)[label2ParamID(param_id)].getMidiFileEvents();
            if (mf_events.empty()) {
                return std::nullopt;
            } else {
                return mf_events;
            }
        }
    }

    [[maybe_unused]] void clear_visualizer_data(const std::string& visualizer_id) {
        std::lock_guard<std::mutex> lock(mutex);
        // if visualizer_id is not valid, do nothing
        if (!is_valid_param_id(visualizer_id)) {
            return;
        }
        (pianoRolls)[label2ParamID(visualizer_id)].clear();
    }

    [[maybe_unused]] void clear_all_visualizers() {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto& [key, value] : pianoRolls) {
            value.clear();
        }
    }

    // returns true if successful
    [[maybe_unused]] bool displayNoteOn(
        const string&  visualizer_id, int noteNumber, float velocity, double time) {
        std::lock_guard<std::mutex> lock(mutex);
        // if visualizer_id is not valid, do nothing
        if (!is_valid_param_id(visualizer_id)) {
            return false;
        } else {
            (pianoRolls)[label2ParamID(visualizer_id)].addNoteOn(
                0, noteNumber, velocity, time);
            return true;
        }
    }

    // returns true if successful
    [[maybe_unused]]  bool displayNoteOff(
        const string& visualizer_id, int noteNumber, double time) {
        std::lock_guard<std::mutex> lock(mutex);
        // if visualizer_id is not valid, do nothing
        if (!is_valid_param_id(visualizer_id)) {
            return false;
        } else {
            (pianoRolls)[label2ParamID(visualizer_id)].addNoteOff(
                0, noteNumber, time);
            return true;
        }
    }

    // returns true if successful
    [[maybe_unused]] bool displayNoteWithDuration(
        const string&  visualizer_id, int noteNumber, float velocity, double time, double duration) {
        std::lock_guard<std::mutex> lock(mutex);
        // if visualizer_id is not valid, do nothing
        if (!is_valid_param_id(visualizer_id)) {
            return false;
        } else {
            (pianoRolls)[label2ParamID(visualizer_id)].addNoteWithDuration(
                0, noteNumber, velocity, time, duration);
            return true;
        }
    }

private:
    std::mutex mutex;
    std::map<std::string, CrossThreadPianoRollData> pianoRolls;

    // check if param_id is valid
    bool is_valid_param_id(const std::string& visualizer_id) {
        auto stat = pianoRolls.find(label2ParamID(visualizer_id)) != pianoRolls.end();
        if (!stat) {
            cout << "visualizer_id " << visualizer_id << " is not valid" << endl;
            cout << "Select a valid param_id from the list below:" << endl;
            for (auto& [key, value] : pianoRolls) {
                cout << key << endl;
            }
        }
        return stat;
    }

};


struct CrossThreadAudioVisualizerData
{
    CrossThreadAudioVisualizerData() = default;

    CrossThreadAudioVisualizerData(const CrossThreadAudioVisualizerData& other) {
        std::lock_guard<std::mutex> lock(mutex);
        displayedAudioBuffer = other.displayedAudioBuffer;
        sample_rate = other.sample_rate;
        should_repaint = other.should_repaint;
    }

    CrossThreadAudioVisualizerData& operator=(const CrossThreadAudioVisualizerData& other) {
        std::lock_guard<std::mutex> lock(mutex);
        displayedAudioBuffer = other.displayedAudioBuffer;
        sample_rate = other.sample_rate;
        should_repaint = other.should_repaint;
        return *this;
    }

    void setAudioBuffer(juce::AudioBuffer<float> audioBuffer_, double sample_rate_,
                        bool isDraggedIn = false) {
        std::lock_guard<std::mutex> lock(mutex);
        displayedAudioBuffer = std::move(audioBuffer_);
        sample_rate = (float) sample_rate_;
        should_repaint = true;
        user_dropped_new_audio = isDraggedIn;
    }

    // call this to access the audio buffer and sample rate
    std::pair<juce::AudioBuffer<float>, double> getAudioBuffer() {
        std::lock_guard<std::mutex> lock(mutex);
        user_dropped_new_audio = false;
        should_repaint = false;
        return {displayedAudioBuffer, sample_rate};
    }

    bool didUserDroppedNewAudio() {
        std::lock_guard<std::mutex> lock(mutex);
        return user_dropped_new_audio;
    }

    bool shouldRepaint() {
        std::lock_guard<std::mutex> lock(mutex);
        auto val = should_repaint;
        should_repaint = false;
        return val;
    }

    float getSampleRate() {
        std::lock_guard<std::mutex> lock(mutex);
        return sample_rate;
    }

private:
    std::mutex mutex;
    juce::AudioBuffer<float> displayedAudioBuffer {};
    float sample_rate{44100};
    bool should_repaint{false};
    bool user_dropped_new_audio{false};

};


struct AudioVisualizersData
{
    explicit AudioVisualizersData(const std::vector<std::string>& param_ids) {
        for (auto& param_id: param_ids) {
            (audioVisualizers)[label2ParamID(param_id)] =
                CrossThreadAudioVisualizerData();
        }
    }

    // do not use this method in DPL thread!!
    [[maybe_unused]] void setVisualizers(std::map<std::string, CrossThreadAudioVisualizerData> audioVisualizers_) {
        std::lock_guard<std::mutex> lock(mutex);
        audioVisualizers = std::move(audioVisualizers_);
    }

    // do not use this method in DPL thread!!
    CrossThreadAudioVisualizerData* getVisualizerResources(const std::string& param_id) {
        std::lock_guard<std::mutex> lock(mutex);

        if (!is_valid_param_id(param_id)) {
            return nullptr;
        } else {
            return &(audioVisualizers)[label2ParamID(param_id)];
        }
    }

    // get the list of visualizer ids on which the user dropped a new sequence
    [[maybe_unused]] std::vector<std::string> get_visualizer_ids_with_user_dropped_new_audio() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<std::string> visualizers_with_new_audio;
        for (auto& [key, value] : audioVisualizers) {
            if (value.didUserDroppedNewAudio()) {
                visualizers_with_new_audio.push_back(key);
            }
        }
        return visualizers_with_new_audio;
    }

    // returns the audio buffer for the visualizer
    [[maybe_unused]] std::optional<std::pair<juce::AudioBuffer<float>, double>> get_visualizer_data(const std::string& param_id) {
        std::lock_guard<std::mutex> lock(mutex);
        if (!is_valid_param_id(param_id)) {
            return std::nullopt;
        } else {
            auto [audio_buffer, sample_rate] = (audioVisualizers)[label2ParamID(param_id)].getAudioBuffer();
            if (audio_buffer.getNumSamples() == 0) {
                return std::nullopt;
            } else {
                return std::make_pair(audio_buffer, sample_rate);
            }
        }
    }

    [[maybe_unused]] void clear_visualizer_data(const std::string& visualizer_id) {
        std::lock_guard<std::mutex> lock(mutex);
        // if visualizer_id is not valid, do nothing
        if (!is_valid_param_id(visualizer_id)) {
            return;
        }
        (audioVisualizers)[label2ParamID(visualizer_id)].setAudioBuffer(
            juce::AudioBuffer<float>(), 44100);
    }

    [[maybe_unused]] bool display_audio(
        const string&  visualizer_id, juce::AudioBuffer<float> audioBuffer_, double sample_rate_) {
        std::lock_guard<std::mutex> lock(mutex);
        // if visualizer_id is not valid, do nothing
        if (!is_valid_param_id(visualizer_id)) {
            return false;
        } else {
            (audioVisualizers)[label2ParamID(visualizer_id)].setAudioBuffer(
                std::move(audioBuffer_), sample_rate_);
            return true;
        }
    }

private:
    std::mutex mutex;
    std::map<std::string, CrossThreadAudioVisualizerData> audioVisualizers;

    // check if param_id is valid
    bool is_valid_param_id(const std::string& visualiser_id) {
        auto stat = audioVisualizers.find(label2ParamID(visualiser_id)) != audioVisualizers.end();
        if (!stat) {
            cout << "visualiser_id " << visualiser_id << " is not valid" << endl;
            cout << "Select a valid param_id from the list below:" << endl;
            for (auto& [key, value] : audioVisualizers) {
                cout << key << endl;
            }
        }
        return stat;
    }

};