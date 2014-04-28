@REM Add post-link commands below.
set COPYCMD=xcopy /q /r /y

set SOURCE_DIR=%CommonProgramFiles%\Microsoft Shared\CoreCon\1.0\Target\wce400\%_TGTCPU%

@echo SOURCE_DIR=%SOURCE_DIR%

%COPYCMD% "%SOURCE_DIR%\*.*" "%SG_OUTPUT_ROOT%\oak\files\"

if [%WINCEREL%]==[1] (
	%COPYCMD% "%SOURCE_DIR%\*.*" "%_FLATRELEASEDIR%"
	)


