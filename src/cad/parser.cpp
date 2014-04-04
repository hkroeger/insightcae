/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "parser.h"
#include "boost/locale.hpp"

namespace insight {
namespace cad {
  
namespace parser {  
  
// solidmodel import(const boost::filesystem::path& filepath)
// {
//   cout << "reading model "<<filepath<<endl;
//   return solidmodel(new SolidModel(filepath));
// }

}
using namespace parser;


bool parseISCADModelFile(const boost::filesystem::path& fn, parser::model& m)
{
  std::wifstream f(fn.c_str());
  return parseISCADModelStream(f, m);
}

bool parseISCADModelStream(std::wistream& in, parser::model& m)
{
  std::wstring contents_raw;
  in.seekg(0, std::wios::end);
  contents_raw.resize(in.tellg());
  in.seekg(0, std::wios::beg);
  in.read(&contents_raw[0], contents_raw.size());
  //in.close();
  std::string contents=boost::locale::conv::from_utf(contents_raw, "Latin1");
  cout<<contents<<endl;
  return parser::parseISCADModel< ISCADParser<std::string::iterator> >(contents.begin(), contents.end(), m);
}


}
}