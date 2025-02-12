//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-22, Lawrence Livermore National Security, LLC and Umpire
// project contributors. See the COPYRIGHT file for details.
//
// SPDX-License-Identifier: (MIT)
//////////////////////////////////////////////////////////////////////////////
#ifndef UMPIRE_DeviceAllocator_HPP
#define UMPIRE_DeviceAllocator_HPP

#include <string.h>

#include "umpire/Allocator.hpp"

namespace umpire {

/*!
 * \brief Lightweight allocator for use in GPU code
 */
class DeviceAllocator {
 public:
  /*
   * Declare friend function that will call constructor.
   */
  friend __host__ DeviceAllocator make_device_allocator(Allocator allocator, size_t size, const std::string& name);

  __host__ __device__ ~DeviceAllocator();

  __host__ __device__ DeviceAllocator(const DeviceAllocator& other);

  /*
   * \brief Allocate size objects of type T.
   *
   * \param size The number of objects to allocate.
   *
   * \return Pointer to the start of the allocated memory.
   */
  __device__ void* allocate(size_t size);

  /*
   * \brief Get the id associated with the
   *  current DeviceAllocator object.
   *
   * \return Returns the id.
   */
  __host__ __device__ int getID();

  /*
   * \brief Get the name associated with the
   *  current DeviceAllocator object.
   *
   * \return Returns the name.
   */
  __host__ __device__ const char* getName();

  /*
   * \brief Deallocate memory associated with Device Allocator object.
   */
  __host__ void destroy();

  /*
   * \brief Determine if this object has been initialized or not.
   *
   * \return Returns true if object has been initialized, false otherwise.
   */
  __host__ __device__ bool isInitialized();

  /*
   * \brief Reset the DeviceAllocator counter pointer back to original
   *  starting point. This will effectively allow the DeviceAllocator to
   *  start overwritting old data.
   *
   *  NOTE: USER BEWARE!! If used incorrectly, this function could end up
   *  causing severe problems. Use with care.
   */
  __host__ __device__ void reset();

 private:
  /*!
   * \brief Construct a new DeviceAllocator that will use allocator to allocate
   * data. Called by make_device_allocator helper function only.
   *
   * \param allocator Allocator to use for allocating memory.
   * \param size Total amount of memory that the DeviceAllocator will have.
   * \param name Name of the DeviceAllocator object.
   * \param id ID associated with this DeviceAllocator object. ID will be used
   *   to reference UMPIRE_DEV_ALLOCS array if necessary.
   */
  __host__ DeviceAllocator(Allocator allocator, size_t size, const std::string& name, int id);

  umpire::Allocator m_allocator;

  char* m_ptr;
  char m_name[64];
  unsigned int* m_counter;

  int m_id;
  size_t m_size;
  bool m_child;
};

} // end of namespace umpire

#endif // UMPIRE_DeviceAllocator_HPP
