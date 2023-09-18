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

#include "lsdynamesh.h"

#include "base/boost_include.h"
#include "base/translations.h"
#include "base/tools.h"
#include "base/linearalgebra.h"
#include "base/toolkitversion.h"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "vtkGenericDataObjectReader.h"
#include "vtkCellData.h"

using namespace insight;


int main(int argc, char *argv[])
{
    try
    {

        GettextInit gti(GETTEXT_DOMAIN, GETTEXT_OUTPUT_DIR, GettextInit::Application);

        insight::UnhandledExceptionHandling ueh;
        insight::GSLExceptionHandling gsl_errtreatment;

        using namespace rapidxml;
        namespace po = boost::program_options;

        typedef std::vector<std::string> StringList;
        typedef std::set<int> IntSet;

        po::options_description desc(_("Allowed options"));
        desc.add_options()
            ("help,h", _("produce help message"))
            ("version,r", _("print version and exit"))
            ("part2set,p", po::value< StringList >(), _("<part id:set id>. part id to be converted to set (both a node set and a shell/solid set will be created)."))
            ("skip", po::value< StringList >(), _("A list (comma separated) of parts to be skipped when writing output"))
            ("input-file,f", po::value< StringList >(), _("Specifies input file and the part ID for all its elements (separated yby colon)"))
            ("output-file,o", po::value< std::string >(), _("Specifies output file."))
            ;

        po::positional_options_description p;
        p.add("input-file", -1);

        auto displayHelp = [&]{
            std::ostream &os = std::cout;

            os << _("Usage:") << std::endl;
            os << "  " << boost::filesystem::path(argv[0]).filename().string() << " ["<<_("options")<<"] " << p.name_for_position(0) << std::endl;
            os << std::endl;
            os << desc << endl;
        };


        po::variables_map vm;
        try
        {
            po::store
                (
                    po::command_line_parser(argc, argv)
                        .options(desc)
                        .positional(p).run(),
                    vm
                    );
            po::notify(vm);
        }
        catch (const po::error& e)
        {
            std::cerr
                << std::endl
                << _("Could not parse command line")<<": "<< e.what()
                << std::endl<<std::endl;
            displayHelp();
            exit(-1);
        }

        if (vm.count("help"))
        {
            displayHelp();
            exit(0);
        }

        if (vm.count("version"))
        {
            cout << std::string(insight::ToolkitVersion::current()) << endl;
            exit(0);
        }

        if (vm.count("output-file")!=1)
        {
            cerr << _("exactly one output file name has to be given!") << endl;
            exit(-1);
        }

        boost::filesystem::path outfn( vm["output-file"].as<std::string>() );

        IntSet tbSkipped;
        for ( const auto& arg: vm["skip"].as<StringList>())
        {
            std::vector<std::string> sl;
            boost::split(sl, arg, boost::is_any_of(","));

            std::transform(sl.begin(), sl.end(),
                std::inserter(tbSkipped, tbSkipped.begin()),
                std::bind(&boost::lexical_cast<int,std::string>, std::placeholders::_1)
            );
        }

        LSDynaMesh tmesh;

        int defaultPartId = 0;
        for ( const auto& arg: vm["input-file"].as<StringList>())
        {
            defaultPartId += 1000;

            std::vector<std::string> fname_partid;
            boost::split(fname_partid, arg, boost::is_any_of(":"));
            insight::assertion(fname_partid.size()>0,
                               "input-file must contain at least a file name, got: ", arg.c_str());
            insight::assertion(fname_partid.size()<3,
                               "input-file must contain only file name and part id, separated by colon, got: ", arg.c_str());

            boost::filesystem::path fn(fname_partid[0]);
            int partId = defaultPartId;
            if (fname_partid.size()>1)
                partId = insight::toNumber<int>(fname_partid[1]);

            insight::assertion(boost::filesystem::exists(fn),
                               "input file does not exist: ", fn.c_str());

            std::cout<<"Processing mesh in "<<fn.string()<<", storing elements in part "<<partId<<std::endl;

            auto reader = vtkSmartPointer<vtkGenericDataObjectReader>::New();
            reader->SetFileName(fn.string().c_str());
            reader->Update();

            std::map<int, int> unhandledCells;
            vtkIdType nodeIdsOfs=tmesh.maxNodeId();
            if (auto *smesh = vtkDataSet::SafeDownCast(reader->GetOutput()))
            {
                auto cei = smesh->GetCellData()->GetArray("CellEntityIds");

                for (int i=0; i<smesh->GetNumberOfCells(); ++i)
                {

                    int effectivePartId=partId;
                    if (cei)
                    {
                        effectivePartId+=cei->GetTuple1(i);
                    }

                    auto *c=smesh->GetCell(i);
                    if (auto *q = vtkQuad::SafeDownCast(c))
                    {
                        tmesh.addQuadElement(smesh, q, effectivePartId, nodeIdsOfs);
                    }
                    else if (auto *t = vtkTriangle::SafeDownCast(c))
                    {
                        tmesh.addTriElement(smesh, t, effectivePartId, nodeIdsOfs);
                    }
                    else if (auto *t = vtkTetra::SafeDownCast(c))
                    {
                        tmesh.addTetElement(smesh, t, effectivePartId, nodeIdsOfs);
                    }
                    else
                    {
                        int ct=c->GetCellType();
                        if (unhandledCells.find(ct)==unhandledCells.end())
                            unhandledCells[ct]=1;
                        else
                            unhandledCells[ct]++;
                    }
                }
            }
            else
            {
                throw insight::Exception("Unhandled data set type:", reader->GetOutput()->GetDataObjectType());
            }

            if (unhandledCells.size()>0)
            {
                std::cerr<<"Unhandled cells:\n";
                for (const auto& uhc: unhandledCells)
                {
                    std::cerr<<" type "<<uhc.first<<": "<<uhc.second<<"\n";
                }
            }

        }

        tmesh.printStatistics(std::cout);

        std::set<int> parts2ElementGroup;
        for ( const auto& arg: vm["part2set"].as<StringList>())
        {
            std::vector<std::string> partId_setId;
            boost::split(partId_setId, arg, boost::is_any_of(":"));
            insight::assertion(partId_setId.size()==2,
                               "expected 'partId:setId', got %s", arg.c_str());
            int partId=toNumber<int>(partId_setId[0]);
            int setId=toNumber<int>(partId_setId[1]);

            parts2ElementGroup.insert(partId);
            tmesh.findNodesOfPart( tmesh.nodeSet(setId), partId );
        }


        std::ofstream of(outfn.string());
        tmesh.write(of, tbSkipped, parts2ElementGroup);
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
        return -1;
    }

    return 0;
}
