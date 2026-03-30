
#include "base/exception.h"
#include "base/translations.h"
#include "base/boost_include.h"
#include "boost/algorithm/string/join.hpp"
#include "openfoam/solveroutputanalyzer.h"
#include "base/linearalgebra.h"
#include <algorithm>
#include <iostream>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

namespace insight
{

class CSVProgressCollector
    : public ProgressDisplayer
{
    std::map<std::string, int> columnTitles;
    arma::mat data_;

    int column(const std::string& title)
    {
        auto i=columnTitles.find(title);
        if (i!=columnTitles.end())
        {
            return i->second;
        }
        else
        {
            int nci=data_.n_cols;
            data_.insert_cols(nci, 1);
            columnTitles[title]=nci;
            return nci;
        }
    }

    int timeRow(double t)
    {
        if (data_.n_rows==0)
        {
            data_.insert_rows(0,1);
            data_(0,0)=t;
            return 0;
        }

        int j=data_.n_rows-1;
        do
        {
            if (fabs(data_(j,0)-t)<SMALL)
            {
                return j;
            }
            else if (data_(j,0)<t)
            {
                // t larger than last in data, append new
                data_.insert_rows(++j, 1);
                data_(j,0)=t;
                return j;
            }
        } while ( (data_(j--,0)>t)&&(j>=0) );

        throw insight::Exception(
            "no matching value found in time history for %g",
            t,
            valueList_to_string(data_.col(0)).c_str()
            );
    }

public:
    CSVProgressCollector()
        : data_(0,1)
    {}

    void update ( const ProgressState& pi ) override
    {
        double t=pi.first;
        const ProgressVariableList& pvl=pi.second;

        auto r=timeRow(t);

        for ( const ProgressVariableList::value_type& i: pvl )
        {
            auto j=column(i.first);
            data_(r,j)=i.second;
        }
    }
    void logMessage(const std::string& line) override {}
    void setActionProgressValue(const std::string& path, double value) override {}
    void setMessageText(const std::string& path, const std::string& message) override {}
    void finishActionProgress(const std::string& path) override {}
    void reset() override {}




    void writeCSV(std::ostream& os) const
    {
        int nc=data_.n_cols;
        int nr=data_.n_rows;

        std::vector<std::string> t(data_.n_cols+1);
        t[0]="Time";
        for (auto& c: columnTitles)
        {
            insight::assertion(
                c.second<t.size(),
                "expected column id less than %d, got %d for %s",
                t.size(), c.second, c.first.c_str() );
            t[c.second]=c.first;
        }

        os << boost::join(t, ";") << "\n";

        for (int j=0; j<nr; ++j)
        {
            for (int i=0; i<nc; ++i)
            {
                if (i>0)
                {
                    os<<";";
                }
                os<<data_(j,i);
            }
            os<<"\n";
        }

        os<<std::endl;
    }

};

}

using namespace insight;

int main(int argc, char* argv[])
{
    GettextInit gti(GETTEXT_DOMAIN, GETTEXT_OUTPUT_DIR, GettextInit::Application);

    namespace po = boost::program_options;

    // Declare the supported options.
    po::options_description desc(_("Allowed options"));
    desc.add_options()
        ("help,h", _("produce help message"))
        ("input-file,f", po::value< std::vector<std::string> >(),_("Specifies input log file. Multiple files will be concatened."))
        ("output-file,o", po::value< std::string >(),_("Specifies CSV output file."))
        ;

    po::positional_options_description p;
    p.add("input-file", -1);

    auto displayHelp = [&]{
        std::ostream &os = std::cout;

        os << _("Usage:") << std::endl;
        os << "  " << boost::filesystem::path(argv[0]).filename().string() << " ["<<_("options")<<"] " << p.name_for_position(0) << std::endl;
        os << std::endl;
        os << desc << std::endl;
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
        std::cerr << std::endl << _("Could not parse command line")<<": "<< e.what() << std::endl<<std::endl;
        displayHelp();
        exit(-1);
    }

    if (vm.count("help"))
    {
        displayHelp();
        exit(0);
    }


    CSVProgressCollector pd;
    SolverOutputAnalyzer soa(pd);

    for (auto& fn: vm["input-file"].as<std::vector<std::string> >())
    {
        std::ifstream f(fn);
        std::string line;
        while (std::getline(f, line))
        {
            soa.update(line);
        }
    }

    auto* outstream = &std::cout;

    std::unique_ptr<std::ostream> outfstream;
    if (vm.count("output-file"))
    {
        outfstream=
            std::make_unique<std::ofstream>(
                vm["output-file"].as<std::string>() );
        outstream=outfstream.get();
    }

    pd.writeCSV(*outstream);

    return 0;
}
