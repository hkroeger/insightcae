#include "labeledarraykeyselectiongenerator.h"

using namespace std;


defineType(LabeledArrayKeySelectionGenerator);
addToStaticFunctionTable(ParameterGenerator, LabeledArrayKeySelectionGenerator, insertrule);


LabeledArrayKeySelectionGenerator::LabeledArrayKeySelectionGenerator(
    const std::string& arrayPath,
    const std::string& defSel,
    const std::string& d)
    : ParameterGenerator(d),
      arrayParameterPath(arrayPath),
      defaultSelection(defSel)
{}


void LabeledArrayKeySelectionGenerator::cppAddRequiredInclude(
    std::set<std::string>& headers) const
{
    headers.insert("<string>");
    headers.insert("\"base/parameters/labeledarraykeyselectionparameter.h\"");
}


std::string LabeledArrayKeySelectionGenerator::cppInsightType() const
{
    return "insight::LabeledArrayKeySelectionParameter";
}


std::string LabeledArrayKeySelectionGenerator::cppStaticType() const
{
    return "std::string";
}


std::string LabeledArrayKeySelectionGenerator::cppDefaultValueExpression() const
{
    return "\"" + defaultSelection + "\"";
}


void LabeledArrayKeySelectionGenerator::cppWriteCreateStatement(
    std::ostream& os,
    const std::string& psvarname) const
{
    os << "std::unique_ptr<" << cppInsightType() << "> " << psvarname << ";\n"
       << "{\n"
       << psvarname << ".reset(new " << cppInsightType() << "(\""
       << arrayParameterPath << "\", \""
       << defaultSelection << "\", "
       << cppInsightTypeConstructorParameters() << "));\n"
       << "}\n";
}
