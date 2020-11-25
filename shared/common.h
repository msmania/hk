#pragma once

void Log(const char* format, ...);

template<typename T, typename U>
T at(void *base,  U offset) {
  return reinterpret_cast<T>(reinterpret_cast<uint8_t*>(base) + offset);
}

template<typename T, typename U>
T at(const void *base,  U offset) {
  return reinterpret_cast<T>(
      reinterpret_cast<const uint8_t*>(base) + offset);
}

#define NEW_IOCTL(ID)\
  CTL_CODE(FILE_DEVICE_UNKNOWN,\
           ID,\
           METHOD_BUFFERED,\
           FILE_READ_DATA | FILE_WRITE_DATA)

#define IOCTL_RECV NEW_IOCTL(0x100)
#define IOCTL_SEND NEW_IOCTL(0x101)

#define DEVICE_NAME_U L"\\\\.\\hk"
#define DEVICE_NAME_K L"\\Device\\hk"
#define SYMLINK_NAME_K L"\\DosDevices\\hk"

struct GlobalConfig {
  enum class Mode {Uninitialized, Trace, LI, CP, CT, K32, CCA};
  Mode mode_;
  uint32_t option_;
  char injectee_[128];
  char targetProcess_[256];
  wchar_t targetImage_[256];
};

struct Payload {
  enum : uint32_t {
    Mode = 1 << 0,
    Injectee = 1 << 1,
    TargetProcess = 1 << 2,
    TargetImage = 1 << 3,
  };
  uint32_t flags_;
  GlobalConfig config_;
};
