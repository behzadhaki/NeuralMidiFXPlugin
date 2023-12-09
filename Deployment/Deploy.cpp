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
        bool new_preset_loaded_since_last_call,
        bool new_midi_file_dropped_on_visualizers,
        bool new_audio_file_dropped_on_visualizers) {

// flags to keep track if any new data is generated and should
    // be sent to the main thread
    bool newPlaybackPolicyShouldBeSent{false};
    bool newPlaybackSequenceGeneratedAndShouldBeSent{false};

    // ===============================================================================
    if (new_preset_loaded_since_last_call) {
        // if the tensor preset has changed, access the data
        cout << clr::blue << "[DPL] Tensor Preset Changed. Updating Model Input" << endl;

        for (const auto& pair : CustomPresetData->tensors()) {
            const std::string& key = pair.first;
            const torch::Tensor& t = pair.second;
        }

        // or

        for (const auto& key: CustomPresetData->keys()) {
            auto t = CustomPresetData->tensor(key);
            if (t != std::nullopt) {
                auto value = *t;
            }
        }

        // update the tensor data (NOTE: ONLY USE THE FOLLOWING METHOD TO UPDATE TENSOR DATA)
        // CustomPresetData->tensor("input_tensor", torch::rand({1, 1, 1, 1}));

    }

    // ===============================================================================
    if (new_midi_file_dropped_on_visualizers) {
        auto ids =
            midiVisualizersData->get_visualizer_ids_with_user_dropped_new_sequences();
        for (const auto& id : ids) {
            auto new_sequence = midiVisualizersData->get_visualizer_data(id);
            if (new_sequence != std::nullopt) {
                for (const auto& event : *new_sequence) {
                    cout << event.getDescription().str() << endl;
                }
            }
        }
    }

    // ===============================================================================
    // displaying contents in a visualizer
    // ===============================================================================
    midiVisualizersData->clear_visualizer_data("MidiDisplay 1");
    midiVisualizersData->displayNoteOn("MidiDisplay 1", 32, 0.1, 0.5);
    midiVisualizersData->displayNoteOff("MidiDisplay 1", 32, 0.5);
    midiVisualizersData->displayNoteWithDuration("MidiDisplay 1", 31, 0.1, 0.5, 0.9);

    // your implementation goes here
    return {newPlaybackPolicyShouldBeSent, newPlaybackSequenceGeneratedAndShouldBeSent};
}
