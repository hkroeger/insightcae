#include "mesh.h"

#include "base/exception.h"
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
    os << toValue(thickness_, inputDeck().lengthUnit()) << "\n";
}




SectionBeam::SectionBeam(
    int id,
    Properties properties,
    int nip )
: Section(id),
    nip_(id), props_(properties)
{}



void SectionBeam::write(std::ostream& os) const
{
    os << "*SECTION_BEAM\n";
    auto firstLine = [&](int elform, int cst) {
        os << id() << ", " << elform << ",, "
           << nip_ << ", " << cst << "\n";
    };

    if (auto *ib = boost::get<IntegratedBeamProperties>(&props_))
    {
        firstLine( 1, int(ib->crossSectionType) );
        os << toValue(ib->edgeLenOrDiameter, inputDeck().lengthUnit())
           << "\n";
    }
    else if (auto *rb = boost::get<ResultantBeamProperties>(&props_))
    {
        auto writeXsecProps = [&]() {
            os << toValue(rb->A, pow<2>(inputDeck().lengthUnit()))<<","
               << toValue(rb->Iyy, pow<4>(inputDeck().lengthUnit()))<<","
               << toValue(rb->Izz, pow<4>(inputDeck().lengthUnit()))<<","
               << toValue(rb->Ixx, pow<4>(inputDeck().lengthUnit()))
               << "\n";
        };

        if (auto *cs = boost::get<CircularCrossSectionProperties>(&rb->cst))
        {
            firstLine( 2, CST::Circle );
            writeXsecProps();
            os << "SECTION_08, "<<toValue(cs->radius, inputDeck().lengthUnit())
               << "\n";
        }
        else if (auto *rs = boost::get<RectangularCrossSectionProperties>(&rb->cst))
        {
            firstLine( 2, CST::Square );
            writeXsecProps();
            os << "SECTION_11, "
               <<toValue(rs->Ly, inputDeck().lengthUnit())<<", "
               <<toValue(rs->Lz, inputDeck().lengthUnit())
               << "\n";
        }
    }
    else if (auto *tr = boost::get<TrussProperties>(&props_))
    {
        if (auto *cs = boost::get<CircularCrossSectionProperties>(&tr->cst))
        {
            firstLine( 3, CST::Circle );
            os << "SECTION_08, "<<toValue(cs->radius, inputDeck().lengthUnit())
               << "\n";
        }
        else if (auto *rs = boost::get<RectangularCrossSectionProperties>(&tr->cst))
        {
            firstLine( 3, CST::Square );
            os << "SECTION_11, "
               <<toValue(rs->Ly, inputDeck().lengthUnit())<<", "
               <<toValue(rs->Lz, inputDeck().lengthUnit())
               << "\n";
        }
        // os << toValue(tr->A, pow<2>(inputDeck().lengthUnit())) << "\n";
    }
    else if (auto *db = boost::get<DiscreteBeamProperties>(&props_))
    {
        firstLine( 6, CST::Square );
        os <<toValue(db->V, pow<3>(inputDeck().lengthUnit()))<<", "
           <<toValue(db->inertia,
                      inputDeck().massUnit()*pow<2>(inputDeck().lengthUnit()) )<<", "
           << db->cid << ", "
           <<toValue(db->A, pow<2>(inputDeck().lengthUnit()))<<", "
           << "\n";
    }
    else
    {
        throw insight::UnhandledSelection();
    }
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




SetShell::SetShell(int id, const Source &source)
    : InputCardWithId(id),
      source_(source)
{}


void SetShell::write(std::ostream& os) const
{
    os << "*SET_SHELL";
    if (auto *gen = boost::get<General>(&source_))
    {
        os << "_GENERAL\n" << id() << "\n";
        switch (gen->type)
        {
            case General::Type::ALL: os << "ALL"; break;
            case General::Type::ELEM: os << "ELEM"; break;
            case General::Type::DELEM: os << "DELEM"; break;
            case General::Type::PART: os << "PART"; break;
            case General::Type::DPART: os << "DPART"; break;
            case General::Type::BOX: os << "BOX"; break;
            case General::Type::DBOX: os << "DBOX"; break;
            default:
                throw insight::UnhandledSelection();
        }
        int ic=1;
        for (auto i = gen->ids.begin(); i!=gen->ids.end(); ++i)
        {
            os << ", " << *i;
            ic++;
            if ((ic>7) || ((i+1)==gen->ids.end()))
            {
                os << "\n";
                ic=1;
            }
        }
    }
    else if (auto *li = boost::get<List>(&source_))
    {
        os << "_LIST\n" << id() << "\n";
        int ic=1;
        for (auto i = li->begin(); i!=li->end(); ++li)
        {
            if (ic>1)
            {
                os << ", ";
            }

            os << *i;
            ic++;

            if ((ic>8) || ((i+1)==li->end()))
            {
                os << "\n";
                ic=1;
            }
        }
    }
    else
        throw insight::UnhandledSelection();
}


} // namespace LSDynaInputCards
} // namespace insight
