Installation
============

Requirements
------------

### General Remarks

InsightCAE uses a lot of third-party software for various tasks.
If you do not need all the features, some of them can be disabled and the requirement list becomes shorter.
Below is a list of the required third-party packages.
If you use an up-to-date Linux distribution, it is not a problem to fulfill the general requirements. You can simply install them through the package manager.

Most functions of InsightCAE are currently about flow simulation with OpenFOAM. There are various versions of OpenFOAM around, all with a slightly different API. InsightCAE supports different versions and forks of OpenFOAM which may even be installed side-by-side.

The CAD module is currently compatible only with OpenCASCADE 7.2 and DXFlib 3.7. Ubuntu packages are provided through our repository.

### Requirements

The following list contains the software packages, which are required to build and use InsightCAE.
The list gives the program name and a link to its web site. 
In brackets, the tested version and the Ubuntu package name is given.

General dependencies :

* [CMake](https://cmake.org/) (3.10, cmake, cmake-curses-gui)
* [Armadillo](http://arma.sourceforge.net/) (6.5, libarmadillo6, libarmadillo-dev)
* [GNU Scientific Library](https://www.gnu.org/software/gsl/) (2.1, libgsl-dev, gsl-bin)
* [Boost](http://www.boost.org/) (1.58.0, libboost-all-dev)
* [Python](https://www.python.org/) (2.7, python2.7, libpython2.7, libpython2.7-dev)
* [Gnuplot](http://gnuplot.info) (4.6, gnuplot)
* [LaTeX](http://www.latex-project.org/) (2e, texlive-base, texlive-latex-extra)
* [SWIG](http://www.swig.org/) (3.0.8, swig)
* [ParaView](http://www.paraview.org/) (4.4, paraview-insightcae, available through http://downloads.silentdynamics.de)
* [Qt](https://www.qt.io/) (5.11, libqt5widgets5)
* [Qwt](http://qwt.sourceforge.net/) (6.1, libqwt-dev, libqwt6abi1)
* [Numpy](http://www.numpy.org/) (1.11, python-numpy)
* [Scipy](https://scipy.org/) (0.17, python-scipy)
* [Matplotlib](https://matplotlib.org/) (1.5, python-matplotlib)
* [ImageMagick](http://www.imagemagick.org/) (6.8.9.9, imagemagick)

Different versions of OpenFOAM are supported:

* [OpenFOAM 2.3](http://openfoam.org/) (2.3.x, openfoam23x-insightcae, available through http://downloads.silentdynamics.de)

and/or

* [OpenFOAM 5.x](http://cfd.direct/) (5.x or current "-dev")

and/or

* [OpenFOAM v1612](http://openfoam.com/) (v1612, openfoamplus-insightcae, available through http://downloads.silentdynamics.de)

and/or

* [FOAM-extend 4.0](http://wikki.co.uk/) (4.0, foamextend40-insightcae, available through http://downloads.silentdynamics.de)

For InsightCAD:

* [OpenCASCADE](https://www.opencascade.com/) (7.2.0, liboce-insightcae, available through http://downloads.silentdynamics.de)
* [FreeCAD](https://www.freecadweb.org/), (0.17, freecad-insightcae, available through http://downloads.silentdynamics.de)
* [DXFlib](http://www.ribbonsoft.com/de/what-is-dxflib) (3.7, libdxflib, available through http://downloads.silentdynamics.de)

### Setting Up the Environment

Some environment variables and paths are required. A script is provided for setting these. Either add a "source" statement to your ~/.bashrc (or to /etc/profile, if you want a system-wide installation).

    source /path/to/insight/thirdparty/bashrc
    source /path/to/insight/bin/insight_setenv.sh

The first line is only required, if you need to compile dependencies yourself. This is already required during build, so activate the modifications by opening a new terminal.
The second script ("insight_setenv.sh") is created during build. So as long as the build process is not completed, you will see an error message when opening the terminal. You may ignore it for now.

### Building Dependencies

If your distribution or system does not provide packages for the required version, you can build it on your own. 
There are some build scripts which can serve as a starting point. Note that it might become necessary to change the settings on your system.
You can build the dependency packages by going through the following steps:

Change into the build directory

    $ cd insight/thirdparty_src

Download and unpack the source tarballs of the packages you need.
You can get the right version from the respective website.
Some source archives have also been copied to this location:

    http://sourceforge.net/projects/insightcae/files/dependencies/TARBALLS

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

### Installation using Binary Packages

Binary packages for the latest LTS version of Ubuntu linux are provided.
Currently, the release "xenial" (16.04 LTS) is supported.
To install the packages, add the silentdynamics apt repository and the associated key by executing (as root):

    $ sudo add-apt-repository http://downloads.silentdynamics.de/ubuntu
    $ sudo apt-key adv --recv-key --keyserver keys.gnupg.net 79F5CBA4
    $ sudo apt-get update

Then install the software by executing:

    $ sudo apt-get install insightcae-base

### Building from Sources

The sources of InsightCAE are hosted in a git repository at Github. For configuring and executing the build process, the software CMake is used.

At first, clone the sources from the git repository by executing:

    $ git clone https://github.com/hkroeger/insightcae.git insight-src
    
During configuration, CMake will search for add-ons in the folder "src/addons" in the InsightCAE source tree. If you want to include any add-on, place its code in this directory now. E.g. if you want to include the add-on "inflow-generator", move (or clone) the directory "inflow-generator" which contains the add-on's code, into the folder "src/addons".

CMake is utilized for building the software out of source in a separate build directory. 
Create a build directory, then configure the build using e.g. ccmake and finally build using make:

    $ mkdir insight && cd insight
    $ ccmake ../insight-src  # path to source
    
See the next section for explanations on the build options. After configuring, build the project by executing "make":

    $ make

If you have not done yet, add the configuration script to your ~/.bashrc script:

    source /path/to/insight/bin/insight_setenv.sh
    
### CMake options

After starting ccmake, the option list is empty. Type "c" for executing the configuration procedure. Now cmake tries to locate the required software. This is a step-by-step procedure and needs to be repeated several times until all packages are detected. Every time the configure step is executed, more options appear.

The most important switches are:

* INSIGHT_BUILD_ADDONS : switches the building of present add-ons on/off
* INSIGHT_BUILD_CAD : switch building of CAD module on/off
* INSIGHT_BUILD_ISCAD : switches the building of InsightCAE-CAD front end on/off
* INSIGHT_BUILD_OPENFOAM : possibility to disable OpenFOAM extensions
* INSIGHT_BUILD_PYTHONBINDINGS : if on, SWIG is required to create python interface
* INSIGHT_BUILD_TOOLKIT : whether to build the core library. Required for most functions
* INSIGHT_BUILD_WORKBENCH : switches building of the workflow GUI on/off. Qt-4 is required, if set to on.

The different supported OpenFOAM versions are also detected. The respective configuration variable contains the location of the bashrc configuration script of each OpenFOAM installation. If your installation is not detected or the wrong one is found, you may edit the content of the individual variables. Their names are:

* OF16ext_BASHRC : path of the OpenFOAM-1.6-ext bashrc
* OF21x_BASHRC : path of the OpenFOAM-2.1.x bashrc
* OF22eng_BASHRC : path of the OpenFOAM-2.2-engysEdition bashrc
* OF22x_BASHRC : path of the OpenFOAM-2.2.x bashrc
* OF23x_BASHRC : path ofthe OpenFOAM-2.3.x bashrc
* OF24x_BASHRC : path of the OpenFOAM-2.4.x bashrc
* OF301_BASHRC : path of the OpenFOAM-3.0.1 bashrc
* OFdev_BASHRC : path of the OpenFOAM-dev bashrc (corresponds to OpenFOAM-5.x)
* OFplus_BASHRC : path of the OpenFOAM-plus bashrc (corresponds to OpenFOAM-4.1)

The other variables allow to correct the detected paths to the several dependency packages. You may edit them as needed.

Once all variables are set the configuration steps does not yield any errors, execute the "generate" step by typing "g". Once this is done, you may run make as described above.
