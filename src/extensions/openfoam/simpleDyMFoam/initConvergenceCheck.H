// initialize values for convergence checks

    scalar eqnResidual = 1, maxResidual = 0;
    scalar convergenceCriterion = 0;

    simple.readIfPresent("convergence", convergenceCriterion);

