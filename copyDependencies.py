#!/usr/bin/python3
# This Python file uses the following encoding: utf-8

import os,sys,subprocess,re,copy,shutil
import xml.etree.cElementTree as ET
import importlib.util
superbuildSourcePath = os.path.dirname(os.path.realpath(__file__))
spec = importlib.util.spec_from_file_location("module.name", os.path.join(superbuildSourcePath, "wix.py"))
wix = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wix)

from optparse import OptionParser

parser = OptionParser()

parser.add_option("-x", "--mxepath", dest="mxepath", metavar='mxepath', default="/opt/mxe",
                help="path to mxe cross compile environment")

parser.add_option("-s", "--insightSourcePath", dest="insightSourcePath", metavar='InsightCAE source path', default=superbuildSourcePath,
                  help="path to insightcae source code (not superbuild source)")

(opts, args) = parser.parse_args()


directories=["bin", "share/insight"]

if not (os.path.exists("./bin") and os.path.exists("./share/insight")):
    print("Please execute in build directory!")
    sys.exit(-1)


fl=wix.FileList(opts.mxepath, ["python.*\\.dll", "opengl.*\\.dll"])

for d in directories:
    fl.addFiles(d)

fl.addFile( os.path.join(opts.mxepath, "usr/i686-w64-mingw32.shared/qt5/plugins/platforms/qwindows.dll"), "bin/plugins/platforms" )

sp=os.path.join(opts.mxepath, "usr/i686-w64-mingw32.shared/share/oce-0.18/src/Shaders")
fl.addFiles( sp, ".*", "share/oce-0.18/Shaders", sp)
#wxs.addEnvVar('CSF_ShadersDirectory','[INSTALLDIR]share\\oce-0.18\\Shaders','3b4ab883-6637-4969-a006-d47ab6e8072b')
#wxs.addEnvVar('INSIGHT_GLOBALSHAREDDIRS','[INSTALLDIR]share\\insight','2dba120f-def7-441d-93a8-abef0b5448e0')

while (fl.addDependencies()>0):
    pass

for fn,(fid,org,targdir) in fl.files.items():
    dst=os.path.join(targdir,fn)
    doit=not os.path.exists(dst);
    if not doit:
        doit=(os.path.getmtime(dst)<os.path.getmtime(org))
    if doit:
        if not os.path.exists(targdir): os.makedirs(targdir)
        print("copy", org, "to", dst)
        shutil.copyfile(org, dst)
