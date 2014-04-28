# All rights reserved ADENEO EMBEDDED 2010
!if 0 
;  Copyright (c) 2007, 2008 BSQUARE Corporation. All rights reserved.

;================================================================================
;             Texas Instruments OMAP(TM) Platform Software
; (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
;
; Use of this software is controlled by the terms and conditions found
; in the license agreement under which this software has been supplied.
;
;================================================================================
!endif

#
# Create MLO (sd card booting file) using xldrtocsd.raw and xldrsd.nb0
#
# Boot ROM requires xldr to be preceeded with two words containing image 
# size and destination address.  The xldrtocsd.raw file contains these
# two words and is prepended to the xldr image.
# 

_FILES=$(_PLATFORMROOT)\$(_TGTPLAT)\files
_SRC=$(_PLATFORMROOT)\$(_TGTPLAT)\src\bootloader\xldr\sdmemory
_TGT=$(SG_OUTPUT_ROOT)\platform\$(_TGTPLAT)\target\$(_TGTCPU)\$(WINCEDEBUG)
_BIN=$(_PLATFORMROOT)\$(_TGTPLAT)\bin

xldr_sd_mlo:
!if "$(NOLINK)" == ""
    romimage $(ROMIMAGE_FLAGS) xldrsd.bib
#   The OMAP boot ROM will not load the NAND XLDR unless some bytes are modified near the image start
    $(_FILES)\fileset.exe $(_TGT)\xldrsd.nb0 0x4 0x3c 0xff
!if "$(WINCEREL)"=="1"
    xcopy /Q /I $(_TGT)\xldrsd.* $(_FLATRELEASEDIR)
!endif
!endif

!if "$(NOLINK)" == ""
    copy /b $(_SRC)\xldrtocsd.raw + $(_TGT)\xldrsd.nb0 $(_TGT)\MLO
!if "$(WINCEREL)"=="1"
    xcopy /Q /I $(_TGT)\MLO $(_FLATRELEASEDIR)
!endif
!endif        
