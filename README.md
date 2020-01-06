# Hk

Hk is a DLL injection tookit via kernel driver.  Currently this uses IAT hooking as an injection technique.

If you're interested in user-mode DLL injection, please check out https://github.com/msmania/procjack.

## Software requirements to build

Don't worry, all free :wink:.

- Visual Studio<br />[https://www.visualstudio.com/downloads/](https://www.visualstudio.com/downloads/)
- Windows 10 SDK<br />[https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk)
- WDK for Windows 10<br />[https://developer.microsoft.com/en-us/windows/hardware/download-kits-windows-hardware-development](https://developer.microsoft.com/en-us/windows/hardware/download-kits-windows-hardware-development)

## How to build

### 1. Prepare a codesign certificate to sign a driver

In the current Windows, drivers need to be signed with a codesign certificate.  So you need to create a self-signed or CA-signed certificate.

`Makefile` in the repo already has a step to sign a driver file.  What you need to do is:

1. Prepare a certificate having 'codeSigning' (1.3.6.1.5.5.7.3.3) in the extended key usage section.  OpenSSL is always your friend.  If you prefer Microsoft'ish way, [this](https://msdn.microsoft.com/en-us/library/windows/desktop/jj835832.aspx) would be useful.

2. On your build machine, install your certificate to the 'My' Certificate store:<br />`certutil -add My <path_to_pfx>`

3. Update `CODESIGN_SHA1` in `./Makefile` with SHA1 hash of your certificate.

4. On you test machine to run the driver on, enable [testsigning mode](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option) or [kernel debugging](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/setting-up-kernel-mode-debugging-in-windbg--cdb--or-ntsd).

### 2. Build

Launch "x64 Native Tools Command Prompt for VS" and run `NMAKE` on the root of this repo.  Binaries will be generated under the subdirectory "bin/amd64".  The current code supports x64 only.

Depending on WDK version you have, you may need to update `WDKINCPATH` and `WDKLIBPATH` in `./Makefile`, too.

## How to set up

1. Copy `hk.sys`, `hkc.exe`, and `probe.dll` to a test machine.  I usually copy them into `C:\hk`.

2. Configure `hk.sys` as a kernel service and start it.

```
> sc create hk binpath= C:\hk\hk.sys type= kernel
> net start hk
```

## How to play

With hkc.exe, you can control:

  - when to inject an injectee ([CreateProcess](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-pssetcreateprocessnotifyroutine)/[CreateThread](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-pssetcreatethreadnotifyroutine)/[LoadImage](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-pssetloadimagenotifyroutine) kernel callback)
  - a path to an injectee [^1]
  - what to inject an injectee into

For example, if you want to inject probe.dll into notepad.exe when a process is created, run the following commands.

```
> C:\hk\hkc.exe --cp notepad.exe
> C:\hk\hkc.exe --inject C:\hk\probe.dll
```

Here are the other options of hkc.exe:

```
> C:\hk\hkc.exe
USAGE: hkc [COMMAND]

  --info
  --inject <injectee>
  --[trace|li|cp|ct] <target>
```

[^1]: An injectee must export a function with Ordinal 0n100, otherwise a target process fails to start.
