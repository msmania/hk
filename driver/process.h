#pragma once

typedef struct _PEB_LDR_DATA {
  uint8_t Reserved1[8];
  uintptr_t Reserved2[3];
  LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _LDR_DATA_TABLE_ENTRY {
  uintptr_t Reserved1[2];
  LIST_ENTRY InMemoryOrderLinks;
  uintptr_t Reserved2[2];
  void *DllBase;
  uintptr_t Reserved3[2];
  UNICODE_STRING FullDllName;
  uint8_t Reserved4[8];
  uintptr_t Reserved5[3];
#pragma warning(push)
#pragma warning(disable: 4201) // we'll always use the Microsoft compiler
  union {
    uint32_t CheckSum;
    uintptr_t Reserved6;
  } DUMMYUNIONNAME;
#pragma warning(pop)
  uint32_t TimeDateStamp;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
  uint8_t Reserved1[16];
  uintptr_t Reserved2[10];
  UNICODE_STRING ImagePathName;
  UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef VOID (NTAPI *PPS_POST_PROCESS_INIT_ROUTINE)();

typedef struct _PEB {
  uint8_t Reserved1[2];
  uint8_t BeingDebugged;
  uint8_t Reserved2[1];
  uintptr_t Reserved3[2];
  PPEB_LDR_DATA Ldr;
  PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
  uintptr_t Reserved4[3];
  uintptr_t AtlThunkSListPtr;
  uintptr_t Reserved5;
  ULONG Reserved6;
  uintptr_t Reserved7;
  uint32_t Reserved8;
  uint32_t AtlThunkSListPtr32;
  uintptr_t Reserved9[45];
  uint8_t Reserved10[96];
  PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
  uint8_t Reserved11[128];
  uintptr_t Reserved12[1];
  uint32_t SessionId;
} PEB, *PPEB;

extern "C" NTSTATUS NTAPI ZwQueryInformationProcess(
    HANDLE ProcessHandle,
    PROCESSINFOCLASS ProcessInformationClass,
    PVOID ProcessInformation,
    uint32_t ProcessInformationLength,
    uint32_t* ReturnLength OPTIONAL);

class Process final {
  HANDLE process_;

public:
  Process(HANDLE Pid, ACCESS_MASK desiredAccess);
  ~Process();
  operator bool() const;
  operator HANDLE() const;

  PEB *Peb() const;
};

class EProcess final {
  PEPROCESS obj_;

public:
  EProcess(HANDLE Pid);
  ~EProcess();
  operator bool() const;
  operator PKPROCESS();
  const char *ProcessName() const;
  void *SectionBase() const;
};

class EThread final {
  PETHREAD obj_;

public:
  EThread(HANDLE Tid);
  ~EThread();
  operator bool() const;
  operator PKTHREAD();
  int CountThreadList() const;
};
