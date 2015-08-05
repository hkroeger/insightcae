#!/bin/bash

./build_boost.sh 2>&1 | tee build_boost.log
./build_cmake.sh 2>&1 | tee build_cmake.log
./build_armadillo.sh 2>&1 | tee build_armadillo.log
./build_gsl.sh 2>&1 | tee build_gsl.log
./build_python.sh 2>&1 | tee build_python.log
./build_qt.sh 2>&1 | tee build_qt.log
./build_qwt.sh 2>&1 | tee build_qwt.log
./build_gnuplot.sh 2>&1 | tee build_gnuplot.log

echo "Completed! Please check log files for errors:"
tail -n 10 *.log
