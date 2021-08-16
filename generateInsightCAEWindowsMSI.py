#!/usr/bin/env python3
# This Python file uses the following encoding: utf-8

import os,sys,subprocess,re,copy
import xml.etree.cElementTree as ET
import importlib.util
sourcePath = os.path.dirname(os.path.realpath(__file__))
spec = importlib.util.spec_from_file_location("module.name", os.path.join(sourcePath, "wix.py"))
wix = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wix)

from optparse import OptionParser

parser = OptionParser()

parser.add_option("-c", "--customer", dest="customer", metavar='customer', default="ce",
                  help="label of customer")

parser.add_option("-x", "--mxepath", dest="mxepath", metavar='mxepath', default="/home/hannes/Programme/mxe",
                help="path to mxe cross compile environment")

(opts, args) = parser.parse_args()


packageName="InsightCAE"

if not (opts.customer=="ce" or opts.customer=="ce-dev"):
    packageName+=" ("+opts.customer+")"

versionMajor=3
versionMinor=0
versionPatch=0
fullVersion=(versionMajor,versionMinor,versionPatch)


packageFileName=copy.copy(packageName)
packageFileName.replace(" ", "_")


directories=["bin", "share/insight"]

if not (os.path.exists("./bin") and os.path.exists("./share/insight")):
    print("Please execute in build directory!")
    sys.exit(-1)



def getDependencies(file):
    deps=set()
    ext=os.path.splitext(file)[-1]
    if ext==".exe" or ext==".dll":
        ret=subprocess.run([os.path.join(opts.mxepath,"usr/x86_64-pc-linux-gnu/bin/peldd"), file], stdout=subprocess.PIPE)
        deps=set([l.decode('UTF-8') for l in ret.stdout.split()])
        print(file, " : ", deps)
    return deps

def findFile(f):
    ret=subprocess.run(["find", os.path.join(opts.mxepath, "usr/i686-w64-mingw32.shared"), "-iname", f], stdout=subprocess.PIPE)
    #ret=subprocess.run(["locate", "-i", "-r", "^"+os.path.join(opts.mxepath, "usr/i686-w64-mingw32.shared")+".*"+f+"$"], stdout=subprocess.PIPE)
    found=[l.decode('UTF-8') for l in ret.stdout.split()]
    print("found:", found)
    if len(found)>1:
        raise RuntimeError("multiple candidaProgramMenuFoldertes for "+f+" found! "+str(found))
    if len(found)==0:
        print("No condidate for",f, "found.")
        return None

    return found[-1]


class FileList:

    files={}

    def addFile(self, sourceFile, targetDir):
        bn=os.path.basename(sourceFile)
        print("add file: ", sourceFile, targetDir, "as", bn)
        self.files[bn] = ( wix.sanitizeId(bn), sourceFile, targetDir )

    def addFiles(self, subdir, pattern=".*", targetprefix="", removeprefix=""):
        files=filter(
            re.compile(pattern).match,
            os.listdir(subdir) )

        for bn in files:

            filepath=os.path.join(subdir,bn)

            if (os.path.isdir(filepath)):
                self.addFiles(filepath, pattern, targetprefix, removeprefix)
            else:
                if targetprefix=="":
                    relp=subdir
                else:
                    relp=os.path.relpath(subdir,removeprefix)
                    if relp!=".":
                        relp=os.path.join(targetprefix,relp)
                    else:
                        relp=targetprefix
                self.addFile(filepath, relp )

    def addDependencies(self):
        added=0
        oldfl=self.files.copy()
        for bn,(compn,sourcef,targdir) in oldfl.items():
            deps=getDependencies(sourcef)
            for df in deps:
                if not df in self.files:
                    fdf=findFile(df)
                    if not fdf is None:
                        added+=1
                        self.files[df]=( wix.sanitizeId(df), fdf, "bin" )

        print("added", added, "dependencies")
        return added
    
    def replaceFile(self, filename, newSourcePath):
        if not filename in self.files:
            raise RuntimeError("file "+filename+" is not in the list of files to include in the package!")
        f=self.files[filename]
        self.files[filename]=(f[0], newSourcePath, f[2])



wxs=wix.WixInput(
    packageName,
    'd6e296e3-61c2-44b4-8736-be4568091ff4', '87be4b31-c7c0-493c-ac97-3736a3417867',
    fullVersion,
    menuentry=("InsightCAE", '5f61d82b-1c22-4e54-b9b6-b7cad11e7aee') )

fl=FileList()

for d in directories:
    fl.addFiles(d)

fl.addFile( os.path.join(opts.mxepath, "usr/i686-w64-mingw32.shared/qt5/plugins/platforms/qwindows.dll"), "bin/plugins/platforms" )

sp=os.path.join(opts.mxepath, "usr/i686-w64-mingw32.shared/share/oce-0.18/src/Shaders")
fl.addFiles( sp, ".*", "share/oce-0.18/Shaders", sp)
wxs.addEnvVar('CSF_ShadersDirectory','[INSTALLDIR]share\\oce-0.18\\Shaders','3b4ab883-6637-4969-a006-d47ab6e8072b')
wxs.addEnvVar('INSIGHT_GLOBALSHAREDDIRS','[INSTALLDIR]share\\insight','2dba120f-def7-441d-93a8-abef0b5448e0')

while (fl.addDependencies()>0):
    pass

# generate remoteServers.xml
wsldistlabel=wix.wslDistributionLabel(opts.customer)
nfn="remoteservers.list.package"
open(nfn, 'w').write("""<?xml version="1.0" encoding="utf-8"?>
<root>
        <remoteServer label="WSL" type="WSLLinux" distributionLabel="{wsldistlabel}" baseDirectory="/root"/>
</root>
""".format(wsldistlabel=wsldistlabel))
fl.replaceFile("remoteservers.list", nfn)

print(fl.files)

wxs.addFiles(fl.files)

wxs.addShortcut('workbenchshortcut', "Workbench", "Simulation apps are run through this front end", '[INSTALLDIR]bin\\workbench.exe', os.path.join(sourcePath, "symbole", 'logo_insight_cae.ico'))
wxs.addShortcut('iscadshortcut', "isCAD", "A script-based CAD editor", '[INSTALLDIR]bin\\iscad.exe', os.path.join(sourcePath, "symbole", 'logo_insight_cae.ico'), False)
wxs.addShortcut('isresulttoolshortcut', "Result Viewer", "A viewer for InsightCAE's ISR result files", '[INSTALLDIR]bin\\isResultTool.exe', os.path.join(sourcePath, "symbole", 'logo_insight_cae.ico'), False)



wxs.write("insightcae.wxs")
subprocess.run([os.path.join(sourcePath,"wixbuild.sh"), "insightcae"])
