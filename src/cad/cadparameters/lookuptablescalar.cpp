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
#include "cadfeature.h"
#include "datum.h"
#include "parser.h"
#include "lookuptablescalar.h"
#include "base/tools.h"
#include "base/boost_include.h"
#include "boost/algorithm/string.hpp"

using namespace boost;

namespace insight 
{
namespace cad 
{

LookupTableScalar::LookupTableScalar(const LookupTableScalar& o, TreeCloneMap& tcm)
   : name_(o.name_),
    keycol_(o.keycol_),
    CL(keyval_),
    depcol_(o.depcol_),
    select_nearest_(o.select_nearest_)
{}
  
LookupTableScalar::LookupTableScalar
(
  const std::string& name, 
  const std::string& keycol, 
  ScalarPtr keyval, 
  const std::string& depcol,
  bool select_nearest
)
: name_(name),
  keycol_(keycol),
  keyval_(keyval),
  depcol_(depcol),
  select_nearest_(select_nearest)
{}

size_t LookupTableScalar::calcHash() const
{
    ParameterListHash h;
    h+=name_;
    h+=keycol_;
    h+=keyval_;
    h+=depcol_;
    h+=select_nearest_;
    return h.getHash();
}

double LookupTableScalar::calcValue() const
{
    boost::filesystem::path fp = sharedModelFilePath(name_+".csv");
    
    if (!boost::filesystem::exists(fp))
    {
        throw insight::Exception(
            "lookup table %s does not exists!", fp.c_str());
    }
    
    std::ifstream f( fp.c_str() );
    std::string line;
    std::vector<std::string> cols;

    do
    {
        getline(f, line);
    }
    while (boost::algorithm::starts_with(line, "#"));

    boost::split(cols, line, boost::is_any_of(";,"));
    size_t nc=cols.size();
    
    auto keycolit=find(cols.begin(), cols.end(), keycol_);

    if (keycolit==cols.end())
        throw insight::Exception(
            "key column %s not found in lookup table %s!",
            keycol_.c_str(), fp.c_str() );

    int ik= keycolit - cols.begin();
    
    auto depcolit=find(cols.begin(), cols.end(), depcol_);

    if (depcolit==cols.end())
        throw insight::Exception(
            "column with depending value %s not found in lookup table %s!",
            depcol_.c_str(), fp.c_str() );

    int id=depcolit - cols.begin();
    
    double tkeyval=*keyval_;
    int ln=1;
    double best_mq=DBL_MAX;
    bool found=false;
    double rvalue=DBL_MAX;
    
    while (!f.eof())
    {
        ln++;
        getline(f, line);
        
        if (line.size()>0)
        {
            boost::split(cols, line, boost::is_any_of(";,"));
            
            if (nc!=cols.size())
            {
                throw insight::Exception(
                        "lookup table %s: unexpected number of columns (%d, expected %d) in line %d!",
                        fp.c_str(), cols.size(), nc, ln );
            }
            
            double kv;
            try 
            {
                kv=toNumber<double>(cols[ik]);
            }
            catch (...)
            {
                throw insight::Exception(
                        "lookup table %s: could not read key value in line %d! (found %s)",
                        fp.c_str(), ln, cols[ik].c_str() );
            }
            
            double mq=fabs(kv-tkeyval);
            if (!select_nearest_)
            {
                if (mq>1e-6) mq=DBL_MAX;
            }
            
            if (mq<best_mq)
            {
                best_mq=mq;
                
                double dv;
                try 
                {
                    dv=toNumber<double>(cols[id]);
                }
                catch (...)
                {
                    throw insight::Exception(
                        "lookup table %s: could not read value in line %d! (found %s)",
                         fp.c_str(), ln, cols[id].c_str() );
                }
                rvalue=dv;
                found=true;
            }
        }
    }

    if (found)
        return rvalue;

    throw insight::Exception(
        "Table lookup of value %g in column %s failed!", tkeyval, keycol_.c_str());
    
    return 0.0;
}

}
}
