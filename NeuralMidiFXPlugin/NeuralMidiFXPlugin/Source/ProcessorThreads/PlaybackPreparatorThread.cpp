//
// Created by Behzad Haki on 2022-12-13.
//

#include "PlaybackPreparatorThread.h"


PlaybackPreparatorThread::PlaybackPreparatorThread() : juce::Thread("PlaybackPreparatorThread") {
}

void PlaybackPreparatorThread::startThreadUsingProvidedResources(
        LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size> *MDL2PPP_ModelOutput_Que_ptr_,
        LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size> *PPP2NMP_GenerationEvent_Que_ptr_) {

    // Provide access to resources needed to communicate with other threads
    // ---------------------------------------------------------------------------------------------
    MDL2PPP_ModelOutput_Que_ptr = MDL2PPP_ModelOutput_Que_ptr_;
    PPP2NMP_GenerationEvent_Que_ptr = PPP2NMP_GenerationEvent_Que_ptr_;


    // Start the thread. This function internally calls run() method. DO NOT CALL run() DIRECTLY.
    // ---------------------------------------------------------------------------------------------
    startThread();
}

void PlaybackPreparatorThread::run() {
    // notify if the thread is still running
    bool bExit = threadShouldExit();

    int cnt{0};

    while (!bExit) {

        if (readyToStop) { break; } // check if thread is ready to be stopped

        // Check if new events are available in the queue
        while (MDL2PPP_ModelOutput_Que_ptr->getNumReady() > 0) {
            // =================================================================================
            // ===         Step 1 . Get New Generations if Any
            // =================================================================================
            auto ModelOutput = MDL2PPP_ModelOutput_Que_ptr->pop(); // get the earliest queued generation data
            // auto ModelOutput = MDL2PPP_ModelOutput_Que_ptr->getLatestOnly(); // or get the last generation data


            // =================================================================================
            // ===         Step 2 . Extract NoteOn/Off/CC events from the generation data
            // =================================================================================

            // ---------------------------------------------------------------------------------
            // --- ExampleStarts ------ ExampleStarts ------ ExampleStarts ---- ExampleStarts --
            /*DBG("PlaybackPreparatorThread: ExampleStarts");
            auto new_sequence = juce::MidiMessageSequence();
            DBG("PlaybackPreparatorThread: ExampleStarts: new_sequence created");
            auto onsets = ModelOutput.tensor1 > 0.5;
            DBG("PlaybackPreparatorThread: ExampleStarts: onsets created");

            // adding some random messages
            double start_ppq = cnt * 4.5;
            double end_ppq = start_ppq + 0.1 * (cnt+1);
            int pitch = int(60 / (cnt + 1));
            auto vel = uint8(127 / (cnt + 1));
            auto msg = juce::MidiMessage::noteOn(1, pitch, vel);
            // DBG("start_ppq: " << start_ppq << " end_ppq: " << end_ppq << " pitch: " << pitch << " vel: " << vel);
            new_sequence.addEvent(
                    juce::MidiMessage::noteOn(1, pitch, vel), start_ppq);
            DBG("PlaybackPreparatorThread: ExampleStarts: new_sequence.addEvent");
            new_sequence.addEvent(
                    juce::MidiMessage::noteOff(1, pitch, vel), end_ppq);
            // DBG("Added Note Off at " << end_ppq);
            DBG("PlaybackPreparatorThread: ExampleStarts: new_sequence.addEvent");
            new_sequence.updateMatchedPairs(); // don't forget this ???????????? is it actually needed?
            DBG("PlaybackPreparatorThread: ExampleStarts: new_sequence.addEvent");*/

            // --- ExampleEnds -------- ExampleEnds -------- ExampleEnds ------ ExampleEnds ----
            // ---------------------------------------------------------------------------------

            // =================================================================================
            // ===         Step 3 . At least once, before sending generations,
            //                  Specify the PlaybackPolicy, Time_unit, OverwritePolicy
            // =================================================================================

            /*
             * Here you Notify the main NMP thread that a new stream of generations is available.
             * Moreover, you also specify what time_unit is used for the generations (eg. sec, ppq, samples)
             * and also specify the OverwritePolicy (whether to delete all previously generated events
             * or to keep them while also adding the new ones in parallel, or only clear the events after
             * the time at which the current event is received)

             *
             * PlaybackPolicy:
             *      1. relative to Now (register the current time, and play messages relative to that)
             *      2. relative to 0 (stream should be played relative to absolute 0 time)
             *      3. relative to playback start (stream should be played relative to the time when playback started)
             *
             * Time_unit:
             *   1. seconds
             *   2. ppq
             *   3. audio samples
             *
             * Overwrite Policy:
             *  1. delete all events in previous stream and use new stream
             *  2. delete all events after now
             *  3. Keep all previous events (that is generations can be played on top of each other)
             */

            // -----------------------------------------------------------------------------------------
            // ------ ExampleStarts ------ ExampleStarts ------ ExampleStarts ------ ExampleStarts -----
            if (cnt % 2 == 0) {

                GenerationPlaybackPolicies newStreamEvent;
                newStreamEvent.SetPaybackPolicy_RelativeToNow();  // or
                // playbackPolicies.SetPlaybackPolicy_RelativeToAbsoluteZero(); // or
                // playbackPolicies.SetPplaybackPolicy_RelativeToPlaybackStart(); // or

                newStreamEvent.SetTimeUnitIsSeconds(); // or
                // playbackPolicies.SetTimeUnitIsPPQ(); // or
                // playbackPolicies.SetTimeUnitIsAudioSamples(); // or

                newStreamEvent.SetOverwritePolicy_DeleteAllEventsInPreviousStreamAndUseNewStream(); // or
                // playbackPolicies.SetOverwritePolicy_DeleteAllEventsAfterNow(); // or
                // playbackPolicies.SetOverwritePolicy_KeepAllPreviousEvents(); // or

                // send to the the main thread (NMP)
                if (newStreamEvent.IsReadyForTransmission()) {
                    auto genEvent = GenerationEvent(newStreamEvent);
                    PPP2NMP_GenerationEvent_Que_ptr->push(genEvent);
                }
            }

            // -------- ExampleEnds -------- ExampleEnds -------- ExampleEnds -------- ExampleEnds -----
            // -----------------------------------------------------------------------------------------

            // =================================================================================
            // ===         Step 4 . Send the new generations to the main NMP thread
            // =================================================================================
            // -----------------------------------------------------------------------------------------
            // ------ ExampleStarts ------ ExampleStarts ------ ExampleStarts ------ ExampleStarts -----

            // -------- ExampleEnds -------- ExampleEnds -------- ExampleEnds -------- ExampleEnds -----
            // -----------------------------------------------------------------------------------------

            cnt++;

        }

        // ============================================================================================================

        // check if thread is still running
        bExit = threadShouldExit();
    }

    // wait for a few ms to avoid burning the CPU
    sleep(thread_configurations::PlaybackPreparator::waitTimeBtnIters);
}

void PlaybackPreparatorThread::prepareToStop() {
    // Need to wait enough to ensure the run() method is over before killing thread
    this->stopThread(100 * thread_configurations::PlaybackPreparator::waitTimeBtnIters);

    readyToStop = true;
}

PlaybackPreparatorThread::~PlaybackPreparatorThread() {
    if (not readyToStop) {
        prepareToStop();
    }
}