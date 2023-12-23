
#pragma once

#include "Source/DeploymentThreads/DeploymentThread.h"

class PluginDeploymentThread: public DeploymentThread {
public:
    // initialize your deployment thread here
    PluginDeploymentThread():DeploymentThread() {

    }

    // this method runs on a per-event basis.
    // the majority of the deployment will be done here!
    std::pair<bool, bool> deploy (
        std::optional<MidiFileEvent> & new_midi_event_dragdrop,
        std::optional<EventFromHost> & new_event_from_host,
        bool gui_params_changed_since_last_call,
        bool new_preset_loaded_since_last_call,
        bool new_midi_file_dropped_on_visualizers,
        bool new_audio_file_dropped_on_visualizers) override {

        // flags to keep track if any new data is generated and should
        // be sent to the main thread
        bool newPlaybackPolicyShouldBeSent{false};
        bool newPlaybackSequenceGeneratedAndShouldBeSent{false};

        cnt += 1;
        cout << "PluginDeploymentThread::deploy() called " << cnt << " times" << endl;
        // cout << "PluginDeploymentThread::deploy() called" << endl;

        // make sure to set these flags to true if you want to send new data to the main thread
        return {newPlaybackPolicyShouldBeSent, newPlaybackSequenceGeneratedAndShouldBeSent};
    }

private:
    // add any member variables or methods you need here
    int cnt = 0;
};