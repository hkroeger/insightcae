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
 */

#ifndef UNIOF_H
#define UNIOF_H


#if defined(OFplus)||defined(OFdev)
#define UNIOF_TMP_NONCONST(x) (x).ref()
#else
#define UNIOF_TMP_NONCONST(x) (x)()
#endif


#if defined(OFplus)||defined(OFdev)
#define UNIOF_BOUNDARY_NONCONST(x) (x).boundaryFieldRef()
#else
#define UNIOF_BOUNDARY_NONCONST(x) (x).boundaryField()
#endif


#if defined(OF301) || defined(OFplus)||defined(OFdev)
#define UNIOF_WALLDIST_Y(wd) (wd).y()
#else
#define UNIOF_WALLDIST_Y(wd) (wd)
#endif

#if defined(OFplus)
#define UNIOF_HEADEROK(ioo,typ) (ioo).typeHeaderOk<typ>()
#else
#define UNIOF_HEADEROK(ioo,typ) (ioo).headerOk()
#endif

#if (defined(OF301) || defined(OFplus)||defined(OFdev))
#define UNIOF_ADDARG(args,j) (args).arg((j)+1)
#else
#define UNIOF_ADDARG(args,j) (args).additionalArgs()[j]
#endif

#if (defined(OF23x)||defined(OF301)||defined(OFplus)||defined(OFdev))
#define UNIOF_ADDOPT(aa,name,typ,desc) aa::addOption(name,typ,desc)
#else
#define UNIOF_ADDOPT(aa,name,typ,desc) aa::validOptions.insert(name,desc)
#endif

#if defined(OFplus)||defined(OFdev)
#define UNIOF_INTERNALFIELD_NONCONST(f) f.ref().field()
#else
#define UNIOF_INTERNALFIELD_NONCONST(f) f.internalField()
#endif


#if defined(OF16ext)
#define UNIOF_LABELULIST unallocLabelList
#elif defined(OFplus)
#define UNIOF_LABELULIST labelList
#else
#define UNIOF_LABELULIST labelUList
#endif


#endif
