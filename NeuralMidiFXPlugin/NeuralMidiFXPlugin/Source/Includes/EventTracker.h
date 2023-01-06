//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/CustomStructsAndLockFreeQueue.h"
#include "../settings.h"
#include <utility>

/*
 * Types:
 *      1: First Buffer Since Start Event
 *      1: New Buffer Event
 *      2: MidiMessage within Buffer Event
 */



/*
 * getBpm
 * getTimeSignature

 * getIsPlaying
 * getIsRecording

 * getTimeInSamples
 * getTimeInSeconds
 * getPpqPosition

 * getIsLooping
 * getLoopPoints

 * getBarCount
 * getPpqPositionOfLastBarStart

 * sampleRate
 * framesize_in_samples

 */
using namespace juce;

class Event {

public:

    int type {0};

    double qpm {-1};
    double numerator {-1};
    double denominator {-1};

    bool isPlaying {false};
    bool isRecording {false};

    int64_t time_in_samples {-1};
    double time_in_seconds {-1};
    double time_in_ppq {-1};

    bool isLooping {false};
    double loop_start_in_ppq {-1};
    double loop_end_in_ppq {-1};

    int64_t bar_count {-1};
    double ppq_position_of_last_bar_start {-1};

    double sample_rate {-1};
    int64_t buffer_size_in_samples {-1};

    juce::MidiMessage message {};

    Event() = default;

    Event(juce::Optional<juce::AudioPlayHead::PositionInfo> Pinfo, double sample_rate_, int64_t buffer_size_in_samples_, bool isFirstFrame) {
        type = isFirstFrame ? 1 : 2;

        if (Pinfo){
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

    Event(juce::Optional<juce::AudioPlayHead::PositionInfo> Pinfo, double sample_rate_,
          int64_t buffer_size_in_samples_, juce::MidiMessage message_) : message(message_) {
        type = 3;

        if (Pinfo){
            qpm = Pinfo->getBpm() ? *Pinfo->getBpm() : -1;
            auto ts = Pinfo->getTimeSignature();
            numerator = ts ? ts->numerator : -1;
            denominator = ts ? ts->denominator : -1;
            isPlaying = Pinfo->getIsPlaying();
            isRecording = Pinfo->getIsRecording();

            auto frame_start_time_in_samples = Pinfo->getTimeInSamples() ? *Pinfo->getTimeInSamples() : -1;
            auto message_start_in_samples = Pinfo->getTimeInSamples() ? message_.getTimeStamp() : -1;
            time_in_samples = frame_start_time_in_samples + message_start_in_samples;

            auto frame_start_time_in_seconds = Pinfo->getTimeInSeconds() ? *Pinfo->getTimeInSeconds() : -1;
            auto message_start_in_seconds = Pinfo->getTimeInSeconds() ? n_samples_to_sec(
                    message_start_in_samples, sample_rate_) : -1;
            time_in_seconds = frame_start_time_in_seconds + message_start_in_seconds;

            auto frame_start_time_in_ppq = Pinfo->getPpqPosition() ? *Pinfo->getPpqPosition() : -1;
            auto message_start_in_ppq = Pinfo->getPpqPosition() ? n_samples_to_ppq(
                    message_start_in_samples, sample_rate_, qpm) : -1;
            time_in_ppq = frame_start_time_in_ppq + message_start_in_ppq;

            isLooping = Pinfo->getIsLooping();
            auto loopPoints = Pinfo->getLoopPoints();
            loop_start_in_ppq = loopPoints ? loopPoints->ppqStart : -1;
            loop_end_in_ppq = loopPoints ? loopPoints->ppqEnd : -1;
            bar_count = Pinfo->getBarCount() ? *Pinfo->getBarCount() : -1;
            ppq_position_of_last_bar_start = Pinfo->getPpqPositionOfLastBarStart() ? *Pinfo->getPpqPositionOfLastBarStart() : -1;
            sample_rate = sample_rate_;
            buffer_size_in_samples = buffer_size_in_samples_;
        }
    }

    double n_samples_to_ppq(double audioSamplePos, double qpm, double sample_rate) {
        auto tmp_ppq = audioSamplePos * qpm / (60 * sample_rate);
        return tmp_ppq;
    }

    double n_samples_to_sec(double audioSamplePos, double sample_rate) {
        auto tmp_sec = audioSamplePos / sample_rate;
        return tmp_sec;
    }

    std::stringstream getDescription() {
        std::stringstream ss;
        ss << "-----------------------------------------------" << std::endl;
        switch (type) {
            case 1:
                ss << "First Frame Metadata Event" << std::endl;
                break;
            case 2:
                ss << "Next Frame Metadata Event" << std::endl;
                break;
            case 3:
                ss << "Midi Within Buffer Event" << std::endl;
                break;
            default:
                ss << "Unknown Event" << std::endl;
                break;
        }

        ss << "qpm: " << qpm << ", ts numerator: " << numerator << ", ts denominator: " << denominator << std::endl;
        ss << "isPlaying: " << isPlaying << ", isRecording: " << isRecording << std::endl;
        ss << "time_in_samples: " << time_in_samples << ", time_in_seconds: " << time_in_seconds << ", time_in_ppq: " << time_in_ppq << std::endl;
        ss << "isLooping: " << isLooping << ", loop_start_in_ppq: " << loop_start_in_ppq << ", loop_end_in_ppq: " << loop_end_in_ppq << std::endl;
        ss << "bar_count: " << bar_count << ", ppq_position_of_last_bar_start: " << ppq_position_of_last_bar_start << std::endl;
        ss << "sample_rate: " << sample_rate << ", buffer_size_in_samples: " << buffer_size_in_samples << std::endl;

        if (type == 3) {
            ss << "message: " << message.getDescription() << std::endl;
        }

        return ss;
    }

    // ==================================================
    // Operators for sorting using time stamp
    // ==================================================

    bool operator < (const Event& e) const { return time_in_samples < e.time_in_samples; }

    bool operator <= (const Event& e) const { return time_in_samples <= e.time_in_samples; }

    bool operator > (const Event& e) const { return time_in_samples > e.time_in_samples; }

    bool operator >= (const Event& e) const { return time_in_samples >= e.time_in_samples; }

    bool operator == (const Event& e) const { return time_in_samples == e.time_in_samples; }

};