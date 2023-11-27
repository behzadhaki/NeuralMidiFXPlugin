#include "../Source/DeploymentThreads/DeploymentThread.h"

// ===================================================================================
// ===         Refer to:
// https://neuralmidifx.github.io/DeploymentStages/????
// ===================================================================================

std::pair<bool, bool>
    DeploymentThread::deploy(
        std::optional<MidiFileEvent> & new_midi_event_dragdrop,
        std::optional<EventFromHost> & new_event_from_host,
        bool gui_params_changed_since_last_call,
        bool new_preset_loaded_since_last_call) {

// flags to keep track if any new data is generated and should
    // be sent to the main thread
    bool newPlaybackPolicyShouldBeSent{false};
    bool newPlaybackSequenceGeneratedAndShouldBeSent{false};

    // ===============================================================================
    if (new_preset_loaded_since_last_call) {
        // if the tensor preset has changed, update the model input
        cout << clr::blue << "[DPL] Tensor Preset Changed. Updating Model Input" << endl;
        TensorPresetTracker.printTensorMap();
    }

    // your implementation goes here
    return {newPlaybackPolicyShouldBeSent, newPlaybackSequenceGeneratedAndShouldBeSent};
}
