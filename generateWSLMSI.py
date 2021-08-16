#!/usr/bin/env python3
# This Python file uses the following encoding: utf-8

import os,sys,subprocess,re
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

parser.add_option("-p", "--password", dest="password", metavar='password', default="",
                  help="password for customer repository")

(opts, args) = parser.parse_args()

packageName='InsightCAE WSL Environment'
wsldistlabel=wix.wslDistributionLabel(opts.customer)
ubunturootfsfile="ubuntu-18.04-server-cloudimg-amd64-wsl.rootfs.tar.gz"

if not (opts.customer=="ce" or opts.customer=="ce-dev"):
    packageName+=" ("+opts.customer+")"



wxs=wix.WixInput(
    packageName,
    'b5a4aa4c-aa68-4244-8ed3-e8c5390fcb75', 'ebf57ecd-4a01-4568-a4fb-7f9f790ee970',
    (1,0,0),
    menuentry=("InsightCAE WSL", 'ef0d219c-e5ac-4ed1-b266-275386bd15fe') )

# WSL image
compn='ubunturootfs'
wxs.addFiles( { 'ubuntu.tgz': (compn, ubunturootfsfile,"") } )

# install command
wslcommand="wsl.exe -d {wslname}".format( wslname=wsldistlabel )
wxs.addExecutionSteps([

 ('installwsl', 'wsl.exe --import {wslname} "[PersonalFolder]{wslname}" "[INSTALLDIR]ubuntu.tgz"'\
        .format( wslname=wsldistlabel ) ),

 ('setupwsl1', "{prefix} apt-key adv --fetch-keys http://downloads.silentdynamics.de/SD_REPOSITORIES_PUBLIC_KEY.gpg"\
        .format( prefix=wslcommand ) ),

 ('setupwsl2', "{prefix} add-apt-repository {repourl}"\
        .format(
            prefix=wslcommand,
            repourl=({
             'ce': "http://downloads.silentdynamics.de/ubuntu",
             'ce-dev': "http://downloads.silentdynamics.de/ubuntu_dev"
            }.get(opts.customer,
              "https://{customer}:{pwd}@rostock.kroegeronline.net/customers/{customer}".format(
                customer=opts.customer, pwd=opts.password )
              ))
        ) ),
              
 ('setupwsl3', "{prefix} apt-get update"\
        .format( prefix=wslcommand ) ),

 ('setupwsl4', "{prefix} apt-get install -y {package}"\
        .format( prefix=wslcommand,
                 package=
                    "insightcae-ce"
                    if opts.customer=="ce" or opts.customer=="ce-dev" else
                    "insightcae" ) ),

 ('setupwsl5', "{prefix} sed -i \"1i source /opt/insightcae/bin/insight_setenv.sh\" /root/.bashrc"\
        .format( prefix=wslcommand ) )
], "InstallFiles", "(NOT Installed) AND (NOT REMOVE)")


# uninstall commands
wxs.addExecutionSteps([
 ( 'uninstallwsl', 'wsl.exe --unregister {wslname}'.format( wslname=wsldistlabel ) )
], "InstallInitialize", "(NOT UPGRADINGPRODUCTCODE) AND (REMOVE=\"ALL\")")


# menu entry
wxs.addShortcut(
    'shellshortcut',
    "WSL Shell", "OpenFOAM Command line",
    "[WindowsFolder]System32\\wsl.exe", arguments='-d {wslname}'.format( wslname=wsldistlabel ),
    icon=os.path.join(sourcePath, "symbole", 'logo_insight_cae.ico') )

wxs.write("insightcae-wsl.wxs")
subprocess.run([os.path.join(sourcePath,"wixbuild.sh"), "insightcae-wsl"])
