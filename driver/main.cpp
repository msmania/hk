#include <ntifs.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "config.h"
#include "peimage.h"
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

void Log(const char* format, ...) {
  char message[1024];
  va_list va;
  va_start(va, format);
  _vsnprintf(message, sizeof(message), format, va);
  vDbgPrintExWithPrefix("[HK] ", DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, message, va);
  va_end(va);
}

void Callback_CreateProcess(HANDLE ParentId,
                            HANDLE ProcessId,
                            BOOLEAN Create) {
  if (!Create) return;

  EProcess eproc(ProcessId);
  if (!eproc) return;

  if (!gConfig.IsEnabled(eproc.ProcessName())) return;

  KAPC_STATE state;
  KeStackAttachProcess(eproc, &state);
  {
    Process proc(ProcessId, GENERIC_ALL);
    PEImage pe(eproc.SectionBase());
    if (pe && proc) {
      pe.UpdateImportDirectory(proc);
    }
  }
  KeUnstackDetachProcess(&state);
}

void Callback_LoadImage(PUNICODE_STRING FullImageName,
                        HANDLE ProcessId,
                        PIMAGE_INFO ImageInfo) {
  if (!gConfig.IsEnabled(*FullImageName)) return;

  Log("LI: %ls\n", FullImageName->Buffer);

  PEImage pe(ImageInfo->ImageBase);
  if (!pe) return;
  Process proc(ProcessId, GENERIC_ALL);
  if (!proc) return;
  pe.UpdateImportDirectory(proc);
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
    case IOCTL_SETLI:
      if (IrpStack->Parameters.DeviceIoControl.InputBufferLength <= sizeof(gConfig.targetForLI_)
          && Irp->AssociatedIrp.SystemBuffer) {
        gConfig.mode_ = GlobalConfig::Mode::LI;
        memcpy(gConfig.targetForLI_,
               Irp->AssociatedIrp.SystemBuffer,
               IrpStack->Parameters.DeviceIoControl.InputBufferLength);
      }
      break;
    case IOCTL_SETINJECTEE:
      if (IrpStack->Parameters.DeviceIoControl.InputBufferLength <= sizeof(gConfig.injectee_)
          && Irp->AssociatedIrp.SystemBuffer) {
        memcpy(gConfig.injectee_,
               Irp->AssociatedIrp.SystemBuffer,
               IrpStack->Parameters.DeviceIoControl.InputBufferLength);
      }
      break;
    case IOCTL_SETCP:
      if (IrpStack->Parameters.DeviceIoControl.InputBufferLength <= sizeof(gConfig.targetForCP_)
          && Irp->AssociatedIrp.SystemBuffer) {
        gConfig.mode_ = GlobalConfig::Mode::CP;
        memcpy(gConfig.targetForCP_,
               Irp->AssociatedIrp.SystemBuffer,
               IrpStack->Parameters.DeviceIoControl.InputBufferLength);
      }
      break;
    case IOCTL_GETINFO:
      if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(gConfig)) {
        if (auto p = reinterpret_cast<GlobalConfig*>(Irp->AssociatedIrp.SystemBuffer)) {
          *p = gConfig;
          Irp->IoStatus.Information = sizeof(gConfig);
        }
      }
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

  status = PsSetCreateProcessNotifyRoutine(Callback_CreateProcess, /*Remove*/FALSE);
  if (!NT_SUCCESS(status)) {
    Log("PsSetCreateProcessNotifyRoutine failed - %08x\n", status);
    goto cleanup;
  }

  status = PsSetLoadImageNotifyRoutine(Callback_LoadImage);
  if (!NT_SUCCESS(status)) {
    Log("PsSetLoadImageNotifyRoutine failed - %08x\n", status);
    goto cleanup;
  }

  gConfig.Init();

cleanup:
  if (!NT_SUCCESS(status) && deviceObject) {
    IoDeleteDevice(deviceObject);
  }
  return status;
}

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
  PAGED_CODE();
  PsRemoveLoadImageNotifyRoutine(Callback_LoadImage);
  PsSetCreateProcessNotifyRoutine(Callback_CreateProcess, /*Remove*/TRUE);

  UNICODE_STRING linkName;
  RtlInitUnicodeString(&linkName, SYMLINK_NAME_K);
  IoDeleteSymbolicLink(&linkName);
  IoDeleteDevice(DriverObject->DeviceObject);

  Log("Driver unloaded.\n");
}
