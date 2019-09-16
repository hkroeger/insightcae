    fvVectorMatrix UEqn
    (
        fvm::ddt(rho, U)
      + fvm::div(phi, U)
      + turbulence->divDevRhoReff(
#if OF_VERSION<010604
                U
#endif
                )
    );
    mrfZones.addCoriolis(rho, UEqn);

#if OF_VERSION>=010604
    UEqn.relax
    (
        mesh.solutionDict().equationRelaxationFactor(U.select(pimple.finalIter()))
    );
#else
    if (oCorr == nOuterCorr - 1)
    {
        if (mesh.solutionDict().relax("UFinal"))
        {
            UEqn.relax(mesh.solutionDict().relaxationFactor("UFinal"));
        }
        else
        {
            UEqn.relax(1);
        }
    }
    else
    {
        UEqn.relax();
    }
#endif

    solve(UEqn == -fvc::grad(p));
