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

/*---------------------------------------------------------------------------*\
Application
    flashInit

Description
    Initialises the fields of a reactingTwoPhaseEulerFoam flash-boiling case
    from a single-phase incompressible simpleFoam precursor simulation.

    The tool reads:
      - velocity U and kinematic pressure p from the precursor case
      - a saturation curve from a CSV file (columns: p[Pa], T_sat[K], and
        optionally h_fg[J/kg])
      - the initial liquid temperature T and reference absolute pressure pRef
        from the command line

    It computes and writes:
      - alpha.<liquid>, alpha.<gas>  based on the equilibrium flash quality
      - U.<liquid>, U.<gas>          copied from the precursor velocity field
      - T.<liquid>                   set uniformly to T
      - T.<gas>                      set to the local saturation temperature
      - p                            absolute pressure (pRef + rhoLiquid*p_prec)
      - p_rgh                        p minus hydrostatic head

    Assumptions:
      - Both cases share the same mesh topology (same cell count and layout).
        If the meshes differ, run OpenFOAM's mapFields between the precursor
        and the target case before calling flashInit.
      - The precursor pressure field p is stored as kinematic pressure [m^2/s^2]
        (i.e. p_absolute/rho), as produced by simpleFoam.
      - The flash quality is computed with the simplified Rayleigh model:
            x = max(0, cpl*(T - T_sat(p_local)) / h_fg(p_local))
        where p_local = pRef + rhoLiquid * p_precursor.
      - Void fraction from quality:
            alpha_gas = x*rhoLiquid / (x*rhoLiquid + (1-x)*rhoGas)

Usage
    flashInit <precursorCase> <satCurveFile>
              -T <K> -pRef <Pa>
              [-phaseLiquid <name>] [-phaseGas <name>]
              [-cpl <J/kgK>] [-hfg <J/kg>]
              [-rhoLiquid <kg/m3>] [-rhoGas <kg/m3>]
\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "uniof.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <thread>

using namespace Foam;

// ─────────────────────────────────────────────────────────────────────────────
// Piecewise-linear saturation curve read from CSV
// ─────────────────────────────────────────────────────────────────────────────

class SaturationCurve
{
    std::vector<scalar> p_;     // pressure [Pa], ascending
    std::vector<scalar> Tsat_;  // saturation temperature [K]
    std::vector<scalar> hfg_;   // latent heat [J/kg], may be empty

    static scalar lerp(
        const std::vector<scalar>& xs,
        const std::vector<scalar>& ys,
        scalar x)
    {
        if (x <= xs.front()) return ys.front();
        if (x >= xs.back())  return ys.back();
        auto it = std::lower_bound(xs.begin(), xs.end(), x);
        std::size_t i = std::size_t(it - xs.begin());
        scalar f = (x - xs[i-1]) / (xs[i] - xs[i-1]);
        return ys[i-1] + f*(ys[i] - ys[i-1]);
    }

public:

    SaturationCurve(const fileName& csvFile)
    {
        std::ifstream f(csvFile.c_str());
        if (!f.good())
            FatalErrorIn("SaturationCurve")
                << "Cannot open saturation curve CSV file: " << csvFile
                << exit(FatalError);

        std::string line;
        label lineNo = 0;
        while (std::getline(f, line))
        {
            ++lineNo;
            // strip comments and whitespace
            auto hashPos = line.find('#');
            if (hashPos != std::string::npos)
                line = line.substr(0, hashPos);
            if (line.find_first_not_of(" \t\r\n") == std::string::npos)
                continue;

            std::istringstream ss(line);
            std::string tok;
            std::vector<scalar> vals;
            while (std::getline(ss, tok, ','))
            {
                // trim whitespace
                auto b = tok.find_first_not_of(" \t");
                auto e = tok.find_last_not_of(" \t\r\n");
                if (b == std::string::npos) continue;
                tok = tok.substr(b, e - b + 1);
                try
                {
                    vals.push_back(scalar(std::stod(tok)));
                }
                catch (const std::exception&)
                {
                    // non-numeric token – skip line (header)
                    vals.clear();
                    break;
                }
            }

            if (vals.size() >= 2)
            {
                p_.push_back(vals[0]);
                Tsat_.push_back(vals[1]);
                if (vals.size() >= 3)
                    hfg_.push_back(vals[2]);
            }
        }

        if (p_.empty())
            FatalErrorIn("SaturationCurve")
                << "No numeric data read from saturation curve file: "
                << csvFile << nl
                << "Expected CSV with columns: p[Pa], T_sat[K] [, h_fg[J/kg]]"
                << exit(FatalError);

        // ensure ascending pressure order
        for (std::size_t i = 1; i < p_.size(); ++i)
        {
            if (p_[i] <= p_[i-1])
                FatalErrorIn("SaturationCurve")
                    << "Saturation curve pressures must be strictly ascending. "
                    << "Problem at row " << label(i+1)
                    << exit(FatalError);
        }

        Info<< "Saturation curve: read " << label(p_.size())
            << " points from " << csvFile
            << " (p range: " << p_.front() << " – " << p_.back() << " Pa)"
            << nl;

        if (!hfg_.empty() && hfg_.size() != p_.size())
            FatalErrorIn("SaturationCurve")
                << "Inconsistent CSV: h_fg column has " << label(hfg_.size())
                << " entries but p column has " << label(p_.size())
                << exit(FatalError);
    }

    scalar Tsat(scalar p) const { return lerp(p_, Tsat_, p); }

    scalar hfg(scalar p) const
    {
        if (hfg_.empty())
            FatalErrorIn("SaturationCurve::hfg")
                << "Latent heat (h_fg) was not provided in the CSV file. "
                << "Supply it as the third CSV column or use the -hfg option."
                << exit(FatalError);
        return lerp(p_, hfg_, p);
    }

    bool hasHfg() const { return !hfg_.empty(); }
};


// ─────────────────────────────────────────────────────────────────────────────
// Helper: create a volScalarField as a copy of an existing field's structure
//         but with all internal values set to `value`.
// ─────────────────────────────────────────────────────────────────────────────

volScalarField makeUniformScalar(
    const word& name,
    const fvMesh& mesh,
    const word& timeName,
    const dimensionSet& dims,
    scalar value)
{
    return volScalarField(
        IOobject(
            name,
            timeName,
            mesh,
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedScalar(name, dims, value)
    );
}


// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    argList::validArgs.append("precursorCase");
    argList::validArgs.append("satCurveFile");

    scalar pClip(100.);

    UNIOF_ADDOPT(argList, "T",           "K",      "initial liquid temperature [K] (required)");
    UNIOF_ADDOPT(argList, "pRef",        "Pa",     "absolute reference pressure [Pa] (required)");
    UNIOF_ADDOPT(argList, "pClip",       "Pa",     "minimum absolute pressure [Pa] (default: 100Pa)");
    UNIOF_ADDOPT(argList, "phaseLiquid", "word",   "liquid phase name (default: liquid)");
    UNIOF_ADDOPT(argList, "phaseGas",   "word",   "gas phase name (default: gas)");
    UNIOF_ADDOPT(argList, "cpl",         "J/kgK",  "liquid specific heat [J/(kg K)] (default: 4182)");
    UNIOF_ADDOPT(argList, "hfg",         "J/kg",   "latent heat [J/kg] – overrides CSV column 3");
    UNIOF_ADDOPT(argList, "rhoLiquid",   "kg/m3",  "liquid density for pressure conversion (default: 1000)");
    UNIOF_ADDOPT(argList, "rhoGas",      "kg/m3",  "gas density for void-fraction conversion (default: 1)");
    UNIOF_ADDOPT(argList, "maxIter",     "int",    "max iterations for isochoric temperature correction (default: 50)");
    UNIOF_ADDOPT(argList, "tol",         "-",      "convergence tolerance on flash quality (default: 1e-6)");
    UNIOF_ADDOPT(argList, "relaxation",  "-",      "under-relaxation factor for quality iteration 0..1 (default: 0.5)");
    UNIOF_ADDOPT(argList, "alphaGasMin",    "-",   "minimum gas volume fraction to avoid numerical instabilities (default: 1e-3)");
    UNIOF_ADDOPT(argList, "expansionFactor", "-",  "polytropic expansion factor kappa >= 1: kappa=1 is isochoric, kappa>1 enhances cooling from vapour pressure drop (default: 1)");
    UNIOF_ADDOPT(argList, "mode",    "word", "initialization mode: 'flash' (default, compute flash quality) or 'saturated' (T=Tsat+Tmargin, alpha.gas=0)");
    UNIOF_ADDOPT(argList, "Tmargin", "K",   "temperature margin above T_sat in saturated mode (default: 5 K)");

#   include "setRootCase.H"
#   include "createTime.H"       // runTime → target two-phase case
#   include "createMesh.H"       // mesh    → target two-phase case

    // ── command-line parsing ──────────────────────────────────────────────────

    const fileName precursorCase(UNIOF_ADDARG(args, 0));
    const fileName satCurveFile (UNIOF_ADDARG(args, 1));

    // mode must be known before checking which options are required
    word mode("flash");
    if (UNIOF_OPTIONFOUND(args, "mode"))
        mode = UNIOF_OPTION(args, "mode");

    if (mode != "flash" && mode != "saturated")
        FatalErrorIn("flashInit")
            << "Unknown mode '" << mode
            << "'. Valid modes are: flash, saturated."
            << exit(FatalError);

    if (mode == "flash" && !UNIOF_OPTIONFOUND(args, "T"))
        FatalErrorIn("flashInit")
            << "Required option -T <temperature [K]> not provided."
            << exit(FatalError);
    if (!UNIOF_OPTIONFOUND(args, "pRef"))
        FatalErrorIn("flashInit")
            << "Required option -pRef <pressure [Pa]> not provided."
            << exit(FatalError);

    const scalar T_in = UNIOF_OPTIONFOUND(args, "T")
        ? readScalar(IStringStream(UNIOF_OPTION(args, "T"))())
        : 0.0;  // unused in saturated mode
    const scalar pRef =
        readScalar(IStringStream(UNIOF_OPTION(args, "pRef"))());

    if (UNIOF_OPTIONFOUND(args, "pClip"))
        pClip = pTraits<scalar>(IStringStream(UNIOF_OPTION(args, "pClip"))());

    word phaseLiquid("liquid");
    if (UNIOF_OPTIONFOUND(args, "phaseLiquid"))
        phaseLiquid = UNIOF_OPTION(args, "phaseLiquid");

    word phaseGas("gas");
    if (UNIOF_OPTIONFOUND(args, "phaseGas"))
        phaseGas = UNIOF_OPTION(args, "phaseGas");

    scalar cpl = 4182.0;
    if (UNIOF_OPTIONFOUND(args, "cpl"))
        cpl = readScalar(IStringStream(UNIOF_OPTION(args, "cpl"))());

    bool hfgFromCmdline = UNIOF_OPTIONFOUND(args, "hfg");
    scalar hfg_fixed = 0.0;
    if (hfgFromCmdline)
        hfg_fixed = readScalar(IStringStream(UNIOF_OPTION(args, "hfg"))());

    scalar rhoLiquid = 1000.0;
    if (UNIOF_OPTIONFOUND(args, "rhoLiquid"))
        rhoLiquid = readScalar(IStringStream(UNIOF_OPTION(args, "rhoLiquid"))());

    scalar rhoGas = 1.0;
    if (UNIOF_OPTIONFOUND(args, "rhoGas"))
        rhoGas = readScalar(IStringStream(UNIOF_OPTION(args, "rhoGas"))());

    label maxIter = 50;
    if (UNIOF_OPTIONFOUND(args, "maxIter"))
        maxIter = readLabel(IStringStream(UNIOF_OPTION(args, "maxIter"))());

    scalar tol = 1e-6;
    if (UNIOF_OPTIONFOUND(args, "tol"))
        tol = readScalar(IStringStream(UNIOF_OPTION(args, "tol"))());

    scalar relaxation = 0.5;
    if (UNIOF_OPTIONFOUND(args, "relaxation"))
        relaxation = readScalar(IStringStream(UNIOF_OPTION(args, "relaxation"))());

    if (relaxation <= 0.0 || relaxation > 1.0)
        FatalErrorIn("flashInit")
            << "relaxation must be in (0, 1], got " << relaxation
            << exit(FatalError);

    scalar alphaGasMin = 1e-3;
    if (UNIOF_OPTIONFOUND(args, "alphaGasMin"))
        alphaGasMin = readScalar(IStringStream(UNIOF_OPTION(args, "alphaGasMin"))());

    scalar expansionFactor = 1.0;
    if (UNIOF_OPTIONFOUND(args, "expansionFactor"))
        expansionFactor = readScalar(IStringStream(UNIOF_OPTION(args, "expansionFactor"))());

    if (expansionFactor < 1.0)
        FatalErrorIn("flashInit")
            << "expansionFactor must be >= 1 (1 = isochoric), got "
            << expansionFactor << exit(FatalError);

    scalar Tmargin = 5.0;
    if (UNIOF_OPTIONFOUND(args, "Tmargin"))
        Tmargin = readScalar(IStringStream(UNIOF_OPTION(args, "Tmargin"))());

    Info<< "Settings:" << nl
        << "  precursor case : " << precursorCase << nl
        << "  saturation CSV : " << satCurveFile  << nl
        << "  T              : " << T_in           << " K"   << nl
        << "  pRef           : " << pRef            << " Pa"  << nl
        << "  pClip          : " << pClip            << " Pa"  << nl
        << "  liquid phase   : " << phaseLiquid               << nl
        << "  gas phase      : " << phaseGas                  << nl
        << "  cpl            : " << cpl             << " J/(kg K)" << nl
        << "  rhoLiquid      : " << rhoLiquid        << " kg/m3"   << nl
        << "  rhoGas         : " << rhoGas           << " kg/m3"   << nl
        << "  maxIter        : " << maxIter                          << nl
        << "  tol            : " << tol                              << nl
        << "  relaxation     : " << relaxation                       << nl
        << "  alphaGasMin    : " << alphaGasMin                      << nl
        << "  expansionFactor: " << expansionFactor                  << nl
        << "  mode           : " << mode                             << nl;
    if (mode == "saturated")
        Info<< "  Tmargin        : " << Tmargin << " K" << nl;
    if (hfgFromCmdline)
        Info<< "  hfg (fixed)    : " << hfg_fixed << " J/kg" << nl;
    Info<< endl;

    // ── saturation curve ─────────────────────────────────────────────────────

    SaturationCurve satCurve(satCurveFile);

    if (!hfgFromCmdline && !satCurve.hasHfg())
        FatalErrorIn("flashInit")
            << "Latent heat h_fg must be supplied either as the third column "
            << "in the saturation CSV file or via the -hfg option."
            << exit(FatalError);

    // ── open precursor case ───────────────────────────────────────────────────

    fileName precRoot = precursorCase.path();
    fileName precDir  = precursorCase.name();
    // handle the case where the path has no directory component
    if (precRoot.empty() || precRoot == ".")
    {
        precRoot = fileName(".");
        precDir  = precursorCase;
    }

    Info<< "Opening precursor case: " << precursorCase << nl << endl;

    Time precTime
    (
        Time::controlDictName,
        precRoot,
        precDir
    );
    precTime.setTime(precTime.times().last(), precTime.times().size()-1);

    fvMesh precMesh
    (
        IOobject
        (
            fvMesh::defaultRegion,
            precTime.timeName(),
            precTime,
            IOobject::MUST_READ
        )
    );

    if (precMesh.nCells() != mesh.nCells())
        FatalErrorIn("flashInit")
            << "Precursor mesh has " << precMesh.nCells() << " cells but "
            << "target mesh has " << mesh.nCells() << " cells." << nl
            << "The two meshes must be identical. "
            << "Run OpenFOAM's mapFields first if the meshes differ."
            << exit(FatalError);

    // ── read precursor fields ─────────────────────────────────────────────────

    Info<< "Reading precursor U from " << precTime.timeName() << nl << endl;
    volVectorField Uprec
    (
        IOobject
        (
            "U",
            precTime.timeName(),
            precMesh,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        ),
        precMesh
    );

    Info<< "Reading precursor p from " << precTime.timeName() << nl << endl;
    volScalarField pPrec
    (
        IOobject
        (
            "p",
            precTime.timeName(),
            precMesh,
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        ),
        precMesh
    );

    // sanity check: p must be kinematic [m²/s²]
    if (pPrec.dimensions() != dimensionSet(0, 2, -2, 0, 0))
        WarningIn("flashInit")
            << "Precursor pressure field p has unexpected dimensions: "
            << pPrec.dimensions() << nl
            << "Expected kinematic pressure [m^2/s^2] from simpleFoam. "
            << "Continuing, but results may be incorrect." << nl << endl;

    // ── gravity field ─────────────────────────────────────────────────────────

    // Read from constant/g if present; default to (0 0 -9.81)
    uniformDimensionedVectorField g
    (
        IOobject
        (
            "g",
            runTime.constant(),
            mesh,
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        dimensionedVector("g", dimAcceleration, vector(0, 0, -9.81))
    );
    Info<< "Gravity: " << g.value() << " m/s^2" << nl << endl;

    // height reference (used for p_rgh = p - rho*(g & (C - hRef)))
    uniformDimensionedScalarField hRef
    (
        IOobject
        (
            "hRef",
            runTime.constant(),
            mesh,
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE
        ),
        dimensionedScalar("hRef", dimLength, scalar(0))
    );

    // gh at cell centres: g · (C - hRef*ghat)
    // hRef is a signed distance along the gravity direction
    const dimensionedVector gHat
    (
        "gHat",
        dimless,
        mag(g.value()) > SMALL ? g.value()/mag(g.value()) : vector(0,0,-1)
    );
    const volScalarField gh
    (
        "gh",
        (g & (mesh.C() - dimensionedVector("hRef", dimLength, hRef.value()*gHat.value())))
    );

    // ── compute output fields cell by cell ────────────────────────────────────

    Info<< "Computing two-phase initial fields..." << nl << endl;

    // Allocate output fields (read existing files for boundary conditions)

    volScalarField alphaLiquid = makeUniformScalar(
        "alpha." + phaseLiquid, mesh, runTime.timeName(),
        dimless, 1.0);

    volScalarField alphaGas = makeUniformScalar(
        "alpha." + phaseGas, mesh, runTime.timeName(),
        dimless, alphaGasMin);

    // velocity: copy structure from precursor, then replace internal values
    volVectorField ULiquid
    (
        IOobject(
            "U." + phaseLiquid,
            runTime.timeName(),
            mesh,
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedVector("zero", dimVelocity, vector::zero)
    );

    volVectorField UGas
    (
        IOobject(
            "U." + phaseGas,
            runTime.timeName(),
            mesh,
            IOobject::READ_IF_PRESENT,
            IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedVector("zero", dimVelocity, vector::zero)
    );

    volScalarField Tliquid = makeUniformScalar(
        "T." + phaseLiquid, mesh, runTime.timeName(),
        dimTemperature, T_in);

    volScalarField Tgas = makeUniformScalar(
        "T." + phaseGas, mesh, runTime.timeName(),
        dimTemperature, T_in);

    volScalarField pAbs = makeUniformScalar(
        "p", mesh, runTime.timeName(),
        dimPressure, pRef);

    volScalarField p_rgh = makeUniformScalar(
        "p_rgh", mesh, runTime.timeName(),
        dimPressure, pRef);

    // statistics
    label nFlashing = 0;
    scalar maxQuality = 0.0;
    scalar sumQuality = 0.0;

    const scalarField& pPrecI  = UNIOF_INTERNALFIELD(pPrec);
    const vectorField& UPrecI  = UNIOF_INTERNALFIELD(Uprec);

    scalarField& alphaLiqI = UNIOF_INTERNALFIELD_NONCONST(alphaLiquid);
    scalarField& alphaGasI  = UNIOF_INTERNALFIELD_NONCONST(alphaGas);
    vectorField& ULiqI      = UNIOF_INTERNALFIELD_NONCONST(ULiquid);
    vectorField& UGasI      = UNIOF_INTERNALFIELD_NONCONST(UGas);
    scalarField& TliqI      = UNIOF_INTERNALFIELD_NONCONST(Tliquid);
    scalarField& TgasI      = UNIOF_INTERNALFIELD_NONCONST(Tgas);
    scalarField& pAbsI      = UNIOF_INTERNALFIELD_NONCONST(pAbs);
    scalarField& p_rghI     = UNIOF_INTERNALFIELD_NONCONST(p_rgh);

    const scalarField& ghI = UNIOF_INTERNALFIELD(gh);

    label totalIters    = 0;   // total iterations summed over all flashing cells
    label nNotConverged = 0;   // cells that hit maxIter without converging
    label maxItersUsed  = 0;   // highest iteration count seen in a single cell
    scalar sumQualityChange = 0.0;  // sum of |x_polytropic - x_isochoric|
    scalar maxQualityChange = 0.0;  // max  |x_polytropic - x_isochoric|

    const label nCells = mesh.nCells();

    // ── thread-parallel cell loop ─────────────────────────────────────────────

    struct ThreadStats
    {
        label  totalIters     = 0;
        label  nNotConverged  = 0;
        label  maxItersUsed   = 0;
        label  nFlashing      = 0;
        scalar sumQuality     = 0.0;
        scalar maxQuality     = 0.0;
        scalar sumQualityChange = 0.0;
        scalar maxQualityChange = 0.0;
    };

    const unsigned nThreads =
        max(1u, std::thread::hardware_concurrency());

    Info<< "Parallelising cell loop over " << nThreads
        << " thread(s)" << nl << endl;

    std::vector<ThreadStats>  tStats(nThreads);
    std::vector<std::thread>  threads;
    threads.reserve(nThreads);

    // chunk size: ceiling division so all cells are covered
    const label chunk = (nCells + label(nThreads) - 1) / label(nThreads);

    const bool modeFlash = (mode == "flash");

    auto cellWork = [&](label start, label end, ThreadStats& st)
    {
        for (label cellI = start; cellI < end; ++cellI)
        {
            // ── absolute local pressure (common to both modes) ────────────────
            const scalar p_local = max(pClip, pRef + rhoLiquid * pPrecI[cellI]);
            pAbsI[cellI] = p_local;

            // ── saturation temperature (common to both modes) ─────────────────
            const scalar Tsat_loc = satCurve.Tsat(p_local);

            // ── velocity: copy from precursor (common to both modes) ──────────
            ULiqI[cellI] = UPrecI[cellI];
            UGasI[cellI] = UPrecI[cellI];

            if (modeFlash)
            {
                // ── flash mode: polytropic iterative quality correction ────────
                //
                //  (1-x)*cpl*(T_in - T_liq) = x*h_fg*kappa   ... (energy)
                //  x = cpl*(T_liq - T_sat) / h_fg             ... (superheat)
                //
                //  Closed-form: x = cpl*DT/(kappa*h_fg + cpl*DT), DT=T_in-T_sat
                //  kappa=1 → isochoric;  kappa→∞ → x→0.

                const scalar hfg_loc =
                    hfgFromCmdline ? hfg_fixed : satCurve.hfg(p_local);

                scalar x        = 0.0;
                label  cellIters = 0;

                if (T_in > Tsat_loc && hfg_loc > SMALL)
                {
                    bool converged = false;

                    for (label iter = 0; iter < maxIter; ++iter)
                    {
                        ++cellIters;

                        const scalar liqFrac = max(1.0 - x, SMALL);
                        const scalar T_liq =
                            T_in - x * hfg_loc * expansionFactor / (liqFrac * cpl);

                        const scalar x_new = max(
                            scalar(0),
                            min(scalar(1), cpl * (T_liq - Tsat_loc) / hfg_loc)
                        );

                        const scalar x_upd = (1.0 - relaxation)*x + relaxation*x_new;

                        if (mag(x_upd - x) < tol)
                        {
                            x = x_upd;
                            converged = true;
                            break;
                        }
                        x = x_upd;
                    }

                    st.totalIters   += cellIters;
                    st.maxItersUsed  = max(st.maxItersUsed, cellIters);
                    if (!converged) ++st.nNotConverged;
                }

                // liquid temperature at converged quality (residual superheat)
                const scalar liqFrac = max(1.0 - x, SMALL);
                const scalar T_liq   = (x > 0.0)
                    ? T_in - x * hfg_loc * expansionFactor / (liqFrac * cpl)
                    : T_in;

                // void fraction from quality: alpha_gas = x*rhoL/(x*rhoL+(1-x)*rhoG)
                scalar alphaG = 0.0;
                if (x > 0.0)
                {
                    const scalar num = x * rhoLiquid;
                    const scalar den = num + (1.0 - x) * rhoGas;
                    alphaG = (den > SMALL) ? num / den : 1.0;
                }

                alphaGasI[cellI] = max(alphaG, alphaGasMin);
                alphaLiqI[cellI] = 1.0 - alphaGasI[cellI];

                // liquid: residual-superheat temperature; gas: saturation temperature
                TliqI[cellI] = T_liq;
                TgasI[cellI] = (x > 0.0) ? Tsat_loc : T_in;

                const scalar rhoMix =
                    alphaLiqI[cellI]*rhoLiquid + alphaGasI[cellI]*rhoGas;
                p_rghI[cellI] = p_local - rhoMix * ghI[cellI];

                // statistics
                if (x > 0.0)
                {
                    ++st.nFlashing;
                    st.sumQuality += x;
                    st.maxQuality  = max(st.maxQuality, x);

                    // quality change vs. isochoric baseline (kappa=1):
                    //   x_iso = cpl*DT / (h_fg + cpl*DT)
                    const scalar DT    = T_in - Tsat_loc;
                    const scalar denom = hfg_loc + cpl*DT;
                    const scalar x_iso = (denom > SMALL) ? cpl*DT/denom : 0.0;
                    const scalar dq    = mag(x - x_iso);
                    st.sumQualityChange += dq;
                    st.maxQualityChange  = max(st.maxQualityChange, dq);
                }
            }
            else // saturated mode
            {
                // ── saturated mode: fully liquid at Tsat + margin ─────────────
                //  alpha.gas is held at the numerical floor (alphaGasMin).
                //  T.liquid = T_sat(p_local) + Tmargin  (superheated by margin)
                //  T.gas    = T_sat(p_local)
                //  The solver evolves the state from this near-saturation IC.

                alphaGasI[cellI] = alphaGasMin;
                alphaLiqI[cellI] = 1.0 - alphaGasMin;

                TliqI[cellI] = Tsat_loc + Tmargin;
                TgasI[cellI] = Tsat_loc;

                const scalar rhoMix =
                    alphaLiqI[cellI]*rhoLiquid + alphaGasI[cellI]*rhoGas;
                p_rghI[cellI] = p_local - rhoMix * ghI[cellI];
            }
        }
    };

    for (unsigned t = 0; t < nThreads; ++t)
    {
        const label start = label(t) * chunk;
        const label end   = min(start + chunk, nCells);
        if (start >= nCells) break;
        threads.emplace_back(cellWork, start, end, std::ref(tStats[t]));
    }
    for (auto& th : threads) th.join();

    // ── reduce per-thread statistics ──────────────────────────────────────────
    for (const ThreadStats& st : tStats)
    {
        totalIters      += st.totalIters;
        nNotConverged   += st.nNotConverged;
        maxItersUsed     = max(maxItersUsed, st.maxItersUsed);
        nFlashing       += st.nFlashing;
        sumQuality      += st.sumQuality;
        maxQuality       = max(maxQuality, st.maxQuality);
        sumQualityChange += st.sumQualityChange;
        maxQualityChange = max(maxQualityChange, st.maxQualityChange);
    }

    // ── boundary conditions (copy precursor BCs for U) ────────────────────────

    forAll(mesh.boundary(), patchI)
    {
        const fvPatchVectorField& Ubf = Uprec.boundaryField()[patchI];
        auto & ULp = UNIOF_BOUNDARY_NONCONST(ULiquid)[patchI];
        if (typeid(ULp) == typeid(Ubf))
        {
            ULp        = Ubf;
        }
        else
        {
            ULp        == Ubf;
        }
        auto UGp = UNIOF_BOUNDARY_NONCONST(UGas)[patchI];
        if (typeid(UGp) == typeid(Ubf))
        {
            UGp        = Ubf;
        }
        else
        {
            UGp        == Ubf;
        }
    }

    // ── report statistics ─────────────────────────────────────────────────────

    Info<< "Flash boiling statistics:" << nl
        << "  Cells with flashing  : " << nFlashing
        << " / " << nCells
        << " (" << 100.0*nFlashing/max(1,nCells) << " %)" << nl
        << "  Max flash quality    : " << maxQuality << nl
        << "  Mean flash quality   : "
        << (nFlashing > 0 ? sumQuality/nFlashing : 0.0) << nl
        << "  min/max alpha." << phaseLiquid << ": "
        << min(alphaLiquid).value() << " / " << max(alphaLiquid).value() << nl
        << "  min/max alpha." << phaseGas << "  : "
        << min(alphaGas).value()  << " / " << max(alphaGas).value()  << nl
        << "  min/max p_abs        : "
        << min(pAbs).value()      << " / " << max(pAbs).value() << " Pa" << nl
        << endl;

    if (modeFlash)
    {
        Info<< "Iterative correction statistics (flashing cells: "
            << nFlashing << "):" << nl
            << "  Non-converged cells  : " << nNotConverged
            << " / " << nFlashing
            << (nNotConverged > 0 ? " *** increase maxIter or relaxation ***" : "") << nl
            << "  Max iterations used  : " << maxItersUsed
            << " / " << maxIter << nl
            << "  Mean iterations used : "
            << (nFlashing > 0 ? scalar(totalIters)/scalar(nFlashing) : 0.0) << nl
            << "  Max quality change vs. isochoric  : " << maxQualityChange << nl
            << "  Mean quality change vs. isochoric : "
            << (nFlashing > 0 ? sumQualityChange/nFlashing : 0.0) << nl
            << endl;
    }

    // ── write ─────────────────────────────────────────────────────────────────

    Info<< "Writing fields to " << runTime.timeName() << nl << endl;

    alphaLiquid.write();
    alphaGas.write();
    ULiquid.write();
    UGas.write();
    Tliquid.write();
    Tgas.write();
    pAbs.write();
    p_rgh.write();

    Info<< "flashInit: done." << nl << endl;
    return 0;
}

// ************************************************************************* //
