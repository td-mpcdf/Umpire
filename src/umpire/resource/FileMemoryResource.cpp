//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-22, Lawrence Livermore National Security, LLC and Umpire
// project contributors. See the COPYRIGHT file for details.
//
// SPDX-License-Identifier: (MIT)
//////////////////////////////////////////////////////////////////////////////

#include "umpire/resource/FileMemoryResource.hpp"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "umpire/util/Platform.hpp"

#if defined(UMPIRE_ENABLE_CUDA)
#include <cuda_runtime_api.h>
#endif

namespace umpire {
namespace resource {

int FileMemoryResource::s_file_counter{0};

FileMemoryResource::FileMemoryResource(Platform platform, const std::string& name, int id, MemoryResourceTraits traits)
    : MemoryResource{name, id, traits}, m_platform{platform}, m_size_map{}
{
}

FileMemoryResource::~FileMemoryResource()
{
  std::vector<void*> leaked_items;

  for (auto const& m : m_size_map) {
    leaked_items.push_back(m.first);
  }

  for (auto const& p : leaked_items) {
    deallocate(p, 0);
  }
}

void* FileMemoryResource::allocate(std::size_t bytes)
{
  // Find output file directory for mmap files
  const char* memory_file_dir{std::getenv("UMPIRE_MEMORY_FILE_DIR")};
  std::string default_dir = "./";
  if (memory_file_dir) {
    default_dir = memory_file_dir;
  }

  // Create name and open file
  std::stringstream ss;
  ss << default_dir << "umpire_mem_" << getpid() << "_" << s_file_counter;
  s_file_counter++;

  int fd{open(ss.str().c_str(), O_RDWR | O_CREAT | O_LARGEFILE, S_IRWXU)};
  if (fd == -1) {
    UMPIRE_ERROR("Opening File { " << ss.str() << " } Failed: " << strerror(errno));
  }

  // Setting Size Of Map File
  const std::size_t pagesize{(std::size_t)sysconf(_SC_PAGE_SIZE)};
  std::size_t rounded_bytes{((bytes + (pagesize - 1)) / pagesize) * pagesize};

  // Truncate file
  int trun{ftruncate64(fd, rounded_bytes)};
  if (trun == -1) {
    int errno_save = errno;
    remove(ss.str().c_str());
    UMPIRE_ERROR("truncate64 Of File { " << ss.str() << " } Failed: " << strerror(errno_save));
  }

  // Using mmap
  void* ptr{mmap(NULL, rounded_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)};
  if (ptr == MAP_FAILED) {
    int errno_save = errno;
    remove(ss.str().c_str());
    UMPIRE_ERROR("mmap Of " << rounded_bytes << " To File { " << ss.str() << " } Failed: " << strerror(errno_save));
  }

  // Storing Information On File
  std::pair<const std::string, std::size_t> info{std::make_pair(ss.str(), rounded_bytes)};
  m_size_map.insert(ptr, info);

  close(fd);
  return ptr;
}

void FileMemoryResource::deallocate(void* ptr, std::size_t UMPIRE_UNUSED_ARG(size))
{
  // Find information about ptr for deallocation
  auto iter = m_size_map.find(ptr);
  // Unmap File
  if (munmap(iter->first, iter->second->second) < 0) {
    UMPIRE_ERROR("munmap Of File { " << iter->second->first.c_str() << " } Failed:" << strerror(errno));
  }
  // Remove File
  if (remove(iter->second->first.c_str()) < 0) {
    UMPIRE_ERROR("remove Of File { " << iter->second->first.c_str() << " } Failed: " << strerror(errno));
  }
  // Remove Information about file in m_size_map
  m_size_map.erase(iter->first);
}

std::size_t FileMemoryResource::getCurrentSize() const noexcept
{
  return 0;
}

std::size_t FileMemoryResource::getHighWatermark() const noexcept
{
  return 0;
}

bool FileMemoryResource::isPageable() noexcept
{
#if defined(UMPIRE_ENABLE_CUDA)
  int pageableMem = 0;
  int cdev = 0;
  cudaGetDevice(&cdev);

  // Device supports coherently accessing pageable memory
  // without calling cudaHostRegister on it
  cudaDeviceGetAttribute(&pageableMem, cudaDevAttrPageableMemoryAccess, cdev);
  if (pageableMem)
    return true;
#endif
  // Note: Regarding omp_target, we pick a default of false here
  // until we can better determine which device omp_offload is using.
  return false;
}

bool FileMemoryResource::isAccessibleFrom(Platform p) noexcept
{
  if (p == Platform::host)
    return true;
  else if (p == Platform::cuda) // TODO: Implement omp_target specific test
    return isPageable();
  else
    return false;
}

Platform FileMemoryResource::getPlatform() noexcept
{
  return m_platform;
}

} // end of namespace resource
} // end of namespace umpire
