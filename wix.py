
import os,sys,subprocess,re,copy
import xml.etree.cElementTree as ET

def sanitizeId(orgname):
    clean="id"+"".join(filter(str.isalnum, orgname))
    print (orgname, " ==>> ", clean)
    return clean

def toWinePath(linuxPath):
    return "z:"+os.path.abspath(linuxPath).replace("/", "\\")

class WixInput:

    def __init__(self, packageName, packageGuid, packageUpgradeCode, fullVersion=(1,0,0), menuentry=("InsightCAE", '5f61d82b-1c22-4e54-b9b6-b7cad11e7aee')):
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
        'SummaryCodepage': '1252'
        })

        self.Media=ET.SubElement(self.Product, "Media", attrib={
        'Id': '1',
        'Cabinet': packageName+'.cab',
        'EmbedCab': 'yes',
        'DiskPrompt': 'Disk #1'
        })

        self.Property=ET.SubElement(self.Product, "Property", attrib={
        'Id': 'DiskPrompt',
        'Value': "silentdynamics InsightCAE %d.%d.%d Installation [1]"%fullVersion
        })

        self.Feature=ET.SubElement(self.Product, "Feature", attrib={
        'Id': 'Complete',
        'Level': '1'
        })

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
            label,cmd = steps[i]
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
