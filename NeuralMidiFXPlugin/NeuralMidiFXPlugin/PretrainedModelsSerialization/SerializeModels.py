import os.path

import torch
from BaseGrooveTransformers.models.transformer import GrooveTransformerEncoder
import params as prm


def load_model_in_eval_mode(model_param_dict):

    # get model name
    model_path = model_param_dict['pyModelPath']

    # load checkpoint
    checkpoint = torch.load(model_path, map_location=model_param_dict['device'])

    # Initialize model
    groove_transformer = GrooveTransformerEncoder(model_param_dict['d_model'],
                                                  model_param_dict['embedding_sz'],
                                                  model_param_dict['embedding_sz'],
                                                  model_param_dict['n_heads'],
                                                  model_param_dict['dim_ff'],
                                                  model_param_dict['dropout'],
                                                  model_param_dict['n_layers'],
                                                  model_param_dict['max_len'],
                                                  model_param_dict['device'])

    # Load model and put in evaluation mode
    groove_transformer.load_state_dict(checkpoint['model_state_dict'])
    groove_transformer.eval()

    return groove_transformer


if __name__ == '__main__':

    for model_name_, model_param_dict_ in prm.model_params.items():
        groove_transformer = load_model_in_eval_mode(model_param_dict_)

        # trace model for serialization
        # https://pytorch.org/tutorials/advanced/cpp_export.html
        example = torch.rand(1, 32, 27)
        traced_script_module = torch.jit.trace(groove_transformer, example)
        traced_script_module.save(os.path.join("serialized", model_name_+".pt"))