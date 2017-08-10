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

#include <locale>
#include <QLocale>
#include <QDir>
#include <QSplashScreen>
#include <QDialog>
#include <QtGui/QApplication>
#include <QMessageBox>

#ifndef Q_MOC_RUN
#include "base/boost_include.h"
#include "base/exception.h"
#include "base/linearalgebra.h"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#endif

#include <qthread.h>


#include "isofcasebuilderwindow.h"


using namespace std;
using namespace insight;


class ISOFApp: public QApplication
{
public:
  ISOFApp ( int &argc, char **argv )
    : QApplication ( argc, argv )
  {}

  ~ISOFApp( )
  {}

  bool notify ( QObject *rec, QEvent *ev )
  {
    try
      {
        return QApplication::notify ( rec, ev );
      }

    catch ( insight::Exception e )
      {
        std::cout << e << std::endl;

        QMessageBox msgBox;
        msgBox.setIcon ( QMessageBox::Critical );
        msgBox.setText ( QString ( e.as_string().c_str() ) );

        msgBox.exec();
      }

    return true;
  }
};


insight::ParameterSet& split_and_check
(
    isofCaseBuilderWindow& wnd, 
    std::vector<std::string>& pair,
    const std::string& s
)
{
    boost::split(pair, s, boost::is_any_of(":"));
    if (pair.size()!=3)
        throw insight::Exception
        (
            "Invalid specification of parameter value in command line!\n"
            "Each parameter has to be given as:\n"
            " (#<Case element ID>|<Patch Name>):<parameter set path>:<value>"
        );

    if (pair[0][0]=='#')
    {
        int ceid=boost::lexical_cast<int>(pair[0][1]);
        return wnd.caseElementParameters(ceid);
    } 
    else
    {
        return wnd.BCParameters(pair[0]);
    }
}

int main ( int argc, char** argv )
{
    try
    {
        insight::UnhandledExceptionHandling ueh;
        insight::GSLExceptionHandling gsl_errtreatment;

        namespace po = boost::program_options;

        typedef std::vector<std::string> StringList;

        // Declare the supported options.
        po::options_description desc ( "Allowed options" );
        desc.add_options()
        ( "help,h", "produce help message" )
        ( "batch,b", "case creation from specified input file" )
        ( "skipbcs,s", "skip BC configuration during input file read and batch case creation" )
        ( "input-file,f", po::value< StringList >(),"Specifies input file. Multiple input files will append to the active configuration." )
        ( "write-only,o", po::value< StringList >(),"restrict output in batch mode to specified files" )
        ("bool", po::value<StringList>(), "boolean variable assignment")
        ("selection,l", po::value<StringList>(), "selection variable assignment")
        ("string", po::value<StringList>(), "string variable assignment")
        ("path,p", po::value<StringList>(), "path variable assignment")
        ("double,d", po::value<StringList>(), "double variable assignment")
        ("vector,v", po::value<StringList>(), "vector variable assignment")
        ("int,i", po::value<StringList>(), "int variable assignment")
        ;

        po::positional_options_description p;
        p.add ( "input-file", -1 );

        po::variables_map vm;
        po::store
        (
            po::command_line_parser ( argc, argv ).options ( desc ).positional ( p ).run(),
            vm
        );
        po::notify ( vm );

        if ( vm.count ( "help" ) )
        {
            std::cout << desc << std::endl;
            exit ( -1 );
        }

        ISOFApp app ( argc, argv );

        // After creation of application object!
        std::locale::global ( std::locale::classic() );
        QLocale::setDefault ( QLocale::C );

        isofCaseBuilderWindow window;
        if ( vm.count ( "input-file" ) )
        {
            BOOST_FOREACH( const std::string& fn, vm["input-file"].as<StringList>())
            {
                if (!boost::filesystem::exists(fn))
                {
                    std::cerr << std::endl 
                        << "Error: input file does not exist: "<<fn
                        <<std::endl<<std::endl;
                    exit(-1);
                }
                window.loadFile ( fn, vm.count ( "skipbcs" ) );
            }
            
            if (vm.count("bool"))
            {
                StringList sets=vm["bool"].as<StringList>();
                BOOST_FOREACH(const string& s, sets)
                {
                    std::vector<std::string> pair;
                    insight::ParameterSet& parameters = split_and_check(window, pair, s);
                    bool v=boost::lexical_cast<bool>(pair[2]);
                    cout << "Setting boolean '"<<pair[1]<<"' = "<<v<<endl;
                    parameters.getBool(pair[1])=v;
                }
            }

            if (vm.count("string"))
            {
                StringList sets=vm["string"].as<StringList>();
                BOOST_FOREACH(const string& s, sets)
                {
                    std::vector<std::string> pair;
                    insight::ParameterSet& parameters = split_and_check(window, pair, s);
                    cout << "Setting string '"<<pair[1]<<"' = \""<<pair[2]<<"\""<<endl;
                    parameters.getString(pair[1])=pair[2];
                }
            }

            if (vm.count("selection"))
            {
                StringList sets=vm["selection"].as<StringList>();
                BOOST_FOREACH(const string& s, sets)
                {
                    std::vector<std::string> pair;
                    insight::ParameterSet& parameters = split_and_check(window, pair, s);
                    cout << "Setting selection '"<<pair[1]<<"' = \""<<pair[2]<<"\""<<endl;
                    parameters.get<SelectionParameter>(pair[1]).setSelection(pair[2]);
                }
            }

            if (vm.count("path"))
            {
                StringList sets=vm["path"].as<StringList>();
                BOOST_FOREACH(const string& s, sets)
                {
                    std::vector<std::string> pair;
                    insight::ParameterSet& parameters = split_and_check(window, pair, s);
                    cout << "Setting path '"<<pair[1]<<"' = \""<<pair[2]<<"\""<<endl;
                    parameters.getPath(pair[1])=pair[2];
                }
            }

            if (vm.count("double"))
            {
                StringList sets=vm["double"].as<StringList>();
                BOOST_FOREACH(const string& s, sets)
                {
                    std::vector<std::string> pair;
                    insight::ParameterSet& parameters = split_and_check(window, pair, s);
                    double v=boost::lexical_cast<double>(pair[2]);
                    cout << "Setting double '"<<pair[1]<<"' = "<<v<<endl;
                    parameters.getDouble(pair[1])=v;
                }
            }

            if (vm.count("vector"))
            {
                StringList sets=vm["vector"].as<StringList>();
                BOOST_FOREACH(const string& s, sets)
                {
                    std::vector<std::string> pair;
                    insight::ParameterSet& parameters = split_and_check(window, pair, s);
                    arma::mat v;
                    stringToValue(pair[2], v);
                    cout << "Setting vector '"<<pair[1]<<"' = "<<v<<endl;
                    parameters.getVector(pair[1])=v;
                }
            }

            if (vm.count("int"))
            {
                StringList sets=vm["int"].as<StringList>();
                BOOST_FOREACH(const string& s, sets)
                {
                    std::vector<std::string> pair;
                    insight::ParameterSet& parameters = split_and_check(window, pair, s);
                    int v=boost::lexical_cast<int>(pair[2]);
                    cout << "Setting int '"<<pair[1]<<"' = "<<v<<endl;
                    parameters.getInt(pair[1])=v;
                }
            }
        
            if ( vm.count ( "batch" ) )
            {
                boost::shared_ptr<std::vector<boost::filesystem::path> > restrictToFiles;
                
                if ( vm.count ( "write-only" ) )
                {
                    restrictToFiles.reset(new std::vector<boost::filesystem::path>);
                    //(vm["write-only"].as<std::vector<boost::filesystem::path> >()) );
                    StringList paths = vm["write-only"].as<StringList>();
                    copy(paths.begin(), paths.end(), std::back_inserter(*restrictToFiles));
                    BOOST_FOREACH(const boost::filesystem::path& f, *restrictToFiles)
                     std::cout<<f<<std::endl;
                }
                
                window.createCase( vm.count ( "skipbcs" ), restrictToFiles );
            }

        }

        if ( !vm.count ( "batch" ) )
        {
            window.show();
            return app.exec();
        }
        else
            return 0;
    }
    catch ( insight::Exception e )
    {
        std::cerr<<e<<std::endl;
    }
}

