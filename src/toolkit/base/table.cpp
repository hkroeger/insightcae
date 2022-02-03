#include "table.h"

#include "base/boost_include.h"


namespace insight {




bool Table::isEmpty(const Line& l)
{
    bool empty=true;
    for (const auto& c: l)
    {
        if (!c.empty()) empty=false;
    }
    return empty;
}




Table::Table()
{}




Table::Table(std::istream& is, const std::string& delimiters)
{
    read(is, delimiters);
}




size_t Table::nRows() const
{
    return data_.size();
}



size_t Table::nCols() const
{
    if (nRows()==0)
        return 0;
    else
        return data_[0].size();
}




void Table::resize(size_t rows, size_t cols)
{
    if (rows!=nRows())
        data_.resize(rows);
    for (auto& r: data_)
    {
        if (r.size()!=cols)
        {
            r.resize(cols);
        }
    }
}




void Table::setLine(size_t i, const Line& l)
{
    resize(
                std::max(nRows(), i+1),
                std::max(nCols(), l.size()) );

    for (size_t j=0; j<l.size(); ++j)
    {
        data_[i][j]=l[j];
    }
}




bool Table::isEmptyLine(size_t i) const
{
    return isEmpty(data_[i]);
}




void Table::splitOff(Table& t, size_t firstLine, size_t lastLine) const
{
    t.data_.clear();
    std::copy(
                data_.begin()+firstLine,
                data_.begin()+lastLine+1,
                std::back_inserter(t.data_)
                );
}




arma::mat Table::xy(size_t xcol, size_t ycol, EmptyFieldTreatment eft) const
{
    arma::mat r = arma::zeros(nRows(), 2);
    std::vector<arma::uword> rowsToSkip;
    for (size_t i=0; i<nRows(); ++i)
    {
        auto xf=data_[i][xcol];
        auto yf=data_[i][ycol];
        if (xf.empty() || yf.empty())
        {
            if ( xf.empty() || (eft==SkipRow) )
            {
                rowsToSkip.push_back(i);
            }
            else if (eft==ReplaceWithZero)
            {
                yf="0";
            }
        }
        else
        {
            r(i, 0)=boost::lexical_cast<double>(xf);
            r(i, 1)=boost::lexical_cast<double>(yf);
        }
    }
    //r.shed_rows(arma::uvec(rowsToSkip));
    for (auto j=rowsToSkip.rbegin();j!=rowsToSkip.rend();++j)
    {
        r.shed_row(*j);
    }
    return r;
}




void Table::write(std::ostream& os, char delimiter) const
{
    for (auto& l: data_)
    {
        for (size_t j=0; j<l.size(); ++j)
        {
            if (j>0)
            {
                os << delimiter;
            }
            os << l[j];
        }
        os<<std::endl;
    }
}




void Table::read(std::istream& is, const std::string& delimiters)
{
    std::vector<std::string> lines;

    {
        std::string line;
        while (getline(is, line))
        {
            auto pos = line.find_first_not_of(" ");
            if ( (pos==std::string::npos) || (line[pos]!='#') )
            {
                lines.push_back(line);
            }
        }
    }

    resize(lines.size(), 1);

    for (size_t i=0; i<lines.size(); ++i)
    {
        Line l;
        boost::split(l, lines[i], boost::is_any_of(delimiters));
        setLine(i, l);
    }
}




} // namespace insight
