#include "readpostprocessingfiles.h"

#include "openfoam/openfoamtools.h"
#include "base/translations.h"

#include "boost/iterator_adaptors.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;



namespace insight {



std::unique_ptr<std::pair<time_t,boost::filesystem::path> >
newestOutputFile(const path &expectedFName)
{
    // find newest out file
    std::map<std::time_t, boost::filesystem::path> candidates;

    auto fileNameBase = expectedFName.filename().stem().string();
    auto fileNameExt = expectedFName.filename().extension().string();
    auto pd = expectedFName.parent_path();

    boost::regex expr(fileNameBase+"(|_.*)"+fileNameExt);

    for ( directory_iterator itr( pd );
         itr != directory_iterator(); ++itr )
    {
        if (!is_directory(itr->status()))
        {
            auto cp = itr->path();
            if (boost::regex_match(cp.filename().string(), expr))
            {
                candidates[last_write_time(cp)]=cp;
            }
        }
    }

    if (candidates.size()<1)
    {
        insight::Warning( _("no valid output file was found in time directory %s!"), pd.c_str());
    }
    else
    {
        // select newest (last in list)
        auto selectedCandidate = candidates.rbegin();
        return std::make_unique<std::pair<time_t,boost::filesystem::path> >(*selectedCandidate);
    }

    return nullptr;
}



TabularInterval::TabularInterval(const arma::mat& tab)
    : Interval( tab(0,0), tab(tab.n_rows-1, 0) ),
    table_(tab)
{}

arma::mat TabularInterval::clippedTable() const
{
    CurrentExceptionContext ex(
        str(format("clipping table %g < t <%g") % A() % clippedB())
        );
    insight::assertion( !toBeIgnored(), "no data!" );
    return arma::mat( table_.rows( find(table_.col(0)>A() && table_.col(0)<clippedB()) ) );
}



/**
 * @brief readSingleTabularFile
 * Reads a single whitespace-delimited text file and returns its rows grouped into named arma::mat matrices.
 * @param ffp
 * path to the file to read
 * @param groupByColumn
 * if >= 0, the column at that index is treated as a group key (a string label); rows are split into separate
  matrices by this key, and the column is removed from the numeric data. If < 0, all rows go into a single group named "default".
 * @param filterChars
 * a string of characters to strip from each line before parsing (e.g. parentheses)
 * @return
 */
std::map<std::string, arma::mat>
readSingleTabularFile(
    const boost::filesystem::path& ffp,
    int groupByColumn,
    const std::string& filterChars
    )
{
    struct GroupData
    {
        int maxCols=-1;
        std::vector<std::vector<double> > rows;
    };

    std::map<
        std::string,
        GroupData
        > groups;


    std::ifstream f( ffp.string() );
    if (!f)
        throw insight::Exception(
            "Failed to open file "+ffp.string()+"!");

    int lineNo=0;
    std::string line;
    while ( getline ( f, line ) )
    {
        lineNo++;
        CurrentExceptionContext ex(
            insight::VerbosityLevel::Loops,
            str(format("reading line %d of file %s")
                % lineNo % ffp.string() ));

        trim(line);

        if ( !starts_with ( line, "#" ) )
        {
            for (auto c: filterChars)
                erase_all(line, std::string(1, c));
            replace_all(line, "\t", " ");

            // eliminate double spaces
            string line_org;
            do {
                line_org=line;
                replace_all(line, "  ", " ");
            } while (line_org!=line);

            std::vector<string> fields;
            split(fields, line,  boost::is_any_of(" "));

            std::string groupName="default";
            if (groupByColumn>=0)
            {
                groupName=fields[groupByColumn];
                fields.erase(fields.begin()+groupByColumn);
            }

            std::vector<double> fieldValues;
            boost::transform
            (
                fields,
                std::back_inserter(fieldValues),
                [](const std::string& t)
                {
                    return toNumber<double>(t);
                }
            );

            if (fieldValues.size()<2)
            {
                throw insight::Exception(
                    _("invalid data: expected at least two columns (time + 1 data), got: %s"),
                    line.c_str() );
            }

            auto& gd=groups[groupName];
            gd.maxCols=std::max<int>(
                gd.maxCols, fieldValues.size());
            gd.rows.push_back(fieldValues);
        }
    }

    std::map<std::string, arma::mat> result;
    // convert into arma::mat's
    for (auto& [gn,gd]: groups)
    {
        arma::mat m = arma::zeros(
            gd.rows.size(), gd.maxCols);

        for (long int i=0; i<gd.rows.size(); ++i)
        {
            m.row(i)=appendZeroColsIfNeeded(
                arma::mat(
                    gd.rows[i].data(),
                    1, gd.rows[i].size() ),
                gd.maxCols);
        }
        result[gn]=m;
    }
    return result;
}



arma::mat readAndCombineTabularFiles
(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& caseLocation,
    const std::string& FOName,
    const std::string& fileNamePattern,
    const std::string& filterChars,
    const std::string& regionName
)
{
    return readAndCombineGroupedTabularFiles(
               cm, caseLocation,
               FOName, fileNamePattern,
               -1, filterChars, regionName )
        .begin()->second;
}



std::map<std::string,arma::mat>
readAndCombineGroupedTabularFiles
(
    const OpenFOAMCase& cm,
    const boost::filesystem::path& caseLocation,
    const std::string& FOName,
    const std::string& fileNamePattern,
    int groupByColumn,
    const std::string& filterChars,
    const std::string& regionName
)
{
    CurrentExceptionContext ex(
        _("reading output files %s for function object %s (filtering out any of '%s')"),
        fileNamePattern.c_str(), FOName.c_str(), filterChars.c_str() );


    path fp;
    if ( cm.OFversion() <170 )
    {
        fp = absolute ( caseLocation ) / FOName;
    }
    else
    {
        fp = absolute ( caseLocation ) / "postProcessing";
        if (!regionName.empty()) fp = fp / regionName;
        fp = fp/ FOName;
    }

    if (!exists(fp))
    {
        throw insight::Exception(
            _("data path %s of function object %s does not exist!"),
            fp.c_str(), FOName.c_str() );
    }

    std::map<std::string, OverlappingIntervals> intervals;

    // find all time directories
    auto tdl = listTimeDirectories ( fp );

    std::time_t lastWriteTime=0;
    for ( const auto& td: tdl ) // loop over all times, start
    {
        auto newest = newestOutputFile(td.second/fileNamePattern);
        if (newest)
        {
            auto ffp = newest->second;
            if (lastWriteTime > newest->first)
            {
                insight::Warning(
                    _("Possible inconsistency in solver output data detected!"
                      "File %s from time directory %s"
                      " was created before the output file of the previous time directory."),
                    ffp.filename().c_str(), td.second.c_str()
                    );
            }
            lastWriteTime=newest->first;

            auto fileData = readSingleTabularFile(ffp, groupByColumn, filterChars);
            for (const auto& rg: fileData)
            {
                intervals[rg.first].insert(
                    lastWriteTime,
                    std::make_shared<TabularInterval>(
                        rg.second ) );
            }
        }
    }

    for (auto& iv: intervals)
    {
        iv.second.clipIntervals();
    }

    std::map<std::string, arma::mat> rdata;

    for (const auto& giv: intervals)
    {
        auto& rows=rdata[giv.first];
        for (const auto& iv: giv.second.intervals())
        {
            const auto& tiv = dynamic_cast<const TabularInterval&>(*iv.second);
            if (rows.n_rows==0)
            {
                rows=tiv.clippedTable();
            }
            else
            {
                auto tc=tiv.clippedTable();
                int minCols = std::max<int>(
                    rows.n_cols, tc.n_cols);
                rows=arma::join_cols(
                    appendZeroColsIfNeeded(rows, minCols),
                    appendZeroColsIfNeeded(tc, minCols)
                );
            }
        }
    }

    //  for (const auto& rg: rows)
    //  {
    //    const auto& crows=rg.second;

    //    arma::mat data;

    //    if (crows.size()>0)
    //    {
    //      size_t nf = crows.begin()->second.size();
    //      data.resize(crows.size(), nf);

    //      arma::uword k=0;
    //      for (auto r=crows.begin(); r!=crows.end(); ++r)
    //      {

    //        if ( nf != r->second.size() )
    //        {
    //          throw insight::Exception(
    //                str(format("Invalid data for time %g: expected %d data columns, got %d.")
    //                    % r->first % nf % r->second.size()
    //                    ));
    //        }

    //        for (arma::uword j=0; j<nf; j++)
    //        {
    //          data(k,j)=r->second[j];
    //        }

    //        ++k;
    //      }
    //    }

    //    rdata[rg.first]=data;
    //  }

    return rdata;
}



} // namespace insight
