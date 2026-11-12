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
 *
 */

// ISCAD grammar rules of the mesh postprocessing actions.
// Kept separate from mesh.cpp, because the boost::spirit instantiations
// and the OpenFOAM/VTK code together need a lot of compiler memory.

#include "mesh.h"
#include "parser_tools.h"
#include "cadfeature.h"

namespace insight
{
namespace cad
{

addToStaticFunctionTable2(
    PostprocAction, InsertRule, insertrule,
    Mesh, &Mesh::insertrule );

addToStaticFunctionTable2(
    PostprocAction, InsertRule, insertrule,
    ExtrudedMesh, &ExtrudedMesh::insertrule );


void Mesh::insertrule(parser::ISCADParser& ruleset)
{
    ruleset.postProcFunctionRules.add
        (
            "gmsh",
            std::make_shared<parser::ISCADParser::PostProcFunctionRule>(
                ( '(' > ruleset.r_path > ')' > qi::lit("<<")
                  > ruleset.r_solidmodel_expression
                  > qi::hold[ parser::kw("L") > '=' > '(' > ruleset.r_scalarExpression > ruleset.r_scalarExpression > ')' ] // Lmax, Lmin
                  > ( ( parser::kw("linear") > qi::attr(false) ) | qi::attr(true) )
                  > qi::hold[
                    ( ( parser::kw("vertexGroups") > '(' > *( ( (ruleset.r_identifier|ruleset.r_string) > '=' > ruleset.r_vertexFeaturesExpression > -( '@' > ruleset.r_scalarExpression ) ) ) > ')' ) | qi::attr(GroupsDesc()) )
                  > ( ( parser::kw("edgeGroups") > '(' > *( ( (ruleset.r_identifier|ruleset.r_string) > '=' > ruleset.r_edgeFeaturesExpression > -( '@' > ruleset.r_scalarExpression ) )  ) > ')' ) | qi::attr(GroupsDesc()) )
                  > ( ( parser::kw("faceGroups") > '(' > *( ( (ruleset.r_identifier|ruleset.r_string) > '=' > ruleset.r_faceFeaturesExpression > -( '@' > ruleset.r_scalarExpression ) )  ) > ')' ) | qi::attr(GroupsDesc()) )
                  > ( ( parser::kw("volumeGroups") > '(' > *( ( (ruleset.r_identifier|ruleset.r_string) > '=' > ruleset.r_solidFeaturesExpression > -( '@' > ruleset.r_scalarExpression ) )  ) > ')' ) | qi::attr(GroupsDesc()) )
                    ]
                  > ( ( parser::kw("vertices") > '(' > *( (ruleset.r_identifier|ruleset.r_string) > '=' > ruleset.r_vectorExpression ) > ')'  )| qi::attr(NamedVertices()) )
                  > ( ( parser::kw("meshSizes") > '(' > *( ruleset.r_vectorExpression > ',' > ruleset.r_scalarExpression > ',' > ruleset.r_scalarExpression ) >> ')' ) | qi::attr(std::vector<MeshSizeBall>()) )
                  > qi::hold[
                    ( ( parser::kw("screwHeads") > '(' > *( (ruleset.r_identifier|ruleset.r_string) > '=' > ruleset.r_solidmodel_expression > -( parser::kw("sub") > ruleset.r_identifier ) > -( '@' > ruleset.r_scalarExpression ) ) > ')'  )| qi::attr(ScrewHeads()) )
                  > ( ( parser::kw("screwBases") > '(' > *( (ruleset.r_identifier|ruleset.r_string) > '=' > ruleset.r_solidmodel_expression > -( parser::kw("sub") > ruleset.r_identifier ) > -( '@' > ruleset.r_scalarExpression ) ) > ')'  )| qi::attr(ScrewBases()) )
                  > ( ( parser::kw("screws")     > '(' > *( (ruleset.r_identifier|ruleset.r_string) > '=' > ruleset.r_solidmodel_expression > -( parser::kw("sub") > ruleset.r_identifier) ) > ')'  )| qi::attr(ScrewBodies()) )
                    ]
                  > ( ( parser::kw("keepTmpDir") > qi::attr(true) ) | qi::attr(false) )
                  > ';' )
                [ qi::_val = phx::bind(
                    &Mesh::create<const boost::filesystem::path&,FeaturePtr,
                                  boost::fusion::vector<ScalarPtr,ScalarPtr>,
                                  bool, const GroupDefinitions&, const NamedVertices&,
                                  const std::vector<MeshSizeBall>&, const ScrewInfos&, bool>,
                                   qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6, qi::_7, qi::_8, qi::_9
                                    ) ]
                )
            );
}


void ExtrudedMesh::insertrule(parser::ISCADParser& rs)
{
    rs.postProcFunctionRules.add
        (
            "gmshExtrusion",
            std::make_shared<parser::ISCADParser::PostProcFunctionRule>(
                ( '(' > rs.r_path > ')' > qi::lit("<<")
                 > rs.r_solidmodel_expression //>> lit("as") >> r_identifier
                 > qi::hold[ ( parser::kw("L") > '=' > '(' > rs.r_scalarExpression > rs.r_scalarExpression > ')'  // Lmax, Lmin
                             >  parser::kw("h") > '=' > rs.r_scalarExpression > parser::kw("nLayers") > '=' > rs.r_scalarExpression ) ]  // h nLayer
                 > ( ( parser::kw("linear") > qi::attr(false) ) | qi::attr(true) )
                 > qi::hold[
                     ( parser::kw("vertexGroups") > '(' > *( ( (rs.r_identifier|rs.r_string) > '=' > rs.r_vertexFeaturesExpression > -( '@' > rs.r_scalarExpression ) ) ) > ')' | qi::attr(GroupsDesc()) )
                     > ( parser::kw("edgeGroups") > '(' > *( ( (rs.r_identifier|rs.r_string) > '=' > rs.r_edgeFeaturesExpression > -( '@' > rs.r_scalarExpression ) )  ) > ')' | qi::attr(GroupsDesc()) )
                     > ( parser::kw("baseFaceGroups") > '(' > *( ( (rs.r_identifier|rs.r_string) > '=' > rs.r_faceFeaturesExpression > -( '@' > rs.r_scalarExpression ) )  ) > ')' | qi::attr(GroupsDesc()) )
                     > ( parser::kw("topFaceGroups") > '(' > *( ( (rs.r_identifier|rs.r_string) > '=' > rs.r_faceFeaturesExpression > -( '@' > rs.r_scalarExpression ) )  ) > ')' | qi::attr(GroupsDesc()) )
                     > ( parser::kw("volumeGroups") > '(' > *( ( (rs.r_identifier|rs.r_string) > '=' > rs.r_solidFeaturesExpression > -( '@' > rs.r_scalarExpression ) )  ) > ')' | qi::attr(GroupsDesc()) )
                    ]
                 > ( parser::kw("vertices") > '(' > *( (rs.r_identifier|rs.r_string) > '=' > rs.r_vectorExpression ) > ')' | qi::attr(NamedVertices()) )
                 > ( (parser::kw("keepTmpDir") > qi::attr(true)) | qi::attr(false) )
                 > ';' )
                    [ qi::_val = phx::bind(
                         &ExtrudedMesh::create<
                             const boost::filesystem::path&,FeaturePtr,
                             boost::fusion::vector<ScalarPtr,ScalarPtr,ScalarPtr,ScalarPtr>,
                             bool, const ExtrudedGroupDefinitions&, const NamedVertices&, bool>,
                         qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6, qi::_7
                         ) ]
                )
            );
}


void SnappyHexMesh::insertrule(parser::ISCADParser& rs)
{
    rs.postProcFunctionRules.add
        (
            "snappyHexMesh",
            std::make_shared<parser::ISCADParser::PostProcFunctionRule>(

                ( '(' > rs.r_path > ',' > rs.r_identifier > ')' > qi::lit("<<")

                 > parser::kw("PiM") > '=' > rs.r_vectorExpression
                 > parser::kw("dx") > '=' > rs.r_scalarExpression

                 > *( rs.r_solidmodel_expression > parser::kw("as") > rs.r_identifier
                     > ( ( parser::kw("resolution") > '=' > rs.r_scalarExpression ) | qi::attr(ScalarPtr()) )
                     > -( '@' > rs.r_scalarExpression > parser::kw("to") > rs.r_scalarExpression )
                     > -( qi::lit(">>") > rs.r_scalarExpression )
                     )

                 > -( parser::kw("edgeRefinements") > '(' > *( rs.r_identifier > '=' > rs.r_edgeFeaturesExpression > '@' > rs.r_scalarExpression ) > ')' )

                 > ';' )
                    [ qi::_val = phx::bind(
                        &SnappyHexMesh::create<
                            const boost::filesystem::path&, const std::string&,
                            VectorPtr, ScalarPtr, GeometrysDesc, boost::optional<EdgeRefineDescs> >,
                        qi::_1, qi::_2, qi::_3, qi::_4, qi::_5, qi::_6 ) ]
                )
            );
}


}
}
