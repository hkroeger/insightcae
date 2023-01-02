#!/usr/bin/python3

import os,sys,subprocess,re,copy,hashlib
import xml.etree.cElementTree as ET
import urllib.request as req

def sanitizeId(orgname):
    clean="id"+"".join(filter(str.isalnum, orgname))
    print (orgname, " ==>> ", clean)
    return clean

def toWinePath(linuxPath):
    return "z:"+os.path.abspath(linuxPath).replace("/", "\\")

def hasChecksum(filename, validsha256):
    sha256_hash = hashlib.sha256()
    with open(filename,"rb") as f:
     # Read and update hash string value in blocks of 4K
     for byte_block in iter(lambda: f.read(4096),b""):
      sha256_hash.update(byte_block)
     print("checksum is ", sha256_hash.hexdigest())
     if sha256_hash.hexdigest()!=validsha256:
         print('Invalid checksum of downloaded file {filename}! Got: {isdig}, should be:  {wantdig}'.format(filename=filename, isdig=sha256_hash.hexdigest(), wantdig=validsha256))
         return False
     else:
         return True
    
def downloadAndCheck(url, filename, validsha256):
    
    if os.path.exists(filename):
        if not hasChecksum(filename, validsha256):
            print("Re-downloading {filename}...".format(filename=filename))
            os.remove(filename)
            
    if not os.path.exists(filename):
        req.urlretrieve(url, filename)

    if not hasChecksum(filename, validsha256):
        raise RuntimeError('Invalid checksum of downloaded file {filename}!'.format(filename=filename))
        os.remove(filename)



class WixInput:

    def __init__(self, packageName, packageGuid, packageUpgradeCode, fullVersion=(1,0,0), menuentry=("InsightCAE", '5f61d82b-1c22-4e54-b9b6-b7cad11e7aee'), compressFiles=True):
        self.wix = ET.Element("Wix", attrib={'xmlns': 'http://schemas.microsoft.com/wix/2006/wi'})

        self.Product = ET.SubElement(self.wix, "Product", attrib={
        'Name':packageName,
        'Manufacturer': 'silentdynamics GmbH',
        'Id': packageGuid,
        'UpgradeCode': packageUpgradeCode,
        'Language': '1033',
        'Codepage': '1252',
        'Version': '%d.%d.%d'%fullVersion
        })

        self.Package=ET.SubElement(self.Product, "Package", attrib={
        'Id': '*',
        'Keywords': 'Installer',
        'Description': "silentdynamics InsightCAE for Windows Installer",
        'Comments': 'comment',
        'Manufacturer': 'silentdynamics GmbH',
        'InstallerVersion': '100',
        'Languages': '1033',
        'Compressed': 'yes',
        'SummaryCodepage': '1252',
        'InstallScope': 'perMachine'
        })

        self.Media=ET.SubElement(self.Product, "Media", attrib={
        'Id': '1',
        'Cabinet': 'disk_1.cab',
        'EmbedCab': 'yes',
        'DiskPrompt': 'Disk #1',
        'CompressionLevel': 'high' if compressFiles else 'none'
        })

        self.Property=ET.SubElement(self.Product, "Property", attrib={
        'Id': 'DiskPrompt',
        'Value': "silentdynamics InsightCAE %d.%d.%d Installation [1]"%fullVersion
        })

        self.Feature=ET.SubElement(self.Product, "Feature", attrib={
        'Id': 'Complete',
        'Level': '1'
        })

        self.Ui=ET.SubElement(self.Product, "UI")
        
        self.TARGETDIR=ET.SubElement(self.Product,
                        "Directory", attrib={'Id':'TARGETDIR', 'Name':'SourceDir'})
        ET.SubElement(
            ET.SubElement(self.TARGETDIR, "Directory", attrib={'Id': "ProgramMenuFolder"}),
                                    "Directory", attrib={'Id': "ApplicationProgramsFolder", 'Name': menuentry[0] })

        self.APF=ET.SubElement(self.Product, "DirectoryRef", attrib={
         'Id': "ApplicationProgramsFolder"
        })

        self.apfc=ET.SubElement(self.APF, "Component", attrib={
         'Id':'ApplicationShortcut',
         'Guid': menuentry[1]
        })
        ET.SubElement(self.apfc, "RemoveFolder", attrib={'Id':'ApplicationProgramsFolder', 'On':'uninstall'})

        ET.SubElement(self.Feature, "ComponentRef", attrib={'Id':'ApplicationShortcut'})

        self.INSTALLDIR=ET.SubElement(
            ET.SubElement( ET.SubElement( self.TARGETDIR,
              "Directory", attrib={'Id':'ProgramFilesFolder', 'Name':'PFiles'}),
              "Directory", attrib={'Id':'silentdynamics', 'Name':'silentdynamics'}),
              "Directory", attrib={'Id':'INSTALLDIR', 'Name':packageName+' %d.%d.%d'%fullVersion})

        self.subdirs={}

        self.InstallExecuteSequence=ET.SubElement(self.Product, "InstallExecuteSequence")

    def addShortcut(self, id, name, description, target, icon=None, addicon=True, arguments=None):
        attrib={
                    'Id': id,
                    'Name': name,
                    'Description': description,
                    'Target': target,
                    'WorkingDirectory': 'INSTALLDIR'
                }
        if not icon is None:
            iconbn=os.path.basename(icon)
            if addicon: ET.SubElement(self.Product, "Icon", attrib={'Id':iconbn, 'SourceFile':toWinePath(icon)})
            attrib['Icon']=iconbn

        if not arguments is None:
            attrib["Arguments"]=arguments

        ET.SubElement(self.apfc, "Shortcut", attrib=attrib)



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
        varcomp=ET.SubElement(self.INSTALLDIR, "Component", attrib={'Id':name, 'Guid':guid})
        var=ET.SubElement(varcomp, "Environment", attrib={'Id':name, 'Name':name,
                'Action': 'set', 'Permanent': 'yes', 'System':'yes', 'Part': 'all', 'Value':value})
        ET.SubElement(self.Feature, "ComponentRef", attrib={'Id':name})

    def addExecutionSteps(self, steps, startAfterEvent="InstallFiles", filter="(NOT Installed) AND (NOT REMOVE)"):
        for i in range(0, len(steps)):

            if len(steps[i])==3:
                label,cmd,progresstext=steps[i]
                ET.SubElement(self.Ui, "ProgressText", attrib={
                    'Action': label
                }).text=progresstext
            elif len(steps[i])==2:
                label,cmd=steps[i]
                
            ET.SubElement(self.Product, "CustomAction", attrib={
                'Id': label,
                'Return': "check",
                'Impersonate': "yes",
                'Execute': "deferred",
                'Directory': "INSTALLDIR",
                'ExeCommand': cmd
            })
            ET.SubElement(self.InstallExecuteSequence, "Custom", attrib={
                'After': startAfterEvent if i==0 else steps[i-1][0],
                'Action': label
            }).text=filter


    def write(self, fn):
        tree = ET.ElementTree(self.wix)
        tree.write(fn)


def wslDistributionLabel(customer):
    wsldistlabel="insightcae-ubuntu-1804"
    if not (customer=="ce" or customer=="ce-dev"):
        wsldistlabel+="-"+customer
    return wsldistlabel



def getDependencies(file, mxepath):
    deps=set()
    ext=os.path.splitext(file)[-1]
    if ext==".exe" or ext==".dll":
        ret=subprocess.run([os.path.join(mxepath,"usr/x86_64-pc-linux-gnu/bin/peldd"), file], stdout=subprocess.PIPE)
        deps=set([l.decode('UTF-8') for l in ret.stdout.split()])
        #print(file, " : ", deps)
    return deps

def findFile(f, mxepath):
    ret=subprocess.run(["find", os.path.join(mxepath, "usr/i686-w64-mingw32.shared"), "-iname", f], stdout=subprocess.PIPE)
    #ret=subprocess.run(["locate", "-i", "-r", "^"+os.path.join(opts.mxepath, "usr/i686-w64-mingw32.shared")+".*"+f+"$"], stdout=subprocess.PIPE)
    found=[l.decode('UTF-8') for l in ret.stdout.split()]
    #print("found:", found)
    if len(found)>1:
        raise RuntimeError("multiple candidates for "+f+" found! "+str(found))
    if len(found)==0:
        print("No condidate for",f, "found.")
        return None

    return found[-1]



class FileList:

    files={}

    def __init__(self, mxepath):
        self.mxepath=mxepath

    def addFile(self, sourceFile, targetDir):
        bn=os.path.basename(sourceFile)
        #print("add file: ", sourceFile, targetDir, "as", bn)
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
            deps=getDependencies(sourcef, self.mxepath)
            for df in deps:
                if not df in self.files:
                    fdf=findFile(df, self.mxepath)
                    if not fdf is None:
                        added+=1
                        self.files[df]=( sanitizeId(df), fdf, "bin" )

        #print("added", added, "dependencies")
        return added

    def replaceFile(self, filename, newSourcePath):
        if not filename in self.files:
            raise RuntimeError("file "+filename+" is not in the list of files to include in the package!")
        f=self.files[filename]
        self.files[filename]=(f[0], newSourcePath, f[2])

