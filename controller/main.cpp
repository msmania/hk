#include <cstdint>
#include <windows.h>
#include <stdio.h>

#include "common.h"

class HkDriver final {
  HANDLE device_;

public:
  HkDriver() {
    device_ = CreateFile(DEVICE_NAME_U,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ,
                         /*lpSecurityAttributes*/nullptr,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         /*hTemplateFile*/nullptr);
    if (device_ == INVALID_HANDLE_VALUE) {
      printf("CreateFile failed - %08lx\n", GetLastError());
    }
  }

  ~HkDriver() {
    if (device_ != INVALID_HANDLE_VALUE) CloseHandle(device_);
  }

  operator bool() const {
    return device_ != INVALID_HANDLE_VALUE;
  }

  void GetInfo() const {
    DWORD dw;
    GlobalConfig config;
    if (!DeviceIoControl(device_,
                         IOCTL_GETINFO,
                         /*lpInBuffer*/nullptr,
                         /*nInBufferSize*/0,
                         /*lpOutBuffer*/&config,
                         /*nOutBufferSize*/sizeof(config),
                         /*lpBytesReturned*/&dw,
                         /*lpOverlapped*/nullptr)) {
      printf("DeviceIoControl failed - %08lx\n", GetLastError());
    }
  
    switch (config.mode_) {
    default:
      printf("Not activated\n");
      break;
    case GlobalConfig::Mode::Trace:
      printf("Mode:     Trace\n"
             "Target:   %hs\n",
             config.targetProcess_);
      break;
    case GlobalConfig::Mode::LI:
      printf("Mode:     LoadImage\n"
             "Target:   %ls\n"
             "Injectee: %hs\n",
             config.targetImage_,
             config.injectee_);
      break;
    case GlobalConfig::Mode::CP:
      printf("Mode:     CreateProcess\n"
             "Target:   %hs\n"
             "Injectee: %hs\n",
             config.targetProcess_,
             config.injectee_);
      break;
    case GlobalConfig::Mode::CT:
      printf("Mode:     CreateThread\n"
             "Target:   %hs\n"
             "Injectee: %hs\n",
             config.targetProcess_,
             config.injectee_);
      break;
    }
  }

  void SetInjectee(const char *newTarget) const {
    GlobalConfig config;

    auto bufferLen = static_cast<uint32_t>(strlen(newTarget) + sizeof(char));
    if (bufferLen > sizeof(config.injectee_))
      return;

    memcpy(config.injectee_, newTarget, bufferLen);

    DWORD dw;
    if (!DeviceIoControl(device_,
                         IOCTL_SETINJECTEE,
                         /*lpInBuffer*/config.injectee_,
                         /*nInBufferSize*/bufferLen,
                         /*lpOutBuffer*/nullptr,
                         /*nOutBufferSize*/0,
                         /*lpBytesReturned*/&dw,
                         /*lpOverlapped*/nullptr)) {
      printf("DeviceIoControl failed - %08lx\n", GetLastError());
    }
  }

  void SetTrace(const char *newTarget) const {
    GlobalConfig config;

    auto bufferLen = static_cast<uint32_t>(strlen(newTarget) + sizeof(char));
    if (bufferLen > sizeof(config.targetProcess_))
      return;

    memcpy(config.targetProcess_, newTarget, bufferLen);

    DWORD dw;
    if (!DeviceIoControl(device_,
                         IOCTL_SETTRACE,
                         /*lpInBuffer*/config.targetProcess_,
                         /*nInBufferSize*/bufferLen,
                         /*lpOutBuffer*/nullptr,
                         /*nOutBufferSize*/0,
                         /*lpBytesReturned*/&dw,
                         /*lpOverlapped*/nullptr)) {
      printf("DeviceIoControl failed - %08lx\n", GetLastError());
    }
  }

  void SetCP(const char *newTarget) const {
    GlobalConfig config;

    auto bufferLen = static_cast<uint32_t>(strlen(newTarget) + sizeof(char));
    if (bufferLen > sizeof(config.targetProcess_))
      return;

    memcpy(config.targetProcess_, newTarget, bufferLen);

    DWORD dw;
    if (!DeviceIoControl(device_,
                         IOCTL_SETCP,
                         /*lpInBuffer*/config.targetProcess_,
                         /*nInBufferSize*/bufferLen,
                         /*lpOutBuffer*/nullptr,
                         /*nOutBufferSize*/0,
                         /*lpBytesReturned*/&dw,
                         /*lpOverlapped*/nullptr)) {
      printf("DeviceIoControl failed - %08lx\n", GetLastError());
    }
  }

  void SetCT(const char *newTarget) const {
    GlobalConfig config;

    auto bufferLen = static_cast<uint32_t>(strlen(newTarget) + sizeof(char));
    if (bufferLen > sizeof(config.targetProcess_))
      return;

    memcpy(config.targetProcess_, newTarget, bufferLen);

    DWORD dw;
    if (!DeviceIoControl(device_,
                         IOCTL_SETCT,
                         /*lpInBuffer*/config.targetProcess_,
                         /*nInBufferSize*/bufferLen,
                         /*lpOutBuffer*/nullptr,
                         /*nOutBufferSize*/0,
                         /*lpBytesReturned*/&dw,
                         /*lpOverlapped*/nullptr)) {
      printf("DeviceIoControl failed - %08lx\n", GetLastError());
    }
  }

  void SetLI(const char *newTarget) const {
    SetCP(newTarget);

    GlobalConfig config;
    int convertedChars = MultiByteToWideChar(
      CP_OEMCP,
      MB_ERR_INVALID_CHARS,
      newTarget,
      -1,
      config.targetImage_,
      sizeof(config.targetImage_) / sizeof(wchar_t));
    if (!convertedChars) {
      printf("MultiByteToWideChar failed - %08lx\n", GetLastError());
      return;
    }

    DWORD dw;
    if (!DeviceIoControl(device_,
                         IOCTL_SETLI,
                         /*lpInBuffer*/config.targetImage_,
                         /*nInBufferSize*/convertedChars * sizeof(wchar_t),
                         /*lpOutBuffer*/nullptr,
                         /*nOutBufferSize*/0,
                         /*lpBytesReturned*/&dw,
                         /*lpOverlapped*/nullptr)) {
      printf("DeviceIoControl failed - %08lx\n", GetLastError());
    }
  }
};

void ShowUsage() {
  printf("USAGE: hkc [COMMAND]\n\n"
         "  --info\n"
         "  --inject <injectee>\n"
         "  --[trace|li|cp|ct] <target>\n"
         );
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    ShowUsage();
    return 1;
  }

  enum {uninitialized, info, inject, trace, li, cp, ct}
    command = uninitialized;
  const char *target = nullptr;

  if (strcmp(argv[1], "--info") == 0)
    command = info;
  else if (argc >= 3) {
    target = argv[2];
    if (strcmp(argv[1], "--inject") == 0)
      command = inject;
    else if (strcmp(argv[1], "--trace") == 0)
      command = trace;
    else if (strcmp(argv[1], "--li") == 0)
      command = li;
    else if (strcmp(argv[1], "--cp") == 0)
      command = cp;
    else if (strcmp(argv[1], "--ct") == 0)
      command = ct;
  }

  if (command != uninitialized) {
    HkDriver driver;
    if (driver) {
      switch (command) {
        case info: driver.GetInfo(); break;
        case inject: driver.SetInjectee(target); break;
        case trace: driver.SetTrace(target); break;
        case li: driver.SetLI(target); break;
        case cp: driver.SetCP(target); break;
        case ct: driver.SetCT(target); break;
      }
    }
  }
  else {
    ShowUsage();
    return 1;
  }
  return 0;
}
