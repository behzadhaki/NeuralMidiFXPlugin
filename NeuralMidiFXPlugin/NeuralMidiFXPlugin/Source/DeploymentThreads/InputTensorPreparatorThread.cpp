//
// Created by Behzad Haki on 2022-12-13.
//

#include "InputTensorPreparatorThread.h"

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
    PrintMessage("qpm: " + std::to_string(qpm));
    std::cout << "qpm: " << qpm << std::endl;
    // =================================================================================

    // =================================================================================
    // ===         2. ACCESSING INFORMATION (EVENTS) RECEIVED FROM HOST
    // =================================================================================

     /**NOTE**
         Information received from host are passed on to you via the new_event_from_host object.

        Multiple Types of Events can be received from the host:

           1. FirstBufferEvent --> always sent at the beginning of the host start
           2. PlaybackSxtoppedEvent --> always sent when the host stops the playback
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
           10. new_event_from_host.lastBarPos()                  --> Position of last bar passed (in quarter notes)
           */

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        /* Warning!
         * DO NOT USE realtime_playback_info for input preparations here, because
         * the notes are most registered prior to NOW and stored in the queue for access
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

        } else if (new_event_from_host->isNoteOffEvent()) {

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



// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                                    DO NOT CHANGE ANYTHING BELOW THIS LINE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
InputTensorPreparatorThread::InputTensorPreparatorThread() : juce::Thread("InputPreparatorThread") {}

void InputTensorPreparatorThread::startThreadUsingProvidedResources(
    LockFreeQueue<EventFromHost, queue_settings::NMP2ITP_que_size> *NMP2ITP_Event_Que_ptr_,
    LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size> *ITP2MDL_ModelInput_Que_ptr_,
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2ITP_Parameters_Queu_ptr_,
    LockFreeQueue<juce::MidiFile, 4> *GUI2ITP_DroppedMidiFile_Que_ptr_,
    RealTimePlaybackInfo *realtimePlaybackInfo_ptr_) {

    // Provide access to resources needed to communicate with other threads
    // ---------------------------------------------------------------------------------------------
    NMP2ITP_Event_Que_ptr = NMP2ITP_Event_Que_ptr_;
    ITP2MDL_ModelInput_Que_ptr = ITP2MDL_ModelInput_Que_ptr_;
    APVM2ITP_Parameters_Queu_ptr = APVM2ITP_Parameters_Queu_ptr_;
    GUI2ITP_DroppedMidiFile_Que_ptr = GUI2ITP_DroppedMidiFile_Que_ptr_;
    realtimePlaybackInfo = realtimePlaybackInfo_ptr_;

    // Start the thread. This function internally calls run() method. DO NOT CALL run() DIRECTLY.
    // ---------------------------------------------------------------------------------------------
    startThread();
}

/*
 * Use this method to print the content of an event
 *  in debugging mode (using DBG() macro)
 *
 * @param event: the event to be printed
 * @param compact_mode: Skips buffer events where no change in buffer is detected
 *      compared to the previous buffer
 * @param event_count: the number of events that have been received so far
 */
void InputTensorPreparatorThread::DisplayEvent(
        const EventFromHost&event, bool compact_mode, double event_count) {

    auto showMessage = [](const std::string& input) {
        // if input is multiline, split it into lines && print each line separately
        std::stringstream ss(input);
        std::string line;

        while (std::getline(ss, line)) {
            std::cout << clr::green << "[ITP] " << line << std::endl;
        }
    };

    if (event.isFirstBufferEvent() || !compact_mode) {
        auto dscrptn = event.getDescription().str();
        if (dscrptn.length() > 0) { showMessage(dscrptn); }
    } else {
        auto dscrptn = event.getDescriptionOfChangedFeatures(event, true).str();
        if (dscrptn.length() > 0) { showMessage(dscrptn); }
    }
}

static string bool2str(bool b) {
    std::to_string(b);
    if (b) { return "true"; } else { return "false"; }
}

void InputTensorPreparatorThread::PrintMessage(const std::string& input) {
    using namespace debugging_settings::InputTensorPreparatorThread;
    if (disable_user_print_requests) { return; }

    // if input is multiline, split it into lines && print each line separately
    std::stringstream ss(input);
    std::string line;
    while (std::getline(ss, line)) { std::cout << clr::green << "[ITP] " << line << std::endl; }
}

void InputTensorPreparatorThread::run() {

    // convert showMessage to a lambda function
    auto showMessage = [](const std::string& input) {
        // if input is multiline, split it into lines && print each line separately
        std::stringstream ss(input);
        std::string line;

        while (std::getline(ss, line)) {
            std::cout << clr::green << "[ITP] " << line << std::endl;
        }
    };

    // notify if the thread is still running
    bool bExit = threadShouldExit();

    double events_received_count = 0;
    int inputs_sent_count = 0;

    chrono_timer chrono_timed_deploy;
    chrono_timer chrono_timed_consecutive_pushes;
    chrono_timed_consecutive_pushes.registerStartTime();
    std::optional<EventFromHost> new_event_from_DAW {};
    std::optional<MidiFileEvent> new_midi_event_dropped_manually {};
    std::optional<ModelInput> ModelInput2send2MDL;

    using namespace debugging_settings::InputTensorPreparatorThread;

    while (!bExit) {

        if (readyToStop) { break; } // check if thread is ready to be stopped
        if (APVM2ITP_Parameters_Queu_ptr->getNumReady() > 0) {
            // print updated values for debugging
            gui_params = APVM2ITP_Parameters_Queu_ptr->pop(); // pop the latest parameters from the queue
            gui_params.registerAccess();                      // set the time when the parameters were accessed

            if (print_received_gui_params) { // if set in Debugging.h
                showMessage(gui_params.getDescriptionOfUpdatedParams());
            }

        } else {
            gui_params.setChanged(false); // no change in parameters since last check
        }

        if (NMP2ITP_Event_Que_ptr->getNumReady() > 0) {
            new_event_from_DAW = NMP2ITP_Event_Que_ptr->pop();      // get the next available event
            new_event_from_DAW
                ->registerAccess();                    // set the time when the event was accessed

            events_received_count++;

            if (print_input_events) { // if set in Debugging.h
                DisplayEvent(*new_event_from_DAW, false, events_received_count);   // display the event
            }

        } else {
            new_event_from_DAW = std::nullopt;
        }
        bool ready2send2MDL = false;

        if (new_event_from_DAW.has_value() || gui_params.changed()) {
            new_midi_event_dropped_manually = std::nullopt;
            chrono_timed_deploy.registerStartTime();
            ready2send2MDL = deploy(new_midi_event_dropped_manually, new_event_from_DAW, gui_params.changed());
            chrono_timed_deploy.registerEndTime();

            if (print_deploy_method_time && chrono_timed_deploy.isValid()) { // if set in Debugging.h
                showMessage(*chrono_timed_deploy.getDescription(" deploy() execution time: "));
            }
        }

        // check if notes received from a manually dropped midi file
        if (GUI2ITP_DroppedMidiFile_Que_ptr->getNumReady() > 0){
            auto midifile = GUI2ITP_DroppedMidiFile_Que_ptr->getLatestOnly();

            if (midifile.getNumTracks() > 0) {
                auto track = midifile.getTrack(0);
                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto msg_ = track->getEventPointer(i)->message;

                    if (print_manually_dropped_midi_messages) { // if set in Debugging.h
                        showMessage("Midi Message Received from Dropped Midi File: ");
                        showMessage(msg_.getDescription().toStdString());
                    }

                    auto isFirst = (i == 0);
                    auto isLast = (i == track->getNumEvents() - 1);
                    new_midi_event_dropped_manually = MidiFileEvent(msg_, isFirst, isLast);
                    new_event_from_DAW = std::nullopt;
                    ready2send2MDL = deploy(new_midi_event_dropped_manually, new_event_from_DAW, false);
                }
            }
        }

        // push to next thread if a new input is provided
        if (ready2send2MDL) {
            model_input.timer.registerStartTime();
            ITP2MDL_ModelInput_Que_ptr->push(model_input);
            inputs_sent_count++;
            chrono_timed_consecutive_pushes.registerEndTime();

            if (print_timed_consecutive_ModelInputs_pushed && chrono_timed_consecutive_pushes.isValid()) {
                if (inputs_sent_count > 1) {
                    auto text = "Time Duration Between ModelInput #" + std::to_string(inputs_sent_count);
                    text += " && #" + std::to_string(inputs_sent_count - 1) + ": ";
                    showMessage(*chrono_timed_consecutive_pushes.getDescription(text));
                } else {
                    auto text = "Time Duration Between Start && First Pushed ModelInput: ";
                    showMessage(*chrono_timed_consecutive_pushes.getDescription(text));
                }
            }
            chrono_timed_consecutive_pushes.registerStartTime();
        }

        // update event trackers accordingly if applicable
        if (new_event_from_DAW.has_value()) {

            if (new_event_from_DAW->isFirstBufferEvent()) { first_frame_metadata_event = *new_event_from_DAW; }
            else if (new_event_from_DAW->isNewBufferEvent()) { frame_metadata_event = *new_event_from_DAW; }
            else if (new_event_from_DAW->isNewBarEvent()) { last_bar_event = *new_event_from_DAW; }
            else if (new_event_from_DAW->isNewTimeShiftEvent()) { last_complete_note_duration_event = *new_event_from_DAW; }

            last_event = *new_event_from_DAW;
        }

        // check if thread is still running
        bExit = threadShouldExit();

        if (!new_event_from_DAW.has_value() && !gui_params.changed()) {
            // wait for a few ms to avoid burning the CPU if new data is not available
            sleep(thread_configurations::InputTensorPreparator::waitTimeBtnIters);
        }
    }


}


void InputTensorPreparatorThread::prepareToStop() {
    // Need to wait enough to ensure the run() method is over before killing thread
    this->stopThread(100 * thread_configurations::InputTensorPreparator::waitTimeBtnIters);

    readyToStop = true;
}

InputTensorPreparatorThread::~InputTensorPreparatorThread() {
    if (!readyToStop) {
        prepareToStop();
    }
}
