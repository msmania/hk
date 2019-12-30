#include <ntifs.h>
#include <stdint.h>

#include "common.h"
#include "process.h"

Process::Process(HANDLE Pid, ACCESS_MASK desiredAccess) : process_{} {
  CLIENT_ID id = {Pid, 0};
  OBJECT_ATTRIBUTES object = {sizeof(object)};
  NTSTATUS status = ZwOpenProcess(&process_, desiredAccess, &object, &id);
  if (!NT_SUCCESS(status))  {
    Log("ZwOpenProcess failed - %08x\n", status);
    process_ = nullptr;
    return;
  }
}

Process::~Process() {
  if (process_) ZwClose(process_);
}

Process::operator bool() const {
  return !!process_;
}

Process::operator HANDLE() const {
  return process_;
}

EProcess::EProcess(HANDLE pid) : obj_{} {
  NTSTATUS status = PsLookupProcessByProcessId(pid, &obj_);
  if (!NT_SUCCESS(status))  {
    Log("PsLookupProcessByProcessId failed - %08x\n", status);
    obj_ = nullptr;
  }
}

EProcess::~EProcess() {
  if (obj_) ObDereferenceObject(obj_);
}

EProcess::operator bool() const {
  return !!obj_;
}

EProcess::operator PKPROCESS() {
  return obj_;
}

const char *EProcess::ProcessName() const {
  return at<const char*>(obj_, 0x450);
}

void *EProcess::SectionBase() const {
  return *at<void* const*>(obj_, 0x3c8);
}
