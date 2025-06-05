#ifndef INSIGHT_LSDYNAINPUTCARDS_MESH_H
#define INSIGHT_LSDYNAINPUTCARDS_MESH_H


#include "boost/variant/variant.hpp"
#include "lsdyna/lsdynainputcard.h"

#include <map>
#include "base/units.h"

namespace insight {
namespace LSDynaInputCards {


class Part;
class Material;





class Node
        : public LSDynaInputCard,
        public std::map<int,si::LengthVector>
{
public:
    typedef std::map<int,si::LengthVector> NodeList;
public:
    Node();
    Node(const NodeList& nodes);

    void write(std::ostream& os) const override;
};




class ElementShell
        : public LSDynaInputCard
{
public:
    struct Element
    {
        const Part *part;
        std::array<int,4> corners;
    };

    typedef std::map<int, Element> ElementList;

    ElementList elements_;

public:
    ElementShell( const ElementList& elements );
    void write(std::ostream& os) const override;
};



class SetShell
    : public InputCardWithId
{
public:
    typedef std::vector<int> List;

    struct General
    {
        enum Type {
            ALL,
            ELEM,
            DELEM,
            PART,
            DPART,
            BOX,
            DBOX
        } type;

        std::vector<int> ids;
    };

    typedef boost::variant<
            List,
            General
        > Source;

private:
    Source source_;

public:
    SetShell(
        int id,
        const Source& source
        );

    void write(std::ostream& os) const override;
};



class Section
        : public InputCardWithId
{
public:
    Section(int id);
};




class SectionShell : public Section
{
public:
    enum ElForm {
        Default = 0,
        QEPH_C0 = 1,
        Membrane = 5,
        QBAT_C0 = 9,
        QBAT_DKT18 = 17,
        QBAT_DKTS3 = 18,
        QBAT_C0_6 = 20
    };
private:
    si::Length thickness_;
    ElForm elform_;
    int nip_;

public:
    SectionShell(int id, const si::Length& thickness, ElForm elform=Default, int nip = 0);
    void write(std::ostream& os) const override;
};




class SectionBeam : public Section
{
public:
    enum ElForm {
        Default = 1,
        ResultantBeam = 2,
        Truss = 3,
        DiscreteBeam = 6
    };

    enum CST {
        Square = 0,
        Circle = 1
    };

    struct IntegratedBeamProperties {
        CST crossSectionType;
        si::Length edgeLenOrDiameter;
    };

    struct CircularCrossSectionProperties {
        si::Length radius;
    };
    struct RectangularCrossSectionProperties {
        si::Length Lz, Ly;
    };

    typedef boost::variant<
        CircularCrossSectionProperties,
        RectangularCrossSectionProperties>
        CrossSectionProperties;

    struct ResultantBeamProperties {
        CrossSectionProperties cst;
        si::Area A;
        si::SecondAreaMoment Iyy, Izz, Ixx;
    };

    struct TrussProperties {
        CrossSectionProperties cst;
        si::Area A;
    };

    struct DiscreteBeamProperties {
        si::Volume V;
        boost::units::quantity<
            decltype(si::kilogram*si::square_meter)::unit_type, double> inertia;
        int cid;
        si::Area A;
    };

    typedef boost::variant<
        IntegratedBeamProperties,
        ResultantBeamProperties,
        TrussProperties,
        DiscreteBeamProperties
    > Properties;

private:
    int nip_;
    Properties props_;

public:
    SectionBeam(
        int id,
        Properties properties,
        int nip = 2 );

    void write(std::ostream& os) const override;
};



class Part
        : public InputCardWithId,
        public std::string
{
    const Section* section_;
    const Material* material_;
public:
    Part(const std::string& name, int pid, const Section* section, const Material* material);
    void write(std::ostream& os) const override;
};




} // namespace LSDynaInputCards
} // namespace insight

#endif // INSIGHT_LSDYNAINPUTCARDS_MESH_H
