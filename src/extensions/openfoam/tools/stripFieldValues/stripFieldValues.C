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
 */

/**
 * stripFieldValues
 *
 * Reads all OpenFOAM field dictionaries from a specified time directory and
 * replaces the internalField entry and every boundary-patch value entry with
 * its arithmetic average, written as a "uniform <value>" entry.
 *
 * The tool works entirely at the dictionary / token level and does NOT require
 * the mesh to be present or consistent with the field sizes.
 *
 * Usage:
 *   stripFieldValues <timeName>  [-case <caseDir>]  [-fields "field1 field2 ..."]
 *
 * If -fields is given, only the listed field files are processed; all other
 * files in the time directory are left untouched.
 */

#include "argList.H"
#include "Time.H"
#include "IFstream.H"
#include "OFstream.H"
#include "IOobject.H"
#include "dictionary.H"
#include "primitiveEntry.H"
#include "IStringStream.H"
#include "OStringStream.H"
#include "scalar.H"
#include "vector.H"
#include "symmTensor.H"
#include "tensor.H"
#include "fileName.H"
#include "wordList.H"
#include "UIListStream.H"

#include "uniof.h"

#include <filesystem>

using namespace Foam;
namespace fs = std::filesystem;


// ─── helper templates ────────────────────────────────────────────────────────

/**
 * Read a "uniform X" or "nonuniform List<T> N ( ... )" entry from an ITstream
 * and return the arithmetic average of all values.
 * The stream is rewound before reading so it is safe to call repeatedly.
 */
template<class T>
T computeAverage(ITstream& its)
{
    its.rewind();

    token firstTok;
    its >> firstTok;

    if (!firstTok.isWord())
        return pTraits<T>::zero;

    const word kw = firstTok.wordToken();

    if (kw == "uniform")
    {
        T val;
        its >> val;
        return val;
    }
    else if (kw == "nonuniform")
    {
        token secondTok;
        its >> secondTok;

        if (secondTok.isCompound())
        {
            // OF-v2112+: the entire list is stored as a single compound token
            // (token::Compound<List<T>>) rather than as individual value tokens.
            // transferCompoundToken() marks secondTok as "moved" (COMPOUND type
            // cleared) and returns a reference to the underlying compound object.
            // The compound's refCount is still shared with the ITstream's copy of
            // the token, so we must NOT delete &c directly — doing so bypasses the
            // refCount mechanism and causes a double-free when the old dictionary
            // entry is later deleted.  Instead, borrow the reference for reading
            // only and let secondTok's destructor handle the refCount decrement.
            token::compound& c = secondTok.transferCompoundToken(its);
            auto* lstPtr = dynamic_cast<token::Compound<List<T>>*>(&c);
            T result = pTraits<T>::zero;
            if (lstPtr && (lstPtr->size()>0))
            {
                T sum = pTraits<T>::zero;
                for (const T& v : *lstPtr)
                    sum += v;
                result = sum / scalar(lstPtr->size());
            }
            return result;
        }
        else if (secondTok.isWord())
        {
            // Traditional tokenised format: List<type>  N  ( v1 v2 ... )
            label n = 0;
            its >> n;

            if (n <= 0)
                return pTraits<T>::zero;

            token parenTok;
            its >> parenTok;  // '('

            T sum = pTraits<T>::zero;
            for (label i = 0; i < n; ++i)
            {
                T val;
                its >> val;
                sum += val;
            }

            return sum / scalar(n);
        }
    }

    return pTraits<T>::zero;
}


/**
 * Replace (or add) the entry named `key` in `dict` with "uniform <avg>".
 *
 * Builds the token list directly rather than going through the
 * primitiveEntry template constructor's OStringStream/IStringStream
 * round-trip.  The round-trip reads "uniform" via ISstream::read(word&)
 * which uses a static char buf[1024]; in certain call-stack states that
 * buf can produce the truncated form "unifor".
 *
 * By constructing token(word("uniform", false)) directly and only using
 * the round-trip for the value part, the "uniform" keyword is never read
 * back through ISstream and cannot be truncated.
 */
template<class T>
void setUniformEntry(dictionary& dict, const word& key, const T& avg)
{
    // Serialize the value part only (not the "uniform" keyword) to tokens.
    OStringStream valOs;
    valOs << avg << token::END_STATEMENT;
    IStringStream valIs(valOs.str());

    // Collect value tokens (scalar, or '(' x y z ')' for vectors, etc.).
    tokenList valToks(10);
    label nVal = 0;
    token t;
    while (!valIs.read(t).bad() && t.good() && !(t == token::END_STATEMENT))
    {
        valToks.newElmt(nVal++) = std::move(t);
    }
    valToks.resize(nVal);

    // Build final token list: explicit word("uniform") + value tokens.
    // The word is constructed directly — no ISstream read involved.
    tokenList toks(1 + nVal);
    for (label i = 0; i < nVal; ++i)
    {
        toks[i + 1] = std::move(valToks[i]);
    }
    toks[0] = token(word("uniform", false));  // false = skip stripInvalid

    dict.add(new primitiveEntry(key, std::move(toks)), true);
}


/**
 * Read the named entry, compute its average, and replace it with the
 * uniform average value.  No-op if the entry is absent.
 */
template<class T>
void stripEntry(dictionary& dict, const word& key)
{
    if (!dict.found(key))
        return;

    ITstream& its = dict.lookup(key);
    const T avg = computeAverage<T>(its);
    setUniformEntry<T>(dict, key, avg);

    Info << "      " << key << "  ->  uniform " << avg << nl;
}


/**
 * Process the internalField and all boundary-patch value entries of a field
 * dictionary in-place, replacing nonuniform data with uniform averages.
 */
template<class T>
void processFieldDict(dictionary& fieldDict)
{
    Info << "    internalField:" << nl;
    stripEntry<T>(fieldDict, "internalField");

    if (fieldDict.found("boundaryField"))
    {
        dictionary& bfDict = fieldDict.subDict("boundaryField");
        const wordList patchNames = bfDict.toc();
        for (const word& patchName : patchNames)
        {
            if (bfDict.isDict(patchName))
            {
                dictionary& patchDict = bfDict.subDict(patchName);
                if (patchDict.found("value"))
                {
                    Info << "    patch " << patchName << ":" << nl;
                    stripEntry<T>(patchDict, "value");
                }
            }
        }
    }
}


/**
 * Process one field file.
 *
 * The FoamFile header is read via IOobject::readHeader(Istream&), which
 * captures the class name without imposing any type check and positions
 * the stream right after the header block.  The remaining content is then
 * read into a plain dictionary, modified, and written back.
 *
 * Returns false when the file was skipped (not a recognised field type).
 */
bool processField(
    const word&     fieldName,
    const word&     timeName,
    const Time&     runTime,
    const fileName& timeDir
)
{
    const fileName filePath = timeDir / fieldName;

    IFstream ifs(filePath);
    if (!ifs.good())
        return false;

    // IOobject is used solely as a lightweight container for the header data.
    // NO_READ prevents any automatic file access; the readHeader call below
    // reads the FoamFile block from our already-opened stream without
    // enforcing a class-name match.
    IOobject io
    (
        fieldName,
        timeName,
        runTime,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    );

    if (!io.readHeader(ifs))
        return false;   // not a valid OpenFOAM dictionary file

    const word fieldClass = io.headerClassName();

    // Read the field body (dimensions, internalField, boundaryField, ...)
    // from the stream position immediately after the FoamFile block.
    dictionary fieldDict(ifs);

    Info << "  " << fieldName << "  (class: " << fieldClass << ")" << nl;

    // Dispatch on the primitive type encoded in the class name.
    // Check SymmTensor before Tensor to avoid a false substring match.
    if      (fieldClass.find("Scalar")     != std::string::npos)
        processFieldDict<scalar>(fieldDict);
    else if (fieldClass.find("SymmTensor") != std::string::npos)
        processFieldDict<symmTensor>(fieldDict);
    else if (fieldClass.find("Tensor")     != std::string::npos)
        processFieldDict<tensor>(fieldDict);
    else if (fieldClass.find("Vector")     != std::string::npos)
        processFieldDict<vector>(fieldDict);
    else
    {
        Info << "    Skipping: unrecognized field class." << nl;
        return false;
    }

    // Write the modified field back.
    //   writeHeader  – banner + FoamFile { class fieldClass; ... }
    //   write(false) – field body without extra wrapping braces
    //   writeEndDivider – trailing // *** //
    OFstream ofs(filePath);
    io.writeHeader(ofs, fieldClass);
    fieldDict.write(ofs, false);
    IOobject::writeEndDivider(ofs);

    return true;
}


// ─── main ────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    argList::noParallel();
    argList::validArgs.append("time");
    argList::addOption
    (
        "fields",
        "wordList",
        "restrict processing to these field names (default: all fields)"
    );

#   include "setRootCase.H"
#   include "createTime.H"

    const word     timeName(UNIOF_ADDARG(args, 0));
    const fileName timeDir = runTime.path() / timeName;

    // Optional field filter (-fields "U p k ...")
    wordList restrictFields;
    if (args.optionFound("fields"))
        args.optionLookup("fields")() >> restrictFields;
    const bool filterFields = !restrictFields.empty();

    Info << "stripFieldValues: processing time directory " << timeDir
         << nl;
    if (filterFields)
        Info << "  restricted to fields: " << restrictFields << nl;
    Info << endl;

    if (!fs::exists(timeDir.c_str()) || !fs::is_directory(timeDir.c_str()))
    {
        FatalErrorIn("main")
            << "Time directory does not exist: " << timeDir
            << exit(FatalError);
    }

    label nProcessed = 0;
    label nSkipped   = 0;

    for (const auto& dirEntry : fs::directory_iterator(timeDir.c_str()))
    {
        if (!dirEntry.is_regular_file())
            continue;

        const word fieldName(dirEntry.path().filename().string());

        if (filterFields && !restrictFields.found(fieldName))
        {
            ++nSkipped;
            continue;
        }

        try
        {
            if (processField(fieldName, timeName, runTime, timeDir))
                ++nProcessed;
            else
                ++nSkipped;
        }
        catch (const std::exception& ex)
        {
            Info << "  WARNING: could not process "
                 << fieldName << ": " << ex.what() << nl;
            ++nSkipped;
        }
    }

    Info << nl
         << "Processed " << nProcessed << " field(s), "
         << "skipped "   << nSkipped   << " non-field file(s)."
         << nl << endl;

    return 0;
}


// ************************************************************************* //
