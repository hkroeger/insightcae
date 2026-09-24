/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * This header used to bundle ~20 unrelated tool classes/functions. It has
 * been split into focused headers (base/base64.h, base/destructionguard.h,
 * base/filesystemtools.h, base/temporaryfile.h, base/remotecommand.h,
 * base/sharedpathlist.h, base/exectimer.h, base/stringconv.h,
 * base/boundingbox.h, base/vtkgeometrytools.h, base/fileio.h,
 * base/templatefile.h, base/meminfo.h, base/rsyncoutputanalyzer.h,
 * base/predictinsertionlocation.h, base/variablenames.h, base/ondemand.h,
 * base/realnp.h, base/resultantforce.h, base/operatingsystem.h). Most are
 * still re-included here for backward compatibility; base/remotecommand.h
 * (SSHCommand/RSYNCCommand/findFreePort/findRemoteFreePort, which pull in
 * boost::process) is the one group narrowed out of this umbrella since its
 * real external consumer set is small and bounded — include it directly if
 * you need it.
 *
 */

#ifndef TOOLS_H
#define TOOLS_H

#include "base/operatingsystem.h"
#include "base/base64.h"
#include "base/destructionguard.h"
#include "base/filesystemtools.h"
#include "base/temporaryfile.h"
#include "base/sharedpathlist.h"
#include "base/exectimer.h"
#include "base/stringconv.h"
#include "base/boundingbox.h"
#include "base/vtkgeometrytools.h"
#include "base/fileio.h"
#include "base/templatefile.h"
#include "base/meminfo.h"
#include "base/rsyncoutputanalyzer.h"
#include "base/predictinsertionlocation.h"
#include "base/variablenames.h"
#include "base/ondemand.h"
#include "base/realnp.h"
#include "base/resultantforce.h"

#endif // TOOLS_H
