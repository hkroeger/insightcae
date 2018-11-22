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




class PVScene
{
public:
//   declareFactoryTableNoArgs(PVScene);
  declareDynamicClass(PVScene);

public:

#include "paraview__PVScene__Parameters.h"
/*
PARAMETERSET>>> PVScene Parameters

resetview = bool false "If true, the view is cleared before rendering. Previous scene will be overlayed otherwise"
imagename = string "" "Image name. Will be used as filename. If blank, the view created but not rendered. This can be useful to overlay with the next scene."

<<<PARAMETERSET
*/

  declareType("PVscene");

  virtual ~PVScene();

  virtual std::string pythonCommands() const =0;
  virtual std::vector<boost::filesystem::path> createdFiles() const;

  static std::string pvec(const arma::mat& v);
};

typedef std::shared_ptr<PVScene> PVScenePtr;



class CustomPVScene
: public PVScene
{

public:
#include "paraview__CustomPVScene__Parameters.h"
/*
PARAMETERSET>>> CustomPVScene Parameters

inherits insight::paraview::PVScene::Parameters

command = string "" "Python snippet to execute in pvBatch"

<<<PARAMETERSET
*/
protected:
  Parameters p_;

public:
  declareType("CustomPVScene");

  CustomPVScene(const ParameterSet&);

  virtual std::string pythonCommands() const;

  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
  virtual ParameterSet getParameters() const { return p_; }
};




class Cutplane
: public PVScene
{

public:
#include "paraview__Cutplane__Parameters.h"
/*
PARAMETERSET>>> Cutplane Parameters

inherits insight::paraview::PVScene::Parameters

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

  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
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

filename = path "isoview.png" "Output filename. Different views will be stored at <file path>/<filename stem>_<view>.<file extension>."

<<<PARAMETERSET
*/
protected:
  Parameters p_;

public:
  declareType("IsoView");

  IsoView(const ParameterSet&);

  virtual std::string pythonCommands() const;
  virtual std::vector<boost::filesystem::path> createdFiles() const;

  static ParameterSet defaultParameters() { return Parameters::makeDefault(); }
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
 dynamicclassconfig "paraview::PVScene" default "IsoView" "Scene configuration"
 ] *0 "Configuration of scenes"

<<<PARAMETERSET
*/

  declareType("ParaviewVisualization");

  ParaviewVisualization(const ParameterSet& ps, const boost::filesystem::path& exepath);

  static ParameterSet defaultParameters();
  static std::string category() { return "General Postprocessing"; }

  virtual ResultSetPtr operator()(ProgressDisplayer* displayer=NULL);
};




}


}


#endif // PARAVIEW_H
