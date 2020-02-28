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

class Pool final {
  constexpr static ULONG TAG = 'xxxx';
  void* base_;

public:
  Pool(POOL_TYPE type, SIZE_T size);
  ~Pool();
  operator bool() const;
  operator void*();
  template <typename T>
  T As() {
    return reinterpret_cast<T>(base_);
  }
};
