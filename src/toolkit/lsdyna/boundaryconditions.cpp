#include "boundaryconditions.h"

#include "base/tools.h"
#include "boost/lexical_cast.hpp"
#include "lsdyna/mesh.h"
#include "lsdynainputdeck.h"
#include "lsdyna/solution.h"
#include <string>

namespace insight {
namespace LSDynaInputCards {




BoundaryPrescribedMotionRigid::BoundaryPrescribedMotionRigid(
        const Part *part,
        DOF dof,
        MotionType motionType,
        const Curve *curve,
        double curveScaleY,
        si::Time tStop, si::Time tStart
        )
    : part_(part),
      dof_(dof),
      motionType_(motionType),
      curve_(curve),
      curveScaleY_(curveScaleY),
      tStop_(tStop), tStart_(tStart)
{}




void BoundaryPrescribedMotionRigid::write(std::ostream& os) const
{
    os << "*BOUNDARY_PRESCRIBED_MOTION_RIGID\n"
       << part_->id() << ", "
       << int(dof_) <<", "
       << int(motionType_) << ", "
       << curve_->id() << ", "
       << curveScaleY_ << ", "
       << 0 << ", "
       << toValue(tStop_, inputDeck().timeUnit()) << ", "
       << toValue(tStart_, inputDeck().timeUnit()) << "\n";
}




BoundarySPC::BoundarySPC(
    boost::variant<Node,NodeSet> subject,
    std::set<DOF> dofs,
    int cid
    )
: subject_(subject),
  dofs_(dofs),
  cid_(cid)
{}




void BoundarySPC::write(std::ostream& os) const
{

    os << "*BOUNDARY_SPC";
    if (auto *subs = boost::get<Node>(&subject_))
    {
        os
            << "_NODE\n"
            << subs->nodeID<<",";
    }
    else if (auto *subns = boost::get<NodeSet>(&subject_))
    {
        os
            << "_SET\n"
            << subns->setID<<",";
    }

    os << ((cid_>=0)?toString(cid_):"")<<","
       << dofs_.count(X) << ","
       << dofs_.count(Y) << ","
       << dofs_.count(Z) << ","
       << dofs_.count(RX) << ","
       << dofs_.count(RY) << ","
       << dofs_.count(RZ) << "\n";
}




LoadShell::LoadShell(
    boost::variant<Element,ElementSet> subject,
    const Curve *curve,
    double fScale,
    si::Time Tstart )
  : subject_(subject),
    curve_(curve),
    fScale_(fScale),
    Tstart_(Tstart)
{}

void LoadShell::write(std::ostream& os) const
{
    os << "*LOAD_SHELL";
    if (auto *elem = boost::get<Element>(&subject_))
    {
        os << "_ELEMENT\n\n";
        os << elem->elementID;
    }
    else if (auto *elset = boost::get<ElementSet>(&subject_))
    {
        os << "_SET\n\n";
        os << elset->setID;
    }
    os << ","
       << curve_->id() << ","
       << fScale_ << ", "
       << toValue(Tstart_, inputDeck().timeUnit()) << "\n";
}





ConstrainedJoint::ConstrainedJoint(Type type, std::set<int> nodes)
    : type_(type), nodes_(nodes)
{}


void ConstrainedJoint::write(std::ostream& os) const
{
    os << "*CONSTRAINED_JOINT_";
    switch (type_)
    {
        case Cylindrical: os << "CYLINDRICAL"; break;
        case Revolute: os << "REVOLUTE"; break;
        case Spherical: os << "SPHERICAL"; break;
        case Locking: os << "LOCKING"; break;
        case Translational: os << "TRANSLATIONAL"; break;
        case Universal: os << "UNIVERSAL"; break;
        case Planar: os << "PLANAR"; break;
        default: throw insight::UnhandledSelection();
    }
    os << "\n";

    int ic=1;
    for (auto i = nodes_.begin(); i!=nodes_.end(); ++i)
    {
        if (ic>1) os << ", ";
        os << *i;
        ic++;
        auto j=i; j++;
        if ((ic>7) || (j==nodes_.end()))
        {
            os << "\n";
            ic=1;
        }
    }
}

} // namespace LSDynaInputCards
} // namespace insight
