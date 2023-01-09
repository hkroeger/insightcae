#include "mesh.h"

#include "lsdyna/lsdynainputdeck.h"
#include "lsdyna/materials.h"

namespace insight {
namespace LSDynaInputCards {



LSDynaInputCards::Node::Node()
{}

LSDynaInputCards::Node::Node(const NodeList& nodes)
    : std::map<int,si::LengthVector>(nodes)
{}


void LSDynaInputCards::Node::write(std::ostream &os) const
{
    os << "*NODE\n";
    for (auto i=begin(); i!=end(); ++i)
    {
        const auto& p = toValue(i->second, inputDeck().lengthUnit());
        os << i->first << ", "
           << p(0) << ", " << p(1) << ", " << p(2)
           << "\n";
    }

}




ElementShell::ElementShell(const ElementList &elements)
    : elements_(elements)
{}

void ElementShell::write(std::ostream &os) const
{
    os << "*ELEMENT_SHELL\n";
    for (const auto& e: elements_)
    {
        os << e.first << ", "
           << e.second.part->id();

        for (int i: e.second.corners)
        {
            if (i>=0) os << ", "<<i;
        }
        os << "\n";
    }
}



Section::Section(int id) : InputCardWithId(id) {}




SectionShell::SectionShell(int id, const si::Length &thickness, ElForm elform, int nip)
    : Section(id),
      thickness_(thickness),
      elform_(elform),
      nip_(nip)
{}


void SectionShell::write(std::ostream& os) const
{
    os << "*SECTION_SHELL\n";
    os << id() << ", " << int(elform_) << ", " << nip_ << "\n";
    os << toValue(thickness_, inputDeck().lengthUnit());
}




Part::Part(
        const std::string& name,
        int pid,
        const Section* section,
        const Material* material )
    : InputCardWithId(pid),
      std::string(name),
      section_(section),
      material_(material)
{}

void Part::write(std::ostream& os) const
{
    os << "*PART\n";
    os << std::string(*this) << "\n";
    os << id() << ", " << section_->id() << ", " << material_->id() << "\n";
}




} // namespace LSDynaInputCards
} // namespace insight
