#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "settings.h"

using namespace std;

namespace DrumsPianoRoll{
struct Parameters
{

    //Raw pointers. They will be owned by either the processor or the APVTS (if you use it)
    // XYSLIDER PARAMETERS
    vector<juce::AudioParameterFloat*> MaxHitsPerVoice;
    vector<juce::AudioParameterFloat*> SampleThresholdsPerVoice;


    Parameters()
    {
        for (size_t i=0; i < HVO_params::num_voices; i++)
        {
            auto voice_label = nine_voice_kit_labels[i];
            MaxHitsPerVoice.push_back(
                new juce::AudioParameterFloat(voice_label+"_MaxNumHits", voice_label+"_MaxNumHits",
                                              0.f, HVO_params::time_steps, nine_voice_kit_default_max_voices_allowed[i])
            );
            SampleThresholdsPerVoice.push_back(
                new juce::AudioParameterFloat(voice_label+"_sampleThresh", voice_label+"_sampleThresh",
                                              0.f, 1.f, nine_voice_kit_default_sampling_thresholds[i])
            );
        }
    }

    ~Parameters()
    {
        for (size_t i=0; i < HVO_params::num_voices; i++)
        {
            delete MaxHitsPerVoice[i];
            delete SampleThresholdsPerVoice[i];
        }
    }

};
}
