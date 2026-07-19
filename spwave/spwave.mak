# Microsoft Developer Studio Generated NMAKE File, Based on spwave.dsp
!IF "$(CFG)" == ""
CFG=spwave - Win32 Release
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの spwave - Win32 Release を設定します。
!ENDIF 

!IF "$(CFG)" != "spwave - Win32 Release" && "$(CFG)" != "spwave - Win32 Debug"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "spwave.mak" CFG="spwave - Win32 Release"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "spwave - Win32 Release" ("Win32 (x86) Application" 用)
!MESSAGE "spwave - Win32 Debug" ("Win32 (x86) Application" 用)
!MESSAGE 
!ERROR 無効な構成が指定されています。
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "spwave - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\spwave.exe"


CLEAN :
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\spwave.res"
	-@erase "$(INTDIR)\swAnalysis.obj"
	-@erase "$(INTDIR)\swAnalysisDialog.obj"
	-@erase "$(INTDIR)\swCursor.obj"
	-@erase "$(INTDIR)\swDialog.obj"
	-@erase "$(INTDIR)\swDraw.obj"
	-@erase "$(INTDIR)\swEdit.obj"
	-@erase "$(INTDIR)\swInfoDialog.obj"
	-@erase "$(INTDIR)\swLabel.obj"
	-@erase "$(INTDIR)\swLabelDialog.obj"
	-@erase "$(INTDIR)\swLabelList.obj"
	-@erase "$(INTDIR)\swWave.obj"
	-@erase "$(INTDIR)\swWaveAudio.obj"
	-@erase "$(INTDIR)\swWindow.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\spwave.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "..\spAudio" /I "..\spComponent" /I "..\spBase" /I ".." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /Fp"$(INTDIR)\spwave.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\spwave.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/o"$(OUTDIR)\spwave.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=spAudio.lib spComponent.lib sp.lib spBase.lib htmlhelp.lib winmm.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\spwave.pdb" /machine:I386 /out:"$(OUTDIR)\spwave.exe" 
LINK32_OBJS= \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\swAnalysis.obj" \
	"$(INTDIR)\swAnalysisDialog.obj" \
	"$(INTDIR)\swCursor.obj" \
	"$(INTDIR)\swDialog.obj" \
	"$(INTDIR)\swDraw.obj" \
	"$(INTDIR)\swEdit.obj" \
	"$(INTDIR)\swInfoDialog.obj" \
	"$(INTDIR)\swLabel.obj" \
	"$(INTDIR)\swLabelDialog.obj" \
	"$(INTDIR)\swLabelList.obj" \
	"$(INTDIR)\swWave.obj" \
	"$(INTDIR)\swWaveAudio.obj" \
	"$(INTDIR)\swWindow.obj" \
	"$(INTDIR)\spwave.res"

"$(OUTDIR)\spwave.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "spwave - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\spwave.exe"


CLEAN :
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\spwave.res"
	-@erase "$(INTDIR)\swAnalysis.obj"
	-@erase "$(INTDIR)\swAnalysisDialog.obj"
	-@erase "$(INTDIR)\swCursor.obj"
	-@erase "$(INTDIR)\swDialog.obj"
	-@erase "$(INTDIR)\swDraw.obj"
	-@erase "$(INTDIR)\swEdit.obj"
	-@erase "$(INTDIR)\swInfoDialog.obj"
	-@erase "$(INTDIR)\swLabel.obj"
	-@erase "$(INTDIR)\swLabelDialog.obj"
	-@erase "$(INTDIR)\swLabelList.obj"
	-@erase "$(INTDIR)\swWave.obj"
	-@erase "$(INTDIR)\swWaveAudio.obj"
	-@erase "$(INTDIR)\swWindow.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\spwave.exe"
	-@erase "$(OUTDIR)\spwave.ilk"
	-@erase "$(OUTDIR)\spwave.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W2 /Gm /GX /ZI /Od /I "..\spAudio" /I "..\spComponent" /I "..\spBase" /I ".." /D "_DEBUG" /D "_MBCS" /D "SW_JAPANESE_VERSION" /D "WIN32" /D "_WINDOWS" /D "USE_CONSOLE" /Fp"$(INTDIR)\spwave.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC_PROJ=/l 0x411 /fo"$(INTDIR)\spwave.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\spwave.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=spAudio.lib spComponent.lib sp.lib spBase.lib htmlhelp.lib winmm.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\spwave.pdb" /debug /machine:I386 /out:"$(OUTDIR)\spwave.exe" 
LINK32_OBJS= \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\swAnalysis.obj" \
	"$(INTDIR)\swAnalysisDialog.obj" \
	"$(INTDIR)\swCursor.obj" \
	"$(INTDIR)\swDialog.obj" \
	"$(INTDIR)\swDraw.obj" \
	"$(INTDIR)\swEdit.obj" \
	"$(INTDIR)\swInfoDialog.obj" \
	"$(INTDIR)\swLabel.obj" \
	"$(INTDIR)\swLabelDialog.obj" \
	"$(INTDIR)\swLabelList.obj" \
	"$(INTDIR)\swWave.obj" \
	"$(INTDIR)\swWaveAudio.obj" \
	"$(INTDIR)\swWindow.obj" \
	"$(INTDIR)\spwave.res"

"$(OUTDIR)\spwave.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("spwave.dep")
!INCLUDE "spwave.dep"
!ELSE 
!MESSAGE Warning: cannot find "spwave.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "spwave - Win32 Release" || "$(CFG)" == "spwave - Win32 Debug"
SOURCE=.\main.c

"$(INTDIR)\main.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\spwave.rc

"$(INTDIR)\spwave.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\swAnalysis.c

"$(INTDIR)\swAnalysis.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\swAnalysisDialog.c

"$(INTDIR)\swAnalysisDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\swCursor.c

"$(INTDIR)\swCursor.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\swDialog.c

"$(INTDIR)\swDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\swDraw.c

"$(INTDIR)\swDraw.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\swEdit.c

"$(INTDIR)\swEdit.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\swInfoDialog.c

"$(INTDIR)\swInfoDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\swLabel.c

"$(INTDIR)\swLabel.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\swLabelDialog.c

"$(INTDIR)\swLabelDialog.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\swLabelList.c

"$(INTDIR)\swLabelList.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\swWave.c

"$(INTDIR)\swWave.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\swWaveAudio.c

"$(INTDIR)\swWaveAudio.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\swWindow.c

"$(INTDIR)\swWindow.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

