//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-22, Lawrence Livermore National Security, LLC and Umpire
// project contributors. See the COPYRIGHT file for details.
//
// SPDX-License-Identifier: (MIT)
//////////////////////////////////////////////////////////////////////////////
#include "umpire/op/CudaCopyOperation.hpp"

#include <cuda_runtime_api.h>

#include "umpire/util/Macros.hpp"

namespace umpire {
namespace op {

void CudaCopyOperation::transform(void* src_ptr, void** dst_ptr,
                                  umpire::util::AllocationRecord* UMPIRE_UNUSED_ARG(src_allocation),
                                  umpire::util::AllocationRecord* UMPIRE_UNUSED_ARG(dst_allocation), std::size_t length)
{
  cudaError_t error = ::cudaMemcpy(*dst_ptr, src_ptr, length, cudaMemcpyDeviceToDevice);

  if (error != cudaSuccess) {
    UMPIRE_ERROR("cudaMemcpy( dest_ptr = " << *dst_ptr << ", src_ptr = " << src_ptr << ", length = " << length
                                           << ", cudaMemcpyDeviceToDevice ) failed with error: "
                                           << cudaGetErrorString(error));
  }
}

camp::resources::EventProxy<camp::resources::Resource> CudaCopyOperation::transform_async(
    void* src_ptr, void** dst_ptr, util::AllocationRecord* UMPIRE_UNUSED_ARG(src_allocation),
    util::AllocationRecord* UMPIRE_UNUSED_ARG(dst_allocation), std::size_t length, camp::resources::Resource& ctx)
{
  auto device = ctx.get<camp::resources::Cuda>();
  auto stream = device.get_stream();

  cudaError_t error = ::cudaMemcpyAsync(*dst_ptr, src_ptr, length, cudaMemcpyDeviceToDevice, stream);

  if (error != cudaSuccess) {
    UMPIRE_ERROR("cudaMemcpy( dest_ptr = " << *dst_ptr << ", src_ptr = " << src_ptr << ", length = " << length
                                           << ", cudaMemcpyDeviceToHost ) failed with error: "
                                           << cudaGetErrorString(error));
  }

  return camp::resources::EventProxy<camp::resources::Resource>{ctx};
}

} // end of namespace op
} // end of namespace umpire
