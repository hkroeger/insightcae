#!/bin/bash

./build_python.sh 2>&1 | tee build_python.log
./build_boost.sh 2>&1 | tee build_boost.log
./build_cmake.sh 2>&1 | tee build_cmake.log
./build_armadillo.sh 2>&1 | tee build_armadillo.log
./build_gsl.sh 2>&1 | tee build_gsl.log
./build_qt.sh 2>&1 | tee build_qt.log
./build_qwt.sh 2>&1 | tee build_qwt.log
./build_gnuplot.sh 2>&1 | tee build_gnuplot.log
./build_numpy.sh 2>&1 | tee build_numpy.log

./build_oce.sh 2>&1 | tee build_oce.log
./build_geom.sh 2>&1 | tee build_geom.log
./build_dxflib.sh 2>&1 | tee build_dxflib.log

./build_paraview.sh 2>&1 | tee build_paraview.log
./build_vtk.sh 2>&1 | tee build_vtk.log

echo "Completed! Please check log files for errors:"
tail -n 10 *.log
