@REM Add pre-link commands below.
set COPYCMD=xcopy /q /r /y

%COPYCMD% netrtwlanu.dll %SG_OUTPUT_ROOT%\oak\files\
if [%WINCEREL%]==[1] (
	%COPYCMD% netrtwlanu.dll %_FLATRELEASEDIR%
	)
