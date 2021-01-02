#include <ntifs.h>
#include <stdint.h>

#include "common.h"
#include "config.h"
#include "heap.h"
#include "magic.h"
#include "peimage.h"

#define IS_INTRESOURCE(_r) ((((uintptr_t)(_r)) >> 16) == 0)
#define MAKEINTRESOURCEW(i) ((wchar_t*)((uintptr_t)((uint16_t)(i))))
#define RT_VERSION MAKEINTRESOURCEW(16)

int CopyAndSplit(const char *src,
                 char *dst,
                 const char *chunks[],
                 int lenChunks,
                 char delim = '|');

namespace {

bool GetRvaSafely(const void *base, const void *target, uint32_t &rva) {
  rva = 0;
  int64_t rvaTry =
    reinterpret_cast<const uint8_t*>(target)
      - reinterpret_cast<const uint8_t*>(base);
  if (rvaTry < 0 || rvaTry > 0xffffffff) return false;

  rva = static_cast<uint32_t>(rvaTry);
  return true;
}

bool IsMemoryPageWritable(HANDLE process, void *page) {
  MEMORY_BASIC_INFORMATION info{};
  SIZE_T len;
  NTSTATUS status = ZwQueryVirtualMemory(
      process, page, MemoryBasicInformation, &info, sizeof(info), &len);
  if (!NT_SUCCESS(status)) {
    Log("ZwQueryVirtualMemory failed - %08x\n", status);
    return false;
  }
  //Log("IsMemoryPageWritable %p: Alloc=%lx Prot=%lx Type=%lx Stat=%lx\n",
  //    page, info.AllocationProtect, info.Protect, info.Type, info.State);
  return info.Protect == PAGE_READWRITE || info.Protect == PAGE_WRITECOPY;
}

bool MakeAreaWritable(HANDLE process, void *start, SIZE_T size) {
  if (IsMemoryPageWritable(process, start)) return true;

  UNICODE_STRING name;
  RtlInitUnicodeString(&name, L"ZwProtectVirtualMemory");
  auto ZwProtectVirtualMemory =
    reinterpret_cast<NTSTATUS(NTAPI*)(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG)>(
      MmGetSystemRoutineAddress(&name));
  if (!ZwProtectVirtualMemory) {
    if (gMagic.KernelBaseAddress
        && gMagic.NT_ZwProtectVirtualMemory) {
      ZwProtectVirtualMemory = at<decltype(ZwProtectVirtualMemory)>(
        gMagic.KernelBaseAddress, gMagic.NT_ZwProtectVirtualMemory);
    }
    else {
      Log("Cannot find ZwProtectVirtualMemory.\n");
      return false;
    }
  }

  ULONG orig;
  NTSTATUS status = ZwProtectVirtualMemory(
    process, &start, &size, PAGE_READWRITE, &orig);
  if (!NT_SUCCESS(status)) {
    Log("ZwProtectVirtualMemory failed - %08x\n", status);
    return false;
  }

  return true;
}

template <typename T>
struct Thunk {
  T names_[2];
  T functions_[2];

  Thunk() = delete;

  void Populate(T value) {
    names_[0] = functions_[0] = value;
    names_[1] = functions_[1] = 0;
  }
};

struct ImportDescriptor : hk::IMAGE_IMPORT_DESCRIPTOR {
  ImportDescriptor() = delete;

  template <typename ThunkT>
  bool Populate(uint8_t *base, const char *name, const ThunkT &thunk) {
    uint32_t rvaToName, rvaToNameArray, rvaToFuncArray;
    if (!GetRvaSafely(base, name, rvaToName)
        || !GetRvaSafely(base, thunk.names_, rvaToNameArray)
        || !GetRvaSafely(base, thunk.functions_, rvaToFuncArray))
      return false;

    OriginalFirstThunk = rvaToNameArray;
    TimeDateStamp = ForwarderChain = 0;
    Name = rvaToName;
    FirstThunk = rvaToFuncArray;
    return true;
  }
};

constexpr int NUM_CHUNKS = 3;

struct NewImportDirectory64 final {
  constexpr static uint64_t OrdinalFlag = hk::ORDINAL_FLAG64;
  char name_[sizeof(GlobalConfig::injectee_)];
  Thunk<uint64_t> thunks_[NUM_CHUNKS];
  ImportDescriptor desc_[NUM_CHUNKS];

  NewImportDirectory64() = delete;
};

struct NewImportDirectory32 final {
  constexpr static uint32_t OrdinalFlag = hk::ORDINAL_FLAG32;
  char name_[sizeof(GlobalConfig::injectee_)];
  Thunk<uint32_t> thunks_[NUM_CHUNKS];
  ImportDescriptor desc_[NUM_CHUNKS];

  NewImportDirectory32() = delete;
};

}

namespace hk {

PEImage::PEImage(void* base) : PEImageBase(base) {}

template<typename NewImportDirectory>
bool PEImage::UpdateImportDirectoryInternal(HANDLE process) {
  if (!base_) return false;

  Heap heap(process, base_,
            sizeof(NewImportDirectory)
              + directories_[IMAGE_DIRECTORY::IMPORT].Size
              + sizeof(BackupDirectory));
  if (!heap) return false;

  auto newDir = at<NewImportDirectory*>(heap, 0);
  auto origDir = at<BackupDirectory*>(
      heap, heap.Size() - sizeof(BackupDirectory));

  uint32_t rvaToDir;
  if (!GetRvaSafely(base_, &newDir->desc_, rvaToDir))
    return false;

  if (!gConfig.injectee_[0]) return false;

  const char *names[NUM_CHUNKS];
  int actualChunks = CopyAndSplit(gConfig.injectee_,
                                  newDir->name_,
                                  names,
                                  NUM_CHUNKS,
                                  '*');
  if (actualChunks < 1)
    return false;
  for (int i = 0; i < actualChunks; ++i) {
    newDir->thunks_[i].Populate(NewImportDirectory::OrdinalFlag | 100);
    if (!newDir->desc_[i].Populate(base_, names[i], newDir->thunks_[i]))
      return false;
  }

  uint32_t rvaOriginal =
    directories_[IMAGE_DIRECTORY::IMPORT].VirtualAddress;

  auto targetEntry = &directories_[IMAGE_DIRECTORY::IMPORT];
  if (!MakeAreaWritable(process, targetEntry, sizeof(*targetEntry)))
    return false;

  origDir->magic_ = BackupDirectory::sMagic;
  origDir->va_ = targetEntry->VirtualAddress;
  origDir->size_ = targetEntry->Size;
  targetEntry->VirtualAddress = rvaToDir;
  targetEntry->Size += sizeof(IMAGE_IMPORT_DESCRIPTOR);
  Log("*(%p) = %08x -> %08x\n", targetEntry, rvaOriginal, rvaToDir);

  memcpy(&newDir->desc_[actualChunks],
         at<const void*>(base_, rvaOriginal),
         directories_[IMAGE_DIRECTORY::IMPORT].Size);

  // Clear the Bound Import Directory
  targetEntry = &directories_[IMAGE_DIRECTORY::BOUND_IMPORT];
  if ((targetEntry->VirtualAddress || targetEntry->Size)
      && MakeAreaWritable(process, targetEntry, sizeof(*targetEntry))) {
    targetEntry->VirtualAddress = targetEntry->Size = 0;
  }

  heap.Detach(); // Just let it leak.
  return true;
}

bool PEImage::UpdateImportDirectory(HANDLE process) {
  switch (arch_) {
  case CPU::amd64:
    return UpdateImportDirectoryInternal<NewImportDirectory64>(process);
  case CPU::x86:
    return UpdateImportDirectoryInternal<NewImportDirectory32>(process);
  case CPU::unknown:
  default:
    return false;
  }
}

void PEImage::DumpImportTable() const {
  if (!base_) return;

  const auto
    dir_start = at<uint8_t*>(
      base_, directories_[IMAGE_DIRECTORY::IMPORT].VirtualAddress),
    dir_end = dir_start + directories_[IMAGE_DIRECTORY::IMPORT].Size;

  int index_desc = 0;
  for (uint8_t* desc_raw = dir_start;
       desc_raw < dir_end;
       desc_raw += sizeof(IMAGE_IMPORT_DESCRIPTOR), ++index_desc) {
    const auto& desc = *at<IMAGE_IMPORT_DESCRIPTOR*>(desc_raw, 0);
    if (!desc.Characteristics) break;
    Log("%d: %p %s\n",
        index_desc, desc_raw, at<const char*>(base_, desc.Name));
  }
}

class ResourceId {
  enum class Type {Number, String, Any};
  Type type_;
  uint16_t number_;
  const wchar_t* string_;

  ResourceId() : type_(Type::Any) {}

public:
  static const ResourceId &Any() {
    static ResourceId any;
    return any;
  }
  ResourceId(uint16_t id)
    : type_(Type::Number), number_(id)
  {}
  ResourceId(const wchar_t *name) {
    if (IS_INTRESOURCE(name)) {
      type_ = Type::Number;
      number_ = static_cast<uint16_t>(
        reinterpret_cast<uintptr_t>(name) & 0xffff);
    }
    else {
      type_ = Type::String;
      string_ = name;
    }
  }

  bool operator==(const ResourceId &other) const {
    if (type_ == Type::Any || other.type_ == Type::Any)
      return true;

    if (type_ != other.type_)
      return false;

    switch (type_) {
    case Type::Number:
      return number_ == other.number_;
    case Type::String:
      return wcscmp(string_, other.string_) == 0;
    default:
      return false;
    }
  }

  bool operator!=(const ResourceId &other) const {
    return !(*this == other);
  }
};

class ResourceDirectory {
  using ResourceIterator = void(*)(const IMAGE_RESOURCE_DATA_ENTRY*, void*);

  class DirectoryEntry {
    const uint8_t* base_;

  public:
    DirectoryEntry(const uint8_t* base) : base_(base) {}

    void Iterate(const uint8_t* addr,
                 const ResourceId &filter,
                 void* context,
                 ResourceIterator func) {
      const auto entry = at<const IMAGE_RESOURCE_DIRECTORY_ENTRY*>(addr, 0);
      if (!entry->Name) return;

      const ResourceId id = entry->NameIsString
        ? ResourceId(at<const wchar_t*>(base_, entry->NameOffset))
        : ResourceId(entry->Id);

      if (id != filter) return;

      if (entry->DataIsDirectory) {
        // Log("Dir -> %p\n", base_ + entry->OffsetToDirectory);
        ResourceDirectory dir(base_);
        dir.Iterate(base_ + entry->OffsetToDirectory,
                    ResourceId::Any(), context, func);
      }
      else {
        // Log("Data %p\n", base_ + entry->OffsetToData);
        const auto data_entry =
          at<const IMAGE_RESOURCE_DATA_ENTRY*>(base_, entry->OffsetToData);
        func(data_entry, context);
      }
    }
  };

  const uint8_t* base_;

public:
  ResourceDirectory(const uint8_t* base) : base_(base) {}

  void Iterate(const uint8_t* addr,
               const ResourceId &filter,
               void* context,
               ResourceIterator func) {
    const auto fastLookup = at<const IMAGE_RESOURCE_DIRECTORY*>(addr, 0);
    const int num_entries =
      fastLookup->NumberOfNamedEntries + fastLookup->NumberOfIdEntries;
    const uint8_t* first_entry = addr + sizeof(IMAGE_RESOURCE_DIRECTORY);
    for (int i = 0; i < num_entries; ++i) {
      DirectoryEntry entry(base_);
      entry.Iterate(first_entry + i * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY),
                    filter, context, func);
    }
  }
};

VS_FIXEDFILEINFO PEImage::GetVersion() const {
  struct ResourceIteratorContext {
    const uint8_t *imagebase_;
    VS_FIXEDFILEINFO version_;
  } context = {base_};

  if (!base_) return context.version_;

  const auto
    dir_start = at<uint8_t*>(
      base_, directories_[IMAGE_DIRECTORY::RESOURCE].VirtualAddress);

  // In 32bit kernel, Resource section is not accessible.
  if (!MmIsAddressValid(dir_start)) {
    return context.version_;
  }

  ResourceDirectory dir(dir_start);
  dir.Iterate(
    dir_start,
    ResourceId(RT_VERSION),
    &context,
    [](const IMAGE_RESOURCE_DATA_ENTRY* data, void* param) {
      struct VS_VERSIONINFO {
        uint16_t wLength;
        uint16_t wValueLength;
        uint16_t wType;
        wchar_t szKey[16]; // L"VS_VERSION_INFO"
        VS_FIXEDFILEINFO Value;
      };

      // |data| may be paged out.
      if (!MmIsAddressValid(const_cast<IMAGE_RESOURCE_DATA_ENTRY*>(data))) {
        return;
      }

      if (data->Size < sizeof(VS_VERSIONINFO)) return;

      auto context = reinterpret_cast<ResourceIteratorContext*>(param);
      const auto ver =
          at<const VS_VERSIONINFO*>(context->imagebase_, data->OffsetToData);
      if (ver->wValueLength != sizeof(VS_FIXEDFILEINFO)
          || ver->Value.dwSignature != 0xFEEF04BD)
        return;

      context->version_ = ver->Value;
    });

  return context.version_;
}

}
