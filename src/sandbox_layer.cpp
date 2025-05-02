#define RLBOX_WASM2C_MODULE_NAME ffmpeg__wrapper

#define RLBOX_SINGLE_THREADED_INVOCATIONS
#define RLBOX_USE_STATIC_CALLS() rlbox_wasm2c_sandbox_lookup_symbol

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <cassert>

#include <torch/extension.h>

#include "ffmpeg_wrapper.h"
#include "sandbox_layer.h"
#include "ffmpeg_wasm2c.h"

#include "rlbox.hpp"
#include "rlbox_wasm2c_sandbox.hpp"

#define release_assert(cond, msg) if (!(cond)) { fputs(msg, stderr); abort(); }

using namespace std;
using namespace rlbox;

RLBOX_DEFINE_BASE_TYPES_FOR(ffmpeg__wrapper, wasm2c);


torch::Tensor load_tensor_from_file(const std::string &filename) {

    // Create sandbox
    rlbox_sandbox_ffmpeg__wrapper sandbox;
    sandbox.create_sandbox();

    // Load audio data into byte vector
    std::ifstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("Failed to open file: " + filename);

    std::vector<char> file_data ((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    if (file_data.size() == 0) return torch::Tensor();

    // Copy file data into sandbox
    rlbox::tainted<char*, rlbox::rlbox_wasm2c_sandbox> file_buffer = sandbox.malloc_in_sandbox<char>(file_data.size());
    memcpy(file_buffer.unverified_safe_pointer_because(file_data.size(), "writing to region"), file_data.data(), file_data.size());

    // Initialize output buffer
    rlbox::tainted<uint8_t**, rlbox::rlbox_wasm2c_sandbox> out_data =
    sandbox.malloc_in_sandbox<uint8_t*>(1);
    rlbox::tainted<size_t*, rlbox::rlbox_wasm2c_sandbox> out_size =
    sandbox.malloc_in_sandbox<size_t>(1);

    // Invoke sanboxed FFmpeg pipeline
    sandbox.invoke_sandbox_function(
        sandboxed_decode_audio,
        file_buffer,
        file_data.size(),
        out_data,
        out_size
    );

    std::cout << out_data.UNSAFE_unverified() << std::endl;
    std::cout << out_size.UNSAFE_unverified() << std::endl;

    bool copied = false;
    auto thing = rlbox::copy_memory_or_deny_access(
        sandbox, out_size, 1, true, copied);

    std::cout << "thing: " << thing;

    auto thing2 = rlbox::copy_memory_or_deny_access(
        sandbox, thing, 1, true, copied);

    std::cout << "thing: " << thing2;
    return torch::ones({2, 3});


    // Release allocated resources
    sandbox.free_in_sandbox(out_data);
    sandbox.free_in_sandbox(file_buffer);

    // Teardown sandbox
    sandbox.destroy_sandbox();
    return torch::ones({2, 3});
}
