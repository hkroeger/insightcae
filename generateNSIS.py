#!/usr/bin/python3
# This Python file uses the following encoding: utf-8

import os,sys,subprocess,re
import urllib.request as req

from optparse import OptionParser

superbuildSourcePath = os.path.dirname(os.path.realpath(__file__))

parser = OptionParser()

parser.add_option("-b", "--branch", dest="branch", metavar='branch', default="master",
                  help="label of the InsightCAE branch")

parser.add_option("-c", "--customer", dest="customer", metavar='customer', default="ce",
                  help="label of customer")

parser.add_option("-p", "--password", dest="password", metavar='password', default="",
                  help="password for customer repository")

parser.add_option("-s", "--insightSourcePath", dest="insightSourcePath", metavar='InsightCAE source path', default=superbuildSourcePath,
                  help="path to insightcae source code (not superbuild source)")

parser.add_option("-x", "--mxepath", dest="mxepath", metavar='mxepath', default="/mxe",
                help="path to mxe cross compile environment")

(opts, args) = parser.parse_args()


out=subprocess.check_output(
        [ "git", "describe" ],
        cwd=superbuildSourcePath
        ).decode().split('\n')
print(out)
m=re.search("^([0-9]+)\\.([0-9]+)[-]*([^ ]+|)$", out[0])
if m is None:
    raise RuntimeError("could not interpret output of git describe: "+out[0])

versionMajor=int(m.group(1))
versionMinor=int(m.group(2))
versionPatch=0
pp=m.group(3).split('-')
if len(pp)==2:
    versionPatch=int(pp[0])
print ("version:", versionMajor, versionMinor, versionPatch)
fullVersion=(versionMajor,versionMinor,versionPatch)


GUID_winInsightCAE='d6e296e3-61c2-44b4-8736-be4568091ff4'
GUID_wsl='b5a4aa4c-aa68-4244-8ed3-e8c5390fcb75'

installerfname="InsightCAEInstaller"
if not (opts.customer=="ce" or opts.customer=="ce-dev"):
    installerfname+="-"+opts.customer
installerfname+="-%d.%d.%d"%fullVersion

print("Generating installation package "+installerfname)

class Dependency:
    def __init__(self, URL=None, file=None, productId="", updateType="reinstall"):
        self.productId=productId
        self.updateType=updateType
        if file is None:
            if URL is None:
                raise RuntimeError("URL and file must not be None at the same time.")
            else:
                self.file=URL.split('/')[-1]
        else:
            self.file=file
        if not os.path.exists(self.file) and not URL is None:
            req.urlretrieve(URL, self.file)

    def config(self, label):
        name,ext=os.path.splitext(os.path.basename(self.file))
        return """
Section "{label}"
    File "/oname=$TEMP\{file}" "{file}"
    {command}
    Delete "$TEMP\{file}"
SectionEnd
""".format(
    label=label,
    file=self.file,
    command=
     """\
push $R0
StrCpy $R0 "{productId}"
push $R1
StrCpy $R1 "{file}"
push $R2
StrCpy $R2 "{updateType}"
Call InstallMSI
""".format(productId=self.productId, file=self.file, updateType=self.updateType) \
     if ext.lower()=='.msi' else \
     'ExecShellWait "" "$TEMP\{file}"'.format(file=self.file)
)


putty=Dependency("http://downloads.silentdynamics.de/thirdparty/putty-64bit-0.76-installer.msi")
gnuplot=Dependency("http://downloads.silentdynamics.de/thirdparty/gp528-win64-mingw.exe")
miktex=Dependency("http://downloads.silentdynamics.de/thirdparty/basic-miktex-21.6-x64.exe")
paraview=Dependency("http://downloads.silentdynamics.de/thirdparty/ParaView-5.8.1-Windows-Python3.7-msvc2015-64bit.exe")

if subprocess.call([
 os.path.join(superbuildSourcePath, "generateInsightCAEWindowsMSI.py"),
    "-c", opts.customer, 
    "-b", opts.branch,
    "-s", opts.insightSourcePath,
    "-x", opts.mxepath,
    "-i", GUID_winInsightCAE,
    "-v", "%d.%d.%d"%fullVersion
])!=0:
    print("Failed to run generateInsightCAEWindowsMSI.py!")
    sys.exit(-1)

insight=Dependency(file="insightcae.msi", productId=GUID_winInsightCAE, updateType="uninstallfirst")

#if subprocess.call([
 #os.path.join(superbuildSourcePath, "generateWSLMSI.py"),
    #"-c", opts.customer,
    #"-p", opts.password,
    #"-s", opts.insightSourcePath,
    #"-i", GUID_wsl
#])!=0:
    #print("Failed to run generateInsightCAEWindowsMSI.py!")
    #sys.exit(-1)

#insightwsl=Dependency(file="insightcae-wsl.msi", productId=GUID_wsl)


nsisScript=("""
!include FileFunc.nsh
!include LogicLib.nsh
!include nsDialogs.nsh
!include {srcPath}/psexec.nsh
!include x64.nsh
!include StrFunc.nsh

Name "InsightCAE on Windows"
OutFile "{outFile}.exe"
RequestExecutionLevel admin #user
LicenseData "{srcPath}/gpl.txt"
Icon "{srcPath}/insightpackage.ico"


Function .onInit
    ${{IfNot}} ${{RunningX64}}
        MessageBox MB_OK "This is a 32bit system! Only 64bit systems are supported. The installer will stop now."
        abort
    ${{EndIf}} 
    
    # create temp directory PLUGINSDIR
    InitPluginsDir
    
    ${{PowerShellExec}} "(get-windowsoptionalfeature -online|where FeatureName -like Microsoft-Windows-Subsystem-Linux).State -eq 'Enabled'"
    Pop $R1
    StrCpy $R1 "$R1" -2 # remove newline
    #MessageBox MB_OK "Powershell return: >$R1<"
    
    StrCmp $R1 "True" isInstalled notInstalled
    
notInstalled:
    # active wsl using auxiliary installer
    File "/oname=$PLUGINSDIR\wsl-activation-installer.exe" "{srcPath}/wsl-activation-installer.exe"
    ExecShellWait "" "$PLUGINSDIR\wsl-activation-installer.exe" "/insightInstallerPath=$EXEPATH"
    Abort
isInstalled:

FunctionEnd

Function InstallMSI
  ; $R0 should contain the GUID of the application, $R1 install file, $R2 updateType
  push $R3
  StrCmp $R0 "" freshinstall
  ReadRegStr $R3 HKLM "Software\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{{$R0}}" "UninstallString"
  StrCmp $R3 "" freshinstall upgradeinstall
upgradeinstall:
  StrCmp $R2 "reinstall" reinstall uninstallfirst
reinstall:
  ExecWait 'msiexec /i \"$TEMP\$R1\" REINSTALL=ALL REINSTALLMODE=vomus'
  goto done
uninstallfirst:
  ExecWait 'msiexec /uninstall "{{$R0}}" /qb'
freshinstall: 
  ExecWait 'msiexec /i \"$TEMP\$R1\"'
done:
  pop $R0
  pop $R1
  pop $R2
  pop $R3
FunctionEnd


Page license
Page components
Page instfiles

""".format(
    outFile=installerfname,
    srcPath=superbuildSourcePath
)) \
+putty.config("Putty") \
+gnuplot.config("Gnuplot") \
+miktex.config("MiKTeX") \
+paraview.config("ParaView 5.8") \
+insight.config("InsightCAE Windows Client and isCAD") \
#+insightwsl.config("WSL Distibution (Ubuntu) with InsightCAE backend and OpenFOAM")

nsisScriptfname=installerfname+".nsis"
open(nsisScriptfname, 'w').write(nsisScript)

subprocess.call(["i686-w64-mingw32.static-makensis", nsisScriptfname])
