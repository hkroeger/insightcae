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

#include "lsdyna/femmesh.h"

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
#include "vtkSTLReader.h"
#include "vtkSelectEnclosedPoints.h"
#include "vtkPointData.h"


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
            ("stl2set,s", po::value< StringList >(), _("<STl file path:set id>. elements and nodes inside given STL will be added to set (both a node set and a shell/solid set will be created)."))
            ("skip", po::value< StringList >(), _("A list (comma separated) of parts to be skipped when writing output"))
            ("input-file,f", po::value< StringList >(), _("Specifies input file and the part ID for all its elements (separated yby colon)"))
            ("format", po::value<std::string>()->default_value("lsdyna"), _("set output format (LSDyna or Radioss)"))
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

        FEMMesh::OutputFormat fmt = FEMMesh::LSDyna;
        if (vm.count("format"))
        {
            auto fmtid = boost::to_lower_copy(vm["format"].as<std::string>());
            if (fmtid=="lsdyna")
                fmt=FEMMesh::LSDyna;
            else if (fmtid=="radioss")
                fmt=FEMMesh::Radioss;
            else
                throw insight::Exception("Unknown mesh output format: %s", fmtid.c_str());
        }


        if (vm.count("output-file")!=1)
        {
            cerr << _("exactly one output file name has to be given!") << endl;
            exit(-1);
        }

        std::map<int,std::pair<vtkSmartPointer<vtkPolyData>,int> > stl2Group;
        if (vm.count("stl2set"))
        {
            for ( const auto& arg: vm["stl2set"].as<StringList>())
            {
                std::vector<std::string> path_setId_maskPartId;
                boost::split(path_setId_maskPartId, arg, boost::is_any_of(":"));
                insight::assertion(path_setId_maskPartId.size()>=2 && path_setId_maskPartId.size()<=3,
                                   "expected 'path:setId[:maskId]', got %s", arg.c_str());

                boost::filesystem::path stlfn(path_setId_maskPartId[0]);
                int setId=toNumber<int>(path_setId_maskPartId[1]);
                int maskPartId = -1;
                if (path_setId_maskPartId.size()>2)
                    maskPartId=toNumber<int>(path_setId_maskPartId[2]);

                insight::assertion(
                    boost::filesystem::exists(stlfn),
                    "STL file %s does not exist!", stlfn.c_str() );

                auto sr=vtkSmartPointer<vtkSTLReader>::New();
                sr->SetFileName(stlfn.string().c_str());
                sr->Update();

                stl2Group[setId]=std::make_pair(sr->GetOutput(), maskPartId);
            }
        }

        boost::filesystem::path outfn( vm["output-file"].as<std::string>() );

        IntSet tbSkipped;
        if (vm.count("skip"))
        {
            for ( const auto& arg: vm["skip"].as<StringList>())
            {
                std::vector<std::string> sl;
                boost::split(sl, arg, boost::is_any_of(","));

                std::transform(sl.begin(), sl.end(),
                    std::inserter(tbSkipped, tbSkipped.begin()),
                    std::bind(&boost::lexical_cast<int,std::string>, std::placeholders::_1)
                );
            }
        }

        FEMMesh tmesh;

        int defaultPartId = 0;
        if (vm.count("input-file"))
        {
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

                tmesh.addVTK(fn, partId);
            }
        }

        tmesh.numberElements(tbSkipped);

        if (vm.count("part2set"))
        {
            for ( const auto& arg: vm["part2set"].as<StringList>())
            {
                std::vector<std::string> partId_setId;
                boost::split(partId_setId, arg, boost::is_any_of(":"));
                insight::assertion(partId_setId.size()==2,
                                   "expected 'partId:setId', got %s", arg.c_str());
                int partId=toNumber<int>(partId_setId[0]);
                int setId=toNumber<int>(partId_setId[1]);

                tmesh.partToSet(partId, setId);
            }
        }

        if (stl2Group.size())
        {
            auto ctrs = vtkSmartPointer<vtkPoints>::New();
            tmesh.getCellCenters(ctrs);
            auto ctrpd = vtkSmartPointer<vtkPolyData>::New();
            ctrpd->SetPoints(ctrs);


            for ( const auto& setid_stl: stl2Group)
            {
#warning need to distinguish shells and vol elems
                auto& set = tmesh.shellSet(setid_stl.first);
                FEMMesh::IdSet *maskElementSet=nullptr;
                if (setid_stl.second.second>=0)
                    maskElementSet=&tmesh.shellSet(setid_stl.second.second);

                auto sep = vtkSmartPointer<vtkSelectEnclosedPoints>::New();
                sep->SetInputData(ctrpd);
                sep->SetSurfaceData(setid_stl.second.first);
                sep->Update();

                auto sp = sep->GetOutput()->GetPointData()->GetArray("SelectedPoints");
                for (vtkIdType i=0; i<sp->GetNumberOfTuples(); ++i)
                {
                    if (sp->GetTuple1(i))
                    {
                        // is inside
                        int elemId=i+1;

                        if (maskElementSet)
                        {
                            // only add, if is in masking set
                            if (maskElementSet->count(elemId)<1) continue;
                        }
                        set.insert(elemId);
                    }
                }
            }
        }

        tmesh.printStatistics(std::cout);

        std::ofstream of(outfn.string());
        tmesh.write(of, fmt);
    }
    catch (std::exception& ex)
    {
        std::cerr<<ex.what()<<std::endl;
        return -1;
    }

    return 0;
}
