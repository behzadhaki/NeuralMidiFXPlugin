#include "Source/DeploymentThreads/InputTensorPreparatorThread.h"

// ===================================================================================
// ===         Refer to:
// https://neuralmidifx.github.io/DeploymentStages/ITP/Deploy
// ===================================================================================

bool InputTensorPreparatorThread::deploy(
    std::optional<MidiFileEvent> & new_midi_event_dragdrop,
    std::optional<EventFromHost> & new_event_from_host,
    bool gui_params_changed_since_last_call) {

    // we only pass the data if the method was called because of a new event
    // and not because of a gui parameter change
    if (new_midi_event_dragdrop.has_value() || new_event_from_host.has_value()) {
        model_input.new_event_from_host = new_event_from_host;
        model_input.new_midi_event_dragdrop = new_midi_event_dragdrop;
        return true;
    }

    return false;
}
