Installation
============

Requirements
------------

The following list contains the software packages, which are required to build and use InsightCAE.
The list gives the program name and a link to its web site. 
In brackets, the tested version and the ubuntu package name is given.

General dependencies :

* [CMake](https://cmake.org/) (3.5, cmake, cmake-curses-gui)
* [Armadillo](http://arma.sourceforge.net/) (6.5, libarmadillo6, libarmadillo-dev)
* [GNU Scientific Library](https://www.gnu.org/software/gsl/) (2.1, libgsl-dev, gsl-bin)
* [Boost](http://www.boost.org/) (1.58.0, libboost-all-dev)
* [SWIG](http://www.swig.org/) (3.0.8, swig)
* [Python](https://www.python.org/) (2.7, python2.7, libpython2.7,libpython2.7-dev)
* [ParaView](http://www.paraview.org/) (4.4, paraview-insightcae, available through http://downloads.silentdynamics.de)
* [Qt](https://www.qt.io/) (4.8, libqt4-dev, libqtcore4, libqtgui4, libqtwebkit4)
* [Qwt](http://qwt.sourceforge.net/) (6.1, libqwt-dev, libqwt6abi1)
* [Gnuplot](http://gnuplot.info) (4.6, gnuplot)
* [Numpy](http://www.numpy.org/) (1.11, python-numpy)
* [Scipy](https://scipy.org/) (0.17, python-scipy)
* [Matplotlib](https://matplotlib.org/) (1.5, python-matplotlib)
* [LaTeX](http://www.latex-project.org/) (2e, texlive-base,texlive-latex-extra)
* [ImageMagick](http://www.imagemagick.org/) (6.8.9.9, imagemagick)

Different versions of OpenFOAM are supported:

* [OpenFOAM 2.3](http://openfoam.org/) (2.3.x, openfoam23x-insightcae, available through http://downloads.silentdynamics.de)

and/or

* [OpenFOAM v1612](http://openfoam.com/) (v1612, openfoamplus-insightcae, available through http://downloads.silentdynamics.de)

and/or

* [FOAM-extend 4.0](http://wikki.co.uk/) (4.0, foamextend40-insightcae, available through http://downloads.silentdynamics.de)

For InsightCAD:

* [OpenCASCADE](https://www.opencascade.com/content/latest-release) (7.2.0, liboce-insightcae, available through http://downloads.silentdynamics.de)
* [FreeCAD](https://www.freecadweb.org/), (0.17, freecad-insightcae, available through http://downloads.silentdynamics.de)
* [DXFlib](http://www.ribbonsoft.com/de/what-is-dxflib) (3.7, libdxflib, available through http://downloads.silentdynamics.de)

For the web workbench (yet experimental)

* [Wt toolkit](https://www.webtoolkit.eu/wt) (3.8, libwt38, libwthttp38)

If your distribution or system does not provide packages for the required version, you can build it on your own. 
There are some build scripts which can serve as a starting point. Note that it might become necessary to change the settings on your system.
You can build the dependency packages by going through the following steps:

First of all, add a "source" statement to your bashrc for settings a proper bin and lib path.
This is already required during build, so insert the follwing statement into your "~/.bashrc"
and activate the changes by continuing in a new terminal:

    source $HOME/<path/to/installation>/insight/thirdparty/bashrc

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

Installation using Binary Packages
----------------------------------

Binary packages for the latest LTS version of Ubuntu linux are provided.
Currently, the release "xenial" (16.04 LTS) is supported.
To install the packages, add the silentdynamics apt repository and the associated key by executing (as root):

    $ sudo add-apt-repository http://downloads.silentdynamics.de/ubuntu
    $ sudo apt-key adv --recv-key --keyserver keys.gnupg.net 79F5CBA4
    $ sudo apt-get update

Then install the software by executing:

    $ sudo apt-get install insightcae-base

Building from Sources
---------------------

The sources of InsightCAE are hosted in a git repository at Github.
Clone the sources from the git repository by executing:

    $ git clone https://github.com/hkroeger/insightcae.git insight-src

CMake is utilized for building the software out of source in a separate build directory. 
Create a build directory, then configure the build using e.g. ccmake and finally build using make:

    $ mkdir insight && cd insight
    $ ccmake ../insight-src
    $ make

After the build has finished, some environment variables need to be set up.
A script is provided therefore. It can be parsed e.g. in your ~/.bashrc script by adding to the end:

    source /path/to/insight/bin/insight_setenv.sh
