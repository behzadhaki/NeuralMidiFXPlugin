#include "Source/DeploymentThreads/PlaybackPreparatorThread.h"

// ===================================================================================
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                       YOUR IMPLEMENTATION SHOULD BE DONE HERE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// ===================================================================================

std::pair<bool, bool> PlaybackPreparatorThread::deploy(bool new_model_output_received, bool did_any_gui_params_change) {

    // =================================================================================
    // ===         1. ACCESSING GUI PARAMETERS
    // =================================================================================

    /* **NOTE**
        If you need to access information from the GUI, you can do so by using the
        following methods:

          Rotary/Sliders: gui_params.getValueFor([slider/button name])
          Toggle Buttons: gui_params.isToggleButtonOn([button name])
          Trigger Buttons: gui_params.wasButtonClicked([button name])
    **NOTE**
       If you only need this data when the GUI parameters CHANGE, you can use the
          provided gui_params_changed_since_last_call flag */

    // ---------------------------------------------------------------------------------
    // --- ExampleStarts ------ ExampleStarts ------ ExampleStarts ---- ExampleStarts --
    // in this example, whenever the slider is moved, I resend a sequence of generations
    // to the processor thread (NMP)
    bool testFlag{false};
    auto newPlaybackDelaySlider = gui_params.getValueFor("Generation Playback Delay");
    if (PlaybackDelaySlider != newPlaybackDelaySlider) {
        PrintMessage("PlaybackPreparatorThread: PlaybackDelaySlider changed from" +
        std::to_string(PlaybackDelaySlider) + " to " + std::to_string(newPlaybackDelaySlider));
        PlaybackDelaySlider = newPlaybackDelaySlider;
        testFlag = true;
    }
    // --- ExampleEnds -------- ExampleEnds -------- ExampleEnds ------ ExampleEnds ----
    // ---------------------------------------------------------------------------------

    // =================================================================================
    // ===         1.b. ACCESSING REALTIME PLAYBACK INFORMATION
    // =================================================================================
    auto realtime_playback_info = realtimePlaybackInfo->get();
    auto sample_rate = realtime_playback_info.sample_rate;
    auto buffer_size_in_samples = realtime_playback_info.buffer_size_in_samples;
    auto qpm = realtime_playback_info.qpm;
    auto numerator = realtime_playback_info.numerator;
    auto denominator = realtime_playback_info.denominator;
    auto isPlaying = realtime_playback_info.isPlaying;
    auto isRecording = realtime_playback_info.isRecording;
    auto current_time_in_samples = realtime_playback_info.time_in_samples;
    auto current_time_in_seconds = realtime_playback_info.time_in_seconds;
    auto current_time_in_quarterNotes = realtime_playback_info.time_in_ppq;
    auto isLooping = realtime_playback_info.isLooping;
    auto loopStart_in_quarterNotes = realtime_playback_info.loop_start_in_ppq;
    auto loopEnd_in_quarterNotes = realtime_playback_info.loop_end_in_ppq;
    auto last_bar_pos_in_quarterNotes = realtime_playback_info.ppq_position_of_last_bar_start;
    // PrintMessage("qpm: " + std::to_string(qpm));
    // =================================================================================

    // =================================================================================
    // ===         Step 2 . Extract NoteOn/Off/CC events from the generation data
    // =================================================================================

    // ---------------------------------------------------------------------------------
    // --- ExampleStarts ------ ExampleStarts ------ ExampleStarts ---- ExampleStarts --
    auto onsets = model_output.tensor1; // I'm not using this tensor below, just an e.g.

    // ...
    // --- ExampleEnds -------- ExampleEnds -------- ExampleEnds ------ ExampleEnds ----
    // ---------------------------------------------------------------------------------

    // =================================================================================
    // ===         Step 3 . At least once, before sending generations,
    //                  Specify the PlaybackPolicy, Time_unit, OverwritePolicy
    // =================================================================================

    /*
     * Here you Notify the main NMP thread that a new stream of generations is available.
     * Moreover, you also specify what time_unit is used for the generations (eg. sec, ppq, samples)
     * && also specify the OverwritePolicy (whether to delete all previously generated events
     * || to keep them while also adding the new ones in parallel, || only clear the events after
     * the time at which the current event is received)

     *
     * PlaybackPolicy:
     *   - Timing Specification:
     *      1. relative to Now (register the current time, && play messages relative to that)
     *      2. relative to 0 (stream should be played relative to absolute 0 time)
     *      3. relative to playback start (stream should be played relative to the time when playback started)
     *
     *   - Time_unit:
     *      1. seconds
     *      2. ppq
     *      3. audio samples
     *
     *   - Overwrite Policy:
     *      1. delete all events in previous stream && use new stream
     *      2. delete all events after now
     *      3. Keep all previous events (that is generations can be played on top of each other)
     *
     *    - Additional Info:
     *      1. Clear generations after pause/stop
     *      2. Loop Generations indefinitely until new one received (loop start at Now, 0, or playback start)
     *
     */

    // -----------------------------------------------------------------------------------------
    // ------ ExampleStarts ------ ExampleStarts ------ ExampleStarts ------ ExampleStarts -----
    bool newPlaybackPolicyShouldBeSent{false};
    bool newPlaybackSequenceGeneratedAndShouldBeSent{false};

    if (testFlag) {
        // 1. ---- Update Playback Policy -----
        // Can be sent just once, || every time the policy changes
        // playbackPolicy.SetPaybackPolicy_RelativeToNow();  // or
        playbackPolicy.SetPlaybackPolicy_RelativeToAbsoluteZero(); // or
        // playbackPolicy.SetPlaybackPolicy_RelativeToPlaybackStart(); // or

        // playbackPolicy.SetTimeUnitIsSeconds(); // or
        playbackPolicy.SetTimeUnitIsPPQ(); // or
        // playbackPolicy.SetTimeUnitIsAudioSamples(); // or FIXME Timestamps near zero don't work well in loop mode

        bool forceSendNoteOffsFirst{true};
        // playbackPolicy.SetOverwritePolicy_DeleteAllEventsInPreviousStreamAndUseNewStream(forceSendNoteOffsFirst); // or
        playbackPolicy.SetOverwritePolicy_DeleteAllEventsAfterNow(forceSendNoteOffsFirst); // or
        // playbackPolicy.SetOverwritePolicy_KeepAllPreviousEvents(forceSendNoteOffsFirst); // or

        // playbackPolicy.SetClearGenerationsAfterPauseStop(false); //
         playbackPolicy.ActivateLooping(4);
//        playbackPolicy.DisableLooping();
        newPlaybackPolicyShouldBeSent = true;

        // 2. ---- Update Playback Sequence -----
        // clear the previous sequence || append depending on your requirements
        playbackSequence.clear();

        // I'm generating a single note to be played at timestamp 4 && delayed by the slider value
        int channel{1};
        int note = rand() % 64;
        float velocity{0.3f};
        double timestamp{0};
        double duration{1};

        // add noteOn Offs (time stamp shifted by the slider value)
        playbackSequence.addNoteOn(channel, note, velocity,
                                   timestamp + newPlaybackDelaySlider);
        playbackSequence.addNoteOff(channel, note, velocity,
                                    timestamp + newPlaybackDelaySlider + duration);
        // or add a note with duration (an octave higher)
        playbackSequence.addNoteWithDuration(channel, note+12, velocity,
                                             timestamp + newPlaybackDelaySlider, duration);

        newPlaybackSequenceGeneratedAndShouldBeSent = true;
    }
    // --- ExampleEnds -------- ExampleEnds -------- ExampleEnds ------ ExampleEnds ----
    // ---------------------------------------------------------------------------------

    // MUST Notify What Data Ready to be Sent
    // If you don't want to send anything, just set both flags to false
    // If you have a new playback policy, set the first flag to true
    // If you have a new playback sequence, set the second flag to true
    return {newPlaybackPolicyShouldBeSent, newPlaybackSequenceGeneratedAndShouldBeSent};
}