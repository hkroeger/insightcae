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

#include "parser_errors.h"
#include "base/translations.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>

#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "boost/variant/get.hpp"

namespace insight {
namespace cad {
namespace parser {


namespace {

bool isUtf8Continuation(char c)
{
    return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

bool isIdentStart(char c)
{
    return std::isalpha(static_cast<unsigned char>(c));
}

bool isIdentChar(char c)
{
    return std::isalnum(static_cast<unsigned char>(c)) || c=='_';
}

std::size_t utf8Length(const std::string& text, std::size_t begin, std::size_t end)
{
    std::size_t n=0;
    for (std::size_t i=begin; i<end && i<text.size(); ++i)
    {
        if (!isUtf8Continuation(text[i])) ++n;
    }
    return n;
}

std::string joinAlternatives(const std::vector<std::string>& items)
{
    std::ostringstream os;
    for (std::size_t i=0; i<items.size(); ++i)
    {
        if (i>0)
        {
            // TRANSLATORS: conjunction for the last item of a list of alternatives, e.g. "',', ')' or number"
            os << ( i+1==items.size() ? std::string(" ")+_("or")+" " : std::string(", ") );
        }
        os << items[i];
    }
    return os.str();
}

void collectExpected(
    const boost::spirit::info& what,
    std::vector<std::string>& result,
    int depth )
{
    using boost::spirit::info;
    using boost::spirit::utf8_string;

    if (depth>20) return;

    const std::string& tag = what.tag;

    // parsers which never are the actual reason of a failure
    static const std::set<std::string> ignored = {
        "unnamed-rule", "eps", "attr", "not-predicate", "and-predicate",
        "kleene", "optional"
    };
    if (ignored.count(tag)) return;

    if (tag=="literal-char" || tag=="literal-string")
    {
        if (const auto* s = boost::get<utf8_string>(&what.value))
            result.push_back("'"+*s+"'");
        return;
    }

    // not static: translations shall follow the current locale
    const std::map<std::string, std::string> primitives = {
        {"alpha", _("letter")},
        {"alnum", _("letter or digit")},
        {"digit", _("digit")},
        {"real", _("number")},
        {"int", _("integer")},
        {"uint", _("integer")},
        {"char", _("character")},
        {"char-set", _("character")},
        {"char-range", _("character")},
        {"eol", _("end of line")},
        {"eoi", _("end of input")},
        {"symbols", _("keyword")}
    };
    auto p = primitives.find(tag);
    if (p!=primitives.end())
    {
        result.push_back(p->second);
        return;
    }

    if (tag=="alternative")
    {
        if (const auto* l = boost::get<std::list<info> >(&what.value))
            for (const auto& alt: *l)
                collectExpected(alt, result, depth+1);
        return;
    }

    // everything else: either a named rule or a directive/operator
    // whose first sub-parser is the one, which failed
    if (boost::get<info::nil_>(&what.value))
    {
        result.push_back(tag); // name of rule
    }
    else if (const auto* i = boost::get<info>(&what.value))
    {
        collectExpected(*i, result, depth+1);
    }
    else if (const auto* pr = boost::get<std::pair<info,info> >(&what.value))
    {
        collectExpected(pr->first, result, depth+1);
    }
    else if (const auto* l = boost::get<std::list<info> >(&what.value))
    {
        // sequence: the first non-trivial element failed
        for (const auto& e: *l)
        {
            auto n=result.size();
            collectExpected(e, result, depth+1);
            if (result.size()>n) break;
        }
    }
    else
    {
        result.push_back(tag);
    }
}

std::size_t editDistance(const std::string& a, const std::string& b)
{
    std::vector<std::size_t> prev(b.size()+1), cur(b.size()+1);
    for (std::size_t j=0; j<=b.size(); ++j) prev[j]=j;
    for (std::size_t i=1; i<=a.size(); ++i)
    {
        cur[0]=i;
        for (std::size_t j=1; j<=b.size(); ++j)
        {
            cur[j]=std::min({
                prev[j]+1,
                cur[j-1]+1,
                prev[j-1] + (a[i-1]==b[j-1] ? 0 : 1)
            });
        }
        std::swap(prev, cur);
    }
    return prev[b.size()];
}

} // anonymous namespace




SourceLocation locateInSource(const std::string& text, std::size_t offset)
{
    SourceLocation loc;
    offset=std::min(offset, text.size());

    std::size_t lineStart=0;
    loc.line=1;
    for (std::size_t i=0; i<offset; ++i)
    {
        if (text[i]=='\n')
        {
            ++loc.line;
            lineStart=i+1;
        }
    }
    loc.column = int(utf8Length(text, lineStart, offset)) + 1;

    std::size_t lineEnd=text.find('\n', lineStart);
    if (lineEnd==std::string::npos) lineEnd=text.size();
    loc.lineText=text.substr(lineStart, lineEnd-lineStart);
    if (!loc.lineText.empty() && loc.lineText.back()=='\r')
        loc.lineText.pop_back();

    return loc;
}




std::size_t skipWhitespaceAndComments(const std::string& text, std::size_t offset)
{
    std::size_t i=offset;
    while (i<text.size())
    {
        if (std::string(" \t\n\r\f\v").find(text[i])!=std::string::npos)
        {
            ++i;
        }
        else if (text.compare(i, 2, "/*")==0)
        {
            auto e=text.find("*/", i+2);
            i = (e==std::string::npos) ? text.size() : e+2;
        }
        else if (text.compare(i, 2, "//")==0 || text[i]=='#')
        {
            auto e=text.find('\n', i);
            i = (e==std::string::npos) ? text.size() : e;
        }
        else
            break;
    }
    return i;
}




std::size_t tokenLength(const std::string& text, std::size_t offset)
{
    if (offset>=text.size()) return 0;

    std::size_t i=offset;
    char c=text[i];

    if (isIdentStart(c))
    {
        while (i<text.size() && isIdentChar(text[i])) ++i;
    }
    else if (std::isdigit(static_cast<unsigned char>(c))
             || (c=='.' && i+1<text.size()
                 && std::isdigit(static_cast<unsigned char>(text[i+1]))) )
    {
        while (i<text.size()
               && (std::isdigit(static_cast<unsigned char>(text[i])) || text[i]=='.'))
            ++i;
        if (i<text.size() && (text[i]=='e' || text[i]=='E'))
        {
            ++i;
            if (i<text.size() && (text[i]=='+' || text[i]=='-')) ++i;
            while (i<text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) ++i;
        }
    }
    else if (c=='\'' || c=='"')
    {
        auto e=text.find_first_of(std::string(1,c)+"\n", i+1);
        if (e==std::string::npos) i=text.size();
        else if (text[e]==c) i=e+1;
        else i=e;
    }
    else
    {
        ++i;
        while (i<text.size() && isUtf8Continuation(text[i])) ++i;
    }

    return i-offset;
}




std::string identifierAt(const std::string& text, std::size_t offset)
{
    if (offset<text.size() && isIdentStart(text[offset]))
        return text.substr(offset, tokenLength(text, offset));
    return std::string();
}




std::string describeToken(const std::string& text, std::size_t offset)
{
    if (offset>=text.size())
        return _("end of input");

    std::string tok=text.substr(offset, tokenLength(text, offset));
    if (tok.size()>30)
        tok=tok.substr(0, 27)+"...";

    if (tok[0]=='\'' || tok[0]=='"')
        return str(boost::format(_("string %s")) % tok);
    return "'"+tok+"'";
}




std::vector<std::string> expectedAlternatives(const boost::spirit::info& what)
{
    std::vector<std::string> items;
    collectExpected(what, items, 0);

    // remove duplicates, keep order
    std::vector<std::string> unique;
    for (const auto& i: items)
    {
        if (std::find(unique.begin(), unique.end(), i)==unique.end())
            unique.push_back(i);
    }
    return unique;
}




std::string describeAlternatives(std::vector<std::string> alternatives)
{
    if (alternatives.empty())
        return _("something else");

    const std::size_t maxItems=8;
    if (alternatives.size()>maxItems)
    {
        alternatives.resize(maxItems);
        alternatives.push_back("...");
    }

    return joinAlternatives(alternatives);
}




std::string describeExpected(const boost::spirit::info& what)
{
    return describeAlternatives(expectedAlternatives(what));
}




std::vector<std::string> similarNames(
    const std::string& name,
    const std::set<std::string>& candidates,
    std::size_t maxSuggestions )
{
    std::string lname=boost::algorithm::to_lower_copy(name);
    std::size_t maxDist = lname.size()<=2 ? 1 : ( lname.size()<=5 ? 2 : 3 );

    std::vector<std::pair<std::size_t, std::string> > matches;
    for (const auto& c: candidates)
    {
        if (c==name) continue;
        auto d=editDistance(lname, boost::algorithm::to_lower_copy(c));
        if (d<=maxDist)
            matches.push_back({d, c});
    }
    std::sort(matches.begin(), matches.end());

    std::vector<std::string> result;
    for (const auto& m: matches)
    {
        if (result.size()>=maxSuggestions) break;
        result.push_back(m.second);
    }
    return result;
}




std::string didYouMean(const std::vector<std::string>& suggestions)
{
    if (suggestions.empty())
        return std::string();

    std::vector<std::string> quoted;
    for (const auto& s: suggestions)
        quoted.push_back("'"+s+"'");
    // TRANSLATORS: %s is a list of names, e.g. "'a', 'b' or 'c'"
    return str(boost::format(_("Did you mean %s?")) % joinAlternatives(quoted));
}




std::string missingSemicolonHint(const std::string& text, std::size_t offset)
{
    auto eol=text.find('\n', offset);
    if (eol==std::string::npos)
        return std::string();

    if (text.substr(offset, eol-offset).find(';')!=std::string::npos)
        return std::string();

    // does a new statement start on one of the following lines?
    auto next=skipWhitespaceAndComments(text, eol);
    auto id=identifierAt(text, next);
    if (id.empty())
        return std::string();

    auto after=skipWhitespaceAndComments(text, next+id.size());
    if ( text.compare(after, 1, ":")==0
         || text.compare(after, 1, "=")==0
         || text.compare(after, 2, "?=")==0
         || text.compare(after, 2, "->")==0 )
    {
        return str(boost::format(_("Maybe the ';' at the end of line %d is missing?"))
                   % locateInSource(text, eol).line);
    }

    return std::string();
}




std::string formatDiagnostic(
    const boost::filesystem::path& file,
    const std::string& text,
    int from_pos, int to_pos,
    const std::string& message,
    const std::vector<std::string>& notes )
{
    std::ostringstream os;

    bool haveLocation = from_pos>=0 && std::size_t(from_pos)<=text.size();

    SourceLocation loc;
    if (haveLocation)
    {
        loc=locateInSource(text, from_pos);
        if (!file.empty()) os << file.string() << ":";
        os << loc.line << ":" << loc.column << ": ";
    }
    else if (!file.empty())
    {
        os << file.string() << ": ";
    }
    os << _("error") << ": " << message;

    if (haveLocation)
    {
        std::string lineNo=std::to_string(loc.line);
        std::string gutter(lineNo.size(), ' ');

        os << "\n " << lineNo << " | " << loc.lineText;

        // caret line: mirror tabs, so that the caret is aligned
        std::size_t lineStart=std::size_t(from_pos);
        while (lineStart>0 && text[lineStart-1]!='\n') --lineStart;

        std::string marker;
        for (std::size_t i=lineStart; i<std::size_t(from_pos); ++i)
        {
            if (text[i]=='\t') marker+='\t';
            else if (!isUtf8Continuation(text[i])) marker+=' ';
        }
        marker+='^';
        std::size_t lineEnd=lineStart+loc.lineText.size();
        std::size_t end=std::min(std::size_t(std::max(to_pos, from_pos)), lineEnd);
        std::size_t tildes = utf8Length(text, std::size_t(from_pos), end);
        if (tildes>1) marker+=std::string(tildes-1, '~');

        os << "\n " << gutter << " | " << marker;
    }

    for (const auto& n: notes)
    {
        if (!n.empty())
            os << "\n  " << n;
    }

    return os.str();
}


} // namespace parser
} // namespace cad
} // namespace insight
