//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/CustomStructsAndLockFreeQueue.h"
#include "../settings.h"
#include <utility>


using namespace juce;


struct BufferMetaData {
    double qpm{-1};
    double numerator{-1};
    double denominator{-1};

    bool isPlaying{false};
    bool isRecording{false};

    int64_t time_in_samples{-1};
    double time_in_seconds{-1};
    double time_in_ppq{-1};

    int64_t delta_time_in_samples{0};
    double delta_time_in_seconds{0};
    double delta_time_in_ppq{0};
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

    void set_time_shift_compared_to_last_frame(const BufferMetaData& last_frame_meta_data) {
        // calculate time shift compared to last frame
        delta_time_in_samples = time_in_samples - last_frame_meta_data.time_in_samples;
        delta_time_in_seconds = time_in_seconds - last_frame_meta_data.time_in_seconds;
        delta_time_in_ppq = time_in_ppq - last_frame_meta_data.time_in_ppq;

        // check if playhead was manually moved forward or backward
        auto expected_next_frame_sample = last_frame_meta_data.time_in_samples +
                last_frame_meta_data.buffer_size_in_samples;
        if (expected_next_frame_sample < this->time_in_samples) {
            DBG("playhead moved FORWARD");
            playhead_force_moved_forward = true;
        } else if (expected_next_frame_sample > this->time_in_samples) {
            DBG("playhead moved BACKWARD");
            playhead_force_moved_backward = true;
        }
    }

    bool wasPlayheadManuallyMovedBackward() const {
        return playhead_force_moved_backward;
    }

    bool wasPlayheadManuallyMovedForward() const {
        return playhead_force_moved_forward;
    }

};

/*
 * Types:
 *
 *     -1: Playback Stopped Event
 *      1: First Buffer Event, Since Start
 *      2: New Buffer Event
 *      3: New Bar Event
 *      4: New Time Shift Event
 *
 *
 *      10: MidiMessage within Buffer Event
 */
// TODO ADD description
class Event {
public:

    int type{0};

    BufferMetaData bufferMetaData;

    // The actual time of the event. If event is a midiMessage, this time stamp
    // can be different from the starting time stam of the buffer found in bufferMetaData.time_in_* fields
    int64_t time_in_samples{-1};
    double time_in_seconds{-1};
    double time_in_ppq{-1};

    juce::MidiMessage message{};

    Event() = default;

    Event(juce::Optional<juce::AudioPlayHead::PositionInfo> Pinfo,
          double sample_rate_, int64_t buffer_size_in_samples_, bool isFirstFrame) {

        type = isFirstFrame ? 1 : 2;

        bufferMetaData = BufferMetaData(Pinfo, sample_rate_, buffer_size_in_samples_);

        time_in_samples = bufferMetaData.time_in_samples;
        time_in_seconds = bufferMetaData.time_in_seconds;
        time_in_ppq = bufferMetaData.time_in_ppq;
    }

    Event(juce::Optional<juce::AudioPlayHead::PositionInfo> Pinfo, double sample_rate_,
          int64_t buffer_size_in_samples_, const juce::MidiMessage &message_) : message(message_) {

        type = 10;

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

    int Type() const { return type; }

    bool isFirstBufferEvent() const { return type == 1; }

    bool isNewBufferEvent() const { return type == 2; }

    bool isNewBarEvent() const { return type == 3; }

    bool isNewTimeShiftEvent() const { return type == 4; }

    bool isMidiMessageEvent() const { return type == 10; }

    bool isNoteOnEvent() const { return message.isNoteOn() and isMidiMessageEvent(); }

    bool isNoteOffEvent() const { return message.isNoteOff() and isMidiMessageEvent(); }

    bool isCCEvent() const { return message.isController() and isMidiMessageEvent(); }

    static double n_samples_to_ppq(double audioSamplePos, double qpm, double sample_rate) {
        auto tmp_ppq = audioSamplePos * qpm / (60 * sample_rate);
        return tmp_ppq;
    }

    static double n_samples_to_sec(double audioSamplePos, double sample_rate) {
        auto tmp_sec = audioSamplePos / sample_rate;
        return tmp_sec;
    }

    std::stringstream getDescriptionOfChangedFeatures(
            Event other, bool ignore_time_changes_for_new_buffer_events) const {
        std::stringstream ss;
        ss << "++ ";

        if (isNewBarEvent()) { ss << " | New Bar"; }
        if (isNewTimeShiftEvent()) { ss << " | New Time Shift"; }

        if (isMidiMessageEvent()) {
            ss << " | message: " << message.getDescription();
        } else {
            if (bufferMetaData.wasPlayheadManuallyMovedBackward()) {
                ss << " | Playhead Manually Moved BACKWARD";
                ss << " by " << bufferMetaData.delta_time_in_samples << " samples, ";
                ss << bufferMetaData.delta_time_in_seconds << " seconds, ";
                ss << bufferMetaData.delta_time_in_ppq << " ppq ";
            }
            if (bufferMetaData.wasPlayheadManuallyMovedForward()) {
                ss << " | Playhead Manually Moved FORWARD";
                ss << " by " << bufferMetaData.delta_time_in_samples << " samples, ";
                ss << bufferMetaData.delta_time_in_seconds << " seconds, ";
                ss << bufferMetaData.delta_time_in_ppq << " ppq ";
            }
        }

        if (bufferMetaData.qpm != other.bufferMetaData.qpm) {
            ss << " | qpm: " << bufferMetaData.qpm;
        }
        if (bufferMetaData.numerator != other.bufferMetaData.numerator) {
            ss << " | ts numerator: " << bufferMetaData.numerator;
        }
        if (bufferMetaData.denominator != other.bufferMetaData.denominator) {
            ss << " | ts denominator: " << bufferMetaData.denominator;
        }

        if (bufferMetaData.isPlaying != other.bufferMetaData.isPlaying) {
            ss << " | isPlaying: " << bufferMetaData.isPlaying;
        }
        if (bufferMetaData.isRecording != other.bufferMetaData.isRecording) {
            ss << " | isRecording: " << bufferMetaData.isRecording;
        }
        if (bufferMetaData.isLooping != other.bufferMetaData.isLooping) {
            ss << " | isLooping: " << bufferMetaData.isLooping;
        }
        if (bufferMetaData.loop_start_in_ppq != other.bufferMetaData.loop_start_in_ppq) {
            ss << " | loop_start_in_ppq: " << bufferMetaData.loop_start_in_ppq;
        }
        if (bufferMetaData.loop_end_in_ppq != other.bufferMetaData.loop_end_in_ppq) {
            ss << " | loop_end_in_ppq: " << bufferMetaData.loop_end_in_ppq;
        }
        if (bufferMetaData.bar_count != other.bufferMetaData.bar_count) {
            ss << " | bar_count: " << bufferMetaData.bar_count;
        }
        if (bufferMetaData.sample_rate != other.bufferMetaData.sample_rate) {
            ss << " | sample_rate: " << bufferMetaData.sample_rate;
        }
        if (bufferMetaData.buffer_size_in_samples != other.bufferMetaData.buffer_size_in_samples) {
            ss << " | buffer_size_in_samples: " << bufferMetaData.buffer_size_in_samples;
        }
        if (!(ignore_time_changes_for_new_buffer_events and isNewBufferEvent())) {
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

        if (ss.str().length() > 3) {
            return ss;
        } else {
            return {};
        }
    }

    std::stringstream getDescription() const {
        std::stringstream ss;
        ss << "++ ";
        if (isNewBarEvent()) { ss << " | New Bar"; }
        if (isMidiMessageEvent()) {
            ss << " | message: " << message.getDescription();
        } else {
            if (bufferMetaData.wasPlayheadManuallyMovedBackward()) {
                ss << " | Playhead Manually Moved BACKWARD";
                ss << " by " << bufferMetaData.delta_time_in_samples << " samples, ";
                ss << bufferMetaData.delta_time_in_seconds << " seconds, ";
                ss << bufferMetaData.delta_time_in_ppq << " ppq ";
            }
            if (bufferMetaData.wasPlayheadManuallyMovedForward()) {
                ss << " | Playhead Manually Moved FORWARD";
                ss << " by " << bufferMetaData.delta_time_in_samples << " samples, ";
                ss << bufferMetaData.delta_time_in_seconds << " seconds, ";
                ss << bufferMetaData.delta_time_in_ppq << " ppq ";
            }
        }

        ss << " | qpm: " << bufferMetaData.qpm;
        ss << " | ts numerator: " << bufferMetaData.numerator;
        ss << " | ts denominator: " << bufferMetaData.denominator;
        ss << " | isPlaying: " << bufferMetaData.isPlaying;
        ss << " | isRecording: " << bufferMetaData.isRecording;
        ss << " | isLooping: " << bufferMetaData.isLooping;
        ss << " | loop_start_in_ppq: " << bufferMetaData.loop_start_in_ppq;
        ss << " | loop_end_in_ppq: " << bufferMetaData.loop_end_in_ppq;
        ss << " | bar_count: " << bufferMetaData.bar_count;
        ss << " | ppq_position_of_last_bar_start: " << bufferMetaData.ppq_position_of_last_bar_start;
        ss << " | sample_rate: " << bufferMetaData.sample_rate;
        ss << " | buffer_size_in_samples: " << bufferMetaData.buffer_size_in_samples;
        ss << " | time_in_samples: " << time_in_samples;
        ss << " | time_in_seconds: " << time_in_seconds;
        ss << " | time_in_ppq: " << time_in_ppq;

        if (ss.str().length() > 3) {
            return ss;
        } else {
            return std::stringstream();
        }

    }

    // ==================================================
    // Operators for sorting using time stamp
    // ==================================================

    bool operator<(const Event &e) const { return time_in_samples < e.time_in_samples; }

    bool operator<=(const Event &e) const { return time_in_samples <= e.time_in_samples; }

    bool operator>(const Event &e) const { return time_in_samples > e.time_in_samples; }

    bool operator>=(const Event &e) const { return time_in_samples >= e.time_in_samples; }

    bool operator==(const Event &e) const { return time_in_samples == e.time_in_samples; }

};