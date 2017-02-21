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
#include <QtGui/QApplication>
#include "base/boost_include.h"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include "workbench.h"

#include "base/analysis.h"
#include "base/exception.h"
#include "base/linearalgebra.h"

#include <qthread.h>
 
 
using namespace boost;
using namespace std;
 
 
class I : public QThread
{
    QSplashScreen* sp_;
    QWidget win_;
    
public:
    I(QSplashScreen* sp, QWidget* win) :sp_(sp), win_(win) 
    {}

    void run() 
    {
        QThread::sleep(3);
        sp_->finish(&win_);
    }
};




int main(int argc, char** argv)
{
    insight::UnhandledExceptionHandling ueh;
    insight::GSLExceptionHandling gsl_errtreatment;

    namespace po = boost::program_options;

    typedef std::vector<std::string> StringList;

    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
    ("help", "produce help message")
    ("libs", po::value< StringList >(),"Additional libraries with analysis modules to load")
    ("input-file,f", po::value< StringList >(),"Specifies input file.")
    ;
    
    po::positional_options_description p;
    p.add("input-file", -1);
    
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        exit(-1);
    }
    
    if (vm.count("libs"))
    {
        StringList libs=vm["libs"].as<StringList>();
        BOOST_FOREACH(const string& l, libs)
        {
           insight::loader.addLibrary(l);
        }
    }
    
    WorkbenchApplication app(argc, argv);

    // After creation of application object!
    std::locale::global(std::locale::classic());
    QLocale::setDefault(QLocale::C);

    QPixmap pixmap(":/resources/insight_workbench_splash.png");
    QSplashScreen splash(pixmap, Qt::WindowStaysOnTopHint|Qt::SplashScreen);
    splash.show();
    splash.showMessage(/*propGeoVersion()+" - */"Wait...");

    workbench window;

    if (vm.count("input-file"))
    {
        boost::filesystem::path fn( vm["input-file"].as<StringList>()[0] );
        window.openAnalysis(boost::filesystem::absolute(fn).c_str());
    }
    window.show();

    app.processEvents();//This is used to accept a click on the screen so that user can cancel the screen

    I w(&splash, &window);
    w.start(); // splash is shown for 5 seconds

    window.raise();

    return app.exec();
}
