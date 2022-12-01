//
// Created by Behzad Haki on 2022-08-05.
//

#ifndef JUCECMAKEREPO_GROOVETHREAD_H
#define JUCECMAKEREPO_GROOVETHREAD_H

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/CustomStructsAndLockFreeQueue.h"
#include "../InterThreadFifos.h"
#include "../settings.h"

class GrooveThread:public juce::Thread, public juce::ChangeBroadcaster
{
public:
    // ============================================================================================================
    // ===          Preparing Thread for Running
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 1 . Construct
    // ------------------------------------------------------------------------------------------------------------
    GrooveThread();
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(LockFreeQueue<BasicNote, GeneralSettings::processor_io_queue_size>* ProcessBlockToGrooveThreadQuePntr,
                                           MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>* GrooveThreadToModelThreadQuePntr,
                                           MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>* GrooveThread2GGroovePianoRollWidgetQuesPntr,
                                           LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* GroovePianoRollWidget2GrooveThread_manually_drawn_noteQuePntr,
                                           LockFreeQueue<std::array<float, 4>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_vel_offset_ranges_QuePntr,
                                           LockFreeQueue<std::array<int, 2>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_record_overdubToggles_QuePntr);
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 3 . start run() thread by calling startThread().
    // ---                  !!DO NOT!! Call run() directly. startThread() internally makes a call to run().
    // ---                  (Implement what the thread does inside the run() method
    // ------------------------------------------------------------------------------------------------------------
    void run() override;
    // ============================================================================================================


    // ============================================================================================================
    // ===          Preparing Thread for Stopping
    // ============================================================================================================
    void prepareToStop();     // run this in destructor destructing object
    ~GrooveThread() override;
    // ============================================================================================================


    // ============================================================================================================
    // ===          Utility Methods and Parameters
    // ============================================================================================================
    void ForceResetGroove();        // reset Groove if requested
    bool readyToStop; // Used to check if thread is ready to be stopped or externally stopped from a parent thread
    void clearStep(int grid_ix, float start_ppq);    // clears a time step ONLY IF OVERDUBBING IS OFF!!!

    void randomizeExistingVelocities();         // randomizes the velocities of the existing groove
    void randomizeExistingOffsets();            // randomizes the offsets of the existing groove
    void randomizeAll();                        // generates a fully random groove
    // ============================================================================================================


private:

    // ============================================================================================================
    // ===          I/O Queues for Receiving/Sending Data
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---          Output Queues
    // ------------------------------------------------------------------------------------------------------------
    MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>* GrooveThread2GGroovePianoRollWidgetQue;
    MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>* GrooveThreadToModelThreadQue;
    // ------------------------------------------------------------------------------------------------------------
    // ---          Input Queues
    // ------------------------------------------------------------------------------------------------------------
    LockFreeQueue<BasicNote, GeneralSettings::processor_io_queue_size>* ProcessBlockToGrooveThreadQue;
    LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue;
    LockFreeQueue<std::array<float, 4>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_vel_offset_ranges_Que;
    LockFreeQueue<std::array<int, 2>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_record_overdubToggles_Que;

    // ============================================================================================================


    // ============================================================================================================
    // ===          Parameters Locally Used for Calculations
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---          Velocity and Offset Ranges for Groove Manipulation / Compression
    // ------------------------------------------------------------------------------------------------------------
    array<float, 2> vel_range;
    array<float, 2> offset_range;
    // ------------------------------------------------------------------------------------------------------------
    // ---          Groove Parameters
    // ------------------------------------------------------------------------------------------------------------
    MonotonicGroove<HVO_params::time_steps> monotonic_groove;
    bool shouldResetGroove {false};
    int overdubEnabled {1};
    int recordEnabled {1};
    // parameters for clearing a step in case overdubbing is off
    int clearStepNumber {0};
    float clearRequestedAtPositionPpq {0};



    bool shouldRandomizeVelocities {false};
    bool shouldRandomizeOffsets {false};
    bool shouldRandomizeAll {false};
    // ============================================================================================================

};

#endif //JUCECMAKEREPO_GROOVETHREAD_H
