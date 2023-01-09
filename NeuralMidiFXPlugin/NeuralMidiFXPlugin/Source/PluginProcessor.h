#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
//#include <vector>
#include <torch/torch.h>

#include "ProcessorThreads/InputTensorPreparatorThread.h"


// #include "gui/CustomGuiTextEditors.h"

using namespace std;


class NeuralMidiFXPluginProcessor : public PluginHelpers::ProcessorBase
{
public:

    NeuralMidiFXPluginProcessor();

    ~NeuralMidiFXPluginProcessor() override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    // InputTensorPreparator Queue
    unique_ptr<LockFreeQueue<Event, 2048>> NMP2ITP_Event_Que;


    // Threads used for generating patterns in the background
    shared_ptr<InputTensorPreparatorThread> inputTensorPreparatorThread;

    // getters
    float get_playhead_pos();

    // APVTS
    juce::AudioProcessorValueTreeState apvts;


    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // model paths
    // juce::StringArray model_paths{get_pt_files_in_default_path()};


private:


    // =========  Queues for communicating Between the main threads in proce    ssor  =============================================

    // holds the playhead position for displaying on GUI
    float playhead_pos;

    // holds the previous start ppq to check restartedFlag status
    double startPpq {0};
    double current_grid {-1};

    //  midiBuffer to fill up with generated data
    juce::MidiBuffer tempBuffer;

    // Parameter Layout for apvts
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // last ppq
    Event last_frame_meta_data;
    std::optional<Event> NewBarEvent;
    std::optional<Event> NewTimeShiftEvent;

    void prepareEventWithPlayheadInfo();

    void sendReceivedInputsAsEvents(MidiBuffer &midiMessages, const Optional<AudioPlayHead::PositionInfo> &Pinfo, double fs, int buffSize);
};
