#pragma once

class Heap final {
  HANDLE process_;
  void* base_;

public:
  Heap(HANDLE process, PVOID desiredBase, SIZE_T size);
  ~Heap();
  operator bool() const;
  operator void*();
  void* Detach();
};
