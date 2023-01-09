#ifndef INSIGHT_LSDYNAINPUTCARDS_MESH_H
#define INSIGHT_LSDYNAINPUTCARDS_MESH_H


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
