#ifndef PARAVIEW_H
#define PARAVIEW_H

#include <string>
#include <vector>

#include "base/boost_include.h"

#include "base/analysis.h"
#include "base/resultset.h"
#include "openfoam/openfoamcase.h"

namespace insight
{


namespace paraview
{

std::string pvec(const arma::mat& v);


class PVScriptElement
{
public:
  declareDynamicClass(PVScriptElement);

public:

#include "paraview__PVScriptElement__Parameters.h"
/*
PARAMETERSET>>> PVScriptElement Parameters

<<<PARAMETERSET
*/

protected:
  std::vector<std::string> objNames_;
  Parameters p_;

public:
  declareType("PVScriptElement");

  PVScriptElement(const Parameters& p, const std::vector<std::string>& objNames);
  virtual ~PVScriptElement();

  virtual std::string pythonCommands() const =0;
  virtual std::vector<boost::filesystem::path> createdFiles() const;
  virtual std::vector<std::string> createdObjects() const;

};

typedef std::shared_ptr<PVScriptElement> PVScriptElementPtr;




class CustomScriptElement
: public PVScriptElement
{

public:
#include "paraview__CustomScriptElement__Parameters.h"
/*
PARAMETERSET>>> CustomScriptElement Parameters
inherits insight::paraview::PVScriptElement::Parameters

command = string "" "Python snippet to execute in pvBatch"
names = array [ string "" "Name of crated PV script object" ] *0 "Names of created PV script object"

<<<PARAMETERSET
*/
protected:
  Parameters p_;

public:
  declareType("CustomPVScene");

  CustomScriptElement(const ParameterSet&);

  std::string pythonCommands() const override;

  virtual ParameterSet getParameters() const { return p_; }
};




class Cutplane
: public PVScriptElement
{

public:
#include "paraview__Cutplane__Parameters.h"
/*
PARAMETERSET>>> Cutplane Parameters
inherits insight::paraview::PVScriptElement::Parameters

name = string "out" "Name of output data set"

dataset = string "" "name of the data set to cut"
field = string "" "name of the field to display on the plane"

p0 = vector (0 0 0) "center point of the cut plane"
normal = vector (0 0 1) "normal vector of the cut plane"

<<<PARAMETERSET
*/
protected:
  Parameters p_;

public:
  declareType("Cutplane");

  Cutplane(const ParameterSet&);

  virtual std::string pythonCommands() const;

  virtual ParameterSet getParameters() const { return p_; }
};



class Arrows
: public PVScriptElement
{

public:
#include "paraview__Arrows__Parameters.h"
/*
PARAMETERSET>>> Arrows Parameters
inherits insight::paraview::PVScriptElement::Parameters

name = string "out" "Name of output data set"

arrows = array [ set {
 from = vector (0 0 0) "begin point in scene units"
 to = vector (1 0 0) "end point in scene units"
} ] *0 "definition of arrows"

<<<PARAMETERSET
*/
protected:
  Parameters p_;

public:
  declareType("Arrows");

  Arrows(const ParameterSet&);

  virtual std::string pythonCommands() const;

  virtual ParameterSet getParameters() const { return p_; }
};




class Streamtracer
: public PVScriptElement
{

public:
#include "paraview__Streamtracer__Parameters.h"
/*
PARAMETERSET>>> Streamtracer Parameters
inherits insight::paraview::PVScriptElement::Parameters

name = string "out" "Name of output data set"

dataset = string "" "name of the data set to cut" *necessary
field = string "U" "name of the vector field from which the stream tracers are to be computed"

maxLen = double 1e10 "Maximum stream line length"

seed = selectablesubset {{

 cloud set {
  center = vector (0 0 0) "center point for the seed point cloud" *necessary
  radius = double 1 "radius of the seed point cloud" *necessary
  number = int 10 "number of points in the seed cloud"
 }

}} cloud "Type of streamline seeding"

<<<PARAMETERSET
*/
protected:
  Parameters p_;

public:
  declareType("Streamtracer");

  Streamtracer(const ParameterSet&);

  virtual std::string pythonCommands() const;

  virtual ParameterSet getParameters() const { return p_; }
};





class PVScene
{


public:
    declareDynamicClass(PVScene);

#include "paraview__PVScene__Parameters.h"
/*
PARAMETERSET>>> PVScene Parameters

sceneElements = array [
 dynamicclassconfig "paraview::PVScriptElement" default "Cutplane" "Scene definition"
 ] *0 "Scene building bricks"

resetview = bool false "If true, the view is cleared before rendering. Previous scene will be overlayed otherwise"
imagename = string "" "Image name. Will be used as filename. If blank, the view created but not rendered. This can be useful to overlay with the next scene."


<<<PARAMETERSET
*/
protected:
  Parameters p_;

public:
  declareType("PVScene");

  PVScene(const Parameters& p);
  virtual ~PVScene();

  std::vector<std::string> objectNames() const;

  virtual std::string pythonCommands() const;

  std::string sceneObjectsBoundingBoxCmd(const std::vector<std::string>& varnames) const;

  virtual std::vector<boost::filesystem::path> createdFiles() const;
};


typedef std::shared_ptr<PVScene> PVScenePtr;




class SingleView
: public PVScene
{

public:
#include "paraview__SingleView__Parameters.h"
/*
PARAMETERSET>>> SingleView Parameters
inherits insight::paraview::PVScene::Parameters

lookAt = vector (0 0 0) "Look at point"
e_up = vector (0 0 1) "Upward direction"
normal = vector (1 0 0) "direction from camera to lookAt point"

<<<PARAMETERSET
*/
protected:
  Parameters p_;

public:
  declareType("SingleView");

  SingleView(const ParameterSet&);

  virtual std::string pythonCommands() const;
  virtual std::vector<boost::filesystem::path> createdFiles() const;

  virtual ParameterSet getParameters() const { return p_; }
};




class IsoView
: public PVScene
{

public:
#include "paraview__IsoView__Parameters.h"
/*
PARAMETERSET>>> IsoView Parameters
inherits insight::paraview::PVScene::Parameters

bbmin = vector (-1 -1 -1) "minimum point of bounding box"
bbmax = vector (1 1 1) "maximum point of bounding box"

e_up = vector (0 0 1) "Upward direction"
e_ax = vector (1 0 0) "Longitudinal direction"

<<<PARAMETERSET
*/
protected:
  Parameters p_;

public:
  declareType("IsoView");

  IsoView(const ParameterSet&);

  virtual std::string pythonCommands() const;
  virtual std::vector<boost::filesystem::path> createdFiles() const;

  virtual ParameterSet getParameters() const { return p_; }
};




class ParaviewVisualization
: public Analysis
{
public:
#include "paraview__ParaviewVisualization__Parameters.h"
/*
PARAMETERSET>>> ParaviewVisualization Parameters

scenes = array [
 dynamicclassconfig "paraview::PVScene" default "SingleView" "Scene definition"
 ] *0 "Configuration of scenes"

<<<PARAMETERSET
*/

  declareType("ParaviewVisualization");

  ParaviewVisualization(const ParameterSet& ps, const boost::filesystem::path& exepath);

  static std::string category() { return "General Postprocessing"; }

  virtual ResultSetPtr operator()(ProgressDisplayer& displayer=consoleProgressDisplayer);

  static std::string OFCaseDatasetName();
};




}


}


#endif // PARAVIEW_H
