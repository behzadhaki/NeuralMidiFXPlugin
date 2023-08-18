#include "Source/DeploymentThreads/InputTensorPreparatorThread.h"

// ===================================================================================
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                       YOUR IMPLEMENTATION SHOULD BE DONE HERE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// ===================================================================================

bool InputTensorPreparatorThread::deploy(
    std::optional<MidiFileEvent> & new_midi_event_dragdrop,
    std::optional<EventFromHost> & new_event_from_host,
    bool gui_params_changed_since_last_call) {
    /*              IF YOU NEED TO PRINT TO CONSOLE FOR DEBUGGING,
     *                  YOU CAN USE THE FOLLOWING METHOD:
     *                      PrintMessage("YOUR MESSAGE HERE");
     */

    /* A flag like this one can be used to check whether || not the model input
        is ready to be sent to the model thread (MDL)*/
    bool SHOULD_SEND_TO_MODEL_FOR_GENERATION_ = false;

    // =================================================================================
    // ===         1.a. ACCESSING GUI PARAMETERS
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

    auto Slider1 = gui_params.getValueFor("Slider 1");
    auto ToggleButton1 = gui_params.isToggleButtonOn("ToggleButton 1");
    auto ButtonTrigger = gui_params.wasButtonClicked("TriggerButton 1");

    // Or get value only when a change occurs
    /*if (gui_params_changed_since_last_call) {
        auto Slider1 = gui_params.getValueFor("Slider 1");
        auto ToggleButton1 = gui_params.isToggleButtonOn("ToggleButton 1");
        auto ButtonTrigger = gui_params.wasButtonClicked("TriggerButton 1");
    }*/

    // =================================================================================

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
    // =================================================================================

    // =================================================================================
    // ===         2. ACCESSING INFORMATION (EVENTS) RECEIVED FROM HOST
    // =================================================================================

    /**NOTE**
         Information received from host are passed on to you via the new_event_from_host object.

        Multiple Types of Events can be received from the host:

           1. FirstBufferEvent --> always sent at the beginning of the host start
           2. PlaybackStoppedEvent --> always sent when the host stops the playback
           3. NewBufferEvent   --> sent at the beginning of every new buffer (if enabled in settings.h)
                                   || when qpm, meter, ... changes (if specified in settings.h)
           4. NewBarEvent     --> sent at the beginning of every new bar (if enabled in settings.h)
           5. NewTimeShiftEvent --> sent every N QuarterNotes (if specified in settings.h)
           6. NoteOn/NoteOff/CC Events --> sent when a note is played (if not filtered in settings.h)
                         Access the note number, velocity, channel, etc. using the following methods:
                              new_event_from_host->getNoteNumber(); new_event_from_host->getVelocity(); new_event_from_host->getChannel();
                              new_event_from_host->getCCNumber();
     Regardless of the event type, you can access the following information at the time of the event:
           1. new_event_from_host.qpm()
           2. new_event_from_host.numerator(), new_event_from_host.denominator()
           4. new_event_from_host.isPlaying(), new_event_from_host.isRecording()
           5. new_event_from_host.BufferStartTime().inSeconds(),    --> time of the beginning of the buffer to which
                                                             the event belongs. (in seconds)
              new_event_from_host.BufferEndTime().inSamples(),      --> time of the end of the buffer to which
                                                             the event belongs. (in samples)
              new_event_from_host.BufferEndTime().inQuarterNotes()  --> time of the end of the buffer to which
                                                             the event belongs. (in quarter notes)
           6. new_event_from_host.Time().inSeconds(),               --> time of the event (in seconds,
              new_event_from_host.Time().inSamples(),                                       samples,
              new_event_from_host.Time().inQuarterNotes()                                  || quarter notes)
           7. new_event_from_host.isLooping()                    --> whether || not the host is looping
           8. new_event_from_host.loopStart(), new_event_from_host.loopEnd() --> loop start && end times (in quarter notes)
           9. new_event_from_host.barCount()                     --> number of bars elapsed since beginning
           10. new_event_from_host.lastBarPos()                  --> Position of last bar passed
           */

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    /* Warning!
         * DO NOT USE realtime_playback_info for input preparations here, because
         * the notes are most likely registered prior to NOW and stored in the queue for access
         * As Such, for this part, use only the information provided within the received
         * new_event_from_host object. */
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    if (new_event_from_host.has_value()) {

        if (new_event_from_host->isFirstBufferEvent()) {

        } else if (new_event_from_host->isPlaybackStoppedEvent()) {

        } else if (new_event_from_host->isNewBufferEvent()) {

        } else if (new_event_from_host->isNewBarEvent()) {

            // the following line should be placed in the correct place in your code
            // in this example we want to send the compiled data to the model
            // on every bar, hence I'll set the flag to true here
            SHOULD_SEND_TO_MODEL_FOR_GENERATION_ = true;

        } else if (new_event_from_host->isNewTimeShiftEvent()) {

        } else if (new_event_from_host->isNoteOnEvent()) {
            auto note_n = new_event_from_host->getNoteNumber();
            auto vel =new_event_from_host->getVelocity();
            auto channel = new_event_from_host->getChannel();
            auto time = new_event_from_host->Time().inSeconds();
        } else if (new_event_from_host->isNoteOffEvent()) {
            auto note_n = new_event_from_host->getNoteNumber();
            auto vel =new_event_from_host->getVelocity();
            auto channel = new_event_from_host->getChannel();
        } else if (new_event_from_host->isCCEvent()) {
        }
    }
    // =================================================================================

    // =================================================================================
    // ===         3. ACCESSING INFORMATION (EVENTS) RECEIVED FROM
    //                Mannually Drag-Dropped Midi Files
    //  ALL INFORMATION HERE IS PROVIDED SEQUENTIALLY ONLY WHENEVER HOST IS PLAYING
    // =================================================================================
    /**NOTE**
         Information received from manually drag-dropped midi files are passed on to you
            via the new_midi_event_dragdrop object.

        Multiple Information are available here:
                1. isFirstMessage() --> true if this is the first message in the midi file
                2. isLastMessage() --> true if this is the last message in the midi file
                3. isNoteOnEvent() --> true if this is a note on event
                4. isNoteOffEvent() --> true if this is a note off event
                5. isCCEvent() --> true if this is a CC event

     If you need to process the midi notes only after the last one, keep them in
     a vector<MidiFileEvent> and process them when the last one is received.
    */
    if (new_midi_event_dragdrop.has_value()) {
        auto time_quarterNotes = new_midi_event_dragdrop->Time();
        auto time_in_seconds = new_midi_event_dragdrop->Time(sample_rate, qpm).inSeconds();
        auto time_in_samples = new_midi_event_dragdrop->Time(sample_rate, qpm).inSamples();
        auto isFirst = new_midi_event_dragdrop->isFirstMessage();
        auto isLast = new_midi_event_dragdrop->isLastMessage();
        auto isNoteOn = new_midi_event_dragdrop->isNoteOnEvent();
        auto isNoteOff = new_midi_event_dragdrop->isNoteOffEvent();
        if (isNoteOn || isNoteOff) {
            auto noteNumber = new_midi_event_dragdrop->getNoteNumber();
            auto velocity = new_midi_event_dragdrop->getVelocity();
        }
        auto isCC = new_midi_event_dragdrop->isCCEvent();
        if (isCC) {
            auto ccNumber = new_midi_event_dragdrop->getCCValue();
        }

        PrintMessage(new_midi_event_dragdrop->getDescription().str());
        PrintMessage(new_midi_event_dragdrop->getDescription(sample_rate,
                                                             qpm).str());

        if (isLast) { // here I'm requesting forward pass only after entire midi file is received
            SHOULD_SEND_TO_MODEL_FOR_GENERATION_ = true;
        }
    }
    // =================================================================================


    // =================================================================================
    // ===         4. Sending data to the model thread (MDL) if necessary
    // =================================================================================

    /* All data to be sent to the model thread (MDL) should be stored in the model_input
            object. This object is defined in the header file of this class.
            The class ModelInput is defined in the file model_input.h && should be modified
            to include all the data you want to send to the model thread.

         Once prepared && should be sent, return true from this function! Otherwise,
         return false. --> NOTE: This is necessary so that the wrapper can know when to
         send the data to the model thread. */

    if (SHOULD_SEND_TO_MODEL_FOR_GENERATION_) {
        // Example:
        //      If should send to model, update model_input && return true
        model_input.tensor1 = torch::rand({1, 32, 27}, torch::kFloat32);
        model_input.someDouble = 0.5f;
        /*  ... set other model_input fields */

        // Notify ITP thread to send the updated data by returning true
        return true;
    } else {
        return false;
    }
    // =================================================================================
}
