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



TabularInterval::TabularInterval(const arma::mat& tab,
                                 std::vector<std::string> colNames)
    : Interval( tab(0,0), tab(tab.n_rows-1, 0) ),
      table_(tab),
      colNames_(std::move(colNames))
{}

const std::vector<std::string>& TabularInterval::colNames() const
{
    return colNames_;
}

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
 *
 * Column names are parsed from the last '#'-prefixed header line seen before each block of data.
 * If the column layout changes mid-file (a new header line appears), each row is stored by name
 * and remapped into the final (last-seen) column layout. Missing values become 0.0.
 * If no header line is present, columns are named "column0", "column1", …
 *
 * @param ffp           path to the file to read
 * @param groupByColumn if >= 0, the column at that index is treated as a group key (a string label);
 *                      rows are split into separate matrices by this key, and the column is removed
 *                      from the numeric data. If < 0, all rows go into a single group named "default".
 * @param filterChars   a string of characters to strip from each DATA line before parsing (e.g. parentheses).
 *                      NOT applied to header lines, so brackets etc. remain valid in column names.
 * @param columnNames   optional: if non-null, receives the final column name list on return.
 * @return              map of group name → matrix (rows = time steps, cols = final column layout)
 */
std::map<std::string, arma::mat>
readSingleTabularFile(
    const boost::filesystem::path& ffp,
    int groupByColumn,
    const std::string& filterChars,
    std::vector<std::string>* columnNames
    )
{
    // Each data row is stored as a map from column name to value so that
    // mid-file column layout changes can be handled correctly.
    using NamedRow = std::map<std::string, double>;

    struct GroupData
    {
        std::vector<NamedRow> rows;
    };

    std::map<std::string, GroupData> groups;

    // Active column names (updated whenever a new '#' header line is seen).
    // filterChars is NOT applied here; brackets etc. are valid in column names.
    std::vector<std::string> currentColNames;
    std::vector<std::string> lastColNames;   // final layout = last header seen
    bool anyHeaderSeen = false;
    int  globalMaxCols = 0;                  // fallback when no header is present


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

        if ( starts_with ( line, "#" ) )
        {
            // --- Header line: parse column names ---
            // Strip the leading '#' but do NOT apply filterChars.
            std::string headerLine = line.substr(1);
            replace_all(headerLine, "\t", " ");
            string hl_org;
            do {
                hl_org = headerLine;
                replace_all(headerLine, "  ", " ");
            } while (hl_org != headerLine);
            trim(headerLine);

            if (!headerLine.empty())
            {
                std::vector<std::string> names;
                split(names, headerLine, boost::is_any_of(" "));

                // Remove the group-key slot so indices match the numeric data.
                if (groupByColumn >= 0
                    && groupByColumn < static_cast<int>(names.size()))
                {
                    names.erase(names.begin() + groupByColumn);
                }

                currentColNames = names;
                lastColNames    = names;
                anyHeaderSeen   = true;
            }
        }
        else
        {
            // --- Data line ---
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

            globalMaxCols = std::max(globalMaxCols,
                                     static_cast<int>(fieldValues.size()));

            // Map each value to its column name (using the currently active header).
            // Values beyond the header width are named "column<i>" as a fallback.
            NamedRow namedRow;
            for (size_t i = 0; i < fieldValues.size(); ++i)
            {
                std::string colName = (i < currentColNames.size())
                    ? currentColNames[i]
                    : "column" + std::to_string(i);
                namedRow[colName] = fieldValues[i];
            }

            groups[groupName].rows.push_back(namedRow);
        }
    }

    // Determine the final column layout (last header seen, or auto-generated).
    std::vector<std::string> finalColNames;
    if (anyHeaderSeen)
    {
        finalColNames = lastColNames;
    }
    else
    {
        for (int i = 0; i < globalMaxCols; ++i)
            finalColNames.push_back("column" + std::to_string(i));
    }

    if (columnNames)
        *columnNames = finalColNames;

    const auto nCols = static_cast<arma::uword>(finalColNames.size());

    std::map<std::string, arma::mat> result;
    // Convert into arma::mat's, remapping by column name into the final layout.
    // Columns absent in a given row's segment stay 0.0.
    for (auto& [gn, gd]: groups)
    {
        arma::mat m = arma::zeros(gd.rows.size(), nCols);

        for (arma::uword i = 0; i < gd.rows.size(); ++i)
        {
            for (arma::uword j = 0; j < nCols; ++j)
            {
                auto it = gd.rows[i].find(finalColNames[j]);
                if (it != gd.rows[i].end())
                    m(i, j) = it->second;
            }
        }
        result[gn] = m;
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

            std::vector<std::string> fileColNames;
            auto fileData = readSingleTabularFile(ffp, groupByColumn, filterChars, &fileColNames);
            for (const auto& rg: fileData)
            {
                intervals[rg.first].insert(
                    lastWriteTime,
                    std::make_shared<TabularInterval>(
                        rg.second, fileColNames ) );
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
        // Determine the final column layout from the newest non-ignored interval.
        // intervals() is sorted ascending by write-time key, so rbegin = newest.
        std::vector<std::string> finalColNames;
        for (auto it = giv.second.intervals().rbegin();
             it != giv.second.intervals().rend(); ++it)
        {
            const auto& tiv = dynamic_cast<const TabularInterval&>(*it->second);
            if (!tiv.toBeIgnored())
            {
                finalColNames = tiv.colNames();
                break;
            }
        }

        const auto nCols = static_cast<arma::uword>(finalColNames.size());
        arma::mat& rows = rdata[giv.first];

        for (const auto& iv: giv.second.intervals())
        {
            const auto& tiv = dynamic_cast<const TabularInterval&>(*iv.second);
            if (tiv.toBeIgnored()) continue;
            auto tc = tiv.clippedTable();
            if (tc.n_rows == 0) continue;

            // Remap tc columns into the final layout order by name.
            arma::mat mapped = arma::zeros(tc.n_rows, nCols);
            const auto& srcNames = tiv.colNames();
            for (arma::uword j = 0; j < nCols; ++j)
            {
                auto sit = std::find(srcNames.begin(), srcNames.end(), finalColNames[j]);
                if (sit != srcNames.end())
                {
                    arma::uword srcCol = static_cast<arma::uword>(
                        std::distance(srcNames.begin(), sit));
                    mapped.col(j) = tc.col(srcCol);
                }
                // else: column absent in this file → stays 0.0
            }

            rows = (rows.n_rows == 0)
                ? mapped
                : arma::mat(arma::join_cols(rows, mapped));
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
