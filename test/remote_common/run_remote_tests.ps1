<#
.SYNOPSIS
    Runs the tests of the remote execution machinery on Windows
    (replacement for ctest on machines without build infrastructure).

.DESCRIPTION
    Runs the test executables testexe_<name>.exe in the same way as they are
    registered with CTest (see test/remote_common/CMakeLists.txt), for the parts
    which are relevant on Windows:

     * unit tests (no remote target needed)
     * backend tests against
         - a WSL distribution        (-WslDistro or env INSIGHT_TEST_WSL_DISTRO)
         - a Linux host via ssh      (-SshHost   or env INSIGHT_TEST_SSH_HOST)
         - the remote server labelled "localhost" in remoteservers.list
                                     (-IncludeSshLocalhost)

    The "localshell" backend requires /bin/bash and is not run on Windows.

    Exit codes of the test executables: 0 = passed, 77 = skipped, other = failed.
    The output of each test is written to <LogDir>\<test>.log.

    Prerequisites on the targets:
     * WSL: "analyze" of the same build in the PATH of the distribution
     * SSH: passwordless ssh, "analyze" (same build), rsync and isPVFindPort.sh
            in the PATH of non-interactive sessions; locally ssh.exe and rsync.exe
            in the PATH

.PARAMETER TestDir
    Directory containing the testexe_*.exe files. Default: directory of this script.

.PARAMETER BinDir
    Directory containing analyze.exe and the InsightCAE DLLs.
    Default: ..\bin relative to TestDir, if it exists, otherwise the directory of
    analyze.exe found in PATH.

.PARAMETER AnalyzeExe
    analyze executable used by the tests (INSIGHT_TEST_ANALYZE_EXE).
    Default: <BinDir>\analyze.exe

.PARAMETER SharedDir
    InsightCAE shared directory (sets INSIGHT_GLOBALSHAREDDIRS), if the
    analysis modules (modules.d) are not found otherwise.

.PARAMETER QtPluginDir
    Qt plugin directory (containing platforms\qoffscreen.dll), set as QT_PLUGIN_PATH.
    Qt does not search PATH for plugins and the test executables are not located
    next to the Qt plugins. Default: the first of <BinDir>\plugins, <BinDir>,
    <BinDir>\..\plugins, which contains platforms\qoffscreen.dll.

.PARAMETER ExtraPath
    Additional directories to prepend to PATH (e.g. location of Qt/boost DLLs).

.PARAMETER WslDistro
    WSL distribution for the wsl-env backend (INSIGHT_TEST_WSL_DISTRO).

.PARAMETER WslBaseDir
    Base directory inside WSL (INSIGHT_TEST_WSL_BASEDIR, default /tmp).

.PARAMETER SshHost
    Host for the ssh-env backend (INSIGHT_TEST_SSH_HOST).

.PARAMETER SshBaseDir
    Base directory on the ssh host (INSIGHT_TEST_SSH_BASEDIR, default /tmp).

.PARAMETER IncludeSshLocalhost
    Also run the backend tests against the remote server labelled "localhost"
    in remoteservers.list (after the precondition check).

.PARAMETER Filter
    Regular expression: only tests whose name matches are run.

.PARAMETER Exclude
    Regular expression: tests whose name matches are not run.

.PARAMETER LogDir
    Directory for the test logs. Default: .\remote-test-logs-<timestamp>

.PARAMETER TimeoutScale
    Factor applied to all test timeouts.

.PARAMETER OutputOnFailure
    Print the last lines of the log of failed tests.

.PARAMETER List
    Only list the tests, which would be run.

.EXAMPLE
    .\run_remote_tests.ps1 -WslDistro Ubuntu-22.04 -OutputOnFailure

.EXAMPLE
    .\run_remote_tests.ps1 -SshHost buildserver -Filter "e2e|workbench"
#>

[CmdletBinding()]
param(
    [string]   $TestDir,
    [string]   $BinDir,
    [string]   $AnalyzeExe,
    [string]   $SharedDir,
    [string]   $QtPluginDir,
    [string[]] $ExtraPath = @(),
    [string]   $WslDistro  = $env:INSIGHT_TEST_WSL_DISTRO,
    [string]   $WslBaseDir = $env:INSIGHT_TEST_WSL_BASEDIR,
    [string]   $SshHost    = $env:INSIGHT_TEST_SSH_HOST,
    [string]   $SshBaseDir = $env:INSIGHT_TEST_SSH_BASEDIR,
    [switch]   $IncludeSshLocalhost,
    [string]   $Filter = ".",
    [string]   $Exclude,
    [string]   $LogDir,
    [double]   $TimeoutScale = 1.0,
    [switch]   $OutputOnFailure,
    [switch]   $List
)

Set-StrictMode -Version 2
$ErrorActionPreference = "Stop"

$SKIP_RETURN_CODE = 77


# ============ test definitions (keep in sync with the CMakeLists.txt files)

# tests without backend: name, timeout [s]
$unitTests = @(
    @{ Name = "remote_serverlist_config";   Timeout = 300 },
    @{ Name = "remote_lookforpattern";      Timeout = 60  },
    @{ Name = "remote_wslversion_parse";    Timeout = 60  },
    @{ Name = "remote_analyzeclient";       Timeout = 300 },
    @{ Name = "remote_analyzeserver_local"; Timeout = 900 },
    @{ Name = "remote_undosteps";           Timeout = 300 }
)

# backend-parametrised tests: name, timeout [s], SSH only
$backendTests = @(
    @{ Name = "remote_server_fileops";       Timeout = 300;  SshOnly = $false },
    @{ Name = "remote_server_backgroundjob"; Timeout = 300;  SshOnly = $false },
    @{ Name = "remote_server_sync";          Timeout = 300;  SshOnly = $false },
    @{ Name = "remote_location";             Timeout = 300;  SshOnly = $false },
    @{ Name = "remote_analyze_e2e";          Timeout = 1200; SshOnly = $false },
    @{ Name = "remote_workbench_remoterun";  Timeout = 1800; SshOnly = $false },
    @{ Name = "remote_ssh_portmapping";      Timeout = 300;  SshOnly = $true  },
    @{ Name = "remote_ssh_lifecycle";        Timeout = 300;  SshOnly = $true  }
)


# ============ locations

if (-not $TestDir) { $TestDir = $PSScriptRoot }
$TestDir = (Resolve-Path $TestDir).Path

if (-not $BinDir)
{
    $candidate = Join-Path $TestDir "..\bin"
    if (Test-Path (Join-Path $candidate "analyze.exe"))
    {
        $BinDir = $candidate
    }
    else
    {
        $cmd = Get-Command analyze.exe -ErrorAction SilentlyContinue
        if ($cmd) { $BinDir = Split-Path $cmd.Source }
    }
}
if ($BinDir) { $BinDir = (Resolve-Path $BinDir).Path }

if (-not $AnalyzeExe -and $BinDir) { $AnalyzeExe = Join-Path $BinDir "analyze.exe" }
if (-not $AnalyzeExe -or -not (Test-Path $AnalyzeExe))
{
    throw "analyze.exe not found (looked for '$AnalyzeExe'). Please specify -BinDir or -AnalyzeExe."
}
$AnalyzeExe = (Resolve-Path $AnalyzeExe).Path

# Qt plugins (offscreen platform plugin for the workbench test)
if (-not $QtPluginDir -and $BinDir)
{
    foreach ($candidate in @((Join-Path $BinDir "plugins"), $BinDir, (Join-Path $BinDir "..\plugins")))
    {
        if (Test-Path (Join-Path $candidate "platforms\qoffscreen.dll"))
        {
            $QtPluginDir = $candidate
            break
        }
    }
}
if ($QtPluginDir)
{
    $QtPluginDir = (Resolve-Path $QtPluginDir).Path
}
else
{
    Write-Host "Warning: Qt plugin directory with platforms\qoffscreen.dll not found (use -QtPluginDir): the workbench tests will fail." -ForegroundColor Yellow
}

if (-not $LogDir)
{
    $LogDir = Join-Path (Get-Location) ("remote-test-logs-" + (Get-Date -Format "yyyyMMdd-HHmmss"))
}


# ============ list of test runs

$runs = New-Object System.Collections.ArrayList

function Add-Run($name, $exe, $arguments, $timeout, $requires)
{
    [void]$runs.Add([pscustomobject]@{
        Name      = $name
        Exe       = $exe
        Arguments = $arguments
        Timeout   = [int]($timeout * $TimeoutScale)
        Requires  = $requires
    })
}

foreach ($t in $unitTests)
{
    Add-Run $t.Name "testexe_$($t.Name).exe" @() $t.Timeout $null
}

$backends = @()
if ($WslDistro)           { $backends += @{ Name = "wsl-env";       Suffix = "wsl_env";       IsSsh = $false; Requires = $null } }
if ($SshHost)             { $backends += @{ Name = "ssh-env";       Suffix = "ssh_env";       IsSsh = $true;  Requires = $null } }
if ($IncludeSshLocalhost)
{
    # precondition (CTest fixture "remote_ssh")
    Add-Run "remote_check_localhost_remote" "testexe_check_localhost_remote.exe" @() 120 $null
    $backends += @{ Name = "ssh-localhost"; Suffix = "ssh_localhost"; IsSsh = $true; Requires = "remote_check_localhost_remote" }
}

foreach ($b in $backends)
{
    foreach ($t in $backendTests)
    {
        if ($t.SshOnly -and -not $b.IsSsh) { continue }
        Add-Run "$($t.Name)_$($b.Suffix)" "testexe_$($t.Name).exe" @("--backend", $b.Name) $t.Timeout $b.Requires
    }
}

$selected = @($runs | Where-Object {
    ($_.Name -match $Filter) -and (-not $Exclude -or $_.Name -notmatch $Exclude)
})

if ($backends.Count -eq 0)
{
    Write-Host "Note: no remote target configured (-WslDistro, -SshHost, -IncludeSshLocalhost): only unit tests are run." -ForegroundColor Yellow
}

if ($List)
{
    foreach ($r in $selected)
    {
        "{0,-50} {1} {2}  (timeout {3} s)" -f $r.Name, $r.Exe, ($r.Arguments -join " "), $r.Timeout
    }
    exit 0
}


# ============ environment

$savedEnv = @{}
function Set-TestEnv($name, $value)
{
    if (-not $savedEnv.ContainsKey($name))
    {
        $savedEnv[$name] = [Environment]::GetEnvironmentVariable($name, "Process")
    }
    [Environment]::SetEnvironmentVariable($name, $value, "Process")
}

$pathEntries = @($ExtraPath) + @($TestDir)
if ($BinDir) { $pathEntries += $BinDir }
Set-TestEnv "PATH" ((($pathEntries | Where-Object { $_ }) -join ";") + ";" + $env:PATH)
Set-TestEnv "INSIGHT_TEST_ANALYZE_EXE" $AnalyzeExe
Set-TestEnv "QT_QPA_PLATFORM" "offscreen"
if ($QtPluginDir) { Set-TestEnv "QT_PLUGIN_PATH" $QtPluginDir }
if ($SharedDir)  { Set-TestEnv "INSIGHT_GLOBALSHAREDDIRS" (Resolve-Path $SharedDir).Path }
if ($WslDistro)  { Set-TestEnv "INSIGHT_TEST_WSL_DISTRO"  $WslDistro }
if ($WslBaseDir) { Set-TestEnv "INSIGHT_TEST_WSL_BASEDIR" $WslBaseDir }
if ($SshHost)    { Set-TestEnv "INSIGHT_TEST_SSH_HOST"    $SshHost }
if ($SshBaseDir) { Set-TestEnv "INSIGHT_TEST_SSH_BASEDIR" $SshBaseDir }


# ============ execution

function Quote-Argument([string]$a)
{
    if ($a -match '[\s"]') { return '"' + ($a -replace '"', '\"') + '"' }
    return $a
}

function Invoke-Test($run)
{
    $exePath = Join-Path $TestDir $run.Exe
    $log = Join-Path $LogDir "$($run.Name).log"

    if (-not (Test-Path $exePath))
    {
        Set-Content -Path $log -Value "test executable not found: $exePath"
        return [pscustomobject]@{ Status = "Missing"; ExitCode = $null; Seconds = 0; Log = $log }
    }

    $outFile = "$log.stdout"
    $errFile = "$log.stderr"

    $psi = @{
        FilePath               = $exePath
        WorkingDirectory       = $TestDir
        RedirectStandardOutput = $outFile
        RedirectStandardError  = $errFile
        NoNewWindow            = $true
        PassThru               = $true
    }
    if ($run.Arguments.Count -gt 0)
    {
        $psi.ArgumentList = ($run.Arguments | ForEach-Object { Quote-Argument $_ }) -join " "
    }

    $sw = [Diagnostics.Stopwatch]::StartNew()
    $p = Start-Process @psi
    $null = $p.Handle # keep the handle: otherwise ExitCode is not available later

    $timedOut = -not $p.WaitForExit($run.Timeout * 1000)
    if ($timedOut)
    {
        # terminate the test and all its children
        & taskkill.exe /PID $p.Id /T /F 2>&1 | Out-Null
        $p.WaitForExit(10000) | Out-Null
    }
    else
    {
        $p.WaitForExit() # flush the redirected output
    }
    $sw.Stop()

    # combine the output into one log
    $content = @()
    $content += "==== $($run.Exe) $($run.Arguments -join ' ')"
    if (Test-Path $outFile) { $content += Get-Content $outFile; Remove-Item $outFile }
    if (Test-Path $errFile)
    {
        $err = @(Get-Content $errFile)
        Remove-Item $errFile
        if ($err.Count -gt 0) { $content += "==== stderr"; $content += $err }
    }
    if ($timedOut) { $content += "==== TIMEOUT after $($run.Timeout) s: process tree killed" }
    Set-Content -Path $log -Value $content

    $exitCode = $null
    if ($timedOut)                         { $status = "Timeout" }
    else
    {
        $exitCode = $p.ExitCode
        if ($exitCode -eq 0)                 { $status = "Passed" }
        elseif ($exitCode -eq $SKIP_RETURN_CODE) { $status = "Skipped" }
        else                                 { $status = "Failed" }
    }

    return [pscustomobject]@{ Status = $status; ExitCode = $exitCode; Seconds = $sw.Elapsed.TotalSeconds; Log = $log }
}


New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

Write-Host "Test executables: $TestDir"
Write-Host "analyze:          $AnalyzeExe"
Write-Host "Qt plugins:       $QtPluginDir"
Write-Host "Logs:             $LogDir"
Write-Host ""

$results = New-Object System.Collections.ArrayList
$statusOf = @{}
$i = 0

try
{
    foreach ($run in $selected)
    {
        $i++
        $label = "{0,3}/{1} {2}" -f $i, $selected.Count, $run.Name

        if ($run.Requires -and $statusOf.ContainsKey($run.Requires) -and $statusOf[$run.Requires] -ne "Passed")
        {
            $r = [pscustomobject]@{ Status = "NotRun"; ExitCode = $null; Seconds = 0; Log = $null }
            Write-Host ("{0,-62} Not Run (precondition {1} failed)" -f $label, $run.Requires) -ForegroundColor Yellow
        }
        else
        {
            Write-Host -NoNewline ("{0,-62} " -f $label)
            $r = Invoke-Test $run

            switch ($r.Status)
            {
                "Passed"  { $color = "Green" }
                "Skipped" { $color = "Cyan" }
                default   { $color = "Red" }
            }
            $text = $r.Status
            if ($r.Status -eq "Failed") { $text += " (exit code $($r.ExitCode))" }
            Write-Host ("{0,-28} {1,8:F1} s" -f $text, $r.Seconds) -ForegroundColor $color

            if ($r.Status -eq "Skipped" -and (Test-Path $r.Log))
            {
                # reason of the skip
                Get-Content $r.Log | Select-String -Pattern "skipping|not available|not configured|requires" |
                    Select-Object -First 1 | ForEach-Object { Write-Host "      $($_.Line)" -ForegroundColor Cyan }
            }

            if ($OutputOnFailure -and $r.Status -in @("Failed", "Timeout", "Missing") -and (Test-Path $r.Log))
            {
                Write-Host "---- last lines of $($r.Log):" -ForegroundColor DarkGray
                Get-Content $r.Log -Tail 60 | ForEach-Object { Write-Host "    $_" }
                Write-Host "----" -ForegroundColor DarkGray
            }
        }

        $statusOf[$run.Name] = $r.Status
        [void]$results.Add([pscustomobject]@{ Name = $run.Name; Status = $r.Status; Seconds = $r.Seconds; Log = $r.Log })
    }
}
finally
{
    foreach ($k in $savedEnv.Keys)
    {
        [Environment]::SetEnvironmentVariable($k, $savedEnv[$k], "Process")
    }
}


# ============ summary

$failed  = @($results | Where-Object { $_.Status -in @("Failed", "Timeout", "Missing") })
$passed  = @($results | Where-Object { $_.Status -eq "Passed" })
$skipped = @($results | Where-Object { $_.Status -eq "Skipped" })
$notRun  = @($results | Where-Object { $_.Status -eq "NotRun" })

Write-Host ""
Write-Host ("{0} passed, {1} failed, {2} skipped, {3} not run (of {4})" -f `
    $passed.Count, $failed.Count, $skipped.Count, $notRun.Count, $results.Count)

if ($failed.Count -gt 0)
{
    Write-Host "The following tests FAILED:" -ForegroundColor Red
    foreach ($f in $failed) { Write-Host ("  {0,-55} {1,-8} {2}" -f $f.Name, $f.Status, $f.Log) -ForegroundColor Red }
}

$results | Export-Csv -NoTypeInformation -Path (Join-Path $LogDir "summary.csv")

if ($failed.Count -gt 0 -or $notRun.Count -gt 0) { exit 1 }
exit 0
