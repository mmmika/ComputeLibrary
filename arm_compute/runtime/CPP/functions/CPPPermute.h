/*
 * Copyright (c) 2017-2019 ARM Limited.
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
#ifndef ARM_COMPUTE_CPPPERMUTE_H
#define ARM_COMPUTE_CPPPERMUTE_H

#include "arm_compute/runtime/CPP/ICPPSimpleFunction.h"

#include "arm_compute/core/Types.h"

namespace arm_compute
{
class ITensor;

/** Basic function to run @ref CPPPermuteKernel */
class CPPPermute : public ICPPSimpleFunction
{
public:
    /** Configure the permute CPP kernel
     *
     * @param[in]  input  The input tensor to permute. Data types supported: U8/S8/QASYMM8/U16/S16/F16/U32/S32/F32
     * @param[out] output The output tensor. Data types supported: Same as @p input
     * @param[in]  perm   Permutation vector
     */
    void configure(const ITensor *input, ITensor *output, const PermutationVector &perm);
    /** Static function to check if given info will lead to a valid configuration of @ref CPPPermute
     *
     * @param[in] input  The input tensor to permute. Data types supported: U8/S8/QASYMM8/U16/S16/F16/U32/S32/F32
     * @param[in] output The output tensor. Data types supported: Same as @p input
     * @param[in] perm   Permutation vector
     *
     * @return a status
     */
    static Status validate(const ITensorInfo *input, const ITensorInfo *output, const PermutationVector &perm);
};
}
#endif /* ARM_COMPUTE_CPPPERMUTE_H */