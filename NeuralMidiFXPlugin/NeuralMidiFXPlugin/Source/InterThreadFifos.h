/*
//
// Created by Behzad Haki on 2022-08-21.
//

#pragma once
#include "settings.h"
#include "Includes/CustomStructsAndLockFreeQueue.h"
*/
/*

// Lock-free Queues  for communicating data between main threads in the Processor
namespace IntraProcessorFifos
{
    // ========= processBlock() To GrooveThread =============================================
    // sends a received note from input to GrooveThread to update the input groove
    unique_ptr<LockFreeQueue<BasicNote, GeneralSettings::processor_io_queue_size>> ProcessBlockToGrooveThreadQue;
    // =================================================================================



    // ========= GrooveThread To Model Thread   =============================================
    unique_ptr<MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>> GrooveThreadToModelThreadQue;


    // ========= Model Thread  To ProcessBlock =============================================
    unique_ptr<GeneratedDataQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::processor_io_queue_size>> ModelThreadToProcessBlockQue;
}
*//*



*/
/**
 *  Lock-free Queues  for communicating data back and forth between
 *  the Processor threads and the GUI widgets
 *//*


namespace GuiIOFifos
{

    // todo -> modelThread to gui ques for notifying if model loaded and also loading other models from a file browser

    // ========= Processor To TextEditor =============================================
    struct ProcessorToTextEditorQues
    {
        // used to communicate with BasicNote logger (i.e. display received notes in a textEditor)
        LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size> notes {};
        // Used for showing text messages on a text editor (only use in a single thread!!)
        StringLockFreeQueue<GeneralSettings::gui_io_queue_size> texts {}; // used for debugging only!
    };
    // =================================================================================



    // =========      PianoRoll_InteractiveMonotonicGroove FIFOs  ===================

    // used for sending velocities and offset compression values from MonotonicGroove XYSlider
    // to GrooveThread in processor
    struct GrooveThread2GGroovePianoRollWidgetQues
    {
        // used for sending the scaled groove to  MonotonicGroove Widget to display
        //        // todo To integrate in code
    };


    */
/*struct GroovePianoRollWidget2GrooveThreadQues
    {
        // LockFreeQueue<array<float, 4>, GeneralSettings::gui_io_queue_size> newVelOffRanges {};                 // todo To integrate in code
        // used for sending a manually drawn note in the MonotonicGroove Widget to GrooveThread
        LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size> manually_drawn_notes {};          // todo To integrate in code
    };*//*

     //LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size> GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue
    // =================================================================================



    // =========         Generated Drums PianoRoll            ========================
    // used for receiving the latest perVoiceSamplingThresholds and maximum notes allowed from Generated Drums XYSliders
    struct ModelThreadToDrumPianoRollWidgetQues
    {
        //unique_ptr<HVOLightQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::gui_io_queue_size>> ModelThreadToDrumPianoRollWidgetQue {};        // todo To integrate in code
    };

    struct DrumPianoRollWidgetToModelThreadQues
    {
        // std::array<LockFreeQueue<float, GeneralSettings::gui_io_queue_size>, HVO_params::num_voices> new_sampling_thresholds {};
        // std::array<LockFreeQueue<float, GeneralSettings::gui_io_queue_size>, HVO_params::num_voices> new_max_number_voices {};
    };
    // =================================================================================

}
*/
