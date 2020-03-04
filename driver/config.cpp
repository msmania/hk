#include <ntddk.h>
#include <stdint.h>
#include "common.h"
#include "config.h"

ConfigInternal gConfig;

void ConfigInternal::Init() {
  memset(this, 0, sizeof(*this));
}

bool ConfigInternal::IsProcessEnabled(const char *target) const {
  return mode_ != Mode::Uninitialized
         && target
         && _stricmp(targetProcess_, target) == 0;
}

bool EndsWith(const UNICODE_STRING &fullString, const wchar_t *leafName) {
  uint16_t stringLen = static_cast<uint16_t>(wcslen(leafName) * sizeof(wchar_t));
  uint16_t bufferLen = stringLen + sizeof(wchar_t);

  if (fullString.Length < stringLen) return false;

  const UNICODE_STRING
    leaf = {
      stringLen, bufferLen,
      const_cast<wchar_t*>(leafName)
    },
    partial = {
      stringLen, bufferLen,
      at<wchar_t*>(fullString.Buffer, fullString.Length - stringLen)
    };
  return RtlEqualUnicodeString(&leaf, &partial, /*CaseInSensitive*/TRUE);
}

bool ConfigInternal::IsImageEnabled(const UNICODE_STRING &target) {
  if (mode_ == Mode::Uninitialized
      || !targetImage_[0]) return false;

  return EndsWith(target, targetImage_);
}

bool ConfigInternal::Import(const void *buffer, uint32_t bufferSize) {
  if (!buffer || bufferSize != sizeof(Payload))
    return false;

  const auto &payload = *reinterpret_cast<const Payload*>(buffer);
  const auto &config = payload.config_;

  if (payload.flags_ & Payload::Mode)
    mode_ = config.mode_;

  if (payload.flags_ & Payload::Injectee)
    memcpy(injectee_, config.injectee_, sizeof(injectee_));

  if (payload.flags_ & Payload::TargetProcess)
    memcpy(targetProcess_, config.targetProcess_, sizeof(targetProcess_));

  if (payload.flags_ & Payload::TargetImage)
    memcpy(targetImage_, config.targetImage_, sizeof(targetImage_));

  return true;
}

uint32_t ConfigInternal::Export(void *buffer, uint32_t bufferSize) const {
  if (bufferSize >= sizeof(Payload)) {
    if (auto payload = reinterpret_cast<Payload*>(buffer)) {
      payload->flags_ = 0;
      payload->config_ = gConfig;
      return sizeof(Payload);
    }
  }
  return 0;
}
