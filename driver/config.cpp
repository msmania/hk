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
         && _stricmp(targetProcess_, target) == 0;
}

bool ConfigInternal::IsImageEnabled(const UNICODE_STRING &target) {
  if (mode_ == Mode::Uninitialized
      || !targetImage_[0]) return false;

  uint16_t stringLen = static_cast<uint16_t>(wcslen(targetImage_) * sizeof(wchar_t));
  uint16_t bufferLen = stringLen + sizeof(wchar_t);

  if (target.Length < stringLen) return false;

  const UNICODE_STRING current = {stringLen, bufferLen, targetImage_};
  const UNICODE_STRING targetUstr = {
    stringLen, bufferLen,
    at<wchar_t*>(target.Buffer, target.Length - stringLen)
    };

  return RtlEqualUnicodeString(&current, &targetUstr,
                               /*CaseInSensitive*/TRUE);
}
