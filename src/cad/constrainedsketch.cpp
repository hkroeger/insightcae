#include "constrainedsketch.h"

#include "base/exception.h"
#include "base/linearalgebra.h"
#include "base/tools.h"

#include "constrainedsketchgrammar.h"

#include "parser.h"
#include "datum.h"
#include <memory>
#include <string>


namespace insight {
namespace cad {




defineType(ConstrainedSketch);
addToStaticFunctionTable(Feature, ConstrainedSketch, insertrule);




size_t ConstrainedSketch::calcHash() const
{
    ParameterListHash p;
    p+=*pl_;
    for (const auto& se: geometry_)
        p+=se.second->hash();
    return p.getHash();
}





ConstrainedSketch::ConstrainedSketch(DatumPtr pl)
    : pl_(pl),
    solverSettings_({rootND, insight::SMALL, 1., 1000})
{}


ConstrainedSketch::ConstrainedSketch( const ConstrainedSketch& other )
    : pl_(other.pl_),
    solverSettings_(other.solverSettings_)
{
    std::map<
        std::comparable_weak_ptr<ConstrainedSketchEntity>,  // in original
        std::shared_ptr<ConstrainedSketchEntity> // in cloned
        > cem;

    // clone entities
    for (const auto& g: other.geometry_)
    {
        auto clone = g.second->clone();
        geometry_.insert({ g.first, clone });
        cem[g.second]=clone;
    }

    // replace (internal) references
    for (auto& g: geometry_)
    {
        for (const auto& d: cem)
        {
            g.second->replaceDependency(d.first, d.second);
        }
    }
}



std::shared_ptr<ConstrainedSketch> ConstrainedSketch::createFromStream(
    DatumPtr pl, istream &in, const ParameterSet& geomPS )
{
    auto sk = ConstrainedSketch::create(pl);
    sk->readFromStream(in);
    return sk;
}




void ConstrainedSketch::readFromStream(istream &in, const ParameterSet &geomPS)
{
    in >> std::noskipws;

    // use stream iterators to copy the stream to a string
    std::istream_iterator<char> it(in);
    std::istream_iterator<char> end;
    std::string contents_raw(it, end);

    std::string::iterator orgbegin,
        first=contents_raw.begin(),
        last=contents_raw.end();

    parser::skip_grammar skip;

    ConstrainedSketchGrammar grammar(
        std::dynamic_pointer_cast<ConstrainedSketch>(shared_from_this()),
        [&geomPS]() ->insight::ParameterSet { return geomPS; });

    bool success = boost::spirit::qi::phrase_parse(
        first,  last,
        grammar, skip
        );

    insight::assertion(
        success,
        "parsing of constrained sketch script was not successful!" );
}




const DatumPtr& ConstrainedSketch::plane() const
{
    return pl_;
}




VectorPtr ConstrainedSketch::sketchPlaneNormal() const
{
    auto dir = pl_->plane().Direction();
    return vec3const( dir.X(), dir.Y(), dir.Z() );
}




ConstrainedSketch::GeometryMap::key_type
ConstrainedSketch::findUnusedID(
    int direction ) const
{
    if (geometry_.size()==0)
    {
        if (direction>0)
            return 1;
        else
            return -1;
    }
    else
    {
        int minid=0;
        for (int id=minid; ; id+=direction)
        {
            if (geometry_.count(id)<1)
                return id;
        }
        if (direction>0)
            return std::max<int>(0,geometry_.rbegin()->first)+1;
        else
            return std::min<int>(0,geometry_.begin()->first)-1;
    }
}




ConstrainedSketch::GeometryMap::key_type
ConstrainedSketch::insertGeometry(
    ConstrainedSketchEntityPtr geomEntity,
    boost::variant<boost::blank,GeometryMap::key_type> keyOrBlank )
{
    GeometryMap::key_type key;
    if (auto *k = boost::get<GeometryMap::key_type>(&keyOrBlank))
    {
        key = *k;
    }
    else
    {
        key = findUnusedID();
    }

    auto i = geometry_.find(key);
    if (i==geometry_.end())
    {
        geometry_.insert({key, geomEntity});
        geometryAdded(key);
    }
    else
    {
        *i->second = *geomEntity;
        geometryChanged(key);
    }
    return key;
}




ConstrainedSketch::GeometryMap::key_type
ConstrainedSketch::setExternalReference(
    std::shared_ptr<ExternalReference> extRef,
    boost::variant<boost::blank,GeometryMap::key_type> keyOrBlank )
{
    GeometryMap::key_type key;
    if (auto *k = boost::get<GeometryMap::key_type>(&keyOrBlank))
    {
        key = -(*k);
    }
    else
    {
        key = findUnusedID(-1);
    }

    auto i = geometry_.find(key);
    if (i==geometry_.end())
    {
        geometry_.insert({ key, extRef });
        geometryAdded(key);
    }
    else
    {
        *i->second = *extRef;
        geometryChanged(key);
    }
    return key;
}




void ConstrainedSketch::eraseGeometry(GeometryMap::key_type geomEntityId)
{
    geometry_.erase(geomEntityId);
    geometryRemoved(geomEntityId);
}




void ConstrainedSketch::eraseGeometry(ConstrainedSketchEntityPtr geomEntity)
{
    auto i=findGeometry(geomEntity);
    if (i!=GeometryMap::const_iterator())
    {
        eraseGeometry(i->first);
    }
}




size_t ConstrainedSketch::size() const
{
    return geometry_.size();
}




void ConstrainedSketch::clear()
{
    while (size()>0)
    {
        auto i = --geometry_.end();
        eraseGeometry( i->first );
    }
}




ConstrainedSketch::GeometryMap::const_iterator
ConstrainedSketch::findGeometry(
    ConstrainedSketchEntityPtr geomEntity ) const
{
    return std::find_if(
        geometry_.begin(), geometry_.end(),
        [&geomEntity](const GeometryMap::value_type& e)
          { return geomEntity==e.second; }
        );
}




ConstrainedSketch::GeometryMap::const_iterator ConstrainedSketch::begin() const
{
    return geometry_.begin();
}




ConstrainedSketch::GeometryMap::const_iterator ConstrainedSketch::end() const
{
    return geometry_.end();
}




ConstrainedSketch::GeometryMap::const_iterator ConstrainedSketch::cbegin() const
{
    return geometry_.cbegin();

}




ConstrainedSketch::GeometryMap::const_iterator ConstrainedSketch::cend() const
{
    return geometry_.cend();
}




std::set<ConstrainedSketchEntityPtr> ConstrainedSketch::filterGeometryByParameters(
    std::function<bool (const ParameterSet &)> filterFunction )
{
    std::set<ConstrainedSketchEntityPtr> ret;
    for (auto& e: *this)
    {
        if (filterFunction(e.second->parameters()))
            ret.insert(e.second);
    }
    return ret;
}




void ConstrainedSketch::operator=(const ConstrainedSketch &o)
{
    Feature::operator=(o);
    pl_=o.pl_;

    std::set<GeometryMap::key_type> remaining;
    std::transform(
        geometry_.begin(), geometry_.end(),
        std::inserter(remaining, remaining.begin()),
        [](const GeometryMap::value_type &v) { return v.first; } );

    std::map<
        std::comparable_weak_ptr<ConstrainedSketchEntity>,  // obj in other
        std::shared_ptr<ConstrainedSketchEntity> // corresponding obj in this
        > cem;

    // assign or add geometry from other sketch
    for (
        auto og = o.geometry_.begin();
        og != o.geometry_.end();
        og++
        )
    {
        auto curId = og->first;
        auto g = geometry_.find(curId);
        if (
            (g!=geometry_.end())
            &&
            (typeid(*g->second)==typeid(*og->second))
           )
        {
            *g->second = *og->second;
            geometryChanged(curId);
            remaining.erase(curId);
            cem[og->second]=g->second;
        }
        else
        {
            if (g!=geometry_.end()) // existing, but of different type
            {
                eraseGeometry(curId);
            }
            auto mycopy=og->second->clone();
            insertGeometry(mycopy, curId);
            cem[og->second]=mycopy;
        }
    }
    for (auto i: remaining)
    {
        eraseGeometry(i);
    }

    // redirect any reference to other sketch entity to this
    for (auto& g: geometry_)
    {
        for (const auto& d: cem)
        {
            g.second->replaceDependency(d.first, d.second);
        }
    }
}




const ConstrainedSketch::SolverSettings& ConstrainedSketch::solverSettings() const
{
    return solverSettings_;
}




void ConstrainedSketch::changeSolverSettings(const ConstrainedSketch::SolverSettings& ss)
{
    solverSettings_=ss;
}




void ConstrainedSketch::resolveConstraints(
    std::function<void(void)> perIterationCallback,
    ProgressDisplayer& progress
    )
{
    struct LocalIndexMapping {
        insight::cad::ConstrainedSketchEntity* entity;
        int iLocal;
        GeometryMap::const_iterator entityIt;
    };
    std::vector<LocalIndexMapping> dofs, constrs;

    for (auto ge=geometry_.begin(); ge!=geometry_.end(); ge++)
    {
        auto &e=ge->second;

        for (int i=0; i<e->nDoF(); ++i)
        {
            dofs.push_back({e.get(), i, ge});
        }
        for (int i=0; i<e->nConstraints(); ++i)
        {
            constrs.push_back({e.get(), i, ge});
        }
    }


    arma::mat x0 = arma::zeros(dofs.size());
    for (int i=0; i<dofs.size(); ++i)
    {
        auto& dm=dofs[i];
        x0(i)=dm.entity->getDoFValue(dm.iLocal);
    }
    std::cout<<"x0="<<x0.t()<<std::endl;

    auto setX = [&](const arma::mat& x)
    {
        for (int i=0; i<dofs.size(); ++i)
        {
            auto& dm=dofs[i];
            dm.entity->setDoFValue(dm.iLocal, x(i));
        }
        for (auto& se: geometry_)
        {
            if ( auto e = std::dynamic_pointer_cast<ASTBase>(se.second) )
            {
                e->invalidate();
            }
        }
    };


    auto solverType = solverSettings_.solver_;

    if (
        (constrs.size()!=dofs.size())
        &&
        (solverType==rootND)
       )
    {
        // progress.message( str(boost::format(
        //     "Number of contraints (%d) not equal to number of DoF (%d)!"
        //     " Cannot use root solver, swithing to minimizer!")
        //      % constrs.size() % dofs.size()) );

        // solverType=minimumND;
        throw insight::Exception(
            "Number of contraints (%d) not equal to number of DoF (%d)!"
            " Cannot use root solver!",
            constrs.size(), dofs.size() );

    }

    arma::mat xsol;
    switch (solverType)
    {
        case minimumND:
        {
            xsol = nonlinearMinimizeND(
                [&](const arma::mat& x) -> double
                {
                    setX(x);

                    double Q=0;
                    for (int i=0;i<constrs.size();++i)
                    {
                        auto& constr = constrs[i];
                        Q+=pow(constr.entity->getConstraintError(constr.iLocal), 2);
                    }
                    insight::dbg()<<"Q="<<Q<<std::endl;
                    return Q;
                },
                x0, solverSettings_.tolerance_, arma::mat(),
                solverSettings_.maxIter_, solverSettings_.relax_
                );
        }
        break;

        case rootND:
        {
            for (int i=0;i<constrs.size();++i)
            {
                auto& constr=constrs[i];
                insight::dbg()<<"constraint "<<i<<" : idx "<<constr.iLocal<<" of "<<constr.entity->type()<<endl;
            }

            try
            {
             xsol = nonlinearSolveND(
                [&](const arma::mat& x) -> arma::mat
                {

                    setX(x);

                    if (perIterationCallback)
                        perIterationCallback();

                    arma::mat Qs=arma::zeros(constrs.size());

                    for (int i=0;i<constrs.size();++i)
                    {
                        auto& constr=constrs[i];
                        Qs(i)=constr.entity->getConstraintError(constr.iLocal);
                    }
                    insight::dbg()<<"Q="<<Qs.t()<<std::endl;
                    return Qs;
                },
                x0, solverSettings_.tolerance_,
                solverSettings_.maxIter_, solverSettings_.relax_
                );
            }
            catch (insight::JacobiDeterminatException& ex)
            {
                std::set<std::string> ucdofs;
                for (int i=0; i<dofs.size(); ++i)
                {
                    auto& dm=dofs[i];
                    if (ex.zeroCols().count(i))
                    {
                        auto i=std::distance(geometry_.cbegin(), dm.entityIt);
                        ucdofs.insert(boost::lexical_cast<std::string>(i));
                    }
                }

                throw insight::Exception(
                    "solve failed because of unconstrained DoFs\n"
                    "enties with unconstrained DoFs are: %s",
                    boost::algorithm::join(ucdofs, ", ").c_str());
            }
        }
        break;
    }

    setX(xsol);
}




void ConstrainedSketchScriptBuffer::insertCommandFor(int entityLabel, const std::string &cmd)
{
    if (entitiesPresent_.find(entityLabel)
        == entitiesPresent_.end())
    {
        script_.push_back(cmd);
        entitiesPresent_.insert(entityLabel);
    }
}




void ConstrainedSketchScriptBuffer::write(ostream &os)
{
    for (auto sl=script_.begin(); sl!=script_.end(); ++sl)
    {
        os<<*sl;
        if ( script_.end() - sl > 1 )
            os << ",";
        os<<std::endl;
    }
}




void ConstrainedSketch::insertrule(parser::ISCADParser& ruleset)
{
    namespace qi = boost::spirit::qi;
    namespace repo = boost::spirit::repository;
    namespace phx   = boost::phoenix;

    typedef
        qi::rule<
            std::string::iterator,
            FeaturePtr(),
            parser::ISCADParser::skipper_type,
            qi::locals<std::shared_ptr<ConstrainedSketch>, std::shared_ptr<ConstrainedSketchGrammar> >
            >
            ConstrainedSketchRule;

    auto noParams = []() { return insight::ParameterSet(); };

    auto *rule = new ConstrainedSketchRule(
        '('

        > ruleset.r_datumExpression
                  [ qi::_a = phx::bind(&ConstrainedSketch::create<DatumPtr>, qi::_1),
               qi::_b = parser::make_shared_<ConstrainedSketchGrammar>()(qi::_a, std::bind(noParams), phx::val(&ruleset)),
                   qi::_val = qi::_a ]  > ','

        > qi::lazy(phx::bind(&ConstrainedSketchGrammar::r_sketch, qi::_b))

        > ')'
    );
    ruleset.addAdditionalRule(rule);

    ruleset.modelstepFunctionRules.add
        (
            "ConstrainedSketch",
            std::make_shared<parser::ISCADParser::ModelstepRule>(*rule)
            );
}




void ConstrainedSketch::generateScript(ostream &os) const
{
    std::map<const ConstrainedSketchEntity*, int> entityLabels;

    for (const auto& se: geometry_)
    {
        entityLabels[se.second.get()]
            = se.first;
    }
//    int i=0;
//    for (const auto& se: geometry_)
//    {
//        entityLabels[se.second.get()]=i++;
//    }

    ConstrainedSketchScriptBuffer sb;
    for (const auto& se: geometry_)
    {
        se.second->generateScriptCommand(
            sb, entityLabels);
    }

    sb.write(os);
}




std::string ConstrainedSketch::generateScriptCommand() const
{
    std::ostringstream os;
    os << type() << "(XY,\n";
    generateScript(os);
    os << ')';
    return os.str();
}




arma::mat ConstrainedSketch::sketchBoundingBox() const
{
    arma::mat min=vec3Zero(), max=vec3Zero();
    for (auto& e: geometry_)
    {
        if (auto p =
            std::dynamic_pointer_cast<insight::cad::SketchPoint>(e.second))
        {
            arma::mat coords=p->value();
            for (int c=0; c<3; ++c)
            {
                min(c)=std::min(min(c), coords(c));
                max(c)=std::max(max(c), coords(c));
            }
        }
    }

    return arma::join_horiz(min, max);
}




std::set<std::string> ConstrainedSketch::layers() const
{
    std::set<std::string> l;
    std::transform(
        geometry_.begin(), geometry_.end(),
        std::inserter(l, l.begin()),
        [](const GeometryMap::value_type& gv) { return gv.second->layerName(); }
        );
    return l;
}




void ConstrainedSketch::build()
{
    ExecTimer t("ConstrainedSketch::build() ["+featureSymbolName()+"]");

    if (!cache.contains(hash()))
    {
        if (!pl_->providesPlanarReference())
            throw insight::Exception("Sketch: Planar reference required!");

        try
        {
            resolveConstraints();
        }
        catch (insight::NonConvergenceException& ex)
        {
            insight::Warning(ex);
        }

        BRep_Builder bb;
        TopoDS_Compound result;
        bb.MakeCompound ( result );

        for ( auto& sg: geometry_ )
        {
            if ( auto f = std::dynamic_pointer_cast<Feature>(sg.second) )
            {
                bb.Add ( result, f->shape() );
            }
        }

        setShape(result);

        cache.insert(shared_from_this());
    }
    else
    {
        this->operator=(*cache.markAsUsed<ConstrainedSketch>(hash()));
    }
}




} // namespace cad
} // namespace insight
