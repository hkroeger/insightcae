#ifndef PARAMETERSBASE_H
#define PARAMETERSBASE_H



#include <memory>
#include <boost/optional.hpp>



namespace insight {

class Parameter;
class ParameterSet;

class ParameterSetBuilder
{
public:
    struct Key {
    private:
        Key() = default;
        friend class ParameterSet;
    };
protected:
    static inline Key key() { return {}; }

};

struct ParametersBase
: public ParameterSetBuilder
{
    boost::optional<std::string> parameterPath;

    ParametersBase();
    ParametersBase(const insight::ParameterSet& p);
    virtual ~ParametersBase();

    virtual void set(insight::ParameterSet& p) const;
    virtual void get(const insight::ParameterSet& p);

    static std::unique_ptr<ParameterSet> makeDefault();

    virtual std::unique_ptr<ParameterSet> cloneParameterSet() const =0;

    virtual std::unique_ptr<ParametersBase> clone() const =0;
};



} // namespace insight

#endif // PARAMETERSBASE_H
