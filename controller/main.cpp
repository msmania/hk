#include <cstdint>
#include <windows.h>
#include <stdio.h>

#include "common.h"

class HkDriver final {
  HANDLE device_;

  Payload Receive() const {
    DWORD dw;
    Payload payload = {};
    if (DeviceIoControl(device_,
                         IOCTL_RECV,
                         /*lpInBuffer*/nullptr,
                         /*nInBufferSize*/0,
                         /*lpOutBuffer*/&payload,
                         /*nOutBufferSize*/sizeof(payload),
                         /*lpBytesReturned*/&dw,
                         /*lpOverlapped*/nullptr)
        && dw == sizeof(payload)) {
      payload.flags_ = 1;
    }
    else {
      printf("DeviceIoControl failed - %08lx\n", GetLastError());
      payload.flags_ = 0;
    }
    return payload;
  }

  bool Send(Payload &payload) const {
    DWORD dw;
    if (!DeviceIoControl(device_,
                         IOCTL_SEND,
                         /*lpInBuffer*/&payload,
                         /*nInBufferSize*/sizeof(payload),
                         /*lpOutBuffer*/nullptr,
                         /*nOutBufferSize*/0,
                         /*lpBytesReturned*/&dw,
                         /*lpOverlapped*/nullptr)) {
      printf("DeviceIoControl failed - %08lx\n", GetLastError());
      return false;
    }
    return true;
  }

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
    auto payload = Receive();
    if (!payload.flags_) return;

    const auto &config = payload.config_;
    switch (config.mode_) {
    default:
      printf("Not activated\n");
      break;
    case GlobalConfig::Mode::Trace:
      printf("Mode:   Trace\n"
             "Target: %hs\n",
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
    case GlobalConfig::Mode::K32:
      printf("Mode:     Kernel32\n"
             "Target:   %hs\n"
             "Injectee: %hs\n",
             config.targetProcess_,
             config.injectee_);
      break;
    }
  }

  void SetInjectee(const char *newTarget) const {
    Payload payload = {Payload::Injectee};
    auto &config = payload.config_;

    auto bufferLen = static_cast<uint32_t>(strlen(newTarget) + sizeof(char));
    if (bufferLen > sizeof(config.injectee_))
      return;

    memcpy(config.injectee_, newTarget, bufferLen);

    Send(payload);
  }

  void SetHookForProcess(const char *newTarget, GlobalConfig::Mode mode) const {
    Payload payload = {Payload::Mode | Payload::TargetProcess};
    auto &config = payload.config_;

    auto bufferLen = static_cast<uint32_t>(strlen(newTarget) + sizeof(char));
    if (bufferLen > sizeof(config.targetProcess_))
      return;

    config.mode_ = mode;
    memcpy(config.targetProcess_, newTarget, bufferLen);

    Send(payload);
  }

  void SetLI(const char *newTarget) const {
    Payload payload =
        {Payload::Mode | Payload::TargetProcess | Payload::TargetImage};
    auto &config = payload.config_;

    auto bufferLen = static_cast<uint32_t>(strlen(newTarget) + sizeof(char));
    if (bufferLen > sizeof(config.targetProcess_))
      return;

    config.mode_ = GlobalConfig::Mode::LI;
    memcpy(config.targetProcess_, newTarget, bufferLen);

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

    Send(payload);
  }
};

void ShowUsage() {
  printf("USAGE: hkc [COMMAND]\n\n"
         "  --info\n"
         "  --inject <injectee>\n"
         "  --[trace|li|cp|ct|k32] <target>\n"
         );
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    ShowUsage();
    return 1;
  }

  enum {uninitialized, info, inject, trace, li, cp, ct, k32}
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
    else if (strcmp(argv[1], "--k32") == 0)
      command = k32;
  }

  if (command != uninitialized) {
    HkDriver driver;
    if (driver) {
      switch (command) {
        case info: driver.GetInfo(); break;
        case inject: driver.SetInjectee(target); break;
        case trace:
          driver.SetHookForProcess(target, GlobalConfig::Mode::Trace);
          break;
        case li: driver.SetLI(target); break;
        case cp:
          driver.SetHookForProcess(target, GlobalConfig::Mode::CP);
          break;
        case ct:
          driver.SetHookForProcess(target, GlobalConfig::Mode::CT);
          break;
        case k32:
          driver.SetHookForProcess(target, GlobalConfig::Mode::K32);
          break;
      }
    }
  }
  else {
    ShowUsage();
    return 1;
  }
  return 0;
}
