#ifndef INSIGHT_TABLE_H
#define INSIGHT_TABLE_H

#include <iostream>
#include <vector>

#include "base/linearalgebra.h"




namespace insight {




class Table;




std::ostream& operator<<(std::ostream& os, const Table& t);
std::istream& operator>>(std::istream& is, Table& t);




class Table
{
    friend std::ostream& operator<<(std::ostream& os, const Table& t);
    friend std::istream& operator>>(std::istream& is, Table& t);

public:
    typedef std::string Field;
    typedef std::vector<Field> Line;
    typedef std::vector<Line> Data;

    static bool isEmpty(const Line& l);

protected:
    Data data_;

public:
    Table();
    Table(std::istream& is, const std::string& delimiters=";");

    size_t nRows() const;
    size_t nCols() const;

    void resize(size_t rows, size_t cols);
    void setLine(size_t i, const Line& l);
    bool isEmptyLine(size_t i) const;
    void splitOff(Table& t, size_t firstLine, size_t lastLine) const;

    template<class OIterator>
    void splitIntoDataSets(OIterator dsi) const
    {
        size_t i0=0, i1=i0, emptyLines=0;

        auto splitOffAndStore = [&]()
        {
            Table t;
            splitOff(t, i0, i1);
            *dsi=t;
            dsi++;
        };

        for (size_t i=0; i<nRows(); ++i)
        {
            if (isEmptyLine(i))
            {
                ++emptyLines;
            }
            else
            {
                if ( emptyLines>1 || (i==nRows()-1) )
                {
                    if (i==nRows()-1) i1=i;
                    splitOffAndStore();
                    i0=i;
                    emptyLines=0;
                }

                i1=i;
            }
        }
    }

    enum EmptyFieldTreatment {
        SkipRow,
        ReplaceWithZero
    };

    arma::mat xy(size_t xcol, size_t ycol, EmptyFieldTreatment eft = EmptyFieldTreatment::SkipRow) const;
    arma::mat mat() const;

    void write(std::ostream& os, char delimiter='\t') const;
    void read(std::istream& is, const std::string& delimiters=";");
};




} // namespace insight

#endif // INSIGHT_TABLE_H
