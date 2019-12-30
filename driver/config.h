#pragma once

struct ConfigInternal final : GlobalConfig {
  void Init();
  bool IsEnabled(const char *target) const;
  bool IsEnabled(const UNICODE_STRING &target);
};

extern ConfigInternal gConfig;
