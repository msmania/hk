#include <ntifs.h>
#include <stdint.h>

#include "common.h"
#include "heap.h"

Heap::Heap(HANDLE process, PVOID desiredBase, size_t size) : process_(process), base_{} {
  desiredBase = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(desiredBase) & ~0xffff);
  for (int i = 0; i < 100; ++i) {
    desiredBase = at<void*>(desiredBase, 0x10000);
    NTSTATUS status = ZwAllocateVirtualMemory(
      process,
      &desiredBase,
      /*ZeroBits*/0,
      &size,
      MEM_RESERVE|MEM_COMMIT,
      PAGE_READWRITE);
    if (status == STATUS_CONFLICTING_ADDRESSES) continue;
    if (!NT_SUCCESS(status))  {
      Log("ZwAllocateVirtualMemory failed - %08lx\n", status);
      base_ = nullptr;
      return;
    }

    base_ = desiredBase;
    //Log("Allocated: %p\n", desiredBase);
    return;
  }

  Log("Can't find a sweetspot to allocate.  Giving up.\n");
}

Heap::~Heap() {
  if (base_) {
    size_t size = 0;
    ZwFreeVirtualMemory(process_, &base_, &size, MEM_RELEASE);
  }
}

Heap::operator bool() const {
  return !!base_;
}

Heap::operator void*() {
  return base_;
}

void* Heap::Detach() {
  auto base = base_;
  base_ = nullptr;
  return base;
}