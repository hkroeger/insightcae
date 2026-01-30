#include "zonebalance.h"

#include "addToRunTimeSelectionTable.H"
#include "cellSet.H"
#include "fieldToCell.H"
#include "fvc.H"
#include "uniof_tools.h"

#include <memory>

namespace Foam {


defineTypeNameAndDebug(zoneBalance, 0);

addToRunTimeSelectionTable(
        functionObject,
        zoneBalance,
        dictionary
);



zoneBalance::zoneBalance(
    const word& name,
    const objectRegistry& obr,
    const dictionary& dict )

  : UniFunctionObject(name, dict),
    mesh_(UNIOF_OBR_TO_MESH(obr)),
    outputFile_(name, mesh_.time())
{}


bool zoneBalance::read(const dictionary &dict)
{
    word selType(dict.lookupOrDefault<word>("cellSelection", "all"));
    if (selType=="all")
    {
        cellSelection_=boost::blank();
    }
    else if (selType=="cellSet")
    {
        cellSelection_=cellSetSelection{word(dict.lookup("cellSet"))};
    }
    else if (selType=="threshold")
    {
        cellSelection_=ThresholdSelection{
            word(dict.lookup("thresholdScalarFieldName")),
            dict.lookupOrDefault<scalar>("lowerThreshold", -GREAT),
            dict.lookupOrDefault<scalar>("upperThreshold", GREAT)
        };
    }
    else if (selType=="highestValueVolumeFraction")
    {
        cellSelection_=HighestValueVolumeFraction{
            word(dict.lookup("thresholdScalarFieldName")),
            readScalar(dict.lookup("volumeFraction"))
        };
    }
    factorFields_=wordList(dict.lookup("factorFields"));

    return true;
}

bool zoneBalance::perform()
{

    auto&rho=mesh_.lookupObject<volScalarField>("rho");
    auto&U=mesh_.lookupObject<volVectorField>("U");

    surfaceScalarField phi(
        // mesh_.lookupObject<surfaceScalarField>("phi");
        linearInterpolate(rho*U)&mesh_.Sf() );

    for (auto& f: factorFields_)
    {
        phi = phi *
              fvc::interpolate(
                  mesh_.lookupObject<volScalarField>(f) );
    }

    auto balance = fvc::surfaceSum(phi);

    // scalarList values;

    label nc;
    scalar V;

    std::shared_ptr<cellSet> cells;

    if (boost::get<boost::blank>(&cellSelection_))
    {
        // values=balance->internalField();
        cells=std::make_shared<cellSet>(
            IOobject(
                "balanceSelection",
                mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE)
            );
        for(label i=0; i<mesh_.nCells(); ++i) cells->insert(i);
        // nc=balance->internalField().size();
        // V=gSum(mesh_.V());
    }
    else if (auto* hv=boost::get<HighestValueVolumeFraction>(&cellSelection_))
    {
        cells=std::make_shared<cellSet>(
            IOobject(
                "balanceSelection",
                mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE)
            );

        auto& rDeltaT=
            mesh_.lookupObject<volScalarField>(hv->thresholdScalarFieldName);

        scalar timeUnresolvedVolPercent = hv->volumeFraction;

        labelList offsets(Pstream::nProcs(), 0);
        offsets[Pstream::myProcNo()]=mesh_.nCells();
        Pstream::listCombineGather(offsets, maxEqOp<label>());
        Pstream::listCombineScatter(offsets);

        label ofs=0;
        forAll(offsets, i)
        {
            label nfs=offsets[i];
            offsets[i]=ofs;
            ofs+=nfs;
        }

        label globNCells=mesh_.nCells();
        reduce(globNCells, sumOp<label>());

        label o=offsets[Pstream::myProcNo()];
        scalarField vol(globNCells, GREAT);
        scalarField rDT(globNCells, GREAT);
        labelList order(globNCells, 0);
        forAll(mesh_.cellVolumes(), ci)
        {
            vol[o+ci]=mesh_.cellVolumes()[ci];
            rDT[o+ci]=rDeltaT[ci];
        }
        Pstream::listCombineGather(vol, minEqOp<scalar>());
        Pstream::listCombineScatter(vol);
        Pstream::listCombineGather(rDT, minEqOp<scalar>());
        Pstream::listCombineScatter(rDT);

        scalar Vthreshold=timeUnresolvedVolPercent*sum(vol);

        sortedOrder(rDT, order/*, typename UList<scalar>::greater(rDT)*/);
        Foam::reverse(order);

        scalar V=0.;
        forAll(order, oi)
        {
            label gfI=order[oi];
            V+=vol[gfI];
            if (V>Vthreshold)
            {
                break;
            }
            else
            {
                label lci=gfI-o; // local cell id
                if (lci>=0 && lci<mesh_.nCells()) // if on this processor
                {
                    cells->insert(lci);
                }
            }
        }
    }
    else if (auto* set=boost::get<cellSetSelection>(&cellSelection_))
    {
        cells=std::make_shared<cellSet>(mesh_, set->cellSetName, cellSet::MUST_READ);
    }
    else if (auto* thr=boost::get<ThresholdSelection>(&cellSelection_))
    {
        // auto& thrFld=mesh_.lookupObject<volScalarField>(thr->thresholdScalarFieldName);
        // thrFld.
        cells=std::make_shared<cellSet>(
            IOobject(
                "balanceSelection",
                mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE)
            );
        fieldToCell fldSel(
            mesh_,
            thr->thresholdScalarFieldName,
            thr->lowerThreshold, thr->upperThreshold );
        fldSel.applyToSet(topoSetSource::ADD, *cells);
    }

    if (cells)
    {
        auto cellIds=cells->toc();

        // values=List(UIndirectList<scalar>(balance, cellIds));

        nc=cells->size();
        V=gSum(List(UIndirectList<scalar>(mesh_.V(), cellIds)));
    }

    reduce(nc, sumOp<label>());

    labelList bndfaces;
    scalarField norm_dir;
    findBndFaces(mesh_, *cells, bndfaces, norm_dir);

    scalarField phif (pick_gf(phi, bndfaces, &norm_dir) );

    balanceSum_=-gSum(phif); // we want inflow positive

    // Info<<"compare: "<<balanceSum_<<" <<>> "<< gSum(values)<<endl;

    Info<<name()<<": balance of phi*"<<factorFields_<<" over "<<nc<< " cells (V="<<V<<") = "<<balanceSum_<<endl;

    return true;
}

bool zoneBalance::write()
{
    outputFile_()
        << mesh_.time().value()
        << tab
        << balanceSum_
        << endl;
    return true;
}

}
