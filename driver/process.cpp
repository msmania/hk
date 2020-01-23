#include <ntifs.h>
#include <stdint.h>

#include "common.h"
#include "process.h"

constexpr int32_t ETHREAD_ThreadListEntry = 0x2f8;
constexpr int32_t EPROCESS_SectionBaseAddress = 0x3c8;
constexpr int32_t EPROCESS_VadRoot = 0x658;
constexpr int32_t MMVAD_SubSection = 0x48;
constexpr int32_t SUBSECTION_ControlArea = 0;
constexpr int32_t CONTROL_AREA_Flags = 0x38;
constexpr int32_t CONTROL_AREA_FilePointer = 0x40;

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
  return *at<void* const*>(obj_, EPROCESS_SectionBaseAddress);
}

RTL_BALANCED_NODE *SearchTree(RTL_BALANCED_NODE *node,
                              bool (*Comparer)(RTL_BALANCED_NODE*)) {
  if (!node) return nullptr;
  if (Comparer(node)) return node;

  auto found = SearchTree(node->Left, Comparer);
  if (found) return found;

  return SearchTree(node->Right, Comparer);
}

void EProcess::SearchVad(const wchar_t *filename) const {
  auto root = *at<RTL_BALANCED_NODE**>(obj_, EPROCESS_VadRoot);
  while (root->ParentValue & ~7) {
    root = reinterpret_cast<RTL_BALANCED_NODE*>(root->ParentValue & ~7);
  }

  auto found = SearchTree(root, [](RTL_BALANCED_NODE *node) -> bool {
    auto subSection = *at<void**>(node, MMVAD_SubSection);
    if (!subSection) return false;

    auto controlArea = *at<void**>(subSection, SUBSECTION_ControlArea);
    if (!controlArea) return false;

    auto flags = *at<uint32_t*>(controlArea, CONTROL_AREA_Flags);
    if (!(flags & 0x80)) return false;

    auto filePointer = *at<void**>(controlArea, CONTROL_AREA_FilePointer);

    return true;
  });

  Log("Found = %p\n", found);
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
  auto p = at<PLIST_ENTRY>(obj_, ETHREAD_ThreadListEntry)->Flink;
  for (;;) {
    ++n;
    if (n > 0xffff) return -1; // something wrong
    auto thread = at<void*>(p, -ETHREAD_ThreadListEntry);
    if (thread == obj_) break;
    p = p->Flink;
  }
  return n - 1;
}
