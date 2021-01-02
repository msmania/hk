#pragma once

namespace hk {

struct IMAGE_DOS_HEADER { // DOS .EXE header
  uint16_t e_magic;       // Magic number
  uint16_t e_cblp;        // Bytes on last page of file
  uint16_t e_cp;          // Pages in file
  uint16_t e_crlc;        // Relocations
  uint16_t e_cparhdr;     // Size of header in paragraphs
  uint16_t e_minalloc;    // Minimum extra paragraphs needed
  uint16_t e_maxalloc;    // Maximum extra paragraphs needed
  uint16_t e_ss;          // Initial (relative) SS value
  uint16_t e_sp;          // Initial SP value
  uint16_t e_csum;        // Checksum
  uint16_t e_ip;          // Initial IP value
  uint16_t e_cs;          // Initial (relative) CS value
  uint16_t e_lfarlc;      // File address of relocation table
  uint16_t e_ovno;        // Overlay number
  uint16_t e_res[4];      // Reserved words
  uint16_t e_oemid;       // OEM identifier (for e_oeminfo)
  uint16_t e_oeminfo;     // OEM information; e_oemid specific
  uint16_t e_res2[10];    // Reserved words
  uint32_t e_lfanew;      // File address of new exe header
};

struct IMAGE_FILE_HEADER {
  uint16_t Machine;
  uint16_t NumberOfSections;
  uint32_t TimeDateStamp;
  uint32_t PointerToSymbolTable;
  uint32_t NumberOfSymbols;
  uint16_t SizeOfOptionalHeader;
  uint16_t Characteristics;
};

struct IMAGE_DATA_DIRECTORY {
  uint32_t VirtualAddress;
  uint32_t Size;
};

enum IMAGE_DIRECTORY {
  EXPORT = 0,     // Export Directory
  IMPORT,         // Import Directory
  RESOURCE,       // Resource Directory
  EXCEPTION,      // Exception Directory
  SECURITY,       // Security Directory
  BASERELOC,      // Base Relocation Table
  DEBUG,          // Debug Directory
  // COPYRIGHT,   // (X86 usage)
  ARCHITECTURE,   // Architecture Specific Data
  GLOBALPTR,      // RVA of GP
  TLS,            // TLS Directory
  LOAD_CONFIG,    // Load Configuration Directory
  BOUND_IMPORT,   // Bound Import Directory in headers
  IAT,            // Import Address Table
  DELAY_IMPORT,   // Delay Load Import Descriptors
  COM_DESCRIPTOR, // COM Runtime descriptor

  ENTRIES = 16,
};

struct IMAGE_OPTIONAL_HEADER32 {
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
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_DIRECTORY::ENTRIES];
};

struct IMAGE_OPTIONAL_HEADER64 {
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
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_DIRECTORY::ENTRIES];
};

constexpr uint16_t MACHINE_I386 = 0x014c;   // Intel 386.
constexpr uint16_t MACHINE_AMD64 = 0x8664;  // AMD64 (K8)
constexpr uint64_t ORDINAL_FLAG64 = 0x8000000000000000;
constexpr uint64_t ORDINAL_FLAG32 = 0x80000000;

struct IMAGE_IMPORT_DESCRIPTOR {
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
};

struct IMAGE_RESOURCE_DIRECTORY {
  uint32_t Characteristics;
  uint32_t TimeDateStamp;
  uint16_t MajorVersion;
  uint16_t MinorVersion;
  uint16_t NumberOfNamedEntries;
  uint16_t NumberOfIdEntries;
};

struct IMAGE_RESOURCE_DIRECTORY_ENTRY {
  union {
    struct {
      uint32_t NameOffset:31;
      uint32_t NameIsString:1;
    } DUMMYSTRUCTNAME;
    uint32_t Name;
    uint16_t Id;
  } DUMMYUNIONNAME;
  union {
    uint32_t OffsetToData;
    struct {
      uint32_t OffsetToDirectory:31;
      uint32_t DataIsDirectory:1;
    } DUMMYSTRUCTNAME2;
  } DUMMYUNIONNAME2;
};

struct IMAGE_RESOURCE_DATA_ENTRY {
  uint32_t OffsetToData;
  uint32_t Size;
  uint32_t CodePage;
  uint32_t Reserved;
};

struct VS_FIXEDFILEINFO {
  uint32_t dwSignature;
  uint32_t dwStrucVersion;
  uint32_t dwFileVersionMS;
  uint32_t dwFileVersionLS;
  uint32_t dwProductVersionMS;
  uint32_t dwProductVersionLS;
  uint32_t dwFileFlagsMask;
  uint32_t dwFileFlags;
  uint32_t dwFileOS;
  uint32_t dwFileType;
  uint32_t dwFileSubtype;
  uint32_t dwFileDateMS;
  uint32_t dwFileDateLS;
};

class PEImageBase {
protected:
  enum class CPU {unknown, amd64, x86} arch_;
  uint8_t* base_;
  IMAGE_DATA_DIRECTORY* directories_;
  uint32_t entrypoint_;

public:
  PEImageBase(void* base)
      : arch_(CPU::unknown), base_{}, directories_{} {
    constexpr uint16_t MZ = 0x5a4d;
    constexpr uint32_t PE = 0x4550;
    constexpr uint16_t PE32 = 0x10b;
    constexpr uint16_t PE32PLUS = 0x20b;

    const auto& dos = *at<IMAGE_DOS_HEADER*>(base, 0);
    if (dos.e_magic != MZ) return;
    if (*at<uint32_t*>(base, dos.e_lfanew) != PE) return;

    const auto& fileHeader =
        *at<IMAGE_FILE_HEADER*>(base, dos.e_lfanew + sizeof(PE));
    if (fileHeader.Machine == MACHINE_AMD64) {
      auto& optHeader = *at<IMAGE_OPTIONAL_HEADER64*>(
        base, dos.e_lfanew + sizeof(PE) + sizeof(IMAGE_FILE_HEADER));
      if (optHeader.Magic != PE32PLUS) return;

      arch_ = CPU::amd64;
      base_ = at<uint8_t*>(base, 0);
      directories_ = optHeader.DataDirectory;
      entrypoint_ = optHeader.AddressOfEntryPoint;
    }
    else if (fileHeader.Machine == MACHINE_I386) {
      auto& optHeader = *at<IMAGE_OPTIONAL_HEADER32*>(
        base, dos.e_lfanew + sizeof(PE) + sizeof(IMAGE_FILE_HEADER));
      if (optHeader.Magic != PE32) return;

      arch_ = CPU::x86;
      base_ = at<uint8_t*>(base, 0);
      directories_ = optHeader.DataDirectory;
      entrypoint_ = optHeader.AddressOfEntryPoint;
    }
  }

  operator bool() const {
    return !!base_;
  }
};

}
