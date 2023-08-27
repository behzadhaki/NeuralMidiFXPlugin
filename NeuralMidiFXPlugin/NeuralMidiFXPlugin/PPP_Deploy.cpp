#include "Source/DeploymentThreads/PlaybackPreparatorThread.h"

// ===================================================================================
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                       YOUR IMPLEMENTATION SHOULD BE DONE HERE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// ===================================================================================

std::pair<bool, bool> PlaybackPreparatorThread::deploy(bool new_model_output_received, bool did_any_gui_params_change) {

    bool newPlaybackPolicyShouldBeSent = false;
    bool newPlaybackSequenceGeneratedAndShouldBeSent = false;

    if (new_model_output_received)
    {
        if (model_output.new_event_from_host) {
            PrintMessage("Received new event from host");
        }

        if (model_output.new_midi_event_dragdrop) {
            PrintMessage("Received new midi event from dragdrop");
        }

    }

    return {newPlaybackPolicyShouldBeSent, newPlaybackSequenceGeneratedAndShouldBeSent};
}