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

  void Init();
};

extern Magic gMagic;
