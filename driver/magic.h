#pragma once

class Magic {
  void InitInternal(uint32_t major,
                    uint32_t minor,
                    uint32_t buildnum);
  void InitInternal(uint32_t major,
                    uint32_t minor,
                    uint32_t buildnum,
                    uint32_t revision);

public:
  void* KernelBaseAddress;
  int EProcess_SectionBaseAddress;
  int EThread_ThreadListEntry;
  int NT_ZwProtectVirtualMemory;

  // Hack Common
  int EProcess_SectionObject;
  int Section_ControlArea;

  // Hack1
  int ControlArea_U2E2;
  int ControlArea_ImageActive_Bit;
  int ControlArea_ImageBaseOkToReuse_Bit;

  // Hack2
  int NT_Context_ReservePtes;
  int NT_MiReservePtesFunc;
  int NT_MiReleasePtesFunc;
  int NT_MiSwitchBaseAddressFunc;

  void Init();
};

extern Magic gMagic;
