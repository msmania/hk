#pragma once

struct ConfigInternal final : GlobalConfig {
  void Init();
  bool IsProcessEnabled(const char *target) const;
  bool IsImageEnabled(const UNICODE_STRING &target);
};

extern ConfigInternal gConfig;
