#ifndef INSIGHT_PROPERTYLIBRARYSELECTIONPARAMETER_H
#define INSIGHT_PROPERTYLIBRARYSELECTIONPARAMETER_H

#include "base/parameters/simpleparameter.h"
#include "base/propertylibrary.h"

namespace insight {

class PropertyLibrarySelectionParameter
        : public StringParameter
{

protected:
    const PropertyLibraryBase* propertyLibrary_;

public:
    declareType ( "librarySelection" );

    PropertyLibrarySelectionParameter (
            const std::string& description,
            bool isHidden=false,
            bool isExpert=false,
            bool isNecessary=false, int order=0 );

    PropertyLibrarySelectionParameter (
            const std::string& value,
            const PropertyLibraryBase& lib,
            const std::string& description,
            bool isHidden=false,
            bool isExpert=false,
            bool isNecessary=false,
            int order=0 );

    bool isDifferent(const Parameter& p) const override;

    std::vector<std::string> items() const;
    bool contains(const std::string& value) const;

    inline void setSelection ( const std::string& sel )
    {
        value_=sel;
    }

    inline const std::string& selection() const
    {
        return value_;
    }


    Parameter* clone() const override;
    void reset(const Parameter& p) override;
};

} // namespace insight

#endif // INSIGHT_PROPERTYLIBRARYSELECTIONPARAMETER_H
