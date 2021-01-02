#include <cstdint>
#include <windows.h>

#include "common.h"
#include "pe.h"

class PEImage : public hk::PEImageBase {
public:
  PEImage(void* base) : hk::PEImageBase(base) {}
  void RevertImportDirectory();
};

void PEImage::RevertImportDirectory() {
  auto landAddr = reinterpret_cast<uint8_t*>(
      at<uintptr_t>(base_,
                    directories_[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress)
          & static_cast<uintptr_t>(-0x1000));

  MEMORY_BASIC_INFORMATION info = {};
  if (VirtualQuery(landAddr, &info, sizeof(info)) != sizeof(info)
      || info.Protect != PAGE_READWRITE) {
    return;
  }
  auto origDir =
      at<BackupDirectory*>(landAddr, 0x1000 - sizeof(BackupDirectory));
  if (origDir->magic_ != BackupDirectory::sMagic) return;

  directories_[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = origDir->va_;
  directories_[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = origDir->size_;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    PEImage pe(GetModuleHandle(nullptr));
    if (!pe) return TRUE;
    pe.RevertImportDirectory();
  }
  return TRUE;
}

void Export100() {}
