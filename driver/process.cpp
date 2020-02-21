#include <ntifs.h>
#include <stdint.h>

#include "common.h"
#include "process.h"

#if defined(_AMD64_)
constexpr int kEProcess_SectionBaseAddress = 0x3c8;
constexpr int kEThread_ThreadListEntry = 0x2f8;
#elif defined(_X86_)
constexpr int kEProcess_SectionBaseAddress = 0x130;
constexpr int kEThread_ThreadListEntry = 0x1d4;
#endif

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
  return *at<void* const*>(obj_, kEProcess_SectionBaseAddress);
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
  auto p = at<PLIST_ENTRY>(obj_, kEThread_ThreadListEntry)->Flink;
  for (;;) {
    ++n;
    if (n > 0xffff) return -1; // something wrong
    auto thread = at<void*>(p, -kEThread_ThreadListEntry);
    if (thread == obj_) break;
    p = p->Flink;
  }
  return n - 1;
}
