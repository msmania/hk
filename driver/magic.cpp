#include <ntifs.h>
#include <stdint.h>

#include "common.h"
#include "heap.h"
#include "magic.h"
#include "peimage.h"

#define HIWORD(dw) static_cast<uint16_t>(static_cast<uint32_t>(dw) >> 16)
#define LOWORD(dw) static_cast<uint16_t>(static_cast<uint32_t>(dw) & 0xffff)

enum SYSTEM_INFORMATION_CLASS {
  SystemModuleInformation = 11,
};

#pragma pack(push, 8)
struct RTL_PROCESS_MODULE_INFORMATION {
  HANDLE Section;
  PVOID MappedBase;
  PVOID ImageBase;
  ULONG ImageSize;
  ULONG Flags;
  USHORT LoadOrderIndex;
  USHORT InitOrderIndex;
  USHORT LoadCount;
  USHORT OffsetToFileName;
  UCHAR FullPathName[256];
};

struct SYSTEM_MODULE_INFORMATION {
  ULONG ModulesCount;
  RTL_PROCESS_MODULE_INFORMATION Modules[1];
};
#pragma pack(pop)

Magic gMagic;

PVOID GetKernelBaseAddress() {
  UNICODE_STRING name;
  RtlInitUnicodeString(&name, L"ZwQuerySystemInformation");
  auto ZwQuerySystemInformation =
    reinterpret_cast<NTSTATUS(NTAPI*)(SYSTEM_INFORMATION_CLASS,
                                      PVOID, ULONG, PULONG)>(
      MmGetSystemRoutineAddress(&name));
  if (!ZwQuerySystemInformation) {
    Log("ZwQuerySystemInformation does not exist.\n");
    return false;
  }

  ULONG bufferLen = sizeof(SYSTEM_MODULE_INFORMATION);
  do {
    Pool pool(PagedPool, bufferLen);
    if (!pool) {
      break;
    }

    NTSTATUS status = ZwQuerySystemInformation(
        SystemModuleInformation, pool, bufferLen, nullptr);
    if (status == STATUS_INFO_LENGTH_MISMATCH) {
      bufferLen *= 2;
      continue;
    }
    else if (!NT_SUCCESS(status))  {
      Log("ZwQuerySystemInformation failed - %08x\n", status);
      break;
    }

    return pool.As<SYSTEM_MODULE_INFORMATION*>()->Modules[0].ImageBase;
  } while (1);

  return nullptr;
}

void Magic::InitInternal(uint32_t major, uint32_t minor, uint32_t buildnum) {
#if defined(_AMD64_)

  if (major == 10) {
    if (buildnum == 18362) {
      // 19H1
      EProcess_SectionBaseAddress = 0x3c8;
      EThread_ThreadListEntry = 0x6b8;
    }
    else if (buildnum == 19041) {
      // 20H1
      EProcess_SectionBaseAddress = 0x520;
      EThread_ThreadListEntry = 0x4e8;
    }
  }
  else if (major == 6 && minor == 1) {
    EProcess_SectionBaseAddress = 0x270;
    EThread_ThreadListEntry = 0x428;
  }

#elif defined(_X86_)

  if (major == 10) {
    EProcess_SectionBaseAddress = 0x130;
    EThread_ThreadListEntry = 0x1d4;
  }
  else if (major == 6 && minor == 1) {
    EProcess_SectionBaseAddress = 0x3c8;
    EThread_ThreadListEntry = 0x2f8;
  }

#endif
}

void Magic::InitInternal(uint32_t major,
                         uint32_t minor,
                         uint32_t buildnum,
                         uint32_t revision) {
  InitInternal(major, minor, buildnum);

  if (major == 6 && minor == 1 && buildnum == 7601 && revision == 24535) {
    NT_ZwProtectVirtualMemory = 0x915e0;
  }
}

void Magic::Init() {
  KernelBaseAddress = GetKernelBaseAddress();
  NT_ZwProtectVirtualMemory = 0;

  do {
    if (!KernelBaseAddress) break;

    PEImage nt(KernelBaseAddress);
    if (!nt) break;

    auto ver = nt.GetVersion();
    if (!ver.dwSignature) break;

    Log("NT version: %d.%d.%d.%d\n",
        HIWORD(ver.dwProductVersionMS),
        LOWORD(ver.dwProductVersionMS),
        HIWORD(ver.dwProductVersionLS),
        LOWORD(ver.dwProductVersionLS));

    InitInternal(HIWORD(ver.dwProductVersionMS),
                 LOWORD(ver.dwProductVersionMS),
                 HIWORD(ver.dwProductVersionLS),
                 LOWORD(ver.dwProductVersionLS));
    return;
  } while (0);

  RTL_OSVERSIONINFOW osinfo = {sizeof(osinfo)};
  NTSTATUS status = RtlGetVersion(&osinfo);
  if (!NT_SUCCESS(status))  {
    Log("RtlGetVersion failed - %08x\n", status);
  }

  Log("OS version: %d.%d.%d\n",
      osinfo.dwMajorVersion,
      osinfo.dwMinorVersion,
      osinfo.dwBuildNumber);
  return InitInternal(osinfo.dwMajorVersion,
                      osinfo.dwMinorVersion,
                      osinfo.dwBuildNumber);
}
