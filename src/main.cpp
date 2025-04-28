#define RLBOX_WASM2C_MODULE_NAME ffmpeg__wrapper

#define RLBOX_SINGLE_THREADED_INVOCATIONS
#define RLBOX_USE_STATIC_CALLS() rlbox_wasm2c_sandbox_lookup_symbol

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cassert>
#include "ffmpeg_wrapper.h"
#include "ffmpeg_wasm2c.h"
#include "rlbox.hpp"
#include "rlbox_wasm2c_sandbox.hpp"

#define release_assert(cond, msg) if (!(cond)) { fputs(msg, stderr); abort(); }

using namespace std;
using namespace rlbox;

RLBOX_DEFINE_BASE_TYPES_FOR(ffmpeg__wrapper, wasm2c);


std::vector<uint8_t> load_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    std::vector<uint8_t> data ((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();
    return data;
}

int main(int argc, char const *argv[]) {

    std::ifstream file(argv[1]);
    std::string line;


    int num_lines = 0;
    float total_time = 0;

    while (std::getline(file, line)) {
        // Declare and create a new sandbox
        rlbox_sandbox_ffmpeg__wrapper sandbox;
        sandbox.create_sandbox();

        auto start_time = std::chrono::high_resolution_clock::now();

        auto file_data = load_file(line);
        if (file_data.size() == 0) continue;

        auto file_buffer = sandbox.malloc_in_sandbox<char>(file_data.size());
        memcpy(file_buffer.unverified_safe_pointer_because(file_data.size(), "writing to region"), file_data.data(), file_data.size());

        auto output_buffer = sandbox.malloc_in_sandbox<DecodedAudio>(1);

        auto ret = sandbox.invoke_sandbox_function(
            sandboxed_decode_audio,
            file_buffer,
            file_data.size(),
            output_buffer
        ).UNSAFE_unverified();

        sandbox.free_in_sandbox(output_buffer);
        sandbox.free_in_sandbox(file_buffer);
        auto end_time = std::chrono::high_resolution_clock::now();

        std::cout << line << ": " << (end_time - start_time).count() << std::endl;

        sandbox.destroy_sandbox();
    }
    file.close();


    return 0;
}