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

#if (OF_VERSION>060000)
#define UNIOF_OPTIONFOUND(a,n) a.found(n)
#else
#define UNIOF_OPTIONFOUND(a,n) a.optionFound(n)
#endif

#if (OF_VERSION>060000)
#define UNIOF_OPTIONREADIFPRESENT(args, name, v) args.readIfPresent(name, v)
#else
#define UNIOF_OPTIONREADIFPRESENT(args, name, v) args.optionReadIfPresent(name, v)
#endif

#if (OF_VERSION>060000)
#define UNIOF_OPTIONLOOKUP(args, name) args.lookup(name)
#else
#define UNIOF_OPTIONLOOKUP(args, name) args.optionLookup(name)
#endif

#if  (OF_VERSION<=010603)
#define UNIOF_WORDRELIST 	wordList
#else
#define UNIOF_WORDRELIST 	wordReList
#endif

#if (OF_VERSION>=040000)
#define UNIOF_TMP_NONCONST(x) (x).ref()
#else
#define UNIOF_TMP_NONCONST(x) (x)()
#endif


#if (OF_VERSION>=040000)
#define UNIOF_BOUNDARY_NONCONST(x) (x).boundaryFieldRef()
#else
#define UNIOF_BOUNDARY_NONCONST(x) (x).boundaryField()
#endif


#if (OF_VERSION>=030000)
#define UNIOF_WALLDIST_Y(wd) (wd).y()
#else
#define UNIOF_WALLDIST_Y(wd) (wd)
#endif

#if (OF_VERSION>=040000)
#define UNIOF_HEADEROK(ioo,typ) (ioo).typeHeaderOk<typ>()
#else
#define UNIOF_HEADEROK(ioo,typ) ( ((ioo).headerOk()) && ((ioo).headerClassName() == pTraits<typ>::typeName) )
#endif

#if (defined(OF_FORK_esi) && (OF_VERSION>=060000))
#define UNIOF_ADDARG(args,j) (args).args()[j+1]
#elif (OF_VERSION>=030000) //(defined(OF301) || defined(OFplus)||defined(OFdev))
#define UNIOF_ADDARG(args,j) (args).arg((j)+1)
#else
#define UNIOF_ADDARG(args,j) (args).additionalArgs()[j]
#endif

#if (OF_VERSION>=020300)
#define UNIOF_ADDOPT(aa,name,typ,desc) aa::addOption(name,typ,desc)
#else
#define UNIOF_ADDOPT(aa,name,typ,desc) aa::validOptions.insert(name,desc)
#endif

#if (defined(OF_FORK_esi) && (OF_VERSION>=060000))
#define UNIOF_OPTION(aa, optname) aa.options()[optname]
#else
#define UNIOF_OPTION(aa, optname) aa.option(optname)
#endif

#if (OF_VERSION>=040000)
#define UNIOF_INTERNALFIELD(f) f.primitiveField()
#else
#define UNIOF_INTERNALFIELD(f) f.internalField()
#endif


#if (OF_VERSION>=040000)
#define UNIOF_DIMINTERNALFIELD(f) (f).internalField()
#else
#define UNIOF_DIMINTERNALFIELD(f) (f).dimensionedInternalField()
#endif

#if (OF_VERSION>=040000)
#define UNIOF_INTERNALFIELD_NONCONST(f) (f).ref().field()
#else
#define UNIOF_INTERNALFIELD_NONCONST(f) (f).internalField()
#endif

#if defined(OF_FORK_extend)
#define UNIOF_LABELULIST unallocLabelList
#elif (OF_VERSION>040000)
#define UNIOF_LABELULIST labelList
#else
#define UNIOF_LABELULIST labelUList
#endif


#define ASSERTION(condition, message) \
{ if (!(condition)) \
    FatalErrorIn("assertion") << message << abort(FatalError); }

#define ERROR(message) \
{ FatalErrorIn("error condition") << message << abort(FatalError); }

#define UNIOF_CREATEPOSTPROCFILE(TIMEVAR, FONAME, FILENAME, STREAMPTRVAR, HEADER) \
if (STREAMPTRVAR.empty()) \
{ \
    if (Pstream::master()) \
    {\
        fileName outdir;\
        word startTimeName=(TIMEVAR).timeName( (TIMEVAR).startTime().value() );\
        if (Pstream::parRun())\
        { outdir =(TIMEVAR).path()/".."/"postProcessing"/(FONAME)/startTimeName; }\
        else \
        { outdir = (TIMEVAR).path()/"postProcessing"/(FONAME)/startTimeName; }\
        mkDir(outdir);\
        STREAMPTRVAR.reset(new OFstream(outdir/(FILENAME)));\
        if (HEADER) { (*HEADER)(STREAMPTRVAR()); STREAMPTRVAR()<<endl; }\
    }\
}

#endif
