#include <ntifs.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "config.h"
#include "heap.h"
#include "peimage.h"
#include "magic.h"
#include "process.h"

extern "C" {
  DRIVER_INITIALIZE DriverEntry;
  DRIVER_UNLOAD DriverUnload;
  NTSTATUS HkDispatchRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp);
}

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, HkDispatchRoutine)
#endif

#define HandleToULong(h) \
  static_cast<uint32_t>(reinterpret_cast<uintptr_t>(h) & 0xffffffff)

void Log(const char* format, ...) {
  char message[1024];
  va_list va;
  va_start(va, format);
  _vsnprintf(message, sizeof(message), format, va);
  vDbgPrintExWithPrefix(
      "[HK] ",
      DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, message, va);
  va_end(va);
}

static void *GetModuleBaseAddress(const Process &proc, int index) {
  if (!proc) return nullptr;

  const PPEB peb = proc.Peb();
  if (!peb && !peb->Ldr) return nullptr;

  auto firstItem = &peb->Ldr->InMemoryOrderModuleList;
  for (auto p = firstItem->Flink;
       p != firstItem && index >= 0;
       p = p->Flink, --index) {
    const auto currentTableEntry =
        CONTAINING_RECORD(p, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
    // Log("PEB: %ls\n", currentTableEntry->FullDllName.Buffer);
    if (index == 0) {
      return currentTableEntry->DllBase;
    }
  }

  return nullptr;
}

template <typename CallbackT>
static void PopulateProcess(HANDLE ProcessId, CallbackT&& Callback) {
  EProcess eproc(ProcessId);
  KAPC_STATE state;
  KeStackAttachProcess(eproc, &state);
  {
    Process proc(ProcessId, GENERIC_ALL);
    Callback(eproc, proc);
  }
  KeUnstackDetachProcess(&state);
}

void Callback_CreateProcess(HANDLE ParentId,
                            HANDLE ProcessId,
                            BOOLEAN Create) {
  if (!Create) return;

  EProcess eproc(ProcessId);
  if (!eproc) return;

  if (!gConfig.IsProcessEnabled(eproc.ProcessName())) return;

  Log("CP: %x --> %x %p\n",
      HandleToULong(ParentId),
      HandleToULong(ProcessId),
      eproc);

  if (gConfig.mode_ != GlobalConfig::Mode::CP
      && gConfig.mode_ != GlobalConfig::Mode::CCA) return;

  PopulateProcess(ProcessId, [](EProcess& eproc, Process& proc) {
    void* original_base = eproc.SectionBase();
    hk::PEImage pe(original_base);
    if (!pe || !proc) return;

    if (gConfig.mode_ == GlobalConfig::Mode::CP)
      pe.UpdateImportDirectory(proc);
    if (gConfig.mode_ == GlobalConfig::Mode::CCA) {
      // Too lazy to parse the VAD tree to find a valid address.
      // The odds are for us because the process is almost brand-new.
      // Cross your fingers..
      uintptr_t promiseland =
          reinterpret_cast<uintptr_t>(original_base) - 0x10000;
      eproc.CrackControlArea(gConfig.option_, promiseland);
    }
  });
}

void Callback_CreateThread(HANDLE ProcessId,
                           HANDLE ThreadId,
                           BOOLEAN Create) {
  if (!Create) return;

  EProcess eproc(ProcessId);
  if (!eproc) return;

  if (!gConfig.IsProcessEnabled(eproc.ProcessName())) return;

  if (gConfig.mode_ != GlobalConfig::Mode::Trace
      && gConfig.mode_ != GlobalConfig::Mode::CT)
    return;

  EThread ethread(ThreadId);
  if (gConfig.mode_ == GlobalConfig::Mode::CT) {
    if (ethread.CountThreadList() != 1) return;
  }

  Log("CT: %x.%x %p\n",
      HandleToULong(ProcessId),
      HandleToULong(ThreadId),
      ethread);

  if (gConfig.mode_ != GlobalConfig::Mode::CT) return;

  PopulateProcess(ProcessId, [](EProcess& eproc, Process& proc) {
    hk::PEImage pe(eproc.SectionBase());
    if (pe && proc) {
      pe.UpdateImportDirectory(proc);
    }
  });
}

bool EndsWith(const UNICODE_STRING &fullString, const wchar_t *leafName);

void Callback_LoadImage(PUNICODE_STRING FullImageName,
                        HANDLE ProcessId,
                        PIMAGE_INFO ImageInfo) {
  EProcess eproc(ProcessId);
  if (!eproc) return;

  if (!gConfig.IsProcessEnabled(eproc.ProcessName())) return;

  const bool do_populate =
    (gConfig.mode_ == GlobalConfig::Mode::LI
     && gConfig.IsImageEnabled(*FullImageName))
    || (gConfig.mode_ == GlobalConfig::Mode::K32
        && EndsWith(*FullImageName, L"kernel32.dll"));

  if (gConfig.mode_ == GlobalConfig::Mode::Trace || do_populate) {
    Log("LI:  %x.%x %p %ls\n",
        HandleToULong(ProcessId),
        HandleToULong(PsGetCurrentThreadId()),
        ImageInfo->ImageBase,
        FullImageName->Buffer);
  }

  if (!do_populate) return;

  PopulateProcess(ProcessId, [](EProcess& eproc, Process& proc) {
    void *targetBase = gConfig.mode_ == GlobalConfig::Mode::K32
        ? GetModuleBaseAddress(proc, 1)
         : eproc.SectionBase();
    hk::PEImage pe(targetBase);
    if (pe && proc) {
      pe.UpdateImportDirectory(proc);
    }
  });
}

NTSTATUS HkDispatchRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
  UNREFERENCED_PARAMETER(DeviceObject);
  PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;

  switch (IrpStack->MajorFunction) {
  case IRP_MJ_CREATE:
    break;
  case IRP_MJ_CLOSE:
    break;
  case IRP_MJ_DEVICE_CONTROL:
    switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_RECV:
      Irp->IoStatus.Information = gConfig.Export(
        Irp->AssociatedIrp.SystemBuffer,
        IrpStack->Parameters.DeviceIoControl.OutputBufferLength);
      break;
    case IOCTL_SEND:
      gConfig.Import(
        Irp->AssociatedIrp.SystemBuffer,
        IrpStack->Parameters.DeviceIoControl.InputBufferLength);
      break;
    }
    break;
  }

  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return Irp->IoStatus.Status;
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
                     _In_ PUNICODE_STRING RegistryPath) {
  PAGED_CODE();
  UNREFERENCED_PARAMETER(RegistryPath);
  NTSTATUS status = STATUS_SUCCESS;
  UNICODE_STRING deviceName;
  UNICODE_STRING linkName;
  RtlInitUnicodeString(&deviceName, DEVICE_NAME_K);
  RtlInitUnicodeString(&linkName, SYMLINK_NAME_K);

  DriverObject->DriverUnload = DriverUnload;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = HkDispatchRoutine;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = HkDispatchRoutine;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HkDispatchRoutine;

  PDEVICE_OBJECT deviceObject = nullptr;
  status = IoCreateDevice(DriverObject,
                          /*DeviceExtensionSize*/0,
                          &deviceName,
                          FILE_DEVICE_UNKNOWN,
                          /*DeviceCharacteristics*/0,
                          /*Exclusive*/FALSE,
                          &deviceObject);
  if (!NT_SUCCESS(status))  {
    Log("IoCreateDevice failed - %08x\n", status);
    goto cleanup;
  }

  status = IoCreateSymbolicLink(&linkName, &deviceName);
  if (!NT_SUCCESS(status)) {
    Log("IoCreateSymbolicLink failed - %08x\n", status);
    goto cleanup;
  }

  status = PsSetCreateProcessNotifyRoutine(Callback_CreateProcess,
                                           /*Remove*/FALSE);
  if (!NT_SUCCESS(status)) {
    Log("PsSetCreateProcessNotifyRoutine failed - %08x\n", status);
    goto cleanup;
  }

  status = PsSetCreateThreadNotifyRoutine(Callback_CreateThread);
  if (!NT_SUCCESS(status)) {
    Log("PsSetCreateThreadNotifyRoutine failed - %08x\n", status);
    goto cleanup;
  }

  status = PsSetLoadImageNotifyRoutine(Callback_LoadImage);
  if (!NT_SUCCESS(status)) {
    Log("PsSetLoadImageNotifyRoutine failed - %08x\n", status);
    goto cleanup;
  }

  gConfig.Init();
  gMagic.Init();

cleanup:
  if (!NT_SUCCESS(status) && deviceObject) {
    IoDeleteDevice(deviceObject);
  }
  return status;
}

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
  PAGED_CODE();
  PsRemoveLoadImageNotifyRoutine(Callback_LoadImage);
  PsRemoveCreateThreadNotifyRoutine(Callback_CreateThread);
  PsSetCreateProcessNotifyRoutine(Callback_CreateProcess, /*Remove*/TRUE);

  UNICODE_STRING linkName;
  RtlInitUnicodeString(&linkName, SYMLINK_NAME_K);
  IoDeleteSymbolicLink(&linkName);
  IoDeleteDevice(DriverObject->DeviceObject);

  Log("Driver unloaded.\n");
}
