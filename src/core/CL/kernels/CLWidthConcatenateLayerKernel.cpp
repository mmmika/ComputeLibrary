/*
 * Copyright (c) 2018-2020 Arm Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "arm_compute/core/CL/kernels/CLWidthConcatenateLayerKernel.h"

#include "arm_compute/core/CL/CLHelpers.h"
#include "arm_compute/core/CL/CLKernelLibrary.h"
#include "arm_compute/core/CL/CLValidate.h"
#include "arm_compute/core/CL/ICLTensor.h"
#include "arm_compute/core/Helpers.h"
#include "arm_compute/core/Utils.h"
#include "arm_compute/core/Window.h"
#include "arm_compute/core/utils/misc/Cast.h"
#include "arm_compute/core/utils/misc/ShapeCalculator.h"

#include "support/StringSupport.h"

namespace arm_compute
{
namespace
{
constexpr unsigned int num_elems_processed_per_iteration = 16;

std::pair<Status, Window> validate_and_configure_window(ITensorInfo *input, unsigned int width_offset, ITensorInfo *output)
{
    // The window needs to be based on input as we copy all the widths of input
    Window                 win = calculate_max_window(*input, Steps(num_elems_processed_per_iteration));
    AccessWindowHorizontal input_access(input, 0, num_elems_processed_per_iteration);
    AccessWindowHorizontal output_access(output, width_offset, num_elems_processed_per_iteration);
    bool                   window_changed = update_window_and_padding(win, input_access, output_access);

    Window win_collapsed = win.collapse(win, Window::DimZ);

    Status err = (window_changed) ? ARM_COMPUTE_CREATE_ERROR(ErrorCode::RUNTIME_ERROR, "Insufficient Padding!") : Status{};
    return std::make_pair(err, win_collapsed);
}
Status validate_arguments(const ITensorInfo *input, unsigned int width_offset, const ITensorInfo *output)
{
    ARM_COMPUTE_RETURN_ERROR_ON_NULLPTR(input, output);
    ARM_COMPUTE_RETURN_ERROR_ON_F16_UNSUPPORTED(input);
    ARM_COMPUTE_RETURN_ERROR_ON(input->data_type() == DataType::UNKNOWN);

    ARM_COMPUTE_RETURN_ERROR_ON_MISMATCHING_DATA_TYPES(input, output);
    ARM_COMPUTE_RETURN_ERROR_ON(input->dimension(0) + width_offset > output->dimension(0));

    for(size_t i = 1; i < Coordinates::num_max_dimensions; ++i)
    {
        ARM_COMPUTE_RETURN_ERROR_ON(input->dimension(i) != output->dimension(i));
    }
    ARM_COMPUTE_RETURN_ERROR_ON(input->num_dimensions() > 4);

    return Status{};
}
} // namespace

CLWidthConcatenateLayerKernel::CLWidthConcatenateLayerKernel()
    : _width_offset(0)
{
}

Status CLWidthConcatenateLayerKernel::validate(const ITensorInfo *input, unsigned int width_offset, const ITensorInfo *output)
{
    ARM_COMPUTE_RETURN_ON_ERROR(validate_arguments(input, width_offset, output));
    ARM_COMPUTE_RETURN_ON_ERROR(validate_and_configure_window(input->clone().get(), width_offset, output->clone().get()).first);
    return Status{};
}

void CLWidthConcatenateLayerKernel::configure(const CLCompileContext &compile_context, ITensorInfo *input, unsigned int width_offset, ITensorInfo *output)
{
    ARM_COMPUTE_ERROR_ON_NULLPTR(input, output);
    ARM_COMPUTE_ERROR_THROW_ON(validate_arguments(input, width_offset, output));

    _width_offset = width_offset;

    // Add build options
    CLBuildOptions build_opts;
    build_opts.add_option("-DDATA_TYPE=" + get_cl_type_from_data_type(input->data_type()));
    build_opts.add_option("-DVEC_SIZE=" + support::cpp11::to_string(num_elems_processed_per_iteration));
    build_opts.add_option("-DWIDTH_OFFSET=" + support::cpp11::to_string(_width_offset));
    build_opts.add_option("-DDEPTH=" + support::cpp11::to_string(input->dimension(2)));

    if(is_data_type_quantized_asymmetric(input->data_type()) && input->quantization_info() != output->quantization_info())
    {
        const UniformQuantizationInfo iqinfo = input->quantization_info().uniform();
        const UniformQuantizationInfo oqinfo = output->quantization_info().uniform();

        build_opts.add_option("-DOFFSET_IN1=" + float_to_string_with_full_precision(iqinfo.offset));
        build_opts.add_option("-DOFFSET_OUT=" + float_to_string_with_full_precision(oqinfo.offset));
        build_opts.add_option("-DSCALE_IN1=" + float_to_string_with_full_precision(iqinfo.scale));
        build_opts.add_option("-DSCALE_OUT=" + float_to_string_with_full_precision(oqinfo.scale));
    }

    // Create kernel
    _kernel = create_kernel(compile_context, "concatenate_width", build_opts.options());
    // Configure kernel window
    auto win_config = validate_and_configure_window(input, width_offset, output);
    ARM_COMPUTE_ERROR_THROW_ON(std::get<0>(win_config));

    ICLKernel::configure_internal(std::get<1>(win_config));

    // Set output valid region
    output->set_valid_region(ValidRegion(Coordinates(), output->tensor_shape()));
}

void CLWidthConcatenateLayerKernel::run_op(ITensorPack &tensors, const Window &window, cl::CommandQueue &queue)
{
    ARM_COMPUTE_ERROR_ON_UNCONFIGURED_KERNEL(this);
    ARM_COMPUTE_ERROR_ON_INVALID_SUBWINDOW(ICLKernel::window(), window);

    const auto src = utils::cast::polymorphic_downcast<const ICLTensor *>(tensors.get_const_tensor(TensorType::ACL_SRC));
    auto       dst = utils::cast::polymorphic_downcast<ICLTensor *>(tensors.get_tensor(TensorType::ACL_DST));

    unsigned int idx = 0;
    add_4D_tensor_argument(idx, src, window);
    add_4D_tensor_argument(idx, dst, window);
    enqueue(queue, *this, window, lws_hint());
}
} // namespace arm_compute
