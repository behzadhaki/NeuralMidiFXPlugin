#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
//#include <vector>
#include <torch/torch.h>

#include "ProcessorThreads/InputTensorPreparatorThread.h"


// #include "gui/CustomGuiTextEditors.h"

using namespace std;


class NeuralMidiFXPluginProcessor : public PluginHelpers::ProcessorBase {

public:

    NeuralMidiFXPluginProcessor();

    ~NeuralMidiFXPluginProcessor() override;

    void processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;

    // InputTensorPreparator Queue
    unique_ptr<LockFreeQueue<Event, queue_settings::NMP2ITP_que_size>> NMP2ITP_Event_Que;


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
