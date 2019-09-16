{
    fvScalarMatrix eEqn
    (
        fvm::ddt(rho, e)
      + fvm::div(phi, e)
      - fvm::laplacian(turbulence->alphaEff(), e)
     ==
      - p*fvc::div(phi/fvc::interpolate(rho))
//       - fvm::SuSp
//         (
//             p*fvc::div(phi/fvc::interpolate(rho))/e,
//             e
//         )
        // viscous heating?
    );

#if OF_VERSION>=010604
    eEqn.relax
    (
        mesh.solutionDict().equationRelaxationFactor(e.select(pimple.finalIter()))
    );
#else
    if (oCorr == nOuterCorr - 1)
    {
        if (mesh.solutionDict().relax("eFinal"))
        {
            eEqn.relax(mesh.solutionDict().relaxationFactor("eFinal"));
        }
        else
        {
            eEqn.relax(1);
        }
    }
    else
    {
        eEqn.relax();
    }
#endif

    eEqn.solve();

    thermo.correct();

    // Recalculate compressibility
    psis = thermo.psi()/thermo.Cp()*thermo.Cv();

    // Recalculate density
    rho = thermo.rho();
    rho.correctBoundaryConditions();
}
