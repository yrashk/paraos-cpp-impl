export module kernel.pmm;

import libpara.concepts;
import libpara.formatting;
import libpara.err;
import libpara.basic_types;
import libpara.sync;

using namespace libpara::err;
using namespace libpara::basic_types;
using namespace libpara::sync;

#include <err.hpp>

export void *operator new(unsigned long sz, void *ptr) { return ptr; }

export namespace kernel::pmm {

template <typename T>
concept allocator = requires(T a, usize size, usize alignment) {
  {a.allocate(size, alignment)};
};

const auto OutOfMemoryError = Error("OutOfMemory");
const auto OverlappedMemoryError = Error("OverlappedMemory");

class Allocator {

public:
  virtual Result<void *> allocate(usize size, usize alignment) = 0;
  virtual usize availableMemory() = 0;

  virtual bool overlaps(void *) { return false; }

protected:
  inline usize alignDown(usize value, usize alignment) {
    return value - (value % alignment);
  }

  inline usize alignUp(usize value, usize alignment) {
    return alignDown(value + alignment - 1, alignment);
  }

  inline bool isAligned(usize value, usize alignment) {
    return alignDown(value, alignment) == value;
  }
};

class WatermarkAllocator : public Allocator {

  void *ptr = nullptr;
  usize sz = 0;
  usize watermark = 0;
  libpara::sync::Lock lock;

public:
  constexpr WatermarkAllocator() {}
  constexpr WatermarkAllocator(void *ptr, usize size) : ptr(ptr), sz(size) {}

  virtual Result<void *> allocate(usize size, usize alignment) {
    auto guard = LockGuard(lock);
    usize ptr_addr = reinterpret_cast<usize>(ptr);
    auto pending_watermark =
        alignUp(ptr_addr + watermark, alignment) - ptr_addr;
    auto new_watermark = pending_watermark + size;
    if (new_watermark > sz)
      return OutOfMemoryError;
    auto result = ptr_addr + pending_watermark;
    watermark = new_watermark;
    return reinterpret_cast<void *>(result);
  }

  virtual usize availableMemory() { return sz - watermark; }

  virtual bool overlaps(void *another_ptr) {
    usize ptr_addr = reinterpret_cast<usize>(ptr);
    usize another_ptr_addr = reinterpret_cast<usize>(another_ptr);
    return another_ptr_addr >= ptr_addr && another_ptr_addr <= (ptr_addr + sz);
  }

  inline bool overlaps(WatermarkAllocator &allocator) {
    return overlaps(allocator.ptr);
  }
};

template <allocator A, int sz> class ChainedAllocator : public Allocator {

  A allocators[sz] = {};
  int added_allocators = 0;

public:
  constexpr ChainedAllocator() {}

  Result<int> addAllocator(A &&allocator) {
    if (added_allocators + 1 > sz) {
      return OutOfMemoryError;
    }
    for (int i = 0; i < added_allocators; i++) {
      if (allocators[i].overlaps(allocator))
        return OverlappedMemoryError;
    }
    allocators[added_allocators] = allocator;
    added_allocators++;
    return added_allocators;
  }

  A &getAllocator(int index) { return allocators[index]; }

  virtual Result<void *> allocate(usize size, usize alignment) {
    for (int i = 0; i < added_allocators; i++) {
      auto alloc = allocators[i].allocate(size, alignment);
      if (alloc.success || alloc != OutOfMemoryError)
        return alloc;
    }
    return OutOfMemoryError;
  }

  virtual usize availableMemory() {
    usize available = 0;
    for (int i = 0; i < added_allocators; i++) {
      available += allocators[i].availableMemory();
    }
    return available;
  }

  virtual bool overlaps(void *another_ptr) {
    for (int i = 0; i < added_allocators; i++) {
      if (allocators[i].overlaps(another_ptr))
        return true;
    }
    return false;
  }
};

template <typename T, allocator A>
Result<T *> allocate(A &allocator,
                     decltype(alignof(T)) alignment = alignof(T)) {
  return reinterpret_cast<T *>(
      tryUnwrap(allocator.allocate(sizeof(T), alignment)));
}

} // namespace kernel::pmm

#include <testing.hpp>

import libpara.testing;

export namespace kernel::pmm::tests {

class TestCase : public libpara::testing::TestCase {

public:
  using libpara::testing::TestCase::TestCase;

  virtual void run() {
    test("WatermarkAllocator allocation");
    {
      usize base = 0x0;
      auto alloc =
          WatermarkAllocator(reinterpret_cast<void *>(base), 1024 * 1024);
      Expect(alloc.availableMemory() == 1024 * 1024);
      Result<u64 *> a = kernel::pmm::allocate<u64>(alloc);
      Expect(a.success);
      Expect(reinterpret_cast<usize>(*a) - base == 0);
      Expect(alloc.availableMemory() == 1024 * 1024 - sizeof(u64));
    }

    test("WatermarkAllocator allocation with custom alignment");
    {
      usize base = 0x0;
      auto alloc =
          WatermarkAllocator(reinterpret_cast<void *>(base), 1024 * 1024);
      Expect(alloc.availableMemory() == 1024 * 1024);
      Result<u8 *> a = kernel::pmm::allocate<u8>(alloc);
      Expect(a.success);
      Expect(reinterpret_cast<usize>(*a) - base == 0);
      Expect(alloc.availableMemory() == 1024 * 1024 - sizeof(u8));

      Result<u64 *> b = kernel::pmm::allocate<u64>(alloc, 1);
      Expect(reinterpret_cast<usize>(*b) - base == 1);
      Expect(alloc.availableMemory() == 1024 * 1024 - sizeof(u64) - sizeof(u8));
    }

    test("ChainedAllocator");
    {
      auto alloc = ChainedAllocator<WatermarkAllocator, 2>();
      Expect(alloc
                 .addAllocator(
                     WatermarkAllocator(reinterpret_cast<void *>(0x0), 1024))
                 .success);
      Expect(alloc
                 .addAllocator(
                     WatermarkAllocator(reinterpret_cast<void *>(0x1000), 1024))
                 .success);
      Expect(alloc.availableMemory() == 1024 * 2);
      kernel::pmm::allocate<u8[1024]>(alloc);
      Expect(alloc.getAllocator(0).availableMemory() == 0);
      Expect(alloc.availableMemory() == 1024);
      kernel::pmm::allocate<u8[1024]>(alloc);
      Expect(alloc.availableMemory() == 0);
      Expect(alloc.getAllocator(1).availableMemory() == 0);
      Expect(!kernel::pmm::allocate<u8[1024]>(alloc).success);
    }
  }
};
} // namespace kernel::pmm::tests

