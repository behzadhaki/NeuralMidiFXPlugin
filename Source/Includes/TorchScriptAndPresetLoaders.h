//
// Created by Behzad Haki on 8/24/2023.
//

# pragma once


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
inline const char* default_processing_scripts_path = TOSTRING(DEFAULT_PROCESSING_SCRIPTS_DIR);
inline const char* default_preset_dir = TOSTRING(DEFAULT_PRESET_DIR);
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

inline torch::jit::Module load_processing_script(const std::string& script_name) {
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

    } else {
        cout << "Processing Script not found at: " + script_path << endl;
        cout << "Make sure the script is available in TorchScripts/ProcessingScripts folder" << endl;
    }

    return torch::jit::load(script_path);
}

inline void save_tensor_map(const std::map<std::string, torch::Tensor>& m, const std::string& file_name) {
    std::string fp = stripQuotes(default_preset_dir) + path_separator + file_name;
    std::ofstream out_file(fp, std::ios::out | std::ios::binary);

    for (const auto& pair : m) {
        const std::string& key = pair.first;
        const torch::Tensor& t = pair.second;

        // Write the key
        auto key_size = (long long) key.size();
        out_file.write(reinterpret_cast<const char*>(&key_size), sizeof(key_size));
        out_file.write(key.c_str(), key_size);

        // Write tensor metadata
        const at::IntArrayRef& sizes = t.sizes();
        auto nb_dims = (int64_t) sizes.size();
        out_file.write(reinterpret_cast<const char*>(&nb_dims), sizeof(nb_dims));
        for (int64_t size : sizes) {
            out_file.write(reinterpret_cast<const char*>(&size), sizeof(int64_t));
        }

        auto scalar_type = static_cast<int64_t>(t.scalar_type());
        out_file.write(reinterpret_cast<const char*>(&scalar_type), sizeof(scalar_type));

        int64_t elem_size = t.element_size();
        out_file.write(reinterpret_cast<const char*>(&elem_size), sizeof(elem_size));

        int64_t num_el = t.numel();
        out_file.write(reinterpret_cast<const char*>(&num_el), sizeof(num_el));

        // Write tensor data
        out_file.write(reinterpret_cast<const char*>(t.data_ptr()), elem_size * num_el);

    }

    out_file.close();
}


inline std::map<std::string, torch::Tensor> load_tensor_map(const std::string& file_name) {
    std::string fp = stripQuotes(default_preset_dir) + path_separator + file_name;
    std::ifstream in_file(fp, std::ios::in | std::ios::binary);
    std::map<std::string, torch::Tensor> m;

    while (in_file.peek() != EOF) {
        // Read the key
        size_t key_size;
        in_file.read(reinterpret_cast<char*>(&key_size), sizeof(key_size));
        std::string key(key_size, '\0');
        in_file.read(&key[0], (long long) key_size);

        // Read tensor metadata
        int64_t nb_dims;
        in_file.read(reinterpret_cast<char*>(&nb_dims), sizeof(nb_dims));

        std::vector<int64_t> dims(nb_dims);
        for (int64_t& dim : dims) {
            in_file.read(reinterpret_cast<char*>(&dim), sizeof(int64_t));
        }

        int64_t scalar_type;
        in_file.read(reinterpret_cast<char*>(&scalar_type), sizeof(scalar_type));

        int64_t elem_size;
        in_file.read(reinterpret_cast<char*>(&elem_size), sizeof(elem_size));

        int64_t num_els;
        in_file.read(reinterpret_cast<char*>(&num_els), sizeof(num_els));

        // Read the tensor data
        std::vector<uint8_t> buffer(elem_size * num_els);
        in_file.read(reinterpret_cast<char*>(buffer.data()), elem_size * num_els);
        torch::Tensor t = torch::from_blob(buffer.data(), at::IntArrayRef(dims), static_cast<torch::ScalarType>(scalar_type)).clone();

        // Assign the tensor to the map
        m[key] = t;
    }

    return m;
}

#include <mutex>

class CustomPresetDataDictionary
{
private:
    std::map<std::string, torch::Tensor> tensorMap;
    std::map<std::string, bool> changeFlags;  // Track changes for each key
    mutable std::mutex mutex;
    bool changed = false;
public:
    CustomPresetDataDictionary() = default;

    std::vector<std::string> keys() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<std::string> keys;
        for (const auto& pair : tensorMap) {
            keys.push_back(pair.first);
        }
        return keys;
    }

    std::vector<torch::Tensor> values() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<torch::Tensor> values;
        for (const auto& pair : tensorMap) {
            values.push_back(pair.second);
        }
        return values;
    }

    std::vector<std::pair<std::string, torch::Tensor>> items() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<std::pair<std::string, torch::Tensor>> items;
        for (const auto& pair : tensorMap) {
            items.emplace_back(pair);
        }
        return items;
    }

    // Method to update the map (thread-safe)
    void tensor(const std::string& tensorLabel, const torch::Tensor& tensor) {
        std::lock_guard<std::mutex> lock(mutex);
        tensorMap[tensorLabel] = tensor;
        // changeFlags[tensorLabel] = true;  // Mark this key as changed
    }

    // Get tensorData by label and reset the change flag for that key
    // returns  nullopt if the key is not found
    std::optional<torch::Tensor> tensor(const std::string& tensorLabel) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = tensorMap.find(tensorLabel);
        if (it != tensorMap.end()) {
            changeFlags[tensorLabel] = false;  // Reset the flag for this key
            return it->second;
        }
        return std::nullopt;
    }

    // Method to check if the content for any of the keys has changed
    bool hasTensorDataChanged() {
        std::lock_guard<std::mutex> lock(mutex);
        return std::any_of(changeFlags.begin(), changeFlags.end(), [](const auto& pair) { return pair.second; });
    }

    // Method to access the map (thread-safe)
    std::map<std::string, torch::Tensor>& operator()() {
        std::lock_guard<std::mutex> lock(mutex);
        return tensorMap;
    }

    // Method to get the map (thread-safe)
    std::map<std::string, torch::Tensor> tensors() {
        std::lock_guard<std::mutex> lock(mutex);
        // Reset all change flags
        for (auto& pair : changeFlags) {
            pair.second = false;
        }
        return tensorMap;
    }

    // update content from a map<string, tensor> (thread-safe)
    void copy_from_map(const std::map<std::string, torch::Tensor>& m) {
        std::lock_guard<std::mutex> lock(mutex);
        tensorMap = m;
        for (const auto& pair : m) {
            const std::string& key = pair.first;
            changeFlags[key] = true;  // Mark all keys as changed
        }
        changed = true;
    }

    // Print the map (thread-safe)
    void printTensorMap() {
        std::lock_guard<std::mutex> lock(mutex);
        for (const auto& pair : tensorMap) {
            const std::string& key = pair.first;
            const torch::Tensor& t = pair.second;
            std::cout << key << ": Tensor Dim:" << t.dim() << " Size: " << t.sizes() <<
            " Type: " << t.scalar_type() << "Min: " << t.min() << " Max: " << t.max() << "Mean: " << t.mean() << std::endl;
        }
    }

    // compare two CustomPresetDataDictionary
    bool operator==(CustomPresetDataDictionary other) {
        printTensorMap();
        other.printTensorMap();

        std::lock_guard<std::mutex> lock(mutex);
        std::lock_guard<std::mutex> lock2(other.mutex);

        // check if all keys are the same
        for (const auto& pair : tensorMap) {
            const std::string& key = pair.first;
            if (other.tensorMap.find(key) == other.tensorMap.end()) return false;
        }

        for (const auto& pair : other.tensorMap) {
            const std::string& key = pair.first;
            if (tensorMap.find(key) == tensorMap.end()) return false;
        }

        // check if all values are the same
        for (const auto& pair : tensorMap) {
            const std::string& key = pair.first;
            const torch::Tensor& t = pair.second;
            if (!torch::equal(t, other.tensorMap.at(key))) return false;
        }

        return true;
    }

    // copy constructor
    CustomPresetDataDictionary(const CustomPresetDataDictionary& other) {
        std::lock_guard<std::mutex> lock(other.mutex);
        tensorMap = other.tensorMap;
        changeFlags = other.changeFlags;
        changed = true;
    }

    // copy assignment operator
    CustomPresetDataDictionary& operator=(const CustomPresetDataDictionary& other) {
        std::lock_guard<std::mutex> lock(mutex);
        std::lock_guard<std::mutex> lock2(other.mutex);
        tensorMap = other.tensorMap;
        changeFlags = other.changeFlags;
        changed = true;
        return *this;
    }
};