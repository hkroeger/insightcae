Syntax
======
In the following, the meaning of lines starting a following characters is:

- "$": command to be executed in a terminal as normal user
- "#": command to be executed in a terminal as the system administrator
- ">": a line in a text file (the content is without the '>')

Installation
============
The first step is to install the CAE tools which you want to interface with Insight CAE:
 * OpenFOAM (see installation instructions at http://openfoam.org/download) 
 * Code_Aster (see installation instructions at http://www.code-aster.org/)
 
Then clone the insight source code:
 $ cd $HOME/<path/to/installation>
 $ git clone https://github.com/hkroeger/insightcae.git insight
   

Then you have currently different choices for the installation of Insight:

A. Build insight against system-installed dependencies 
   This is usually possible but might cause problems due to unsupported versions 
   of dependencies on some linux distributions.
   
   For some distributions, we provide installation packages for missing dependencies.
   Note that on cluster systems it is necessary to install the packages on every
   host where you wish to execute insight or computation jobs that use insight
   extensions. An alternative (option B) is to compile the dependency by yourself and store 
   the binaries in your $HOME (of course, this works only if it is network mounted 
   across the whole cluster).
   
B. Build insight together with dependencies
   This is required, if 1) does not work, like on older distributions
   and e.g. cluster installations. You can also build only a subset of the
   dependencies by yourself and use as much from the system as possible.

   
   
STEPS for Option A
==================

1. Prepare system,
   install basic packages for compilation.
   
   * Fedora-20: 
   
     For a minimum functionality install from repositories:
     
     # yum install gcc-c++ git cmake armadillo-devel boost-devel gsl-devel python-devel gnuplot 
     
     additionally for complete functionality:
     
     # yum install libX11-devel libXext-devel libXmu-devel mesa-libGL-devel mesa-libOSMesa-devel qt-devel \
        qwt-devel swig doxygen graphviz python-matplotlib numpy texlive OCE-devel
        
     and precompiled from the insight download area:
     
     # rpm -ivh http://sourceforge.net/projects/insightcae/files/dependencies/dxflib-3.7.5-fedora20.x86_64.rpm
     
     # rpm -ivh http://sourceforge.net/projects/insightcae/files/dependencies/salomegeom-7.2.0-fedora20.x86_64.rpm
     
   * Fedora-22: 
   
     For a minimum functionality install from repositories:
     
     # yum install gcc-c++ git cmake armadillo-devel boost-devel gsl-devel python-devel gnuplot 
     
     additionally for complete functionality:
     
     # yum install libX11-devel libXext-devel libXmu-devel mesa-libGL-devel mesa-libOSMesa-devel qt-devel \
        qwt-devel swig doxygen graphviz python-matplotlib numpy texlive OCE-devel OCE-draw
        
     and precompiled from the insight download area:
     
     # rpm -ivh http://sourceforge.net/projects/insightcae/files/dependencies/dxflib-3.7.5-fedora22.x86_64.rpm
     
     # rpm -ivh http://sourceforge.net/projects/insightcae/files/dependencies/salomegeom-7.2.0-fedora22.x86_64.rpm
     
   * CentOS 7.0
   
     For a minimum functionality install from repositories:
     
     # yum install gcc-c++ git cmake boost-devel gsl-devel python-devel gnuplot
     and you have to compile yourself according to Option B:
     
      * armadillo

     Note: cmake-2.8.11 (default in this distribution) creates buggy RPMs 
     which will conflict with the package "filesystem". At least cmake-2.8.12 is required.

     additionally for complete functionality:
     
     # yum install libX11-devel libXext-devel libXmu-devel mesa-libGL-devel mesa-libOSMesa-devel qt-devel \
        qwt-devel swig doxygen graphviz python-matplotlib numpy texlive

     and precompiled from the insight download area (alternatively compile yourself according to B):
     
     # rpm -ivh http://sourceforge.net/projects/insightcae/files/dependencies/dxflib-3.7.5-centos7.x86_64.rpm
     
     # rpm -ivh http://sourceforge.net/projects/insightcae/files/dependencies/OCE-0.13.1-centos7.x86_64.rpm
     
     # rpm -ivh http://sourceforge.net/projects/insightcae/files/dependencies/salomegeom-7.2.0-centos7.x86_64.rpm

2. Build Insight
   
   (Optional) If you want to use any addons, navigate to the addon directory 
   in the source tree and clone them:
   
   $ cd insight/src/addons
   
   $ git clone url://.....
   
   $ cd ../../..
   
   Create a build directory
   
   $ cd insight; mkdir build; cd build
   
   Configure
   
   $ ccmake ..
   
   Now you can set options, correct paths, disable certain parts of insight etc.
   
   Then compile, e.g. in parallel on 4 processors (change this according to your hardware):
   
   $ make -j4
   
   (Optional) build the documentation:
   
   $ make doc
   
3. Set up your environment
   
   Add a statement to your "~/.bashrc" file to
   include the bin/ and lib/ dirs to the search
   path:
   
   > source <path/for/installation>/insight/build/bin/setenv.sh
   
   
   
STEPS for Option B
==================

1. Make sure, the basic headers and libraries for program compilation are installed.
   
2. Build dependencies

   First of all, add a "source" statement to your bashrc for settings a proper bin and lib path.
   This is already required during build, so insert the follwing statement into your "~/.bashrc"
   and activate the changes by continuing in a new terminal:
   
   > source $HOME/<path/to/installation>/insight/thirdparty/bashrc
   
   Change into the build directory
   
   $ cd insight/thirdparty_src
   
   Download and unpack dependency tarball pack:
   
   $ wget http://sourceforge.net/projects/insightcae/files/dependencies/insight-dependency-tarballs.tgz
   
   $ tar xzf insight-dependency-tarballs.tgz
   
   Then build the required packages. For each package, there is a script named "build_XXX.sh".
   In the simplest case, it is sufficient to execute it. In case the build configuration needs to
   be changed, the script can serve as a starting point.
   
   The order of building the packages might be important, depending on what you need to build and
   what is already on the system. The following order should work:
   
   $ ./build_boost.sh
   
   $ ./build_cmake.sh
   
   $ ./build_armadillo.sh
   
   $ ./build_gsl.sh
   
   $ ./build_python.sh
   
   $ ./build_qt.sh
   
   $ ./build_qwt.sh
   
   $ ./build_gnuplot.sh
   
   $ ./build_dxflib.sh
   
   $ ./build_oce.sh
   
Continue with step A.2)