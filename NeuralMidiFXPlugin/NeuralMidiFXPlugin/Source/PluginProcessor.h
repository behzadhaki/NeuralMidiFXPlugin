#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
//#include <vector>
#include "InterThreadFifos.h"
#include <torch/torch.h>

#include "ProcessorThreads/GrooveThread.h"
#include "ProcessorThreads/ModelThread.h"
#include "ProcessorThreads/APVTSMediatorThread.h"

// #include "gui/CustomGuiTextEditors.h"

using namespace std;


class MidiFXProcessor : public PluginHelpers::ProcessorBase
{
public:

    MidiFXProcessor();

    ~MidiFXProcessor() override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;

    // grooveThread i.o queues w/ GroovePianoRollWidget
    unique_ptr<MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>> GrooveThread2GGroovePianoRollWidgetQue;

    // modelThread i.o queues w/ DrumPianoRoll Widget
    unique_ptr<HVOLightQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::gui_io_queue_size>> ModelThreadToDrumPianoRollWidgetQue;

    // GUI THREADS HOSTED IN PROCESSOR

    // Threads used for generating patterns in the background
    shared_ptr<GrooveThread> grooveThread;
    shared_ptr<ModelThread> modelThread;
    unique_ptr<APVTSMediatorThread> apvtsMediatorThread;

    // getters
    float get_playhead_pos();

    // APVTS
    juce::AudioProcessorValueTreeState apvts;

    LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* get_pointer_GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue();

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // model paths
    juce::StringArray model_paths{get_pt_files_in_default_path()};

private:
    // =========  Queues for communicating Between the main threads in proce    ssor  =============================================
    shared_ptr<LockFreeQueue<BasicNote, GeneralSettings::processor_io_queue_size>> ProcessBlockToGrooveThreadQue;
    shared_ptr<MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>> GrooveThreadToModelThreadQue;
    shared_ptr<GeneratedDataQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::processor_io_queue_size>> ModelThreadToProcessBlockQue;
    shared_ptr<LockFreeQueue<std::array<float, 4>, GeneralSettings::gui_io_queue_size>> APVTS2GrooveThread_groove_vel_offset_ranges_Que;
    shared_ptr<LockFreeQueue<std::array<int, 2>, GeneralSettings::gui_io_queue_size>> APVTS2GrooveThread_groove_record_overdubToggles_Que;
    shared_ptr<LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>> APVTS2ModelThread_max_num_hits_Que;
    shared_ptr<LockFreeQueue<std::array<float, HVO_params::num_voices+1>, GeneralSettings::gui_io_queue_size>> APVTS2ModelThread_sampling_thresholds_and_temperature_Que;
    shared_ptr<LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>> GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue;
    shared_ptr<LockFreeQueue<std::array<int, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>> APVTS2ModelThread_midi_mappings_Que;

    // holds the latest generations to loop over
    GeneratedData<HVO_params::time_steps, HVO_params::num_voices> latestGeneratedData;

    // holds the playhead position for displaying on GUI
    float playhead_pos;

    // holds the previous start ppq to check restartedFlag status
    double startPpq {0};
    double current_grid {-1};

    //  midiBuffer to fill up with generated data
    juce::MidiBuffer tempBuffer;

    // Parameter Layout for apvts
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();




};
