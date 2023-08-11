# Preparing your model

Note: This is a modified version of the [NeutoneMIDI SDK](https://github.com/QosmoInc/neutone_sdk/tree/neutone_midi) guidelines on serializing a model


Serializing a model in to [Torchscript](https://pytorch.org/tutorials/beginner/Intro_to_TorchScript_tutorial.html)
allows the operations to be replicated in a C++ environment,
such as those used in Digital Audio Workstations. Generally this process is done at the end of the research process, 
after a model has been fully trained. With that said,
we highly recommend attempting to serialize your model *before* training it, to ensure that there are no issues ahead of time. 

## Methods
There are two ways to serialize a PyTorch model:
- [Scripting](https://pytorch.org/docs/stable/generated/torch.jit.script.html)
- [Tracing](https://pytorch.org/docs/stable/generated/torch.jit.script.html)

So what is the difference? **Tracing** is a static method, in which a single 'dummy' input is provided to the model,
and the resultant operations are traced. This can lead to issues; if there is a dynamic logic-flow, such as an 'if'
statement, the recording operation will only capture whichever path the dummy input triggered. **Scripting**,
on the other hand, is more robust and can theoretically capture any number of dynamic operations. 

We therefor recommend scripting whenever possible, as it is more likely to produce accurate results.
However, in some circumstances tracing is the only option. For example, we have found that 
[HuggingFace](https://huggingface.co/docs/transformers/v4.17.0/en/serialization) models only 
support Tracing. 

### Scripting a model
In case the entire functionality of your model is encoded in the forward() function:
```angular2html
trained_model = MyModel(init_args)
scripted_model = torch.jit.script(trained_model)
torch.jit.save(scripted_model, "filename.pt")
```

You can combine multiple models / functionalities by combining them into a single forward
function of a new meta-model, and then scripting it. This is particularly useful when your 
model has a sampling process that is separate of the forward() function. 

```angular2html
class Sampler(torch.nn.Module):
    def __init__(self, args):
        super(args, self).__init__()

    def forward(self, x):
        # Here you can specify any operations needed for sampling from the output of the model
        y = x + 1
        return y

class FullModel(torch.nn.Module):
    def __init__(self, trained_model, sampler):
        super(self, trained_model, sampler).__init__()
        self.model = trained_model
        self.sampler = sampler

    def forward(self, x):
        # The full process occurs here
        logits = self.model(x)
        output = self.sampler(logits)
        return output

# Create the model
trained_model = MyModel(init_args)
sampler = Sampler(init_args)
full_model = FullModel(trained_model, sampler)

# Now you can script it all together
scripted_model = torch.jit.script(full_model)
torch.jit.save(scripted_model, "filename.pt")

```

### Tracing a model

Below is an example of how to trace a HuggingFace GPT-2 model:

```angular2html
with open(os.path.join(train_path, "config.json")) as fp:
    config = json.load(fp)
vocab_size = config["vocab_size"]
dummy_input = torch.randint(0, vocab_size, (1, 2048))
partial_model = GPT2LMHeadModel.from_pretrained(train_path, torchscript=True)
traced_model = torch.jit.trace(partial_model, example_inputs=dummy_input)
torch.jit.save(traced_model, "traced_model.pt")
```

Notably, you can combine a Traced module with other components and then Script it. This is helpful
in the above case, as the 'Generate' function requires dynamic processes that cannot be captured with
tracing. Using the combine method detailed above, you can load this Traced module alongside a custom
Generate/Sample function, and then script them all together. 

To be clear, we suggest scripting a model whenever possible. With tracing, it will record
the exact set of operations that are performed on the dummy input. There are much 
higher likelihoods of missing important parts of the model's functionality when tracing. 

## Common Issues

Scripting a custom model can be a tricky endeavor. We have identified a couple common issues:

#### Custom Functions & Type Hinting

When writing a function that is not already included in the PyTorch library (i.e. forward()) you will 
often need to provide [type hints](https://docs.python.org/3/library/typing.html). Only a specific number
of data types are supported, see the [torchscript language reference](https://pytorch.org/docs/stable/jit_language_reference.html)
for a full breakdown. 

#### Library imports
You cannot use 3rd party libraries (other than PyTorch) in your model class. Any extra data will need to be saved
in **self** via a native python data type. For example, if you need to read a json file, then do so while initializing
the model and save the data in another format, such as a dict. 

#### Nested Functions
Any nested function in your model will prevent serialization. Be sure to define every function at the top-level
of the class. With that said, there is no issue in calling a function within another function:

```asm
def encode(self, x):
    return x + 1

def forward(self, x, y):
    z = self.encode(x)
    return z + y
```