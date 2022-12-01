

# Info
This repo has been generated from https://github.com/eyalamirmusic/JUCECmakeRepoPrototype
The only difference with the template above is that cmakelists and the xx_components.cpp have been updated so as to be able to use pytorch within the apps/plugins

# Installation

### Libtorch
STEP 1. Install libtorch (or if you have torch skip) - https://pytorch.org/cppdocs/installing.html

--------

1. go to https://pytorch.org/get-started/locally/ and get the C++ LibTorch Source

In my case:	https://download.pytorch.org/libtorch/cpu/libtorch-macos-1.10.2.zip

4. right click on Finder icon and click on "Go to Folder"

    <img width="182" alt="image" src="https://user-images.githubusercontent.com/35939495/182013337-4a1505b5-c50d-44dd-924d-9f54e9b4ea2c.png">

5. Write "~/" in the path to go to user folder

    <img width="460" alt="image" src="https://user-images.githubusercontent.com/35939495/182013369-9e9c1cd8-d70e-44d5-882f-c3f01854d46e.png">

6. Unzip the Libtorch file here

7. Go to the following path "~/libtorch/lib". Look for the "*.dylib* files and open each one manually

    <img width="1408" alt="image" src="https://user-images.githubusercontent.com/35939495/182013744-12ac339e-e01e-48a4-a247-b9873aba5e4d.png">	

### Trained models
STEP 2. Prepare the location of trained models

1. Go to /Library/ folder in root (/Library)

   ![img_1.png](img_1.png)

2. Create a folder called NeuralMidiFXPlugin
3. Move into this folder, and create a folder called  trained_models

4. Move all the model_x.pt files available in the repository to the /Library/NeuralMidiFXPlugin/trained_models folder
   

      .pt files can be found here
   
      NeuralMidiFXPlugin/NeuralMidiFXPlugin/PretrainedModelsSerialization/serialized
      
After the setup the folder should look like the following:

![img_2.png](img_2.png)

### Run CMAKE
load this project in your favorite IDE
(CLion/Visual Studio/VSCode/etc)
and click 'build' for one of the targets (templates, JUCE examples, Projucer, etc).

You can also generate a project for an IDE by using (Mac):
```
cmake -G Xcode -B build
```
Windows:
```
cmake -G "Visual Studio 16 2019" -B build
```
