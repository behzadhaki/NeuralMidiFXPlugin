#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
//#include <vector>
#include <torch/torch.h>
#include "DeploymentThreads/Configs_Model.h"
#include "DeploymentThreads/InputTensorPreparatorThread.h"
#include "DeploymentThreads/ModelThread.h"
#include "DeploymentThreads/PlaybackPreparatorThread.h"
#include "Includes/APVTSMediatorThread.h"
#include "Includes/LockFreeQueue.h"
#include "Includes/GenerationEvent.h"
#include "Includes/APVTSMediatorThread.h"
#include <chrono>
#include <mutex>

// #include "gui/CustomGuiTextEditors.h"

using namespace std;

struct GenerationsToDisplay {
public:
    double fs {44100};
    double qpm {-1};
    double playhead_pos {0};
    PlaybackPolicies policy;
    juce::MidiMessageSequence sequence_to_display;
    std::mutex mutex;

    void setSequence(const juce::MidiMessageSequence& sequence) {
        std::lock_guard<std::mutex> lock(mutex);
        sequence_to_display = sequence;
    }

    void setFs(double fs_) {
        std::lock_guard<std::mutex> lock(mutex);
        fs = fs_;
    }

    void setQpm(double qpm_) {
        std::lock_guard<std::mutex> lock(mutex);
        qpm = qpm_;
    }

    void setPlayheadPos(double playhead_pos_) {
        std::lock_guard<std::mutex> lock(mutex);
        playhead_pos = playhead_pos_;
    }

    void setPolicy(PlaybackPolicies policy_) {
        std::lock_guard<std::mutex> lock(mutex);
        policy = policy_;
    }

    std::optional<juce::MidiMessageSequence> getSequence() {
        std::lock_guard<std::mutex> lock(mutex);
        juce::MidiMessageSequence sequence_to_display_copy{sequence_to_display};
        sequence_to_display.clear();
        if (sequence_to_display_copy.getNumEvents() > 0) {
            return sequence_to_display_copy;
        } else {
            return std::nullopt;
        }
    }

    std::optional<double> getFs() {
        std::lock_guard<std::mutex> lock(mutex);
        return fs;
    }

    std::optional<double> getQpm() {
        std::lock_guard<std::mutex> lock(mutex);
        return qpm;
    }

    std::optional<double> getPlayheadPos() {
        std::lock_guard<std::mutex> lock(mutex);
        return playhead_pos;
    }

    std::optional<PlaybackPolicies> getPolicy() {
        std::lock_guard<std::mutex> lock(mutex);
        return policy;
    }

};

struct StandAloneParams {
    float qpm{-1};
    int is_playing{0};
    int is_recording{0};
    int denominator{4};
    int numerator{4};

    int64_t TimeInSamples{0};
    double TimeInSeconds{0};
    double PpqPosition{0};

    juce::AudioProcessorValueTreeState* apvts;


    explicit StandAloneParams(juce::AudioProcessorValueTreeState* apvtsPntr) {
        apvts = apvtsPntr;
        qpm = *apvts->getRawParameterValue(label2ParamID("TempoStandalone"));
        is_playing = int(*apvts->getRawParameterValue(label2ParamID("IsPlayingStandalone")));
        is_recording = int(*apvts->getRawParameterValue(label2ParamID("IsRecordingStandalone")));
        denominator = int(*apvts->getRawParameterValue(label2ParamID("TimeSigDenominatorStandalone")));
        numerator = int(*apvts->getRawParameterValue(label2ParamID("TimeSigNumeratorStandalone")));
    }

    // call update at the beginning of each processBlock
    bool update() {
        bool changed = false;

        float new_qpm = *apvts->getRawParameterValue(label2ParamID("TempoStandalone"));
        if (new_qpm != qpm) {
            qpm = new_qpm;
            changed = true;
        }
        int new_is_playing = int(*apvts->getRawParameterValue(label2ParamID("IsPlayingStandalone")));
        if (new_is_playing != is_playing) {
            is_playing = new_is_playing;
            changed = true;
        }
        int new_is_recording = int(*apvts->getRawParameterValue(label2ParamID("IsRecordingStandalone")));
        if (new_is_recording != is_recording) {
            is_recording = new_is_recording;
            changed = true;
        }
        int new_denominator = int(*apvts->getRawParameterValue(label2ParamID("TimeSigDenominatorStandalone")));
        if (new_denominator != denominator) {
            denominator = new_denominator;
            changed = true;
        }
        int new_numerator = int(*apvts->getRawParameterValue(label2ParamID("TimeSigNumeratorStandalone")));
        if (new_numerator != numerator) {
            numerator = new_numerator;
            changed = true;
        }
        return changed;
    }

    // call this after setting the PositionInfo data
    void PreparePlayheadForNextFrame(int64_t buffer_size, double fs) {
        if (is_playing) {
            TimeInSamples += buffer_size;
            TimeInSeconds += buffer_size / fs;
            PpqPosition += buffer_size / fs * qpm / 60;
        } else {
            TimeInSamples = 0;
            TimeInSeconds = 0;
            PpqPosition = 0;
        }
    }
};

class NeuralMidiFXPluginProcessor : public PluginHelpers::ProcessorBase {

public:

    NeuralMidiFXPluginProcessor();

    ~NeuralMidiFXPluginProcessor() override;

    void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;

    // Queues
    unique_ptr<LockFreeQueue<EventFromHost, queue_settings::NMP2ITP_que_size>> NMP2ITP_Event_Que;
    unique_ptr<LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size>> ITP2MDL_ModelInput_Que;
    unique_ptr<LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size>> MDL2PPP_ModelOutput_Que;
    unique_ptr<LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size>> PPP2NMP_GenerationEvent_Que;
    unique_ptr<LockFreeQueue<juce::MidiMessageSequence, 32>> NMP2GUI_IncomingMessageSequence;

    // APVTS Queues
    unique_ptr<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>> APVM2ITP_GuiParams_Que;
    unique_ptr<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>> APVM2MDL_GuiParams_Que;
    unique_ptr<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>> APVM2PPP_GuiParams_Que;

    // Drag/Drop Midi Queues
    unique_ptr<LockFreeQueue<juce::MidiFile, 4>> GUI2ITP_DroppedMidiFile_Que;
    unique_ptr<LockFreeQueue<juce::MidiFile, 4>> PPP2GUI_GenerationMidiFile_Que;

    // Threads used for generating patterns in the background
    shared_ptr<InputTensorPreparatorThread> inputTensorPreparatorThread;
    shared_ptr<ModelThread> modelThread;
    shared_ptr<PlaybackPreparatorThread> playbackPreparatorThread;


    // APVTS
    juce::AudioProcessorValueTreeState apvts;
    unique_ptr<APVTSMediatorThread> apvtsMediatorThread;

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    // Getters
    float get_playhead_pos() const;

    // realtime playback info
    unique_ptr<RealTimePlaybackInfo> realtimePlaybackInfo{};

    // Playback Data
    PlaybackPolicies playbackPolicies{};
    juce::MidiMessageSequence playbackMessageSequence{};
    BufferMetaData phead_at_start_of_new_stream{};
    time_ time_anchor_for_playback{};

    // mutex protected structures for interacting with the GUI
    GenerationsToDisplay generationsToDisplay{};
    mutex playbckAnchorMutex;
    time_ TimeAnchor;
    bool shouldSendTimeAnchorToGUI{false};

    // standalone
    unique_ptr<StandAloneParams> standAloneParams;

private:
    // =========  Queues for communicating Between the main threads in processor  ===============

    // holds the playhead position for displaying on GUI
    float playhead_pos{-1.0f};
    time_ playhead_start_time{};
    std::optional<juce::MidiMessage> getMessageIfToBePlayed(
            time_ now_, const juce::MidiMessage &msg,
            int buffSize, double fs, double qpm);

    //  midiBuffer to fill up with generated data
    juce::MidiBuffer tempBuffer;

    // Parameter Layout for apvts
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // EventFromHost Place Holders for cross buffer events
    EventFromHost last_frame_meta_data{};
    std::optional<EventFromHost> NewBarEvent;
    std::optional<EventFromHost> NewTimeShiftEvent;
    juce::MidiMessageSequence incoming_messages_sequence;

    // Gets DAW info and midi messages,
    // Wraps messages as Events
    // then sends them to the InputTensorPreparatorThread via the NMP2ITP_Event_Que
    void sendReceivedInputsAsEvents(
            MidiBuffer &midiMessages, const Optional<AudioPlayHead::PositionInfo> &Pinfo,
            double fs,
            int buffSize);



    // utility methods
    void PrintMessage(const std::string& input);

    // MidiIO Standalone
    unique_ptr<MidiOutput> mVirtualMidiOutput;

};
