!ifndef PSEXEC_INCLUDED
!define PSEXEC_INCLUDED
 
!macro PowerShellExecMacro PSCommand
  InitPluginsDir
  ;Save command in a temp file
  Push $R1
  FileOpen $R1 $PLUGINSDIR\tempfile.ps1 w
  FileWrite $R1 "${PSCommand}"
  FileClose $R1
  Pop $R1
 
  !insertmacro PowerShellExecFileMacro "$PLUGINSDIR\tempfile.ps1"
!macroend
 
!macro PowerShellExecLogMacro PSCommand
  InitPluginsDir
  ;Save command in a temp file
  Push $R1
  FileOpen $R1 $PLUGINSDIR\tempfile.ps1 w
  FileWrite $R1 "${PSCommand}"
  FileClose $R1
  Pop $R1
 
  !insertmacro PowerShellExecFileLogMacro "$PLUGINSDIR\tempfile.ps1"
!macroend
 
!macro PowerShellExecFileMacro PSFile
  !define PSExecID ${__LINE__}
  Push $R0
 
  nsExec::ExecToStack 'powershell -inputformat none -ExecutionPolicy RemoteSigned -File "${PSFile}"  '
 
  Pop $R0 ;return value is first on stack
  ;script output is second on stack, leave on top of it
  IntCmp $R0 0 finish_${PSExecID}
  SetErrorLevel 2
 
finish_${PSExecID}:
  Exch ;now $R0 on top of stack, followed by script output
  Pop $R0
  !undef PSExecID
!macroend
 
!macro PowerShellExecFileLogMacro PSFile
  !define PSExecID ${__LINE__}
  Push $R0
 
  nsExec::ExecToLog 'powershell -inputformat none -ExecutionPolicy RemoteSigned -File "${PSFile}"  '
  Pop $R0 ;return value is on stack
  IntCmp $R0 0 finish_${PSExecID}
  SetErrorLevel 2
 
finish_${PSExecID}:
  Pop $R0
  !undef PSExecID
!macroend
 
!define PowerShellExec `!insertmacro PowerShellExecMacro`
!define PowerShellExecLog `!insertmacro PowerShellExecLogMacro`
!define PowerShellExecFile `!insertmacro PowerShellExecFileMacro`
!define PowerShellExecFileLog `!insertmacro PowerShellExecFileLogMacro`
 
!endif
