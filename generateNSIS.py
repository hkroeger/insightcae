#!/usr/bin/env python3
# This Python file uses the following encoding: utf-8

import os,sys,subprocess,re
import urllib.request as req

from optparse import OptionParser

sourcePath = os.path.dirname(os.path.realpath(__file__))

parser = OptionParser()

parser.add_option("-c", "--customer", dest="customer", metavar='customer', default="ce",
                  help="label of customer")

parser.add_option("-p", "--password", dest="password", metavar='password', default="",
                  help="password for customer repository")

(opts, args) = parser.parse_args()


installerfname="InsightCAEInstaller"
if not (opts.customer=="ce" or opts.customer=="ce-dev"):
    installerfname+="-"+opts.customer

print("Generating installation package "+installerfname)

class Dependency:
    def __init__(self, URL=None, file=None):
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
     "ExecWait 'msiexec /i \"$TEMP\{file}\"'".format(file=self.file) \
     if ext=='msi' else \
     'ExecShellWait "" "$TEMP\{file}"'.format(file=self.file)
)


putty=Dependency("https://the.earth.li/~sgtatham/putty/latest/w64/putty-64bit-0.76-installer.msi")
gnuplot=Dependency("https://sourceforge.net/projects/gnuplot/files/gnuplot/5.2.8/gp528-win64-mingw.exe")
miktex=Dependency("https://miktex.org/download/ctan/systems/win32/miktex/setup/windows-x64/basic-miktex-21.6-x64.exe")
paraview=Dependency("https://www.paraview.org/paraview-downloads/download.php?submit=Download&version=v5.8&type=binary&os=Windows&downloadFile=ParaView-5.8.1-Windows-Python3.7-msvc2015-64bit.exe", "ParaView-5.8.1-Windows-Python3.7-msvc2015-64bit.exe")

subprocess.call([
 os.path.join(sourcePath, "generateInsightCAEWindowsMSI.py"),
    "-c", opts.customer
])
insight=Dependency(file="insightcae.msi")

subprocess.call([
 os.path.join(sourcePath, "generateWSLMSI.py"),
    "-c", opts.customer,
    "-p", opts.password
])
insightwsl=Dependency(file="insightcae-wsl.msi")


nsisScript=("""
!include FileFunc.nsh
!include LogicLib.nsh
!include nsDialogs.nsh
!include x64.nsh


Name "InsightCAE on Windows"
OutFile "{outFile}.exe"
RequestExecutionLevel user
LicenseData "{srcPath}/gpl.txt"
Icon "{srcPath}/insightpackage.ico"


Function .onInit
    ${{IfNot}} ${{RunningX64}}
        MessageBox MB_OK "This is a 32bit system! Only 64bit systems are supported. The installer will stop now."
        abort
    ${{EndIf}} 
    
    # create temp directory PLUGINSDIR
    InitPluginsDir
    
    # active wsl using auxiliary installer, if required
    IfFileExists "$WINDIR\Sysnative\wsl.exe" exists notInstalled
    exists:
        ClearErrors
        ExecWait '"$WINDIR\Sysnative\wsl.exe" --list'
        IfErrors 0 noError
    notInstalled:
        File "/oname=$PLUGINSDIR\wsl-activation-installer.exe" "{srcPath}/wsl-activation-installer.exe"
        ExecShellWait "" "$PLUGINSDIR\wsl-activation-installer.exe" "/insightInstallerPath=$EXEPATH"
        Abort
    noError:
FunctionEnd


Page license
Page components
Page instfiles

""".format(
    outFile=installerfname,
    srcPath=sourcePath
)) \
+putty.config("Putty") \
+gnuplot.config("Gnuplot") \
+miktex.config("MiKTeX") \
+paraview.config("ParaView 5.8") \
+insight.config("InsightCAE Windows Client and isCAD") \
+insightwsl.config("WSL Distibution (Ubuntu) with InsightCAE backend and OpenFOAM")

nsisScriptfname=installerfname+".nsis"
open(nsisScriptfname, 'w').write(nsisScript)

subprocess.call(["i686-w64-mingw32.static-makensis", nsisScriptfname])
