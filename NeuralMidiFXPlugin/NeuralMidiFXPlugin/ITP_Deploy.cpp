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

    // check that the deploy() method was called because of a new midi event
    if (new_event_from_host.has_value()) {
        if (new_event_from_host->isNewTimeShiftEvent()) {
            SHOULD_SEND_TO_MODEL_FOR_GENERATION_ = true;
        }

        if (new_event_from_host->isFirstBufferEvent()) {
            // clear hits, velocities, offsets
            ITPdata.hits = torch::zeros({1, 32, 1}, torch::kFloat32);
            ITPdata.velocities = torch::zeros({1, 32, 1}, torch::kFloat32);
            ITPdata.offsets = torch::zeros({1, 32, 1}, torch::kFloat32);
        }

        if (new_event_from_host->isNoteOnEvent()) {
            auto ppq  = new_event_from_host->Time().inQuarterNotes(); // time in ppq
            auto velocity = new_event_from_host->getVelocity(); // velocity
            auto div = round(ppq / .25f);
            auto offset = (ppq - (div * .25f)) / 0.125 * 0.5 ;
            auto grid_index = (long long) fmod(div, 32);

            // check if louder if overlapping
            if (ITPdata.hits[0][grid_index][0].item<float>() > 0) {
                if (ITPdata.velocities[0][grid_index][0].item<float>() < velocity) {
                    ITPdata.velocities[0][grid_index][0] = velocity;
                    ITPdata.offsets[0][grid_index][0] = offset;
                }
            } else {
                ITPdata.hits[0][grid_index][0] = 1;
                ITPdata.velocities[0][grid_index][0] = velocity;
                ITPdata.offsets[0][grid_index][0] = offset;
            }
        }
    }

    return SHOULD_SEND_TO_MODEL_FOR_GENERATION_;
}
