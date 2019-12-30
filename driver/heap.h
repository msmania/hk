#pragma once

class Heap final {
  HANDLE process_;
  void* base_;

public:
  Heap(HANDLE process, PVOID desiredBase, size_t size);
  ~Heap();
  operator bool() const;
  operator void*();
  void* Detach();
};
