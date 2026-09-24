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


#include "base64.h"
#include "base/fileio.h"
#include "base/exception.h"

#include <sstream>

#include <boost/format.hpp>
#include "boost/archive/iterators/base64_from_binary.hpp"
#include "boost/archive/iterators/binary_from_base64.hpp"
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>

namespace insight
{


const std::string base64_padding[] = {"", "==","="};


std::string base64_encode(const std::string& s)
{
  insight::CurrentExceptionContext ex(
        boost::str(boost::format("performing base64 encode of buffer of size %d")
                   % s.size() )
        );

  namespace bai = boost::archive::iterators;

  std::stringstream os;

  // convert binary values to base64 characters
  typedef bai::base64_from_binary
  // retrieve 6 bit integers from a sequence of 8 bit bytes
  <bai::transform_width<const char *, 6, 8> > base64_enc; // compose all the above operations in to a new iterator

  std::copy(base64_enc(s.c_str()), base64_enc(s.c_str() + s.size()),
            std::ostream_iterator<char>(os));

  os << base64_padding[s.size() % 3];
  return os.str();
}


char* base64_encode(
    rapidxml::xml_document<> &doc,
    const std::string& file_content_ )
{
    using namespace boost::archive::iterators;
    typedef
        //          insert_linebreaks<         // insert line breaks every 72 characters
        base64_from_binary<    // convert binary values to base64 characters
            transform_width<   // retrieve 6 bit integers from a sequence of 8 bit bytes
                const char*, 6, 8
                >
            >
            //              ,72 >
            base64_enc; // compose all the above operations in to a new iterator

    char tail[3] = {0,0,0};
    size_t len=file_content_.size();
    unsigned int one_third_len = len/3;
    unsigned int len_rounded_down = one_third_len*3;
    unsigned int j = len_rounded_down + one_third_len;
    unsigned int base64length = ((4 * file_content_.size() / 3) + 3) & ~3;

    auto *xml_content = doc.allocate_string(0, base64length+1);
    std::copy(
        base64_enc(file_content_.c_str()),
        base64_enc(file_content_.c_str()+len_rounded_down),
        xml_content
        );

    if (len_rounded_down != len)
    {
        unsigned int i=0;
        for(; i < len - len_rounded_down; ++i)
        {
            tail[i] = file_content_[len_rounded_down+i];
        }

        std::copy(base64_enc(tail), base64_enc(tail + 3), xml_content + j);

        for(i=len + one_third_len + 1; i < j+4; ++i)
        {
            xml_content[i] = '=';
        }
    }

    xml_content[base64length]=0;
    return xml_content;
}



std::string
base64_encode(
    const boost::filesystem::path& f )
{
  std::string contents_raw;
  readFileIntoString(f, contents_raw);
  return base64_encode(contents_raw);
}




std::shared_ptr<std::string> base64_decode(const std::string& sourceBuffer)
{
    std::shared_ptr<std::string> targetBuffer;
    base64_decode(sourceBuffer, targetBuffer);
    return targetBuffer;
}


void
base64_decode(const char *src, size_t size,
    std::shared_ptr<std::string>& targetBuffer  )
{
//  char *src = a->value();
//  size_t size = a->value_size();

  if ((size>0) && src[size - 1] == '=')
  {
    --size;
    if ((size>0) && src[size - 1] == '=')
    {
       --size;
    }
  }

  if (size == 0)
  {
    if (targetBuffer) targetBuffer->clear();
  }
  else
  {
    using namespace boost::archive::iterators;

    typedef
      transform_width<
       binary_from_base64<
        remove_whitespace<
         const char*
        >
       >,
       8, 6
      >
      base64_dec;

      targetBuffer.reset(new std::string( base64_dec(src), base64_dec(src + size) ));
  }
}

void base64_decode(const std::string& sourceBuffer, std::shared_ptr<std::string>& targetBuffer)
{
    base64_decode(
                sourceBuffer.c_str(),
                sourceBuffer.size(),
                targetBuffer );
}


}
