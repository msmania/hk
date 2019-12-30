#include <ntifs.h>
#include <stdint.h>

#include "common.h"
#include "config.h"
#include "heap.h"
#include "peimage.h"

PEImage::PEImage(void* base) : base_{}, directories_{} {
  constexpr uint16_t MZ = 0x5a4d;
  constexpr uint32_t PE = 0x4550;
  constexpr uint16_t PE32 = 0x10b;
  constexpr uint16_t PE32PLUS = 0x20b;

  const auto& dos = *at<PIMAGE_DOS_HEADER>(base, 0);
  if (dos.e_magic != MZ) return;
  if (*at<uint32_t*>(base, dos.e_lfanew) != PE) return;

  const auto& fileHeader = *at<PIMAGE_FILE_HEADER>(base, dos.e_lfanew + sizeof(PE));
  if (fileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) return;

  auto& optHeader = *at<PIMAGE_OPTIONAL_HEADER64>(
    base, dos.e_lfanew + sizeof(PE) + sizeof(IMAGE_FILE_HEADER));
  if (optHeader.Magic != PE32PLUS) return;

  base_ = at<uint8_t*>(base, 0);
  directories_ = optHeader.DataDirectory;
  entrypoint_ = optHeader.AddressOfEntryPoint;
}

PEImage::operator bool() const {
  return !!base_;
}

static bool GetRvaSafely(const void *base, const void *target, uint32_t &rva) {
  rva = 0;
  int64_t rvaTry =
    reinterpret_cast<const uint8_t*>(target)
      - reinterpret_cast<const uint8_t*>(base);
  if (rvaTry < 0 || rvaTry > 0xffffffff) return false;

  rva = static_cast<uint32_t>(rvaTry);
  return true;
}

static bool IsMemoryPageWritable(HANDLE process, void *page) {
  MEMORY_BASIC_INFORMATION info{};
  size_t len;
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

static bool MakeAreaWritable(HANDLE process, void *start, size_t size) {
  if (IsMemoryPageWritable(process, start)) return true;

  UNICODE_STRING name;
  RtlInitUnicodeString(&name, L"ZwProtectVirtualMemory");
  auto ZwProtectVirtualMemory =
    reinterpret_cast<NTSTATUS(NTAPI*)(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG)>(
      MmGetSystemRoutineAddress(&name));
  if (!ZwProtectVirtualMemory) {
    Log("ZwProtectVirtualMemory does not exist.\n");
    return false;
  }

  ULONG orig;
  NTSTATUS status = ZwProtectVirtualMemory(
    process, &start, &size, PAGE_WRITECOPY, &orig);
  if (!NT_SUCCESS(status)) {
    Log("ZwProtectVirtualMemory failed - %08x\n", status);
    return false;
  }

  return true;
}

bool PEImage::UpdateImportDirectory(HANDLE process) {
  if (!base_) return false;

  struct NewImportDirectory final {
    char name_[sizeof(GlobalConfig::injectee_)];
    uint64_t names_[2];
    uint64_t functions_[2];
    IMAGE_IMPORT_DESCRIPTOR desc_[1];

    NewImportDirectory() = delete;
  };

  Heap heap(process, base_,
            sizeof(NewImportDirectory)
              + directories_[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);
  if (!heap) return false;

  auto newDir = at<NewImportDirectory*>(heap, 0);

  uint32_t rvaToDir, rvaToName,
           rvaToNameArray, rvaToFuncArray,
           rvaToFunctionName, rvaToFunction;
  if (!GetRvaSafely(base_, &newDir->desc_, rvaToDir)
      || !GetRvaSafely(base_, newDir->name_, rvaToName)
      || !GetRvaSafely(base_, newDir->names_, rvaToNameArray)
      || !GetRvaSafely(base_, newDir->functions_, rvaToFuncArray))
    return false;

  memset(newDir, 0, sizeof(NewImportDirectory));
  memcpy(newDir->name_, gConfig.injectee_, sizeof(newDir->name_));
  newDir->names_[0] = newDir->functions_[0] = 0x8000000000000064;
  newDir->desc_[0].Name = rvaToName;
  //newDir->desc_[0].ForwarderChain = -1;
  newDir->desc_[0].OriginalFirstThunk = rvaToNameArray;
  newDir->desc_[0].FirstThunk = rvaToFuncArray;

  uint32_t rvaOriginal =
    directories_[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

  auto targetEntry = &directories_[IMAGE_DIRECTORY_ENTRY_IMPORT];

  if (!MakeAreaWritable(process, targetEntry, sizeof(*targetEntry)))
    return false;

  targetEntry->VirtualAddress = rvaToDir;
  targetEntry->Size += sizeof(IMAGE_IMPORT_DESCRIPTOR);
  Log("*(%p) = %08x -> %08x\n", targetEntry, rvaOriginal, rvaToDir);

  memcpy(&newDir->desc_[1],
         at<const void*>(base_, rvaOriginal),
         directories_[IMAGE_DIRECTORY_ENTRY_IMPORT].Size);

  heap.Detach(); // Just let it leak.
  return true;
}

void PEImage::DumpImportTable() const {
  if (!base_) return;

  const auto
    dir_start = at<uint8_t*>(
      base_, directories_[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress),
    dir_end = dir_start + directories_[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;

  int index_desc = 0;
  for (uint8_t* desc_raw = dir_start;
       desc_raw < dir_end;
       desc_raw += sizeof(IMAGE_IMPORT_DESCRIPTOR), ++index_desc) {
    const auto& desc = *at<PIMAGE_IMPORT_DESCRIPTOR>(desc_raw, 0);
    if (!desc.Characteristics) break;
    Log("%d: %p %s\n",
        index_desc, desc_raw, at<const char*>(base_, desc.Name));
  }
}
