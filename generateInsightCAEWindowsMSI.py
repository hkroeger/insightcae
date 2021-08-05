#!/usr/bin/env python3
# This Python file uses the following encoding: utf-8

import os,sys,subprocess,re
import xml.etree.cElementTree as ET

packageName="InsightCAE"
versionMajor=3
versionMinor=0
versionPatch=0
fullVersion=(versionMajor,versionMinor,versionPatch)

def sanitizeId(orgname):
    clean="id"+"".join(filter(str.isalnum, orgname))
    print (orgname, " ==>> ", clean)
    return clean

def toWinePath(linuxPath):
    return "z:"+os.path.abspath(linuxPath).replace("/", "\\")


class WixInput:
    wix = ET.Element("Wix", attrib={'xmlns': 'http://schemas.microsoft.com/wix/2006/wi'})

    Product = ET.SubElement(wix, "Product", attrib={
    'Name':packageName,
    'Manufacturer': 'silentdynamics GmbH',
    'Id': 'd6e296e3-61c2-44b4-8736-be4568091ff4',
    'UpgradeCode': '87be4b31-c7c0-493c-ac97-3736a3417867',
    'Language': '1033',
    'Codepage': '1252',
    'Version': '%d.%d.%d'%fullVersion
    })

    Package=ET.SubElement(Product, "Package", attrib={
    'Id': '*',
    'Keywords': 'Installer',
    'Description': "silentdynamics InsightCAE for Windows Installer",
    'Comments': 'comment',
    'Manufacturer': 'silentdynamics GmbH',
    'InstallerVersion': '100',
    'Languages': '1033',
    'Compressed': 'yes',
    'SummaryCodepage': '1252'
    })

    Media=ET.SubElement(Product, "Media", attrib={
    'Id': '1',
    'Cabinet': packageName+'.cab',
    'EmbedCab': 'yes',
    'DiskPrompt': 'Disk #1'
    })

    Property=ET.SubElement(Product, "Property", attrib={
    'Id': 'DiskPrompt',
    'Value': "silentdynamics InsightCAE %d.%d.%d Installation [1]"%fullVersion
    })

    INSTALLDIR=ET.SubElement(
        ET.SubElement( ET.SubElement( ET.SubElement(Product,
                "Directory", attrib={'Id':'TARGETDIR', 'Name':'SourceDir'}),
            "Directory", attrib={'Id':'ProgramFilesFolder', 'Name':'PFiles'}),
          "Directory", attrib={'Id':'silentdynamics', 'Name':'silentdynamics'}),
         "Directory", attrib={'Id':'INSTALLDIR', 'Name':packageName+'-%d.%d.%d'%fullVersion})


    Feature=ET.SubElement(Product, "Feature", attrib={
    'Id': 'Complete',
    'Level': '1'
    })

    subdirs={}

    def getDirNode(self, relpath):
        if relpath=="" or relpath==".":
            return self.INSTALLDIR
        elif not relpath in self.subdirs:
            #create
            pd=self.getDirNode(os.path.dirname(relpath))
            bn=os.path.basename(relpath)
            node=ET.SubElement(pd, "Directory", attrib={'Id':sanitizeId(bn), 'Name':bn})
            self.subdirs[relpath]=node
        return self.subdirs[relpath]


    def addFiles(self, fileList):
        for bn,(compn,sourcef,targdir) in fileList.items():
            node=self.getDirNode(targdir)
            Component=ET.SubElement(node, "Component", attrib={ 'Id': compn, 'Guid': '*' })
            File=ET.SubElement(Component, "File", attrib={ 'Id': compn, 'Name': bn, 'DiskId': '1', 'Source':toWinePath(sourcef), 'KeyPath': 'yes' })

            ET.SubElement(self.Feature, "ComponentRef", attrib={'Id': compn})

    def addEnvVar(self,name,value,guid):
        varcomp=ET.SubElement(wxs.INSTALLDIR, "Component", attrib={'Id':name, 'Guid':guid})
        var=ET.SubElement(varcomp, "Environment", attrib={'Id':name, 'Name':name,
                'Action': 'set', 'Permanent': 'yes', 'System':'yes', 'Part': 'all', 'Value':value})
        ET.SubElement(self.Feature, "ComponentRef", attrib={'Id':name})

    def write(self, fn):
        tree = ET.ElementTree(self.wix)
        tree.write(fn)



mxepath="/home/hannes/Programme/mxe"

directories=["bin", "share/insight"]

if not (os.path.exists("./bin") and os.path.exists("./share/insight")):
    print("Please execute in build directory!")
    sys.exit(-1)






def getDependencies(file):
    deps=set()
    ext=os.path.splitext(file)[-1]
    if ext==".exe" or ext==".dll":
        ret=subprocess.run([os.path.join(mxepath,"usr/x86_64-pc-linux-gnu/bin/peldd"), file], stdout=subprocess.PIPE)
        deps=set([l.decode('UTF-8') for l in ret.stdout.split()])
        print(file, " : ", deps)
    return deps

def findFile(f):
    ret=subprocess.run(["find", os.path.join(mxepath, "usr/i686-w64-mingw32.shared"), "-iname", f], stdout=subprocess.PIPE)
    found=[l.decode('UTF-8') for l in ret.stdout.split()]
    print("found:", found)
    if len(found)>1:
        raise RuntimeError("multiple candidates for "+f+" found! "+str(found))
    if len(found)==0:
        print("No condidate for",f, "found.")
        return None

    return found[-1]





class FileList:

    files={}

    def addFile(self, sourceFile, targetDir):
        bn=os.path.basename(sourceFile)
        print("add file: ", sourceFile, targetDir)
        self.files[bn] = ( sanitizeId(bn), sourceFile, targetDir )

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
                        self.files[df]=( sanitizeId(df), fdf, "bin" )

        print("added", added, "dependencies")
        return added



wxs=WixInput()
fl=FileList()

for d in directories:
    fl.addFiles(d)

fl.addFile( os.path.join(mxepath, "usr/i686-w64-mingw32.shared/qt5/plugins/platforms/qwindows.dll"), "bin/plugins/platforms" )

sp=os.path.join(mxepath, "usr/i686-w64-mingw32.shared/share/oce-0.18/src/Shaders")
fl.addFiles( sp, ".*", "share/oce-0.18/Shaders", sp)
wxs.addEnvVar('CSF_ShadersDirectory','[INSTALLDIR]share\\oce-0.18\\Shaders','3b4ab883-6637-4969-a006-d47ab6e8072b')

while (fl.addDependencies()>0):
    pass

print(fl.files)

wxs.addFiles(fl.files)

scriptPath = os.path.dirname(os.path.realpath(__file__))
wxs.write(packageName+".wxs")
subprocess.run([os.path.join(scriptPath,"wixbuild.sh"), packageName])
