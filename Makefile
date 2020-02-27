!IF "$(PLATFORM)"=="X64" || "$(PLATFORM)"=="x64"
ARCH=amd64
!ELSE
ARCH=x86
!ENDIF

WDKINCPATH=C:\Program Files (x86)\Windows Kits\10\Include\10.0.18362.0\km
WDKLIBPATH=C:\Program Files (x86)\Windows Kits\10\Lib\10.0.18362.0\km\$(PLATFORM)
CODESIGN_SHA1=19fe76577ba49493677e0a39b0b11c2abab56b2e

OUTDIR=bin\$(ARCH)
OBJDIR=obj\$(ARCH)

OBJS_CONTROLLER=\
	controller\$(OBJDIR)\main.obj\

OBJS_PROBE=\
	probe\$(OBJDIR)\dllmain.obj\

OBJS_DRIVER=\
	driver\$(OBJDIR)\config.obj\
	driver\$(OBJDIR)\heap.obj\
	driver\$(OBJDIR)\main.obj\
	driver\$(OBJDIR)\peimage.obj\
	driver\$(OBJDIR)\process.obj\

CC=cl
LINKER=link
SIGNTOOL=signtool
RD=rd/s/q
RM=del/q

CONTROLLER=hkc.exe
DRIVER=hk.sys
PROBE=probe.dll

# warning C4514: unreferenced inline function has been removed
# warning C4710: function not inlined
# warning C5045: Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
CFLAGS=\
	/nologo\
	/c\
	/Od\
	/Wall\
	/Zi\
	/Fo"$(OBJDIR)\\"\
	/Fd"$(OBJDIR)\\"\
	/I"shared"\
	/wd4514\
	/wd4710\
	/wd5045\

# warning C4201: nonstandard extension used: nameless struct/union
# warning C4820: X bytes padding added after data member
CFLAGS_DRIVER=\
	$(CFLAGS)\
	/GF\
	/Gy\
	/GR-\
	/Gz\
	/kernel\
	/cbstring\
	/fp:precise\
	/Zp8\
	/Zc:wchar_t-\
	/Zc:forScope\
	/Zc:inline\
!IF "$(ARCH)"=="amd64"
	/d2epilogunwind\
	/D_AMD64_\
!ELSE
	/D_X86_\
!ENDIF
	/I"$(WDKINCPATH)"\
	/I"$(WDKINCPATH)\crt"\
	/wd4201\
	/wd4820\

CFLAGS_PROBE=\
	$(CFLAGS)\

LFLAGS=\
	/NOLOGO\
	/DEBUG\
	/INCREMENTAL:NO\

LFLAGS_CONSOLE=\
	$(LFLAGS)\
	/SUBSYSTEM:CONSOLE\

LFLAGS_DRIVER=\
	$(LFLAGS)\
	/SUBSYSTEM:NATIVE\
	/DRIVER\
	/KERNEL\
	/ENTRY:"DriverEntry"\
	/RELEASE\
	/SECTION:"INIT,d"\
	/NODEFAULTLIB\
	/MANIFEST:NO\
	/MERGE:"_TEXT=.text;_PAGE=PAGE"\
	/LIBPATH:"$(WDKLIBPATH)"\

LFLAGS_PROBE=\
	$(LFLAGS)\
	/SUBSYSTEM:WINDOWS\
	/DLL\
	/DEF:probe\probe.def\

LIBS_DRIVER=\
	bufferoverflowfastfailk.lib\
	ntoskrnl.lib\

all: $(OUTDIR)\$(CONTROLLER) $(OUTDIR)\$(DRIVER) $(OUTDIR)\$(PROBE)

$(OUTDIR)\$(CONTROLLER): $(OBJS_CONTROLLER)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS_CONSOLE) /PDB:"$(@R).pdb" /OUT:$@ $**

$(OUTDIR)\$(DRIVER): $(OBJS_DRIVER)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS_DRIVER) $(LIBS_DRIVER) /PDB:"$(@R)_driver.pdb" /IMPLIB:"$(@R).lib" /OUT:$@ $**
	$(SIGNTOOL) sign /ph /sha1 $(CODESIGN_SHA1) $@

$(OUTDIR)\$(PROBE): $(OBJS_PROBE)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS_PROBE) /PDB:"$(@R).pdb" /OUT:$@ $**

{controller}.cpp{controller\$(OBJDIR)}.obj:
	@if not exist controller\$(OBJDIR) mkdir controller\$(OBJDIR)
	$(CC) $(CFLAGS) /DUNICODE /Fo"controller\$(OBJDIR)\\" /Fd"controller\$(OBJDIR)\\" $<

{driver}.cpp{driver\$(OBJDIR)}.obj:
	@if not exist driver\$(OBJDIR) mkdir driver\$(OBJDIR)
	$(CC) $(CFLAGS_DRIVER) /Fo"driver\$(OBJDIR)\\" /Fd"driver\$(OBJDIR)\\" $<

{probe}.cpp{probe\$(OBJDIR)}.obj:
	@if not exist probe\$(OBJDIR) mkdir probe\$(OBJDIR)
	$(CC) $(CFLAGS_PROBE) /Fo"probe\$(OBJDIR)\\" /Fd"probe\$(OBJDIR)\\" $<

clean:
	@if exist controller\$(OBJDIR) $(RD) controller\$(OBJDIR)
	@if exist driver\$(OBJDIR) $(RD) driver\$(OBJDIR)
	@if exist probe\$(OBJDIR) $(RD) probe\$(OBJDIR)
	@if exist $(OUTDIR)\$(CONTROLLER) $(RM) $(OUTDIR)\$(CONTROLLER)
	@if exist $(OUTDIR)\$(CONTROLLER:exe=pdb) $(RM) $(OUTDIR)\$(CONTROLLER:exe=pdb)
	@if exist $(OUTDIR)\$(DRIVER) $(RM) $(OUTDIR)\$(DRIVER)
	@if exist $(OUTDIR)\hk_driver.pdb $(RM) $(OUTDIR)\hk_driver.pdb
	@if exist $(OUTDIR)\$(PROBE) $(RM) $(OUTDIR)\$(PROBE)
	@if exist $(OUTDIR)\$(PROBE:dll=pdb) $(RM) $(OUTDIR)\$(PROBE:dll=pdb)
	@if exist $(OUTDIR)\$(PROBE:dll=exp) $(RM) $(OUTDIR)\$(PROBE:dll=exp)
	@if exist $(OUTDIR)\$(PROBE:dll=lib) $(RM) $(OUTDIR)\$(PROBE:dll=lib)
