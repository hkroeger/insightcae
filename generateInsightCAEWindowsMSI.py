#!/usr/bin/python3
# This Python file uses the following encoding: utf-8

import os,sys,subprocess,re,copy
import xml.etree.cElementTree as ET
import importlib.util
superbuildSourcePath = os.path.dirname(os.path.realpath(__file__))
spec = importlib.util.spec_from_file_location("module.name", os.path.join(superbuildSourcePath, "wix.py"))
wix = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wix)

from optparse import OptionParser

parser = OptionParser()

parser.add_option("-b", "--branch", dest="branch", metavar='branch', default="master",
                  help="label of the InsightCAE branch")

parser.add_option("-c", "--customer", dest="customer", metavar='customer', default="ce",
                  help="label of customer")

parser.add_option("-x", "--mxepath", dest="mxepath", metavar='mxepath', default="/mxe",
                help="path to mxe cross compile environment")

parser.add_option("-i", "--productId", dest="productId", metavar='productId', default="",
                  help="product id")

parser.add_option("-v", "--version", dest="version", metavar='version', default="0.0.0",
                  help="set package version")

parser.add_option("-s", "--insightSourcePath", dest="insightSourcePath", metavar='InsightCAE source path', default=superbuildSourcePath,
                  help="path to insightcae source code (not superbuild source)")

(opts, args) = parser.parse_args()



vvv=opts.version.split('.')
if (len(vvv)!=3):
    raise RuntimeError("could not understand version: "+opts.version)
versionMajor=int(vvv[0])
versionMinor=int(vvv[1])
versionPatch=int(vvv[2])
print ("version:", versionMajor, versionMinor, versionPatch)
fullVersion=(versionMajor,versionMinor,versionPatch)

packageName="InsightCAE"
if not (opts.customer=="ce" or opts.customer=="ce-dev"):
    packageName+=" ("+opts.customer+")"


directories=["bin", "share/insight"]



packageFileName=copy.copy(packageName)
packageFileName.replace(" ", "_")



if not (os.path.exists("./bin") and os.path.exists("./share/insight")):
    print("Please execute in build directory!")
    sys.exit(-1)



wxs=wix.WixInput(
    packageName,
    opts.productId, '87be4b31-c7c0-493c-ac97-3736a3417867',
    fullVersion,
    menuentry=("InsightCAE", '5f61d82b-1c22-4e54-b9b6-b7cad11e7aee') )

fl=wix.FileList(opts.mxepath)

for d in directories:
    fl.addFiles(d)

fl.addFile( os.path.join(opts.mxepath, "usr/i686-w64-mingw32.shared/qt5/plugins/platforms/qwindows.dll"), "bin/plugins/platforms" )

# adding doesn't work, too big(?)
#for f in list(filter(re.compile("^insightcae.*\\.tar\\.gz").search, os.listdir("/insight-src/wsl"))):
#    fl.addFile( "/insight-src/wsl/"+f, "share/insight/wsl" )
    
sp=os.path.join(opts.mxepath, "usr/i686-w64-mingw32.shared/share/oce-0.18/src/Shaders")
fl.addFiles( sp, ".*", "share/oce-0.18/Shaders", sp)
wxs.addEnvVar('CSF_ShadersDirectory','[INSTALLDIR]share\\oce-0.18\\Shaders','3b4ab883-6637-4969-a006-d47ab6e8072b')
wxs.addEnvVar('INSIGHT_GLOBALSHAREDDIRS','[INSTALLDIR]share\\insight','2dba120f-def7-441d-93a8-abef0b5448e0')

while (fl.addDependencies()>0):
    pass

# generate remoteServers.xml
wsldistlabel=wix.wslDistributionLabel(opts.customer)
#nfn="remoteservers.list"
#open(nfn, 'w').write("""<?xml version="1.0" encoding="utf-8"?>
#<root>
#        <remoteServer label="WSL" type="WSLLinux" distributionLabel="{wsldistlabel}" baseDirectory="/home/user"/>
#</root>
#""".format(wsldistlabel=wsldistlabel))
#fl.replaceFile("remoteservers.list", nfn)
#fl.addFile(nfn, "share/insight")

print(fl.files)

wxs.addFiles(fl.files)

wxs.addShortcut('workbenchshortcut', "Workbench", "Simulation apps are run through this front end", '[INSTALLDIR]bin\\workbench.exe', os.path.join(opts.insightSourcePath, "symbole", 'logo_insight_cae.ico'))
wxs.addShortcut('iscadshortcut', "isCAD", "A script-based CAD editor", '[INSTALLDIR]bin\\iscad.exe', os.path.join(superbuildSourcePath, "symbole", 'logo_insight_cae.ico'), False)
wxs.addShortcut('isresulttoolshortcut', "Result Viewer", "A viewer for InsightCAE's ISR result files", '[INSTALLDIR]bin\\isResultTool.exe', os.path.join(opts.insightSourcePath, "symbole", 'logo_insight_cae.ico'), False)
#wxs.addShortcut(
#    'shellshortcut',
#    "WSL Shell", "OpenFOAM Command line",
#    "[WindowsFolder]System32\\wsl.exe", arguments='-d {wslname}'.format( wslname=wsldistlabel ),
#    icon=os.path.join(opts.insightSourcePath, "symbole", 'logo_insight_cae.ico'), addicon=False )


wxs.write("insightcae.wxs")
subprocess.run([os.path.join(superbuildSourcePath,"wixbuild.sh"), "insightcae"])
