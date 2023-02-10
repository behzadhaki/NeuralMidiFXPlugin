#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
//#include <vector>
#include <torch/torch.h>
#include "DeploymentSettings/Model.h"
#include "DeploymentThreads/InputTensorPreparatorThread.h"
#include "DeploymentThreads/ModelThread.h"
#include "DeploymentThreads/PlaybackPreparatorThread.h"
#include "Includes/APVTSMediatorThread.h"
#include "Includes/LockFreeQueue.h"
#include "Includes/GenerationEvent.h"
#include "Includes/APVTSMediatorThread.h"
#include <chrono>

// #include "gui/CustomGuiTextEditors.h"

using namespace std;


class NeuralMidiFXPluginProcessor : public PluginHelpers::ProcessorBase {

public:

    NeuralMidiFXPluginProcessor();

    ~NeuralMidiFXPluginProcessor() override;

    void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;

    // Queues
    unique_ptr<LockFreeQueue<Event, queue_settings::NMP2ITP_que_size>> NMP2ITP_Event_Que;
    unique_ptr<LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size>> ITP2MDL_ModelInput_Que;
    unique_ptr<LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size>> MDL2PPP_ModelOutput_Que;
    unique_ptr<LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size>> PPP2NMP_GenerationEvent_Que;

    // APVTS Queues
    unique_ptr<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>> APVM2ITP_GuiParams_Que;
    unique_ptr<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>> APVM2MDL_GuiParams_Que;
    unique_ptr<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>> APVM2PPP_GuiParams_Que;

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

private:
    // =========  Queues for communicating Between the main threads in processor  ===============

    // holds the playhead position for displaying on GUI
    float playhead_pos{-1.0f};

    //  midiBuffer to fill up with generated data
    juce::MidiBuffer tempBuffer;

    // Parameter Layout for apvts
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Event Place Holders for cross buffer events
    Event last_frame_meta_data{};
    std::optional<Event> NewBarEvent;
    std::optional<Event> NewTimeShiftEvent;

    // Gets DAW info and midi messages,
    // Wraps messages as Events
    // then sends them to the InputTensorPreparatorThread via the NMP2ITP_Event_Que
    void sendReceivedInputsAsEvents(
            MidiBuffer &midiMessages, const Optional<AudioPlayHead::PositionInfo> &Pinfo,
            double fs,
            int buffSize);

    // Playback Data
    juce::MidiMessageSequence playbackSequence{};  // holds messages to play
    std::optional<GenerationPlaybackPolicies> playbackPolicies{std::nullopt};
    BufferMetaData phead_at_start_of_new_stream{};

    chrono::system_clock::time_point now;

    // utility methods
    void PrintMessage(const std::string& input);
};
