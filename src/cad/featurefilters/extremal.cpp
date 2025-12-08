#include "cadfeature.h"
#include "datum.h"
#include "extremal.h"

namespace insight {
namespace cad {


extremal::extremal(const scalarQuantityComputer& qtc, int rank, int lrank)
  : rank_(rank),
    lrank_(std::max(rank,lrank)),
    qtc_(qtc.clone())
{
}

void extremal::initialize(ConstFeaturePtr m)
{
    Filter::initialize(m);
    qtc_->initialize(m);
}

void extremal::firstPass(FeatureID feature)
{
    if (qtc_->isValidForFeature(feature))
    {
        ranking_.insert({criterion(feature), feature});
    }
}

bool extremal::checkMatch(FeatureID feature) const
{
    if (ranking_.size()>0)
    {
        int irank=0;
        for (
            auto rng=ranking_.begin();
            rng!=ranking_.end();
            rng=ranking_.upper_bound(rng->first)
            )
        {
            if ( (irank>=rank_)
                && ((lrank_<0) || (irank<=lrank_)) )
            {
                for (auto i=rng; i!=ranking_.upper_bound(rng->first); ++i)
                {
                    if (i->second==feature)
                    {
                        //std::cout<<"Feature #"<<feature<<" rank="<<j<<" match!"<<std::endl;
                        return true;
                    }
                }
            }

            irank++;
        }
    }
    return false;
}
} // namespace cad
} // namespace insight
