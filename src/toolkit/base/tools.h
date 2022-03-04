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

#ifndef TOOLS_H
#define TOOLS_H

class vtkPolyData;
class vtkCellArray;

#include "vtkSmartPointer.h"
#include "vtkPolyDataAlgorithm.h"

#include "base/exception.h"
#include "base/boost_include.h"
#include "boost/process.hpp"
#include "boost/process/args.hpp"
#include "base/linearalgebra.h"

#include <istream>

#include <boost/mpl/clear.hpp>

#include "rapidxml/rapidxml.hpp"


namespace insight
{


/**
 * wrapper for calling virtual functions before destruction
 */
template <typename T>
class DestructionGuard : public T
{
  std::function<void()> preDestruction_;

public:
  template<class ...Args>
  DestructionGuard(Args&&... addArgs)
   : T(std::forward<Args>(addArgs)...),
     preDestruction_([](){})
  {}

  void setPreDestructionFunction(std::function<void()> preDestruction)
  {
      preDestruction_=preDestruction;
  }

  ~DestructionGuard()
  {
     preDestruction_();
     // now T is destructed
  }
};




template<typename StringType, typename StringIteratorType, typename AccessFunctionType>
StringType findUnusedLabel(
        StringIteratorType begin,
        StringIteratorType end,
        const StringType& desiredLabel,
        AccessFunctionType accessFunction,
        int maxAttempts=99
        )
{
    CurrentExceptionContext ex("finding an unused label in a list of labels");

    StringType lbl = desiredLabel;

    for (int attempt=1; attempt<maxAttempts; ++attempt)
    {
        insight::dbg()<<"attempt "<<attempt<<std::endl;

        bool found=false;
        for (auto it=begin; it!=end; ++it)
        {
            if ( accessFunction(it) == lbl )
            {
                found=true;
                break;
            }
        }
        if (found)
        {
            insight::dbg()<<"try lbl="<<lbl<<std::endl;
            lbl = desiredLabel + "_" + boost::lexical_cast<StringType>(attempt);
        }
        else
        {
            insight::dbg()<<"return lbl="<<lbl<<std::endl;
            return lbl;
        }
    }

    throw insight::Exception(
                str(boost::format("Could not find an unused label within %d attempts")
                    % maxAttempts ) );
}




template<typename StringType, typename StringIteratorType>
StringType findUnusedLabel(
        StringIteratorType begin,
        StringIteratorType end,
        const StringType& desiredLabel,
        int maxAttempts=99
        )
{
    return findUnusedLabel(begin, end, desiredLabel,
                           [](StringIteratorType it) -> StringType
                           {
                               return static_cast<StringType>(*it);
                           },
                           maxAttempts );
}


/**
 * @brief directoryIsWritable
 * checks, if a file can be created in the directory
 * @param directory
 * @return
 */
bool directoryIsWritable( const boost::filesystem::path& directory );

/**
 * @brief isInWritableDirectory
 * checks, if the given directory is writable, or,
 * if it does not exists, if the parent directory is writable
 * @param pathToTest
 * @return
 */
bool isInWritableDirectory( const boost::filesystem::path& pathToTest );

class GlobalTemporaryDirectory
    : public boost::filesystem::path
{

  GlobalTemporaryDirectory();

  static std::unique_ptr<GlobalTemporaryDirectory> td_;

public:

  static const GlobalTemporaryDirectory& path();

  /**
   * @brief clear
   * Removes the temporary directory and all its contents.
   * This is only provided for use in test programs.
   * Cleanup is intendend to be done automatically at program exit.
   */
  static void clear();

  ~GlobalTemporaryDirectory();

};


/**
 * @brief ensureFileExtension
 * adds an extension to the file name, if required
 * @param filePath
 * the file path to check
 * @param extension
 * the desired extension including the dot, e.g. ".ist"
 * @return
 */
boost::filesystem::path ensureFileExtension(const boost::filesystem::path& filePath, const std::string& extension);


std::string timeCodePrefix();



class TemporaryFile
{
  boost::filesystem::path tempFilePath_;
  std::unique_ptr<std::ofstream> stream_;

  TemporaryFile(const TemporaryFile& other); // forbid copies

public:
  TemporaryFile(const std::string& fileNameModel="%%%%%.dat", const boost::filesystem::path& baseDir=boost::filesystem::path());
  ~TemporaryFile();

  std::ostream& stream();
  void closeStream();
  const boost::filesystem::path& path() const;
};


/**
 * @brief The SSHCommand class
 * wraps SSH command with unique interface in Linux and Windows
 */
class SSHCommand
{
  std::string hostName_;
  std::vector<std::string> args_;

public:
  SSHCommand(const std::string& hostName, const std::vector<std::string>& arguments);

  boost::filesystem::path command() const;
  std::vector<std::string> arguments() const;
};


class RSYNCCommand
{
  std::vector<std::string> args_;

public:
  RSYNCCommand(const std::vector<std::string>& arguments);

  boost::filesystem::path command() const;
  std::vector<std::string> arguments() const;
};


/**
 * @brief The SharedPathList class
 * contains the user directory first, the global path second
 */
class SharedPathList 
: public std::vector<boost::filesystem::path>
{
public:
  SharedPathList();
  virtual ~SharedPathList();
  virtual boost::filesystem::path getSharedFilePath(const boost::filesystem::path& file);
  
  void insertIfNotPresent(const boost::filesystem::path& sp);
  void insertFileDirectoyIfNotPresent(const boost::filesystem::path& sp);
  boost::filesystem::path findFirstWritableLocation(
          const boost::filesystem::path& subPath ) const;
  
  static SharedPathList searchPathList;
};


class ExecTimer
: public boost::timer::auto_cpu_timer
{
public:
    ExecTimer(const std::string& name);

    ~ExecTimer();
};


void copyDirectoryRecursively(const boost::filesystem::path& sourceDir, const boost::filesystem::path& destinationDir);


template<class T>
T toNumber(const std::string& s)
{
  try {
    return boost::lexical_cast<T>( boost::algorithm::trim_copy(s) );
  } catch (const boost::bad_lexical_cast& e) {
    throw insight::Exception("expected a number, got \""+s+"\"");
  }
}

bool isNumber(const std::string& s);


class LineMesh_to_OrderedPointTable
        : public std::vector<arma::mat>
{
    void calcConnectionInfo(vtkCellArray* lines);

public:
    typedef std::vector<vtkIdType> idList;
    typedef std::map<vtkIdType,idList> idListMap;

    idListMap pointCells_, cellPoints_;
    std::set<vtkIdType> endPoints_;

    std::vector<vtkIdType> pointIds_; // mapping of each point in ordered point table to vtkPoint index

    LineMesh_to_OrderedPointTable(vtkPolyData* pd);

    inline vtkIdType nEndpoints() const { return vtkIdType(endPoints_.size()); }

    const std::vector<vtkIdType>& pointIds() const;

    arma::mat extractOrderedData(vtkDataArray* data) const;

    void printSummary(std::ostream&, vtkPolyData* pd=nullptr) const;

    /**
     * @brief txyz
     * convert point list into a single matrix
     * @return
     * matrix with first column: distance coordinate,
     * cols 2,3,4: x, y, z
     */
    arma::mat txyz() const;
};



arma::mat computeOffsetContour(const arma::mat& polyLine, double thickness, const arma::mat& normals);




class vtk_Transformer
{
public:
  virtual ~vtk_Transformer();
  virtual vtkSmartPointer<vtkPolyDataAlgorithm> apply_VTK_Transform(vtkSmartPointer<vtkPolyDataAlgorithm> in) =0;
};

class vtk_ChangeCS
    : public vtk_Transformer
{
  arma::mat m_;
public:
  vtk_ChangeCS
  (
      arma::mat from_ex,
      arma::mat from_ez,
      arma::mat to_ex,
      arma::mat to_ez
  );

  /**
    * initialize from matrix 4x4 = 16 coefficients
    */
  vtk_ChangeCS
  (
      const double *coeffs
  );

  /**
    * initialize from callback function
    */
  vtk_ChangeCS
  (
      std::function<double(int, int)> init_func,
      int nrows=4, int idx_ofs=0
  );

  vtkSmartPointer<vtkPolyDataAlgorithm> apply_VTK_Transform(vtkSmartPointer<vtkPolyDataAlgorithm> in) override;
};

typedef vtk_Transformer* vtk_TransformerPtr;
typedef std::vector<const vtk_Transformer*> vtk_TransformerList;


vtkSmartPointer<vtkPolyDataAlgorithm>
readSTL
(
  const boost::filesystem::path& path,
  const vtk_TransformerList& trsf = vtk_TransformerList()
);

/**
  * return bounding box of model
  * first col: min point
  * second col: max point
  */
arma::mat STLBndBox(
  vtkSmartPointer<vtkPolyDataAlgorithm> stl_data_Set
);

/**
  * return bounding box of model
  * first col: min point
  * second col: max point
  */
arma::mat PolyDataBndBox(
  vtkSmartPointer<vtkPolyData> stl_data_Set
);

void writeSTL
(
   vtkSmartPointer<vtkPolyDataAlgorithm> stl,
   const boost::filesystem::path& outfile
);


std::string collectIntoSingleCommand( const std::string& cmd, const std::vector<std::string>& args = std::vector<std::string>() );
std::string escapeShellSymbols(const std::string& expr);


int findFreePort();
int findRemoteFreePort(const std::string& SSHHostName);


/**
 * @brief readStreamIntoString
 * stream needs to be opened in binary mode (std::ios::in | std::ios::binary)
 * @param in
 * @param fileContent
 */
void readStreamIntoString
(
    std::istream& in,
    std::string& fileContent
);

void readFileIntoString
(
    const boost::filesystem::path& fileName,
    std::string& fileContent
);


class TemplateFile
    : public std::string
{
public:
  TemplateFile(const std::string& hardCodedTemplate);
  TemplateFile(std::istream& in);
  TemplateFile(const boost::filesystem::path& in);

  void replace(const std::string& keyword, const std::string& content);

  void write(std::ostream& os) const;
  void write(const boost::filesystem::path& outfile) const;
};

struct MemoryInfo
{
  long long memTotal_, memFree_;

  MemoryInfo();
};


class RSyncProgressAnalyzer
    : public boost::process::ipstream
{
public:
  RSyncProgressAnalyzer();
  void runAndParse(boost::process::child& rsyncProcess, std::function<void(int,const std::string&)> progressFunction);
};


template<class OrgKeyType, class KeyType = OrgKeyType>
int predictSetInsertionLocation(const std::set<OrgKeyType>& org_keys, const KeyType& newKey)
{
  std::set<KeyType> keys;
  std::transform(
        org_keys.begin(), org_keys.end(),
        std::inserter(keys, keys.begin()),
        [](const typename std::set<OrgKeyType>::value_type& i)
        {
          return static_cast<KeyType>(i);
        }
  );
  keys.insert(newKey);
  auto i=keys.find(newKey);
  return std::distance(keys.begin(), i);
}



template<class KeyType, class Container>
int predictInsertionLocation(const Container& org_data, const KeyType& newKey)
{
  std::set<KeyType> org_keys;
  // retrieve keys only
  std::transform(
        org_data.begin(), org_data.end(),
        std::inserter(org_keys, org_keys.begin()),
        [](const typename Container::value_type& i)
        {
          return static_cast<KeyType>(i.first);
        }
  );
  return predictSetInsertionLocation(org_keys, newKey);
}



std::string getMandatoryAttribute(rapidxml::xml_node<> &node, const std::string& attributeName);
std::shared_ptr<std::string> getOptionalAttribute(rapidxml::xml_node<> &node, const std::string& attributeName);


}

#endif // TOOLS_H
