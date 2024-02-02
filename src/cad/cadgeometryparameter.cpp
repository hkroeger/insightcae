#include "cadgeometryparameter.h"

#include "cadmodel.h"
#include "parser.h"
#include "base/rapidxml.h"

namespace insight {




// defineType(CADGeometryParameter);
// addToFactoryTable(Parameter, CADGeometryParameter);




// void CADGeometryParameter::resetCADGeometry() const
// {
//     if (! (script().empty() || featureLabel().empty()) )
//     {
//         std::string modelscript( featureLabel()+": "+script()+";\n" );

//         insight::cad::ModelPtr model(new insight::cad::Model); // std::make_shared causes crash!!??

//         int success = insight::cad::parseISCADModel(modelscript, model.get());
//         CADGeometry_ = model->modelsteps().at(featureLabel());
//     }
// }

// CADGeometryParameter::CADGeometryParameter(
//     const std::string& description,
//     bool isHidden, bool isExpert, bool isNecessary, int order
//     )
// : Parameter(description, isHidden, isExpert, isNecessary, order)
// {}




// CADGeometryParameter::CADGeometryParameter(
//     const std::string& featureLabel,
//     const std::string& script,
//     const std::string& description,
//     bool isHidden, bool isExpert, bool isNecessary, int order
//     )
//     : Parameter(description, isHidden, isExpert, isNecessary, order),
//       featureLabel_(featureLabel),
//       script_(script)
// {
//     resetCADGeometry();
// }

// const std::string &CADGeometryParameter::featureLabel() const
// {
//     return featureLabel_;
// }

// void CADGeometryParameter::setFeatureLabel(const std::string &label)
// {
//     featureLabel_=label;
//     resetCADGeometry();
// }

// const std::string& CADGeometryParameter::script() const
// {
//     return script_;
// }


// void CADGeometryParameter::setScript(const std::string& script)
// {
//     script_=script;
//     resetCADGeometry();
// }


// //void CADGeometryParameter::setCADModel(cad::ModelPtr cadmodel)
// //{
// //    cadmodel_=cadmodel;
// //}

// cad::FeaturePtr CADGeometryParameter::featureGeometry() const
// {
// //    insight::assertion(
// //                bool(cadmodel_),
// //                "there was no CAD model assigned!" );
// //    return cadmodel_->modelsteps().at(featureLabel_);
//     return CADGeometry_;
// }


// std::string CADGeometryParameter::latexRepresentation() const
// {
//     return featureLabel_;
// }

// std::string CADGeometryParameter::plainTextRepresentation(int indent) const
// {
//     return featureLabel_;
// }

// rapidxml::xml_node<>* CADGeometryParameter::appendToNode
// (
//     const std::string& name,
//     rapidxml::xml_document<>& doc,
//     rapidxml::xml_node<>& node,
//     boost::filesystem::path inputfilepath
// ) const
// {
//     auto n = Parameter::appendToNode(name, doc, node, inputfilepath);
//     appendAttribute(doc, *n, "featureLabel", featureLabel_);
//     n->value(doc.allocate_string(script_.c_str()));
//     return n;
// }

// void CADGeometryParameter::readFromNode
// (
//     const std::string& name,
//     rapidxml::xml_node<>& node,
//     boost::filesystem::path inputfilepath
// )
// {
//     using namespace rapidxml;
//     xml_node<>* child = findNode(node, name, type());
//     if (child)
//     {
//         auto valueattr=child->first_attribute ( "featureLabel" );
//         insight::assertion(valueattr, "No value attribute present in "+name+"!");
//         featureLabel_=valueattr->value();

//         script_ = child->value();

//         resetCADGeometry();
//     }
// }

// CADGeometryParameter *CADGeometryParameter::cloneCADGeometryParameter() const
// {
//     auto ncgp=new CADGeometryParameter(
//           featureLabel_, script_,
//           description_.simpleLatex(),
//                 isHidden_, isExpert_, isNecessary_, order_ );
// //    if (cadmodel_)
// //        ncgp->setCADModel(cadmodel_);
//     return ncgp;
// }


// Parameter *CADGeometryParameter::clone() const
// {
//     return cloneCADGeometryParameter();
// }

// void CADGeometryParameter::operator=(const CADGeometryParameter &op)
// {
//     description_ = op.description_;
//     isHidden_ = op.isHidden_;
//     isExpert_ = op.isExpert_;
//     isNecessary_ = op.isNecessary_;
//     order_ = op.order_;

//     featureLabel_ = op.featureLabel_;
//     script_ = op.script_;
// //    cadmodel_ = op.cadmodel_;
// }

// bool CADGeometryParameter::isDifferent(const Parameter & op) const
// {
//     if (const auto *sp = dynamic_cast<const CADGeometryParameter*>(&op))
//     {
//       return (featureLabel_!=sp->featureLabel())
//             || (script_!=sp->script());
//     }
//     else
//       return true;
// }

// int CADGeometryParameter::nChildren() const
// {
//     return 0;
// }



} // namespace insight
