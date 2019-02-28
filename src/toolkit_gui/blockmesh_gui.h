#ifndef BLOCKMESH_GUI_H
#define BLOCKMESH_GUI_H

#include "openfoam/blockmesh_templates.h"

#include "cadtypes.h"


namespace insight
{
namespace bmd
{



class blockMeshDict_Box_ParameterSet_Visualizer
 : public ParameterSet_Visualizer
{
public:
    typedef blockMeshDict_Box::Parameters Parameters;
    typedef std::map<std::string, cad::FeaturePtr> ItemList;

protected:
    ItemList items_;

public:
    virtual void update(const ParameterSet& ps);
    virtual void updateVisualizationElements(QoccViewWidget*, QModelTree*);
};




class blockMeshDict_Cylinder_ParameterSet_Visualizer
 : public ParameterSet_Visualizer
{
public:
    typedef blockMeshDict_Cylinder::Parameters Parameters;
    typedef std::map<std::string, cad::FeaturePtr> ItemList;

protected:
    ItemList items_;

public:
    virtual void update(const ParameterSet& ps);
    virtual void updateVisualizationElements(QoccViewWidget*, QModelTree*);
};




class blockMeshDict_Sphere_ParameterSet_Visualizer
 : public ParameterSet_Visualizer
{
public:
    typedef blockMeshDict_Sphere::Parameters Parameters;
    typedef std::map<std::string, cad::FeaturePtr> ItemList;

protected:
    ItemList items_;

public:
    virtual void update(const ParameterSet& ps);
    virtual void updateVisualizationElements(QoccViewWidget*, QModelTree*);
};



}
}


#endif // BLOCKMESH_GUI_H
