#include <ntddk.h>
#include <stdint.h>
#include "common.h"
#include "config.h"

ConfigInternal gConfig;

void ConfigInternal::Init() {
  memset(this, 0, sizeof(*this));
}

bool ConfigInternal::IsEnabled(const char *target) const {
  return mode_ == Mode::CP
          && _stricmp(targetForCP_, target) == 0;
}

bool ConfigInternal::IsEnabled(const UNICODE_STRING &target) {
  if (mode_ != Mode::LI) return false;

  uint16_t stringLen = static_cast<uint16_t>(wcslen(targetForLI_) * sizeof(wchar_t));
  uint16_t bufferLen = stringLen + sizeof(wchar_t);

  if (target.Length < stringLen) return false;

  const UNICODE_STRING current = {stringLen, bufferLen, targetForLI_};
  const UNICODE_STRING targetUstr = {
    stringLen, bufferLen,
    at<wchar_t*>(target.Buffer, target.Length - stringLen)
    };

  return RtlEqualUnicodeString(&current, &targetUstr,
                               /*CaseInSensitive*/TRUE);
}
