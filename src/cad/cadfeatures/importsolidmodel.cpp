/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "importsolidmodel.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "base/exception.h"
#include "base/tools.h"
#include "base/translations.h"

#include "TDocStd_Document.hxx"
//#include "XCAFApp_Application.hxx"
//#include "XCAFDoc.hxx"
//#include "XCAFDoc_DocumentTool.hxx"
//#include "XCAFDoc_ShapeTool.hxx"
//#include "XSControl_WorkSession.hxx"
//#include "XSControl_TransferReader.hxx"
//#include "XSControl_TransferWriter.hxx"
#include "StepData_StepModel.hxx"
#include "TDF_LabelSequence.hxx"
#if (OCC_VERSION_MAJOR>=7)
#else
#include "Handle_StepRepr_RepresentationItem.hxx"
#endif
#include "STEPConstruct.hxx"
#include "STEPCAFControl_Writer.hxx"
//#include "STEPCAFControl_Reader.hxx"
#include "StepRepr_RepresentationItem.hxx"
#include "StepShape_AdvancedFace.hxx"
#include "XSControl_WorkSession.hxx"
#include "XSControl_TransferReader.hxx"
#include "APIHeaderSection_MakeHeader.hxx"
#include "TDataStd_Name.hxx"
#include "TDF_ChildIterator.hxx"
#include "TDF_ChildIDIterator.hxx"
#include "TransferBRep.hxx"
#include "Transfer_Binder.hxx"
#include "Transfer_TransientProcess.hxx"


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


    
    
defineType(Import);
//addToFactoryTable(Feature, Import);
addToStaticFunctionTable(Feature, Import, insertrule);
addToStaticFunctionTable(Feature, Import, ruleDocumentation);




Import::Import ( const TopoDS_Shape& shape )
: importSource_(shape)
{}




Import::Import(const filesystem::path& filepath)
: importSource_(filepath)
{}




Import::Import ( FeatureSetPtr creashapes )
: importSource_(creashapes)
{}




size_t Import::calcHash() const
{
    ParameterListHash h;
    h+=this->type();

    if (auto *f=boost::get<boost::filesystem::path>(&importSource_))
    {
        h+=*f;
    }
    else if (auto *s=boost::get<TopoDS_Shape>(&importSource_))
    {
        h+=*s;
    }
    else if (auto *fs=boost::get<FeatureSetPtr>(&importSource_))
    {
        h+=**fs;
    }
    else
        throw insight::UnhandledSelection();

    return h.getHash();
}


void Import::build()
{
  ExecTimer t("Import::build() ["+featureSymbolName()+"]");

  if (!cache.contains(hash()))
  {

      if (auto *f=boost::get<boost::filesystem::path>(&importSource_))
      {
          boost::filesystem::path fp = *f;
          if (!boost::filesystem::exists(fp))
          {
              fp=sharedModelFilePath(f->string());
              if (!boost::filesystem::exists(fp))
              {
                  throw insight::Exception(_("File not found: %s"), f->string().c_str());
              }
          }
          setShapeFromFile(fp);

          for (FeatureID i: allSolidsSet())
          {
              providedSubshapes_[boost::str(boost::format("solid%d")%i)]=Import::create(subsolid(i));
          }

          setFeatureSymbolName("importedFrom_"+fp.string());
      }
      else if (auto *s=boost::get<TopoDS_Shape>(&importSource_))
      {
          setShape(*s);
      }
      else if (auto *fs=boost::get<FeatureSetPtr>(&importSource_))
      {
          auto& featSet=*fs;

          std::vector<TopoDS_Shape> shapes;
          for (const FeatureID& id: featSet->data())
          {
              if (featSet->shape()==Vertex)
              {
                  shapes.push_back(featSet->model()->vertex(id));
              }
              else if (featSet->shape()==Edge)
              {
                  shapes.push_back(featSet->model()->edge(id));
              }
              else if (featSet->shape()==Face)
              {
                  shapes.push_back(featSet->model()->face(id));
              }
              else if (featSet->shape()==Solid)
              {
                  shapes.push_back(featSet->model()->subsolid(id));
              }
              else
                  throw insight::UnhandledSelection();
          }

          TopoDS_Shape result;

          if (shapes.size()==1)
          {
              result=shapes.front();
          }
          else
          {
              TopoDS_Compound comp;
              BRep_Builder builder;
              builder.MakeCompound( comp );

              for (auto ss: shapes)
              {
                  builder.Add(comp, ss);
              }

              result=comp;
          }

          setShape(result);
      }
      else
          throw insight::UnhandledSelection();


    cache.insert(shared_from_this());
  }
  else
  {
      this->operator=(*cache.markAsUsed<Import>(hash()));
  }

}




void Import::insertrule(parser::ISCADParser& ruleset)
{
  ruleset.modelstepFunctionRules.add
  (
    "import",	
    std::make_shared<parser::ISCADParser::ModelstepRule>(
      ( '(' >> 
            ruleset.r_path
            >> ')' )
       [ qi::_val = phx::bind(
                       &Import::create<const filesystem::path&>,
                       qi::_1) ]
    )
  );

  ruleset.modelstepFunctionRules.add
    (
        "asModel",
        std::make_shared<parser::ISCADParser::ModelstepRule>(

            ( '(' > ( ruleset.r_vertexFeaturesExpression | ruleset.r_edgeFeaturesExpression | ruleset.r_faceFeaturesExpression | ruleset.r_solidFeaturesExpression ) >> ')' )
                [ qi::_val = phx::bind(Import::create<FeatureSetPtr>, qi::_1) ]

            )
        );

}




FeatureCmdInfoList Import::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "import", "( <path> )",
            _("Imports a feature from a file. "
              "The format is recognized from the filename extension. "
              "Supported formats are IGS, STP, BREP.")
        ),
        FeatureCmdInfo
        (
            "asModel", "( <verticesSelection>|<edgesSelection>|<facesSelection>|<solidSelection> )",
            "Creates a new feature from selected entities of an existing feature."
        )
    };
}



}
}
