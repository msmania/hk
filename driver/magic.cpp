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

/*  Friendly cheetsheet <3
dt nt!_ETHREAD ThreadListEntry
dt nt!_EPROCESS SectionObject SectionBaseAddress
dt nt!_SECTION u1.ControlArea
dt nt!_CONTROL_AREA u2.e2.ImageActive u2.e2.ImageBaseOkToReuse
? nt!MiReservePtes-nt
? nt!MiReleasePtes-nt
? nt!MiSwitchBaseAddress-nt
!! -ci "uf nt!MiRelocateImageAgain" findstr nt!MiReservePtes
  * after this, run the "ub" command to find a mysterious number passed
  * to MiReservePtes which should be something like nt!MiState+0x2680.
*/
void Magic::InitInternal(uint32_t major, uint32_t minor, uint32_t buildnum) {
  EProcess_SectionObject = 0;

#if defined(_AMD64_)

  if (major == 10) {
    if (buildnum == 18362) {
      // 19H1
      EThread_ThreadListEntry = 0x6b8;
      EProcess_SectionBaseAddress = 0x3c8;
    }
    else if (buildnum == 19041 || buildnum == 19042) {
      // 20H1 & 20H2
      EThread_ThreadListEntry = 0x4e8;
      EProcess_SectionObject = 0x518;
      EProcess_SectionBaseAddress = 0x520;

      Section_ControlArea = 0x28;
      ControlArea_U2E2 = 0x5c;
      ControlArea_ImageActive_Bit = 0x16;
      ControlArea_ImageBaseOkToReuse_Bit = 0x17;

      // Especially don't trust the following numbers because they depend on
      // not only a build number but also a patch level.  You should examine
      // your kernel yourself, otherwise you will hit BSOD.  :P
      if (buildnum == 19041) {
        NT_MiReservePtesFunc = 0x3276e0;
        NT_MiReleasePtesFunc = 0x2c27f0;
        NT_MiSwitchBaseAddressFunc = 0x60a130;
        NT_Context_ReservePtes = 0xc4edc0;
      }
      else if (buildnum == 19042) {
        NT_MiReservePtesFunc = 0x32b250;
        NT_MiReleasePtesFunc = 0x2c26b0;
        NT_MiSwitchBaseAddressFunc = 0x60a130;
        NT_Context_ReservePtes = 0xc4ee00;
      }
    }
  }
  else if (major == 6 && minor == 1) {
    EThread_ThreadListEntry = 0x428;
    EProcess_SectionBaseAddress = 0x270;
  }

#elif defined(_X86_)

  if (major == 10) {
    if (buildnum == 18363) {
      // 19H1
      EThread_ThreadListEntry = 0x3bc;
      EProcess_SectionBaseAddress = 0x130;
    }
  }
  else if (major == 6 && minor == 1) {
    EThread_ThreadListEntry = 0x2f8;
    EProcess_SectionBaseAddress = 0x3c8;
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
