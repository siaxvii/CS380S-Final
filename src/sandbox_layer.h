#ifndef __SANDBOX_LAYER_H__
#define __SANDBOX_LAYER_H__

#include <torch/extension.h>

torch::Tensor load_tensor_from_file(const std::string &filename);

#endif // __SANDBOX_LAYER_H__