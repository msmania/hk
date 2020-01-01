#pragma once

void Log(const char* format, ...);

template<typename T, typename U>
T at(void *base,  U offset) {
  return reinterpret_cast<T>(reinterpret_cast<uint8_t*>(base) + offset);
}

#define NEW_IOCTL(ID)\
  CTL_CODE(FILE_DEVICE_UNKNOWN,\
           ID,\
           METHOD_BUFFERED,\
           FILE_READ_DATA | FILE_WRITE_DATA)

#define IOCTL_GETINFO     NEW_IOCTL(0x100)
#define IOCTL_SETINJECTEE NEW_IOCTL(0x101)
#define IOCTL_SETTRACE    NEW_IOCTL(0x102)
#define IOCTL_SETLI       NEW_IOCTL(0x103)
#define IOCTL_SETCP       NEW_IOCTL(0x104)
#define IOCTL_SETCT       NEW_IOCTL(0x105)

#define DEVICE_NAME_U L"\\\\.\\hk"
#define DEVICE_NAME_K L"\\Device\\hk"
#define SYMLINK_NAME_K L"\\DosDevices\\hk"

struct GlobalConfig {
  enum class Mode {Uninitialized, Trace, LI, CP, CT};
  Mode mode_;
  char injectee_[128];
  char targetProcess_[256];
  wchar_t targetImage_[256];
};
