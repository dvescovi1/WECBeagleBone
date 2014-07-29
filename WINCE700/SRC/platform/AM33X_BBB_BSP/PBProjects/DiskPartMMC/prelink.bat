@REM Add pre-link commands below.
set COPYCMD=xcopy /q /r /y

%COPYCMD% SCRIPTS\*.* %SG_OUTPUT_ROOT%\oak\files\
if [%WINCEREL%]==[1] (
	%COPYCMD% SCRIPTS\*.* %_FLATRELEASEDIR%
	)
