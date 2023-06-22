#include "constrainedsketch.h"

#include "base/tools.h"

#include "constrainedsketchgrammar.h"

#include "parser.h"
#include "datum.h"

#include "gsl/gsl_multiroots.h"


namespace insight {
namespace cad {




defineType(ConstrainedSketch);
addToStaticFunctionTable(Feature, ConstrainedSketch, insertrule);




size_t ConstrainedSketch::calcHash() const
{
    ParameterListHash p;
    p+=*pl_;
    for (const auto& se: geometry_)
        p+=se->hash();
    return p.getHash();
}





ConstrainedSketch::ConstrainedSketch(DatumPtr pl)
    : pl_(pl),
    solverTolerance_(1e-12),
    solverType_(minimumND)
{}






std::shared_ptr<ConstrainedSketch> ConstrainedSketch::createFromStream(
    DatumPtr pl, istream &in, const ParameterSet& geomPS )
{
    auto sk = ConstrainedSketch::create(pl);

    in >> std::noskipws;

    // use stream iterators to copy the stream to a string
    std::istream_iterator<char> it(in);
    std::istream_iterator<char> end;
    std::string contents_raw(it, end);

    insight::dbg()<<contents_raw<<std::endl;

    std::string::iterator orgbegin,
        first=contents_raw.begin(),
        last=contents_raw.end();

    parser::skip_grammar skip;

    ConstrainedSketchGrammar grammar(sk, [&geomPS]() ->insight::ParameterSet { return geomPS; });
    bool success = boost::spirit::qi::phrase_parse(
        first,  last,
        grammar, skip
        );

    insight::assertion(
        success,
        "parsing of constrained sketch script was not successful!" );

    return sk;
}




const DatumPtr& ConstrainedSketch::plane() const
{
    return pl_;
}


std::set<std::shared_ptr<ConstrainedSketchEntity> >&
ConstrainedSketch::geometry()
{
    return geometry_;
}

const std::set<ConstrainedSketchEntityPtr> &ConstrainedSketch::geometry() const
{
    return geometry_;
}

std::set<ConstrainedSketchEntityPtr> ConstrainedSketch::filterGeometryByParameters(
    std::function<bool (const ParameterSet &)> filterFunction )
{
    std::set<ConstrainedSketchEntityPtr> ret;
    std::copy_if(
        geometry().begin(), geometry().end(),
        std::inserter(ret, ret.begin()),
        [&](const ConstrainedSketchEntityPtr& e)
        { return filterFunction(e->parameters()); }
        );
    return ret;
}

void ConstrainedSketch::operator=(const ConstrainedSketch &o)
{
    Feature::operator=(o);
    pl_=o.pl_;
    geometry_=o.geometry_;
}

double ConstrainedSketch::solverTolerance() const
{
    return solverTolerance_;
}

void ConstrainedSketch::setSolverTolerance(double tol)
{
    solverTolerance_=tol;
}

ConstrainedSketch::SolverType ConstrainedSketch::solverType() const
{
    return solverType_;
}

void ConstrainedSketch::setSolverType(SolverType t)
{
    solverType_=t;
}

arma::mat vec(const gsl_vector * x)
{
    arma::mat r = arma::zeros(x->size);
    for (size_t i=0; i<x->size; ++i)
    {
        r(i) = gsl_vector_get(x, i);

        insight::assertion(
            !std::isnan(r(i)),
            "NaN at element %d", i);
    }
    return r;
}

void gsl_vector_set(const arma::mat& x, gsl_vector * xo)
{
    insight::assertion(
        xo->size == x.n_elem,
        "gsl_vector must be initialized to the same size as input vector (expected %d, got %d)",
        x.n_elem, xo->size);

    for (int i=0; i<x.n_elem; ++i)
    {
        insight::assertion(
            !std::isnan(x(i)),
            "nan in input vector at %d", i);
        gsl_vector_set (xo, i, x(i));
    }
}


int f_adapter (const gsl_vector * x, void * p, gsl_vector * y)
{
    auto F = *reinterpret_cast<std::function<arma::mat(const arma::mat&)>*>(p);

    arma::mat vx = vec(x);

    insight::dbg()<<"x="<<vx.t()<<std::endl;

    arma::mat my = F( vx );

    gsl_vector_set (my, y);

    return GSL_SUCCESS;
}


arma::mat nonlinearSolveND(
    std::function<arma::mat(const arma::mat& x)> obj,
    const arma::mat& x0,
    double tol,
    std::function<void(void)> perIterationCallback
    )
{

    const double relax=0.1;
    const gsl_multiroot_fsolver_type *T;
    gsl_multiroot_fsolver *s;

    int status;
    size_t n = x0.n_elem;
    size_t i, iter = 0;

    gsl_multiroot_function f = {&f_adapter, n, &obj};

    gsl_vector *x = gsl_vector_alloc (n);
    gsl_vector * olditer_p = gsl_vector_alloc (n);
    for (int i=0; i<n; ++i)
        gsl_vector_set (x, i, x0(i));

    //T = gsl_multiroot_fsolver_hybrids;
    T = gsl_multiroot_fsolver_hybrid;
    s = gsl_multiroot_fsolver_alloc (T, n);
    gsl_multiroot_fsolver_set (s, &f, x);

    gsl_vector_set_all (s->dx, 0.1);
    //    print_state (iter, s);

    do
    {
        std::cout<<"iter="<<iter<<std::endl;
        gsl_vector_memcpy(olditer_p, s->x);

        iter++;
        status = gsl_multiroot_fsolver_iterate (s);

        //      print_state (iter, s);

        if (status)   /* check if solver is stuck */
            break;

        status =
            gsl_multiroot_test_residual (s->f, tol);


//        // relax
//        for (int i=0; i<n; i++)
//        {
//            gsl_vector_set(s->x, i,
//                           relax * gsl_vector_get(s->x, i)
//                               +
//                               (1.-relax) * gsl_vector_get(olditer_p, i)
//                           );
//        }

        if (perIterationCallback)
            perIterationCallback();
    }
    while (status == GSL_CONTINUE && iter < 1000);

    printf ("status = %s\n", gsl_strerror (status));

    arma::mat result=arma::zeros(n);
    for (int i=0;i<n;++i)
        result(i)=gsl_vector_get(s->x,i);

    gsl_multiroot_fsolver_free (s);
    gsl_vector_free (x);

    return result;
}



void ConstrainedSketch::resolveConstraints(
    std::function<void(void)> perIterationCallback
    )
{
    struct LocalIndexMapping { insight::cad::ConstrainedSketchEntity* entity; int iLocal; };
    std::vector<LocalIndexMapping> dofs, constrs;

    for (auto& e: geometry())
    {
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
            insight::dbg()<<"set "<<dm.entity->type()<<", "<<dm.iLocal<<"="<<x(i)<<std::endl;
            dm.entity->setDoFValue(dm.iLocal, x(i));
        }
        for (auto& se: geometry())
        {
            if ( auto e = std::dynamic_pointer_cast<ASTBase>(se) )
            {
                e->invalidate();
            }
        }
    };

    arma::mat xsol;
    switch (solverType_)
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
            x0, solverTolerance_, arma::mat(), 10000,
            0.1
            );
    }
    break;

    case rootND:
    {
        insight::assertion(constrs.size()==dofs.size(),
                           "Cannot solve: number of contraints (%d) not equal to number of DoF (%d)!",
                           constrs.size(), dofs.size());

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
                    insight::dbg()<<"get "<<constr.entity->type()<<", "<<constr.iLocal<<"="<<Qs(i)<<std::endl;
                }
                insight::dbg()<<"Q="<<Qs.t()<<std::endl;
                return Qs;
            },
            x0, solverTolerance_,
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

    int i=0;
    for (const auto& se: geometry())
    {
        entityLabels[se.get()]=i++;
    }

    ConstrainedSketchScriptBuffer sb;
    for (const auto& se: geometry())
    {
        se->generateScriptCommand(sb, entityLabels);
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
            if ( auto f = std::dynamic_pointer_cast<Feature>(sg) )
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
