#!/usr/bin/env python3
# This Python file uses the following encoding: utf-8

import os,sys,subprocess,re
import xml.etree.cElementTree as ET

packageName='InsightCAE WSL Environment'
ubunturootfsfile="ubuntu-18.04-server-cloudimg-amd64-wsl.rootfs.tar.gz"

def toWinePath(linuxPath):
    return "z:"+os.path.abspath(linuxPath).replace("/", "\\")


wix = ET.Element("Wix", attrib={
    'xmlns': 'http://schemas.microsoft.com/wix/2006/wi'
})

Product = ET.SubElement(wix, "Product", attrib={
    'Name':packageName,
    'Manufacturer': 'silentdynamics GmbH',
    'Id': 'b5a4aa4c-aa68-4244-8ed3-e8c5390fcb75',
    'UpgradeCode': 'ebf57ecd-4a01-4568-a4fb-7f9f790ee970',
    'Language': '1033',
    'Codepage': '1252',
    'Version': '1.0'
})

Package=ET.SubElement(Product, "Package", attrib={
    'Id': '*',
    'Keywords': 'Installer',
    'Description': "This installer sets up the WSL backend environment for silentdynamics InsightCAE for Windows",
    'Comments': 'comment',
    'Manufacturer': 'silentdynamics GmbH',
    'InstallerVersion': '100',
    'Languages': '1033',
    'Compressed': 'yes',
    'SummaryCodepage': '1252'
})

Media=ET.SubElement(Product, "Media", attrib={
    'Id': '1',
    'Cabinet': 'insightcae-wsl-windows.cab',
    'EmbedCab': 'yes',
    'DiskPrompt': 'Disk #1'
})

Property=ET.SubElement(Product, "Property", attrib={
    'Id': 'DiskPrompt',
    'Value': "silentdynamics InsightCAE WSL [1]"
})

INSTALLDIR=ET.SubElement(
    ET.SubElement( ET.SubElement( ET.SubElement(Product,
        "Directory", attrib={'Id':'TARGETDIR', 'Name':'SourceDir'}),
       "Directory", attrib={'Id':'ProgramFilesFolder', 'Name':'PFiles'}),
      "Directory", attrib={'Id':'silentdynamics', 'Name':'silentdynamics'}),
     "Directory", attrib={'Id':'INSTALLDIR', 'Name':'InsightCAE-WSL'})

#PersonalFolder=ET.SubElement(Product,
#             "Directory", attrib={'Id':'PERSONALFOLDER', 'Name':'PersonalFolder'})

Feature=ET.SubElement(Product, "Feature", attrib={
    'Id': 'Complete',
    'Level': '1'
})

#wslsearch=ET.SubElement(Product, "Property", attrib={
#    'Id': 'WSL_EXE'
#})
#ds=ET.SubElement(wslsearch, "DirectorySearch", attrib={
# 'Id': "Windows",
# 'Path': '[WindowsFolder]'
#})
#ET.SubElement(ds, "FileSearch", attrib={
# 'Id': "wsl_exe",
# 'Name': "wsl.exe",
# 'AssignToProperty': "yes"
#})

# WSL image
compn='ubunturootfs'
Component=ET.SubElement(INSTALLDIR, "Component", attrib={ 'Id': compn, 'Guid': '*' })
File=ET.SubElement(Component, "File", attrib={ 'Id': compn, 'Name': 'ubuntu.tgz', 'DiskId': '1', 'Source':toWinePath(ubunturootfsfile), 'KeyPath': 'yes' })
ET.SubElement(Feature, "ComponentRef", attrib={'Id': compn})

InstallExecuteSequence=ET.SubElement(Product, "InstallExecuteSequence")

ET.SubElement(Product, "CustomAction", attrib={
    'Id': 'installwsl',
    'Return': "check",
    'Impersonate': "yes",
    'Execute': "deferred",
    'Directory': "INSTALLDIR",
    'ExeCommand': 'wsl.exe --import insightcae-ubuntu-1804 "[PersonalFolder]insightcae-ubuntu-1804" "[INSTALLDIR]ubuntu.tgz"'
})
ET.SubElement(InstallExecuteSequence, "Custom", attrib={
    'After': "InstallFiles",
    'Action': "installwsl"
}).text="(NOT Installed) AND (NOT REMOVE)"
ET.SubElement(Product, "CustomAction", attrib={
    'Id': 'setupwsl1',
    'Return': "check",
    'Impersonate': "yes",
    'Execute': "deferred",
    'Directory': "INSTALLDIR",
    'ExeCommand': "wsl.exe -d insightcae-ubuntu-1804 apt-key adv --fetch-keys http://downloads.silentdynamics.de/SD_REPOSITORIES_PUBLIC_KEY.gpg"
})
ET.SubElement(Product, "CustomAction", attrib={
    'Id': 'setupwsl2',
    'Return': "check",
    'Impersonate': "yes",
    'Execute': "deferred",
    'Directory': "INSTALLDIR",
    'ExeCommand': "wsl.exe -d insightcae-ubuntu-1804 add-apt-repository http://downloads.silentdynamics.de/ubuntu"
})
ET.SubElement(Product, "CustomAction", attrib={
    'Id': 'setupwsl3',
    'Return': "check",
    'Impersonate': "yes",
    'Execute': "deferred",
    'Directory': "INSTALLDIR",
    'ExeCommand': "wsl.exe -d insightcae-ubuntu-1804 apt-get update"
})
ET.SubElement(Product, "CustomAction", attrib={
    'Id': 'setupwsl4',
    'Return': "check",
    'Impersonate': "yes",
    'Execute': "deferred",
    'Directory': "INSTALLDIR",
    'ExeCommand': "wsl.exe -d insightcae-ubuntu-1804 apt-get install -y insightcae-ce"
})
ET.SubElement(Product, "CustomAction", attrib={
    'Id': 'setupwsl5',
    'Return': "check",
    'Impersonate': "yes",
    'Execute': "deferred",
    'Directory': "INSTALLDIR",
    'ExeCommand': "wsl.exe -d insightcae-ubuntu-1804 sed -i \"1i source /opt/insightcae/bin/insight_setenv.sh\" /root/.bashrc"
})

ET.SubElement(InstallExecuteSequence, "Custom", attrib={
    'After': "installwsl",
    'Action': "setupwsl1"
}).text="(NOT Installed) AND (NOT REMOVE)"
ET.SubElement(InstallExecuteSequence, "Custom", attrib={
    'After': "setupwsl1",
    'Action': "setupwsl2"
}).text="(NOT Installed) AND (NOT REMOVE)"
ET.SubElement(InstallExecuteSequence, "Custom", attrib={
    'After': "setupwsl2",
    'Action': "setupwsl3"
}).text="(NOT Installed) AND (NOT REMOVE)"
ET.SubElement(InstallExecuteSequence, "Custom", attrib={
    'After': "setupwsl3",
    'Action': "setupwsl4"
}).text="(NOT Installed) AND (NOT REMOVE)"
ET.SubElement(InstallExecuteSequence, "Custom", attrib={
    'After': "setupwsl4",
    'Action': "setupwsl5"
}).text="(NOT Installed) AND (NOT REMOVE)"

#ET.SubElement(Product, "CustomAction", attrib={
#    'Id': 'removetempfile',
#    'Return': "check",
#    'Impersonate': "yes",
#    'Execute': "deferred",
#    'Directory': "INSTALLDIR",
#    'ExeCommand': 'cmd.exe /c del "[PersonalFolder]ubuntu.tgz"'  # not working because of insufficient rights
#})
#ET.SubElement(InstallExecuteSequence, "Custom", attrib={
#    'Action': "removetempfile",
#    'After': "installwsl"
#}).text="(NOT Installed) AND (NOT REMOVE)"

ET.SubElement(Product, "CustomAction", attrib={
    'Id': 'uninstallwsl',
    'Return': "check",
    'Impersonate': "yes",
    'Execute': "deferred",
    'Directory': "INSTALLDIR",
    'ExeCommand': 'wsl.exe --unregister insightcae-ubuntu-1804'
})
ET.SubElement(InstallExecuteSequence, "Custom", attrib={
    'Action': "uninstallwsl",
    'After': "InstallInitialize"
}).text="(NOT UPGRADINGPRODUCTCODE) AND (REMOVE=\"ALL\")"


tree = ET.ElementTree(wix)
tree.write(packageName+".wxs")
scriptPath = os.path.dirname(os.path.realpath(__file__))
subprocess.run([os.path.join(scriptPath,"wixbuild.sh"), packageName])
