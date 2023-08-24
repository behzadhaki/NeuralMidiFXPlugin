#include "Source/DeploymentThreads/InputTensorPreparatorThread.h"

// ===================================================================================
// ===         Refer to:
// https://neuralmidifx.github.io/DeploymentStages/ITP/Deploy
// ===================================================================================

bool InputTensorPreparatorThread::deploy(
    std::optional<MidiFileEvent> & new_midi_event_dragdrop,
    std::optional<EventFromHost> & new_event_from_host,
    bool gui_params_changed_since_last_call) {

    bool SHOULD_SEND_TO_MODEL_FOR_GENERATION_ = false;

    // your implementation goes here

    return SHOULD_SEND_TO_MODEL_FOR_GENERATION_;
}
