#pragma once

#include "pe.h"

namespace hk {

class PEImage : public PEImageBase {
  template<typename NewImportDirectory>
  bool UpdateImportDirectoryInternal(HANDLE process);

public:
  PEImage(void* base);

  bool UpdateImportDirectory(HANDLE process);
  void DumpImportTable() const;
  hk::VS_FIXEDFILEINFO GetVersion() const;
};

}
