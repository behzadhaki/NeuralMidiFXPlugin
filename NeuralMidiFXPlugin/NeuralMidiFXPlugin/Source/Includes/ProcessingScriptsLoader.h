//
// Created by Behzad Haki on 8/24/2023.
//

# pragma once


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
inline const char* default_processing_scripts_path = TOSTRING(DEFAULT_PROCESSING_SCRIPTS_DIR);
#if defined(_WIN32) || defined(_WIN64)
inline const char* path_separator = R"(\)";
#else
inline const char* path_separator = "/";
#endif

inline std::string stripQuotes(const std::string &input) {
    if (input.size() < 2) return input;
    if (input.front() == '"' && input.back() == '"') {
        return input.substr(1, input.size() - 2);
    }
    return input;
}

inline torch::jit::Module load_processing_script(std::string script_name) {

    auto subfolders = {"Models", "ProcessingScripts"};

    // first check if the script is available in the default path
    std::string script_path = stripQuotes(std::string(default_processing_scripts_path)) +
                              std::string(path_separator) +
                              script_name;

    ifstream myFile;
    myFile.open(script_path);
    if (myFile.is_open())
    {
        cout << "Processing Script found at: " + script_path << " -- Trying to load script..."
             << endl;
        myFile.close();
        return torch::jit::load(script_path);

    } else {
        // try to find the script in the subfolders
        for (auto subfolder : subfolders) {
            script_path = stripQuotes(std::string(default_processing_scripts_path)) +
                          std::string(path_separator) +
                          subfolder +
                          std::string(path_separator) +
                          script_name;
            myFile.open(script_path);
            if (myFile.is_open())
            {
                cout << "Processing Script found at: " + script_path << " -- Trying to load script..."
                     << endl;
                myFile.close();
                return torch::jit::load(script_path);
            }
        }
    }

    cout << "Processing Script not found in: " + script_path << endl;
    cout << "Make sure the script is available in TorchScripts/ OR TorchScripts/Models OR TorchScripts/ProcessingScripts  folders" << endl;

}
