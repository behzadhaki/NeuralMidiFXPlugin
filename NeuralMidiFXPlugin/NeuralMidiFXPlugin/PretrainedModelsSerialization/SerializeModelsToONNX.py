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

        # Providing input and output names sets the display names for values
        # within the model's graph. Setting these does not change the semantics
        # of the graph; it is only for readability.
        #
        # The inputs to the network consist of the flat list of inputs (i.e.
        # the values you would pass to the forward() method) followed by the
        # flat list of parameters. You can partially specify names, i.e. provide
        # a list here shorter than the number of inputs to the model, and we will
        # only set that subset of names, starting from the beginning.


        with torch.no_grad():
            # trace model for serialization
            # https://pytorch.org/tutorials/advanced/cpp_export.html

            # # Export Input Layer
            # input_names = [ "input_layer_in" ]
            # output_names = [ "input_layer_out" ]
            # example = torch.rand(1, 32, 27)
            # torch.onnx.export(groove_transformer.InputLayerEncoder, example, os.path.join("serializedONNX", model_name_+"_input_layer.onnx"), verbose=True, input_names=input_names, output_names=output_names)
            #
            # Export Encoder Layer
            input_names = [ "encoder_in" ]
            output_names = [ "encoder_out" ]
            example = torch.rand(32, 1, model_param_dict_["d_model"])

            first_encoder_layer = groove_transformer.Encoder.named_modules().__next__()[1]
            print(first_encoder_layer.named_modules().__next__()[1])
            torch.onnx.export(first_encoder_layer.named_modules().__next__()[1], example, os.path.join("serializedONNX", model_name_+"_encoder.onnx"), verbose=True, input_names=input_names, output_names=output_names)

            # Export Output Layer
            # input_names = [ "output_layer_in" ]
            # output_names = [ "output_layer_out" ]
            # example = torch.rand(1, 32, model_param_dict_["d_model"])
            # torch.onnx.export(groove_transformer.OutputLayer, example, os.path.join("serializedONNX", model_name_+"_output_layer.onnx"), verbose=True, input_names=input_names, output_names=output_names)


        # # validate with onnx
        # import onnx
        #
        # # Load the ONNX model
        # model = onnx.load(os.path.join("serializedONNX", model_name_+".onnx"))
        #
        # # Check that the model is well formed
        # onnx.checker.check_model(model)
        #
        # # Print a human readable representation of the graph
        # print(onnx.helper.printable_graph(model.graph))