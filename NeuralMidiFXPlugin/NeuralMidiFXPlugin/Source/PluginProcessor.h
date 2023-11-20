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
