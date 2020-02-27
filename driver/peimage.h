#pragma once

typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
  uint16_t e_magic;                     // Magic number
  uint16_t e_cblp;                      // Bytes on last page of file
  uint16_t e_cp;                        // Pages in file
  uint16_t e_crlc;                      // Relocations
  uint16_t e_cparhdr;                   // Size of header in paragraphs
  uint16_t e_minalloc;                  // Minimum extra paragraphs needed
  uint16_t e_maxalloc;                  // Maximum extra paragraphs needed
  uint16_t e_ss;                        // Initial (relative) SS value
  uint16_t e_sp;                        // Initial SP value
  uint16_t e_csum;                      // Checksum
  uint16_t e_ip;                        // Initial IP value
  uint16_t e_cs;                        // Initial (relative) CS value
  uint16_t e_lfarlc;                    // File address of relocation table
  uint16_t e_ovno;                      // Overlay number
  uint16_t e_res[4];                    // Reserved words
  uint16_t e_oemid;                     // OEM identifier (for e_oeminfo)
  uint16_t e_oeminfo;                   // OEM information; e_oemid specific
  uint16_t e_res2[10];                  // Reserved words
  uint32_t e_lfanew;                    // File address of new exe header
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
  uint16_t Machine;
  uint16_t NumberOfSections;
  uint32_t TimeDateStamp;
  uint32_t PointerToSymbolTable;
  uint32_t NumberOfSymbols;
  uint16_t SizeOfOptionalHeader;
  uint16_t Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
  uint32_t VirtualAddress;
  uint32_t Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

#define IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
#define IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
#define IMAGE_DIRECTORY_ENTRY_RESOURCE        2   // Resource Directory
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION       3   // Exception Directory
#define IMAGE_DIRECTORY_ENTRY_SECURITY        4   // Security Directory
#define IMAGE_DIRECTORY_ENTRY_BASERELOC       5   // Base Relocation Table
#define IMAGE_DIRECTORY_ENTRY_DEBUG           6   // Debug Directory
//      IMAGE_DIRECTORY_ENTRY_COPYRIGHT       7   // (X86 usage)
#define IMAGE_DIRECTORY_ENTRY_ARCHITECTURE    7   // Architecture Specific Data
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR       8   // RVA of GP
#define IMAGE_DIRECTORY_ENTRY_TLS             9   // TLS Directory
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG    10   // Load Configuration Directory
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT   11   // Bound Import Directory in headers
#define IMAGE_DIRECTORY_ENTRY_IAT            12   // Import Address Table
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT   13   // Delay Load Import Descriptors
#define IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR 14   // COM Runtime descriptor

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct _IMAGE_OPTIONAL_HEADER {
  uint16_t Magic;
  uint8_t  MajorLinkerVersion;
  uint8_t  MinorLinkerVersion;
  uint32_t SizeOfCode;
  uint32_t SizeOfInitializedData;
  uint32_t SizeOfUninitializedData;
  uint32_t AddressOfEntryPoint;
  uint32_t BaseOfCode;
  uint32_t BaseOfData;
  uint32_t ImageBase;
  uint32_t SectionAlignment;
  uint32_t FileAlignment;
  uint16_t MajorOperatingSystemVersion;
  uint16_t MinorOperatingSystemVersion;
  uint16_t MajorImageVersion;
  uint16_t MinorImageVersion;
  uint16_t MajorSubsystemVersion;
  uint16_t MinorSubsystemVersion;
  uint32_t Win32VersionValue;
  uint32_t SizeOfImage;
  uint32_t SizeOfHeaders;
  uint32_t CheckSum;
  uint16_t Subsystem;
  uint16_t DllCharacteristics;
  uint32_t SizeOfStackReserve;
  uint32_t SizeOfStackCommit;
  uint32_t SizeOfHeapReserve;
  uint32_t SizeOfHeapCommit;
  uint32_t LoaderFlags;
  uint32_t NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER32, *PIMAGE_OPTIONAL_HEADER32;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
  uint16_t Magic;
  uint8_t  MajorLinkerVersion;
  uint8_t  MinorLinkerVersion;
  uint32_t SizeOfCode;
  uint32_t SizeOfInitializedData;
  uint32_t SizeOfUninitializedData;
  uint32_t AddressOfEntryPoint;
  uint32_t BaseOfCode;
  uint64_t ImageBase;
  uint32_t SectionAlignment;
  uint32_t FileAlignment;
  uint16_t MajorOperatingSystemVersion;
  uint16_t MinorOperatingSystemVersion;
  uint16_t MajorImageVersion;
  uint16_t MinorImageVersion;
  uint16_t MajorSubsystemVersion;
  uint16_t MinorSubsystemVersion;
  uint32_t Win32VersionValue;
  uint32_t SizeOfImage;
  uint32_t SizeOfHeaders;
  uint32_t CheckSum;
  uint16_t Subsystem;
  uint16_t DllCharacteristics;
  uint64_t SizeOfStackReserve;
  uint64_t SizeOfStackCommit;
  uint64_t SizeOfHeapReserve;
  uint64_t SizeOfHeapCommit;
  uint32_t LoaderFlags;
  uint32_t NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

#define IMAGE_FILE_MACHINE_I386  0x014c  // Intel 386.
#define IMAGE_FILE_MACHINE_AMD64 0x8664  // AMD64 (K8)
#define IMAGE_ORDINAL_FLAG64 0x8000000000000000
#define IMAGE_ORDINAL_FLAG32 0x80000000

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
  union {
    uint32_t Characteristics;    // 0 for terminating null import descriptor
    uint32_t OriginalFirstThunk; // RVA to original unbound IAT (PIMAGE_THUNK_DATA)
  } DUMMYUNIONNAME;
  uint32_t TimeDateStamp;        // 0 if not bound,
                                 // -1 if bound, and real date\time stamp
                                 //     in IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT (new BIND)
                                 // O.W. date/time stamp of DLL bound to (Old BIND)

  uint32_t ForwarderChain;       // -1 if no forwarders
  uint32_t Name;
  uint32_t FirstThunk;           // RVA to IAT (if bound this IAT has actual addresses)
} IMAGE_IMPORT_DESCRIPTOR;
typedef IMAGE_IMPORT_DESCRIPTOR UNALIGNED *PIMAGE_IMPORT_DESCRIPTOR;

class PEImage {
  enum class CPU {unknown, amd64, x86} arch_;
  uint8_t* base_;
  PIMAGE_DATA_DIRECTORY directories_;
  uint32_t entrypoint_;

  struct NewImportDirectory64 final {
    constexpr static uint64_t OrdinalFlag = IMAGE_ORDINAL_FLAG64;
    char name_[sizeof(GlobalConfig::injectee_)];
    uint64_t names_[2];
    uint64_t functions_[2];
    IMAGE_IMPORT_DESCRIPTOR desc_[1];

    NewImportDirectory64() = delete;
  };

  struct NewImportDirectory32 final {
    constexpr static uint32_t OrdinalFlag = IMAGE_ORDINAL_FLAG32;
    char name_[sizeof(GlobalConfig::injectee_)];
    uint32_t names_[2];
    uint32_t functions_[2];
    IMAGE_IMPORT_DESCRIPTOR desc_[1];

    NewImportDirectory32() = delete;
  };

  template<typename NewImportDirectory>
  bool UpdateImportDirectoryInternal(HANDLE process);

public:
  PEImage(void* base);
  operator bool() const;

  bool UpdateImportDirectory(HANDLE process);
  void DumpImportTable() const;
};
