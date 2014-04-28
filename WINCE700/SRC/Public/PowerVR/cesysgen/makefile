#
# Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/ 
# ALL RIGHTS RESERVED
#
################################################################################
# Name         : makefile
# Title        : CEBuild makefile for powervr
# Author       : Imagination Technologies
# Created      : 27/10/10
#
#  Copyright   : 2010 by Imagination Technologies. All rights reserved.
#              : No part of this software, either material or conceptual 
#              : may be copied or distributed, transmitted, transcribed,
#              : stored in a retrieval system or translated into any 
#              : human or computer language in any form by any means,
#              : electronic, mechanical, manual or other-wise, or 
#              : disclosed to third parties without the express written
#              : permission of Imagination Technologies Limited, Unit 8,
#              : HomePark Industrial Estate, King's Langley, Hertfordshire,
#              : WD4 8LZ, U.K.
#
# Description  : Makefile
#
#
# Modifications:-
# $Log: makefile $
# 
################################################################################

WINCEPROJ=powervr

!IF "$(SYSGEN_POWERVR)" == "1"

!IF "$(PVR_CONSUMER_ROOT)" != ""
################################################################################
########################### Source build #######################################
################################################################################

!INCLUDE $(PVR_CONSUMER_ROOT)\builds\windowsce\makefile

!ELSE
!IF "$(PVR_BINARY_BUILD)" == "1"
################################################################################
########################### Binary build #######################################
################################################################################

!INCLUDE $(_PUBLICROOT)\common\cesysgen\cesysgen.mak

################################################################################
# Preproc
################################################################################
preproc: copy_binaries copy_files

################################################################################
# Postproc
################################################################################
postproc: 

################################################################################
# Copy binaries to output directory
################################################################################
copy_binaries:
!IF "$(SYSGEN_PVR_SGXCOREREV_121)" == "1"
	-@COPY $(_PUBLICROOT)\powervr\oak\target\Rev121\$(_CPUINDPATH)\*.*	$(SG_OUTPUT_OAKTGT)
!ENDIF
!IF "$(SYSGEN_PVR_SGXCOREREV_125)" == "1"
	-@COPY $(_PUBLICROOT)\powervr\oak\target\Rev125\$(_CPUINDPATH)\*.*	$(SG_OUTPUT_OAKTGT)
!ENDIF
	
################################################################################
# Copy files to output directory
################################################################################
copy_files:
	-@COPY $(_PUBLICROOT)\powervr\oak\files\*.*	$(SG_OUTPUT_OAKTGT)
	
!ELSE
################################################################################
############################### No build #######################################
################################################################################
!MESSAGE * No PowerVR sources or binaries are available!
!MESSAGE * Please set PVR_CONSUMER_ROOT to build the PowerVR source tree.

!ENDIF
!ENDIF
!ELSE
!MESSAGE * POWERVR SYSGEN is disabled Bypassing PowerVR

!INCLUDE $(_PUBLICROOT)\common\cesysgen\cesysgen.mak

################################################################################
# Preproc
################################################################################
preproc: 
	@echo PowerVR not built (preproc) files not Copied

################################################################################
# Postproc
################################################################################
postproc: 
!ENDIF

