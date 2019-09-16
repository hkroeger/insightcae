{
    U = UEqn.H()/UEqn.A();

    mrfZones.correctBoundaryVelocity(U);
    
#if OF_VERSION>=010604
    while (pimple.correctNonOrthogonal())
#else
    for (int nonOrth = 0; nonOrth <= nNonOrthCorr; nonOrth++)
#endif
    {
        // Calculate phi for boundary conditions
        phi = rhof*
        (
            (fvc::interpolate(U) & mesh.Sf())
        );        
        mrfZones.relativeFlux(fvc::interpolate(rho), phi);

        surfaceScalarField phid2 = rhoReff/rhof*phi;

        surfaceScalarField phid("phid", psisf/rhof*phi);

        // Store pressure for under-relaxation
        p.storePrevIter();

        volScalarField divPhid
        (
            "divPhid",
            fvc::div(phid)
        );

        fvScalarMatrix pEqn
        (
            fvm::ddt(psis, p)
          + fvm::div(phid, p)
            // Convective flux relaxation terms
          + fvc::div(phid2)
          - fvm::laplacian(rho*rUA, p)
        );

        pEqn.solve();

        // Calculate the flux
#if OF_VERSION>=010604
        if (pimple.finalNonOrthogonalIter())
#else
        if (nonOrth == nNonOrthCorr)
#endif
        {
            phi = phid2 + pEqn.flux();
        }

        // Relax the pressure
        p.relax();
    }

#   include "compressibleContinuityErrs.H"

    U -= fvc::grad(p)/UEqn.A();
    U.correctBoundaryConditions();
}
