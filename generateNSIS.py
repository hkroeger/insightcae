#!/usr/bin/python3
# This Python file uses the following encoding: utf-8

import os,sys,subprocess,re
import urllib.request as req

from optparse import OptionParser

superbuildSourcePath = os.path.dirname(os.path.realpath(__file__))

parser = OptionParser()

parser.add_option("-c", "--customer", dest="customer", metavar='customer', default="ce",
                  help="label of customer")

parser.add_option("-s", "--insightSourcePath", dest="insightSourcePath", metavar='InsightCAE source path', default=superbuildSourcePath,
                  help="path to insightcae source code (not superbuild source)")

parser.add_option("-b", "--insightBuildPath", dest="insightBuildPath", metavar='InsightCAE build path', default="build", 
                  help="path to insightcae build directory")


parser.add_option("-x", "--mxepath", dest="mxepath", metavar='mxepath', default="/opt/mxe",
                help="path to mxe cross compile environment")

parser.add_option("-o", "--skipotherdeps", dest="skipOtherDeps", action="store_true", default=False,
                help="skip non-insightcae parts")

(opts, args) = parser.parse_args()


out=subprocess.check_output(
        [ "git", "describe" ],
        cwd=superbuildSourcePath
        ).decode().split('\n')

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

wslimages=list(filter(re.compile("^insightcae-wsl-.*-%d.%d.%d\\.tar\\..*"%fullVersion).match, os.listdir(".")))
if len(wslimages)>1:
    raise "unexpected: more than one wsl image present. Please clean build directory."
if len(wslimages)<1:
    raise "unexpected: no wsl image present. Please check build directory."
wslimage=wslimages[0]

class Dependency:
    def __init__(self, URL=None, file=None, note=None):
        if file is None:
            if URL is None:
                raise RuntimeError("URL and file must not be None at the same time.")
            else:
                self.localfile=URL.split('/')[-1]
                self.filename=os.path.basename(self.localfile)
        else:
            self.localfile=file
            self.filename=os.path.basename(self.localfile)
        if not os.path.exists(self.localfile) and not URL is None:
            req.urlretrieve(URL, self.localfile)
            
        self.command='ExecShellWait "" "$TEMP\{file}"'.format(
            file=self.filename)
        
        self.note=note

    def config(self, label):
        return """
Section "{label}"
    File "/oname=$TEMP\{file}" "{localfile}"
"""
+(self.note is None?"":"MessageBox MB_OK \""+self.note"\"")
        +"""
    {command}
    Delete "$TEMP\{file}"
SectionEnd
""".format(
        label=label,
        file=self.filename,
        localfile=self.localfile,
        command=self.command )



class MSIDependency(Dependency):
    def __init__(self, URL=None, file=None, productId="", updateType="reinstall"):
        super(MSIDependency, self).__init__(URL=URL, file=file)
        self.command="""
push $R0
StrCpy $R0 "{productId}"
push $R1
StrCpy $R1 "{file}"
push $R2
StrCpy $R2 "{updateType}"
Call InstallMSI
""".format(productId=productId, file=self.filename, updateType=updateType)



class WSLImageDependency(Dependency):
    def __init__(self, URL=None, file=None):
        super(WSLImageDependency, self).__init__(URL=URL, file=file)
        m=re.search("^(.*)-([0-9]*\\.[0-9]*\\.[0-9]*).tar.(.*)$", self.filename)
        imgname=m.group(1)
        print(imgname)
        open("remoteservers.list", 'w').write("""
<?xml version="1.0" encoding="utf-8"?>
<root>
 <remoteServer label="{label}" type="WSLLinux" distributionLabel="{label}" baseDirectory="/home/user"/>
</root>
""".format(label=imgname))
        self.command="""
ClearErrors
ExecWait 'wsl --import "{imgname}" "$PROFILE\\\\{imgname}" "$TEMP\\\\{file}"'
IfErrors 0 noError
  MessageBox MB_OK "The installation of the WSL backend image failed!"
noError:
CreateDirectory "$PROFILE\\.insight\\share"
File "/oname=$PROFILE\\.insight\\share\\remoteservers.list" "remoteservers.list"
""".format(file=self.filename, imgname=imgname)


putty=MSIDependency("http://downloads.silentdynamics.de/thirdparty/putty-64bit-0.76-installer.msi")
gnuplot=Dependency("http://downloads.silentdynamics.de/thirdparty/gp528-win64-mingw.exe")
miktex=Dependency("http://downloads.silentdynamics.de/thirdparty/basic-miktex-21.6-x64.exe")
python=Dependency("http://downloads.silentdynamics.de/thirdparty/python-3.6.8rc1.exe", 
                  note="Please check the option 'Add python.exe to PATH' in the Python installer!$\\r$\\nIf this is omitted, the InsightCAE executables will not run.")
paraview=Dependency("http://downloads.silentdynamics.de/thirdparty/ParaView-5.8.1-Windows-Python3.7-msvc2015-64bit.exe")
insightwsl=WSLImageDependency(file=wslimage)

if subprocess.call([
 os.path.join(superbuildSourcePath, "generateInsightCAEWindowsMSI.py"),
    "-c", opts.customer, 
    "-s", opts.insightSourcePath,
    "-x", opts.mxepath,
    "-i", GUID_winInsightCAE,
    "-v", "%d.%d.%d"%fullVersion
], cwd=opts.insightBuildPath)!=0:
    print("Failed to run generateInsightCAEWindowsMSI.py!")
    sys.exit(-1)

insight=MSIDependency(file=os.path.join(opts.insightBuildPath, "insightcae.msi"),
                      productId=GUID_winInsightCAE, updateType="uninstallfirst")



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
+((putty.config("Putty") \
+gnuplot.config("Gnuplot") \
+miktex.config("MiKTeX") \
+python.config("Python 3.6") \
+paraview.config("ParaView 5.8")) if not opts.skipOtherDeps else "\n") \
+insightwsl.config("WSL Distibution (Ubuntu) with InsightCAE backend and OpenFOAM") \
+insight.config("InsightCAE Windows Client and isCAD")

nsisScriptfname=installerfname+".nsis"
open(nsisScriptfname, 'w').write(nsisScript)

subprocess.call(["i686-w64-mingw32.static-makensis", nsisScriptfname])
