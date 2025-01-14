#ifndef SAMPLING_H
#define SAMPLING_H

#include <string>
#include <vector>

#include "base/supplementedinputdata.h"
#include "openfoam/openfoamcase.h"
#include "base/boost_include.h"




namespace insight {



namespace sampleOps
{

class set
{

public:
#include "sampling__sampleOps_set__Parameters.h"
/*
PARAMETERSET>>> sampleOps_set Parameters

name = string "unnamed" "Name of the set"

createGetters
<<<PARAMETERSET
*/


public:
    set(ParameterSetInput ip = ParameterSetInput() );
    virtual ~set();

    virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const =0;

    inline const std::string& name() const { return p().name; }

    virtual set* clone() const =0;
};


#ifndef SWIG
inline set* new_clone(const set& op)
{
    return op.clone();
}
#endif

/**
 * Creates a cyclic patch or cyclic patch pair (depending on OF version)
 * from two other patches
 */
struct ColumnInfo
{
    size_t col, ncmpt;
};

//typedef std::map<std::string, ColumnInfo > ColumnDescription;
class ColumnDescription
    : public std::map<std::string, ColumnInfo >
{
public:
    inline bool contains(const std::string& name) const { return this->find(name) != this->end(); }
    inline long int colIndex(const std::string& name) const
    {
        auto it = this->find(name);
        if (it != this->end() )
            return it->second.col;
        else
            return -1;
    }
};





class line
    : public set
{
public:
#include "sampling__sampleOps_line__Parameters.h"
    /*
PARAMETERSET>>> sampleOps_line Parameters
inherits set::Parameters

points = vector (0 0 0) "Matrix of points"

createGetters
<<<PARAMETERSET
*/

public:
    line(ParameterSetInput ip = ParameterSetInput() );
    virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
    virtual set* clone() const;

    /**
   * reads the sampled data from the files
   * OF writes different files for scalars, vectors tensors.
   * They are all read and combined into a single matrix in the above order by column.
   * Only the last results in the last time folder is returned
   */
    arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location,
                          ColumnDescription* coldescr=NULL,
                          const std::string& time="" // empty string means latest
                          ) const;
};




class uniformLine
    : public set
{
public:
#include "sampling__sampleOps_uniformLine__Parameters.h"
    /*
PARAMETERSET>>> sampleOps_uniformLine Parameters
inherits set::Parameters

start = vector (0 0 0) "Start point of line"
end = vector (0 0 0) "End point of line"
np = int 100 "Number of points"

createGetters
<<<PARAMETERSET
*/

protected:
    line l_;

public:
    uniformLine(ParameterSetInput ip = ParameterSetInput() );
    virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
    virtual set* clone() const;

    /**
   * reads the sampled data from the files
   * OF writes different files for scalars, vectors tensors.
   * They are all read and combined into a single matrix in the above order by column.
   * Only the last results in the last time folder is returned
   */
    arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location,
                          ColumnDescription* coldescr=NULL,
                          const std::string& time="" // empty string means latest
                          ) const;
};




class circumferentialAveragedUniformLine
    : public set
{
public:
public:
#include "sampling__sampleOps_circumferentialAveragedUniformLine__Parameters.h"
    /*
PARAMETERSET>>> sampleOps_circumferentialAveragedUniformLine Parameters
inherits set::Parameters

start = vector (0 0 0) "Start point of line"
end = vector (1 0 0) "End point of line"
axis = vector (0 0 1) "Axis of rotation for circumferential averaging"
angle= double 6.28318530718 "Angular span for circumferential averaging"
angularOffset = double 0 "Angle from radial axis to first profile"

np = int 100 "Number of points along line"
nc = int 100 "Number of line for homogeneous averaging"

createGetters
<<<PARAMETERSET
*/

protected:
    double L_;
    arma::mat x_, dir_;
    boost::ptr_vector<line> lines_;

public:
    circumferentialAveragedUniformLine(ParameterSetInput ip = ParameterSetInput() );
    virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
    virtual set* clone() const;

    arma::mat rotMatrix(int i, double angularOffset=0) const;
    inline std::string setname(int i) const { return p().name+"-"+boost::lexical_cast<std::string>(i); }
    arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location,
                          ColumnDescription* coldescr=NULL,
                          const std::string& time="" // empty string means latest
                          ) const;
};




class linearAveragedPolyLine
    : public set
{
public:
#include "sampling__sampleOps_linearAveragedPolyLine__Parameters.h"
    /*
PARAMETERSET>>> sampleOps_linearAveragedPolyLine Parameters
inherits set::Parameters

points = vector (0 0 0) "Matrix of sample points"
dir1=vector (1 0 0) "direction 1, defines spacing between profiles in direction 1"
dir2=vector (0 0 1) "direction 2, defines spacing between profiles in direction 2"
nd1 = int 10 "Number of profiles along dir1"
nd2 = int 10 "Number of profiles along dir2"

createGetters
<<<PARAMETERSET
*/

protected:
    arma::mat x_;
    boost::ptr_vector<line> lines_;

public:
    linearAveragedPolyLine(ParameterSetInput ip = ParameterSetInput() );
    virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
    virtual set* clone() const;

    inline std::string setname(int i, int j) const { return p().name+"-"+boost::lexical_cast<std::string>(i*p().nd1+j); }
    arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location,
                          ColumnDescription* coldescr=NULL,
                          const std::string& time="" // empty string means latest
                          ) const;
};




class linearAveragedUniformLine
    : public set
{

public:
#include "sampling__sampleOps_linearAveragedUniformLine__Parameters.h"
    /*
PARAMETERSET>>> sampleOps_linearAveragedUniformLine Parameters
inherits set::Parameters

start = vector (0 0 0) "start point of base profile"
end = vector (1 0 0) "end point of base profile"
np = int 100 "Number of sample points on base profile"

dir1=vector (1 0 0) "direction 1, defines spacing between profiles in direction 1"
dir2=vector (0 0 1) "direction 2, defines spacing between profiles in direction 2"
nd1 = int 10 "Number of profiles along dir1"
nd2 = int 10 "Number of profiles along dir2"

createGetters
<<<PARAMETERSET
*/

protected:
    linearAveragedPolyLine pl_;

public:
    linearAveragedUniformLine(ParameterSetInput ip = ParameterSetInput() );
    virtual void addIntoDictionary(const OpenFOAMCase& ofc, OFDictData::dict& sampleDict) const;
    virtual set* clone() const;

    arma::mat readSamples(const OpenFOAMCase& ofc, const boost::filesystem::path& location,
                          ColumnDescription* coldescr=NULL,
                          const std::string& time="" // empty string means latest
                          ) const;
};




template<class T>
const T& findSet(const boost::ptr_vector<sampleOps::set>& sets, const std::string& name)
{
    const T* ptr=NULL;
    for (const set& s: sets)
    {
        if (s.name()==name)
        {
            ptr=dynamic_cast<const T*>(&s);
            if (ptr!=NULL) return *ptr;
        }
    }
    insight::Exception("Could not find a set with name "+name+" matching the requested type!");
    return *ptr;
}



}




void sample(const OpenFOAMCase& ofc,
            const boost::filesystem::path& location,
            const std::vector<std::string>& fields,
            const boost::ptr_vector<sampleOps::set>& sets,
            std::vector<std::string> addopts = { "-latestTime" }
            );



} // namespace insight

#endif // SAMPLING_H
