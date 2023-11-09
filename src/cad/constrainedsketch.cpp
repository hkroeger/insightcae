#include "constrainedsketch.h"

#include "base/tools.h"

#include "constrainedsketchgrammar.h"

#include "parser.h"
#include "datum.h"


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
    solverSettings_({rootND, 1e-20, 1., 1000})
{}






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




ConstrainedSketch::GeometryMap::key_type ConstrainedSketch::findUnusedID() const
{
    auto minid=geometry_.begin()->first;
    if (minid>0)
        return minid-1;
    else
    {
        for (int id=minid+1; ; ++id)
        {
            if (geometry_.count(id)<1)
                return id;
        }
    }
    return geometry_.rbegin()->first+1;
}




void ConstrainedSketch::insertGeometry(ConstrainedSketchEntityPtr geomEntity, GeometryMap::key_type key)
{
    if (key<0)
    {
        geometry_.insert({ findUnusedID(), geomEntity });
    }
    else
    {
        auto i = geometry_.find(key);
        if (i==geometry_.end())
        {
            geometry_.emplace( key, geomEntity );
        }
        else
        {
            *i->second = *geomEntity;
        }
    }
}

void ConstrainedSketch::eraseGeometry(GeometryMap::key_type geomEntityId)
{
    geometry_.erase(geomEntityId);
}

void ConstrainedSketch::eraseGeometry(ConstrainedSketchEntityPtr geomEntity)
{
    geometry_.erase(findGeometry(geomEntity));
}


size_t ConstrainedSketch::size() const
{
    return geometry_.size();
}


void ConstrainedSketch::clear()
{
    while (size()>0)
    {
        geometry_.erase( --geometry_.end() );
    }
}


ConstrainedSketch::GeometryMap::const_iterator
ConstrainedSketch::findGeometry(
    ConstrainedSketchEntityPtr geomEntity ) const
{
    return std::find_if(
        geometry_.begin(), geometry_.end(),
        [&geomEntity](const GeometryMap::value_type& e) { return geomEntity==e.second; }
        );
}


ConstrainedSketch::GeometryMap::const_iterator ConstrainedSketch::begin() const
{
    return geometry_.begin();
}

ConstrainedSketch::GeometryMap::const_iterator ConstrainedSketch::cbegin() const
{
    return geometry_.cbegin();
}

ConstrainedSketch::GeometryMap::const_iterator ConstrainedSketch::end() const
{
    return geometry_.end();
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
    geometry_=o.geometry_;
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
    struct LocalIndexMapping { insight::cad::ConstrainedSketchEntity* entity; int iLocal; };
    std::vector<LocalIndexMapping> dofs, constrs;

    for (auto& ge: geometry_)
    {
        auto &e=ge.second;

        for (int i=0; i<e->nDoF(); ++i)
        {
            dofs.push_back({e.get(), i});
        }
        for (int i=0; i<e->nConstraints(); ++i)
        {
            constrs.push_back({e.get(), i});
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
        progress.message( str(boost::format(
            "Number of contraints (%d) not equal to number of DoF (%d)!"
            " Cannot use root solver, swithing to minimizer!")
             % constrs.size() % dofs.size()) );

        solverType=minimumND;
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
                solverSettings_.maxIter_, solverSettings_.relax_,
                [](){}
                /*perIterationCallback*/
                );
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
        '(' > ruleset.r_datumExpression
                  [ qi::_a = phx::bind(&ConstrainedSketch::create<DatumPtr>, qi::_1),
                   qi::_b = parser::make_shared_<ConstrainedSketchGrammar>()(qi::_a, std::bind(noParams)),
                   qi::_val = qi::_a ]
        > ',' > qi::lazy(phx::bind(&ConstrainedSketchGrammar::r_sketch, qi::_b))
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

    for (auto& e: *this)
    {
        entityLabels[e.second.get()]=e.first;
    }
//    int i=0;
//    for (const auto& se: geometry_)
//    {
//        entityLabels[se.second.get()]=i++;
//    }

    ConstrainedSketchScriptBuffer sb;
    for (const auto& se: geometry_)
    {
        se.second->generateScriptCommand(sb, entityLabels);
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


void ConstrainedSketch::build()
{
    ExecTimer t("ConstrainedSketch::build() ["+featureSymbolName()+"]");

    if (!cache.contains(hash()))
    {
        if (!pl_->providesPlanarReference())
            throw insight::Exception("Sketch: Planar reference required!");

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
