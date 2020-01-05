#pragma once

struct ConfigInternal final : GlobalConfig {
  void Init();
  bool IsProcessEnabled(const char *target) const;
  bool IsImageEnabled(const UNICODE_STRING &target);

  bool Import(const void *buffer, uint32_t bufferSize);
  uint32_t Export(void *buffer, uint32_t bufferSize) const;
};

extern ConfigInternal gConfig;
