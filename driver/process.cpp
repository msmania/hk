#include <ntifs.h>
#include <stdint.h>

#include "common.h"
#include "magic.h"
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

PEB *Process::Peb() const {
  uint32_t bytesReturned;
  PROCESS_BASIC_INFORMATION info;
  NTSTATUS status = ZwQueryInformationProcess(process_,
                                              ProcessBasicInformation,
                                              &info,
                                              sizeof(info),
                                              &bytesReturned);
  if (!NT_SUCCESS(status))  {
    Log("ZwQueryInformationProcess failed - %08x\n", status);
    return nullptr;
  }
  return info.PebBaseAddress;
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
  UNICODE_STRING name;
  RtlInitUnicodeString(&name, L"PsGetProcessImageFileName");
  auto PsGetProcessImageFileName =
    reinterpret_cast<const char*(NTAPI*)(PEPROCESS)>(
      MmGetSystemRoutineAddress(&name));
  if (!PsGetProcessImageFileName) {
    Log("PsGetProcessImageFileName does not exist.\n");
    return nullptr;
  }

  return PsGetProcessImageFileName(obj_);
}

void *EProcess::SectionBase() const {
  return *at<void* const*>(obj_, gMagic.EProcess_SectionBaseAddress);
}

EThread::EThread(HANDLE tid) : obj_{} {
  NTSTATUS status = PsLookupThreadByThreadId(tid, &obj_);
  if (!NT_SUCCESS(status))  {
    Log("PsLookupThreadByThreadId failed - %08x\n", status);
    obj_ = nullptr;
  }
}

EThread::~EThread() {
  if (obj_) ObDereferenceObject(obj_);
}

EThread::operator bool() const {
  return !!obj_;
}

EThread::operator PKTHREAD() {
  return obj_;
}

int EThread::CountThreadList() const {
  int n = 0;
  auto p = at<PLIST_ENTRY>(obj_, gMagic.EThread_ThreadListEntry)->Flink;
  for (;;) {
    ++n;
    if (n > 0xffff) return -1; // something wrong
    auto thread = at<void*>(p, -gMagic.EThread_ThreadListEntry);
    if (thread == obj_) break;
    p = p->Flink;
  }
  return n - 1;
}
