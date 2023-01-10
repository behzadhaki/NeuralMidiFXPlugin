#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
//#include <vector>
#include <torch/torch.h>
#include "model_settings.h"
#include "ProcessorThreads/InputTensorPreparatorThread.h"

// #include "gui/CustomGuiTextEditors.h"

using namespace std;


class NeuralMidiFXPluginProcessor : public PluginHelpers::ProcessorBase {

public:

    NeuralMidiFXPluginProcessor();

    ~NeuralMidiFXPluginProcessor() override;

    void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;

    //Queues
    unique_ptr<LockFreeQueue<Event, queue_settings::NMP2ITP_que_size>> NMP2ITP_Event_Que;
    unique_ptr<LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size>> ITP2MDL_ModelInput_Que;
    unique_ptr<LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size>> MDL2PPP_ModelOutput_Que;
    unique_ptr<LockFreeQueue<Event, queue_settings::PPP2NMP_que_size>> PPP2NMP_Event_Que;

    // Threads used for generating patterns in the background
    shared_ptr<InputTensorPreparatorThread> inputTensorPreparatorThread;

    // APVTS
    juce::AudioProcessorValueTreeState apvts;

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
    Event last_frame_meta_data;
    std::optional<Event> NewBarEvent;
    std::optional<Event> NewTimeShiftEvent;

    //
    void sendReceivedInputsAsEvents(
            MidiBuffer &midiMessages, const Optional<AudioPlayHead::PositionInfo> &Pinfo,
            double fs,
            int buffSize);

};
