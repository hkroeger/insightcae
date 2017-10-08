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

#include "freecadmodel.h"
#include "base/boost_include.h"

using namespace boost;
using namespace boost::filesystem;
using namespace boost::algorithm;

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

namespace insight
{
namespace cad
{

    
    
    
defineType(FreeCADModel);
addToFactoryTable(Feature, FreeCADModel);




FreeCADModel::FreeCADModel()
{
}




FreeCADModel::FreeCADModel
(
  const boost::filesystem::path& filename, 
  const std::string& solidname, 
  FreeCADModelVarList vars
)
: filename_(filename),
  solidname_(solidname),
  vars_(vars)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=filename_;
  h+=solidname_;
  BOOST_FOREACH(const FreeCADModelVar& v, vars)
  {
      h+=boost::fusion::at_c<0>(v);
      h+=boost::fusion::at_c<1>(v)->value();
  }
}




FeaturePtr FreeCADModel::create ( const boost::filesystem::path& filename, const std::string& solidname, FreeCADModelVarList vars )
{
    return FeaturePtr(new FreeCADModel(filename, solidname, vars));
}




void FreeCADModel::build()
{
    boost::filesystem::path infilename = filename_;
    if ( !exists ( infilename ) ) {
        infilename=sharedModelFilePath ( filename_.string() );
    }

    boost::filesystem::path filename = filename_;
    std::string solidname = solidname_;


    filename =
        boost::filesystem::unique_path ( temp_directory_path() / "%%%%-%%%%-%%%%-%%%%.brep" );
    boost::filesystem::path macrofilename =
        boost::filesystem::unique_path ( temp_directory_path() / "%%%%-%%%%-%%%%-%%%%.FCMacro" );

    {

        std::string vargs="";
        for ( FreeCADModelVarList::const_iterator it=vars_.begin(); it!=vars_.end(); it++ ) {
            std::vector<std::string> vpath=boost::fusion::at_c<0> ( *it );
            if ( vpath.size() ==0 ) {
                throw insight::Exception ( "Internal error: zero length of parameter path!" );
            }

            double vval=boost::fusion::at_c<1> ( *it )->value();
            std::string cmd="doc";
            for ( int i=0; i<vpath.size()-1; i++ ) {
                cmd+=str ( format ( ".getObjectsByLabel(\"%s\")[0]" ) %vpath[i] );
            }
            cmd+=str ( format ( ".setDatum(\"%s\", %g)" ) %vpath.back() %vval );
            vargs+=cmd+"\n";
        }

        std::ofstream mf ( macrofilename.c_str() );
        mf << str ( format (
"import FreeCAD\n"
"import Part\n"

"FreeCAD.open(\"%s\")\n"
"doc=FreeCAD.getDocument( \"%s\" );\n"
+vargs+
"doc.recompute()\n"
"obj=doc.getObjectsByLabel('%s')\n"
"Part.export(obj, \"%s\")\n"
                    )
                    % infilename.string()
                    % infilename.filename().stem().string()
                    % solidname
                    % filename.string()
                  );
    }

    std::string cmd = str ( format ( "FreeCADCmd %s" ) % macrofilename );
//     std::string cmd = str( format("fcstd2dxf.py %s %s %s") % fn % ln % filename );
    cout<<"CMD=\""<<cmd<<"\""<<endl;
    if ( ::system ( cmd.c_str() ) || !boost::filesystem::exists ( filename ) ) {
        throw insight::Exception ( "Conversion of FreeCAD file "+infilename.string()+" into BREP "+filename.string()+" failed!" );
    }
    boost::filesystem::remove ( macrofilename );

    loadShapeFromFile ( filename );

}




void FreeCADModel::insertrule ( parser::ISCADParser& ruleset ) const
{
    ruleset.modelstepFunctionRules.add
    (
        "FreeCADModel",
        typename parser::ISCADParser::ModelstepRulePtr ( new typename parser::ISCADParser::ModelstepRule (

                    ( '(' 
                      >> ruleset.r_path >> ','
                      >> ruleset.r_string
                      >> ( ( ',' >> ( ( ruleset.r_identifier % '.' ) >> '=' >> ruleset.r_scalarExpression ) % ',' ) | qi::attr ( FreeCADModelVarList() ) )
                      >> ')' )
                    [ qi::_val = phx::bind(&FreeCADModel::create, qi::_1, qi::_2, qi::_3 ) ]

                ) )
    );
}




FeatureCmdInfoList FreeCADModel::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "FreeCADModel",
         
            "( <path:filename>, <string:solidname> [, <identifier>[.<identifier>] = <scalar:value>, ... ] )",
         
            "Rebuild a model in FreeCAD and imports the result. An arbitrary number of parameter/value pairs can be passed into FreeCAD."
        )
    );
}




}
}
