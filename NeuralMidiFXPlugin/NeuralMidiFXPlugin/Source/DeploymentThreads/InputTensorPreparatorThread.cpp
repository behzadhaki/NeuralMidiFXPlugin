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
        std::optional<Event> &new_event,
        bool gui_params_changed_since_last_call) {

    /*              IF YOU NEED TO PRINT TO CONSOLE FOR DEBUGGING,
     *                  YOU CAN USE THE FOLLOWING METHOD:
     *                      PrintMessage("YOUR MESSAGE HERE");
     */

    /* A flag like this one can be used to check whether or not the model input
        is ready to be sent to the model thread (MDL)*/
    bool SHOULD_SEND_TO_MODEL_FOR_GENERATION_ = false;

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

    auto Slider1 = gui_params.getValueFor("Slider 1");
    auto ToggleButton1 = gui_params.isToggleButtonOn("ToggleButton 1");
    auto ButtonTrigger = gui_params.wasButtonClicked("TriggerButton 1");


    // =================================================================================
    // ===         2. ACCESSING INFORMATION (EVENTS) RECEIVED FROM HOST
    // =================================================================================

     /**NOTE**
         Information received from host are passed on to you via the new_event object.

        Multiple Types of Events can be received from the host:

           1. FirstBufferEvent --> always sent at the beginning of the host start
           2. PlaybackSxtoppedEvent --> always sent when the host stops the playback
           3. NewBufferEvent   --> sent at the beginning of every new buffer (if enabled in settings.h)
                                   OR when qpm, meter, ... changes (if specified in settings.h)
           4. NewBarEvent     --> sent at the beginning of every new bar (if enabled in settings.h)
           5. NewTimeShiftEvent --> sent every N QuarterNotes (if specified in settings.h)
           6. NoteOn/NoteOff/CC Events --> sent when a note is played (if not filtered in settings.h)
                         Access the note number, velocity, channel, etc. using the following methods:
                              new_event->getNoteNumber(); new_event->getVelocity(); new_event->getChannel();
                              new_event->getCCNumber();
     Regardless of the event type, you can access the following information at the time of the event:
           1. new_event.qpm()
           2. new_event.numerator(), new_event.denominator()
           4. new_event.isPlaying(), new_event.isRecording()
           5. new_event.BufferStartTime().inSeconds(),    --> time of the beginning of the buffer to which
                                                             the event belongs. (in seconds)
              new_event.BufferEndTime().inSamples(),      --> time of the end of the buffer to which
                                                             the event belongs. (in samples)
              new_event.BufferEndTime().inQuarterNotes()  --> time of the end of the buffer to which
                                                             the event belongs. (in quarter notes)
           6. new_event.Time().inSeconds(),               --> time of the event (in seconds,
              new_event.Time().inSamples(),                                       samples,
              new_event.Time().inQuarterNotes()                                  or quarter notes)
           7. new_event.isLooping()                    --> whether or not the host is looping
           8. new_event.loopStart(), new_event.loopEnd() --> loop start and end times (in quarter notes)
           9. new_event.barCount()                     --> number of bars elapsed since beginning
           10. new_event.lastBarPos()                  --> Position of last bar passed (in quarter notes)*/

    if (new_event.has_value()) {
        if (new_event->isFirstBufferEvent()) {

        } else if (new_event->isPlaybackStoppedEvent()) {

        } else if (new_event->isNewBufferEvent()) {

        } else if (new_event->isNewBarEvent()) {

            // the following line should be placed in the correct place in your code
            // in this example we want to send the compiled data to the model
            // on every bar, hence I'll set the flag to true here
            SHOULD_SEND_TO_MODEL_FOR_GENERATION_ = true;

        } else if (new_event->isNewTimeShiftEvent()) {


        } else if (new_event->isNoteOnEvent()) {

        } else if (new_event->isNoteOffEvent()) {

        } else if (new_event->isCCEvent()) {

        }
        // =================================================================================

        // =================================================================================
        // ===         3. Sending data to the model thread (MDL)
        // =================================================================================

        /* All data to be sent to the model thread (MDL) should be stored in the model_input
            object. This object is defined in the header file of this class.
            The class ModelInput is defined in the file model_input.h and should be modified
            to include all the data you want to send to the model thread.

         Once prepared and should be sent, return true from this function! Otherwise,
         return false. --> NOTE: This is necessary so that the wrapper can know when to
         send the data to the model thread. */

        if (SHOULD_SEND_TO_MODEL_FOR_GENERATION_) {
            // Example:
            //      If should send to model, update model_input and return true
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
}



// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                                    DO NOT CHANGE ANYTHING BELOW THIS LINE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
InputTensorPreparatorThread::InputTensorPreparatorThread() : juce::Thread("InputPreparatorThread") {}

void InputTensorPreparatorThread::startThreadUsingProvidedResources(
        LockFreeQueue<Event, queue_settings::NMP2ITP_que_size> *NMP2ITP_Event_Que_ptr_,
        LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size> *ITP2MDL_ModelInput_Que_ptr_,
        LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2ITP_Parameters_Queu_ptr_) {

    // Provide access to resources needed to communicate with other threads
    // ---------------------------------------------------------------------------------------------
    NMP2ITP_Event_Que_ptr = NMP2ITP_Event_Que_ptr_;
    ITP2MDL_ModelInput_Que_ptr = ITP2MDL_ModelInput_Que_ptr_;
    APVM2ITP_Parameters_Queu_ptr = APVM2ITP_Parameters_Queu_ptr_;

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
        const Event &event, bool compact_mode, double event_count) {

    auto showMessage = [](const std::string& input) {
        // if input is multiline, split it into lines and print each line separately
        std::stringstream ss(input);
        std::string line;

        while (std::getline(ss, line)) {
            std::cout << clr::green << "[ITP] " << line << std::endl;
        }
    };

    if (event.isFirstBufferEvent() or !compact_mode) {
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

    // if input is multiline, split it into lines and print each line separately
    std::stringstream ss(input);
    std::string line;
    while (std::getline(ss, line)) { std::cout << clr::green << "[ITP] " << line << std::endl; }
}

void InputTensorPreparatorThread::run() {

    // convert showMessage to a lambda function
    auto showMessage = [](const std::string& input) {
        // if input is multiline, split it into lines and print each line separately
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
    std::optional<Event> new_event{};
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
            new_event = NMP2ITP_Event_Que_ptr->pop();      // get the next available event
            new_event->registerAccess();                    // set the time when the event was accessed

            events_received_count++;

            if (print_input_events) { // if set in Debugging.h
                DisplayEvent(*new_event, false, events_received_count);   // display the event
            }

        } else {
            new_event = std::nullopt;
        }

        if (new_event.has_value() or gui_params.changed()) {
            chrono_timed_deploy.registerStartTime();
            auto ready2send2MDL = deploy(new_event, gui_params.changed());
            chrono_timed_deploy.registerEndTime();

            if (print_deploy_method_time and chrono_timed_deploy.isValid()) { // if set in Debugging.h
                showMessage(*chrono_timed_deploy.getDescription(" deploy() execution time: "));
            }

            // push to next thread if a new input is provided
            if (ready2send2MDL) {
                model_input.timer.registerStartTime();
                ITP2MDL_ModelInput_Que_ptr->push(model_input);
                inputs_sent_count++;
                chrono_timed_consecutive_pushes.registerEndTime();
                if (print_timed_consecutive_ModelInputs_pushed and chrono_timed_consecutive_pushes.isValid()) {
                    if (inputs_sent_count > 1) {
                        auto text = "Time Duration Between ModelInput #" + std::to_string(inputs_sent_count);
                        text += " and #" + std::to_string(inputs_sent_count - 1) + ": ";
                        showMessage(*chrono_timed_consecutive_pushes.getDescription(text));
                    } else {
                        auto text = "Time Duration Between Start and First Pushed ModelInput: ";
                        showMessage(*chrono_timed_consecutive_pushes.getDescription(text));
                    }
                }
                chrono_timed_consecutive_pushes.registerStartTime();
            }
        }

        // update event trackers accordingly if applicable
        if (new_event.has_value()) {

            if (new_event->isFirstBufferEvent()) { first_frame_metadata_event = *new_event; }
            else if (new_event->isNewBufferEvent()) { frame_metadata_event = *new_event; }
            else if (new_event->isNewBarEvent()) { last_bar_event = *new_event; }
            else if (new_event->isNewTimeShiftEvent()) { last_complete_note_duration_event = *new_event; }

            last_event = *new_event;
        }

        // check if thread is still running
        bExit = threadShouldExit();

        if (!new_event.has_value() and !gui_params.changed()) {
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
    if (not readyToStop) {
        prepareToStop();
    }
}
