@REM if defined PVR_BUILD_TRACE echo PVR_BUILD_TRACE: %~f0 [%*]
@rem ###############################################################################
@rem Name         : cebasecesysgen.bat
@rem Title        : Setup Environment variables
@rem Author       : Imagination Technologies
@rem Created      : 28/10/10
@rem
@rem  Copyright   : 2010 by Imagination Technologies. All rights reserved.
@rem              : No part of this software, either material or conceptual 
@rem              : may be copied or distributed, transmitted, transcribed,
@rem              : stored in a retrieval system or translated into any 
@rem              : human or computer language in any form by any means,
@rem              : electronic, mechanical, manual or other-wise, or 
@rem              : disclosed to third parties without the express written
@rem              : permission of Imagination Technologies Limited, Unit 8,
@rem              : HomePark Industrial Estate, King's Langley, Hertfordshire,
@rem              : WD4 8LZ, U.K.
@rem
@rem Description  : batch file
@rem
@rem
@rem Modifications:-
@rem
@rem $Log: cebasecesysgen.bat $

@rem ############################ Enable Powervr build ##############################
@rem set SYSGEN_POWERVR=1

if "%PVR_CONSUMER_ROOT%"=="" goto :no_source
@rem ###############################################################################
@rem ########################### Source build ######################################
@rem ###############################################################################

call %PVR_CONSUMER_ROOT%\builds\windowsce\cebasecesysgen.bat %1
goto :EOF

:no_source

@rem ###############################################################################
@rem ########################### Binary build ######################################
@rem ###############################################################################
set PVR_BINARY_BUILD=1
