//
// Created by Behzad Haki on 2022-08-03.
//

#include "ModelAPI.h"
#include <shared_plugin_helpers/shared_plugin_helpers.h>  // fixme remove this


using namespace std;

/*template<typename Iterator>
inline std::vector<size_t> n_largest_indices(Iterator it, Iterator end, size_t n) {
    struct Element {
        Iterator it;
        size_t index;
    };

    std::vector<Element> top_elements;
    top_elements.reserve(n + 1);

    for(size_t index = 0; it != end; ++index, ++it) {
        top_elements.insert(std::upper_bound(top_elements.begin(), top_elements.end(), *it, [](auto value, auto element){return value > *element.it;}), {it, index});
        if (index >= n)
            top_elements.pop_back();
    }

    std::vector<size_t> result;
    result.reserve(top_elements.size());

    for(auto &element: top_elements)
        result.push_back(element.index);

    return result;
}*/


static torch::Tensor vector2tensor(vector<float> vec)
{
    auto size = (int) vec.size();
    auto t = torch::zeros({size}, torch::kFloat32);
    for (int i=0; i<size; i++)
    {
        t[i] = vec[i];
    }

    return t;
}


//default constructor
MonotonicGrooveTransformerV1::MonotonicGrooveTransformerV1(){
    model_path = "NO Path Specified Yet!!";
}


bool MonotonicGrooveTransformerV1::loadModel(std::string model_path_, int time_steps_, int num_voices_)
{

    model_path = model_path_;
    time_steps = time_steps_;
    num_voices = num_voices_;
    ifstream myFile;
    myFile.open(model_path);
    // Check for errors

    // check if path works fine
    if (myFile.fail())
    {
        return false;
    }
    else
    {
        myFile.close();

        model = LoadModel(model_path, true);
        hits_logits = torch::zeros({time_steps_, num_voices_});
        hits_probabilities = torch::zeros({time_steps_, num_voices_});
        hits = torch::zeros({time_steps_, num_voices_});
        velocities = torch::zeros({time_steps_, num_voices_});
        offsets = torch::zeros({time_steps_, num_voices_});
        per_voice_sampling_thresholds = vector2tensor(nine_voice_kit_default_sampling_thresholds);
        per_voice_max_count_allowed = nine_voice_kit_default_max_voices_allowed;
        return true;
    }

}

bool MonotonicGrooveTransformerV1::changeModel(std::string model_path_)
{
    ifstream myFile;
    myFile.open(model_path_);
    // Check for errors

    // check if path works fine
    if (myFile.fail())
    {
        return false;
    }
    else
    {
        myFile.close();
        model = LoadModel(model_path_, true);
        model_path_ = model_path;
        return true;
    }
}

// getters
torch::Tensor MonotonicGrooveTransformerV1::get_hits_logits() { return hits_logits; }
torch::Tensor MonotonicGrooveTransformerV1::get_hits_probabilities() { return hits_probabilities; }
torch::Tensor MonotonicGrooveTransformerV1::get_velocities() { return velocities; }
torch::Tensor MonotonicGrooveTransformerV1::get_offsets() { return offsets; }


// setters
void MonotonicGrooveTransformerV1::set_sampling_thresholds(vector<float> per_voice_thresholds)
{
    assert(per_voice_thresholds.size()==num_voices &&
           "thresholds dim [num_voices]");

    per_voice_sampling_thresholds = vector2tensor(per_voice_thresholds);
}

void MonotonicGrooveTransformerV1::set_max_count_per_voice_limits(vector<float> perVoiceMaxNumVoicesAllowed)
{
    assert(perVoiceMaxNumVoicesAllowed.size()==num_voices &&
           "thresholds dim [num_voices]");

    per_voice_max_count_allowed = perVoiceMaxNumVoicesAllowed;
}

// returns true if temperature has changed
bool MonotonicGrooveTransformerV1::set_sampling_temperature(float temperature)
{
    if (sampling_temperature == temperature)
    {
        return false;
    }
    sampling_temperature = temperature;
    DBG("TEMPERATURE UPDATED to " << sampling_temperature);
    return true;
}

// Passes input through the model and updates logits, vels and offsets
void MonotonicGrooveTransformerV1::forward_pass(torch::Tensor monotonicGrooveInput)
{
    assert(monotonicGrooveInput.sizes()[0]==time_steps &&
           "shape [time_steps, num_voices*3]");
    assert(monotonicGrooveInput.sizes()[1]==(num_voices*3) &&
           "shape [time_steps, num_voices*3]");

    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(monotonicGrooveInput);
    auto outputs = model.forward(inputs);
    auto hLogit_v_o_tuples = outputs.toTuple();
    hits_logits = hLogit_v_o_tuples->elements()[0].toTensor().view({time_steps, num_voices});
    hits_probabilities = torch::sigmoid(hits_logits/sampling_temperature).view({time_steps, num_voices});
    velocities = hLogit_v_o_tuples->elements()[1].toTensor().view({time_steps, num_voices});
    offsets = hLogit_v_o_tuples->elements()[2].toTensor().view({time_steps, num_voices});
}

std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> MonotonicGrooveTransformerV1::
    sample(std::string sample_mode)
{
    assert (sample_mode=="Threshold" or sample_mode=="SampleProbability");

    hits = torch::zeros({time_steps, num_voices});

    auto row_indices = torch::arange(0, time_steps);
    if (sample_mode=="Threshold")
    {
        // read CPU accessors in  https://pytorch.org/cppdocs/notes/tensor_basics.html
        // asserts accessed part of tensor is 2-dimensional and holds floats.
        //auto hits_probabilities_a = hits_probabilities.accessor<float,2>();

        for (int voice_i=0; voice_i < num_voices; voice_i++){
            // Get voice threshold value
            auto thres_voice_i  = per_voice_sampling_thresholds[voice_i];
            // Get probabilities of voice hits at all timesteps
            auto voice_hit_probs = hits_probabilities.index(
                {row_indices, voice_i});
            auto tup = voice_hit_probs.topk((int) per_voice_max_count_allowed[size_t(voice_i)]);
            auto candidate_probs = std::get<0>(tup);
            auto candidate_prob_indices = std::get<1>(tup);

            // Find locations exceeding threshold and set to 1 (hit)
            auto accepted_candidate_indices = candidate_probs>=thres_voice_i;
            auto active_time_indices = candidate_prob_indices.index({accepted_candidate_indices});


            hits.index_put_({active_time_indices, voice_i}, 1);
        }
    }

    // Set non-hit vel and offset values to 0
    // velocities = velocities * hits;
    // offsets = offsets * hits;

    // DBG(tensor2string(hits));

    return {hits, velocities, offsets};
}

// ------------------------------------------------------------
// -------------------UTILS------------------------------------
// ------------------------------------------------------------
// converts a tensor to string to be used with DBG for debugging
std::string tensor2string (torch::Tensor tensor)
{
    std::ostringstream stream;
    stream << tensor;
    std::string tensor_string = stream.str();
    return tensor_string;
}

// Loads model either in eval mode or train modee
inline torch::jit::script::Module LoadModel(std::string model_path, bool eval)
{
    torch::jit::script::Module model;
    model = torch::jit::load(model_path);
    if (eval)
    {
        model.eval();
    }
    else
    {
        model.train();
    }
    return model;
}

