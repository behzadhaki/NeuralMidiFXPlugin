### Description
In this directory, we find the python source code for serializing the GrooveTransformer models.


### Python Environment Setup (using venv)

create a virtual environment for installing the dependencies

    python3 -m venv TorchOSC_venv

Activate the environment

    source TorchOSC_venv/bin/activate

upgrade pip

    pip3 install --upgrade pip

goto https://pytorch.org/get-started/locally/ and get the write pip command for installing torch on your computer.
(double check the installers using the link)

    MAC and Win:
    pip3 install torch torchvision torchaudio

    Linux:
    pip3 install torch==1.10.2+cpu torchvision==0.11.3+cpu torchaudio==0.10.2+cpu -f https://download.pytorch.org/whl/cpu/torch_stable.html


### Running Python script

Go to CMC folder, and activate environment

    source TorchOSC_venv/bin/activate

Finally, run the python script
    
    SerializeModels.py


Create a directory (at root not user -- don't use ~/Li...)

    /Library/Groove2Drum/trained_models

Move all .pt files from /serialized folder to /trained_models folder just created

    copy from /serialized   -->  /Library/Groove2Drum/trained_models
 