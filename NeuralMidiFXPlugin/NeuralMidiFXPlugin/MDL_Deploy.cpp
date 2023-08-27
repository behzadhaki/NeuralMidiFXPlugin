#include "Source/DeploymentThreads/ModelThread.h"

using namespace debugging_settings::ModelThread;

// ===================================================================================
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                       YOUR IMPLEMENTATION SHOULD BE DONE HERE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// ===================================================================================

bool ModelThread::deploy(bool new_model_input_received,
                         bool did_any_gui_params_change) {

    // we only pass the data if the method was called because of a new model_input

    if (new_model_input_received) {
        model_output.new_event_from_host = model_input.new_event_from_host;
        model_output.new_midi_event_dragdrop = model_input.new_midi_event_dragdrop;
        
        return true;
    }

    return false;
}