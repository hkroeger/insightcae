#include "constrainedsketch.h"

#include "base/exception.h"
#include "base/linearalgebra.h"
#include "base/tools.h"

#include "boost/range/adaptor/indexed.hpp"
#include "cadfeature.h"
#include "cadfeatures/wire.h"
#include "constrainedsketchgrammar.h"

#include "parser.h"
#include "datum.h"
#include <algorithm>
#include <memory>
#include <string>


namespace insight {
namespace cad {




defineType(ConstrainedSketch);
addToStaticFunctionTable(Feature, ConstrainedSketch, insertrule);


const std::string ConstrainedSketch::defaultLayerName  = "standard";


size_t ConstrainedSketch::calcHash() const
{
    ParameterListHash p;
    p+=*pl_;
    for (const auto& se: geometry_)
        p+=se.second->hash();
    return p.getHash();
}



std::unique_ptr<LayerProperties> LayerProperties::create()
{
    return std::unique_ptr<LayerProperties>(new LayerProperties);
}

std::unique_ptr<LayerProperties> LayerProperties::create(
    const ParameterSet& parameters )
{
    return std::unique_ptr<LayerProperties>(
        new LayerProperties(parameters.copyEntries(), "") );
}



void
ConstrainedSketchParametersDelegate::changeDefaultParameters(
    ConstrainedSketchEntity& e) const
{}

std::unique_ptr<LayerProperties>
ConstrainedSketchParametersDelegate::createDefaultLayerProperties(
    const std::string &layerName) const
{
    return LayerProperties::create();
}


std::shared_ptr<ConstrainedSketchParametersDelegate>
    noParametersDelegate(
        new ConstrainedSketchParametersDelegate
        );



defineType(ConstrainedSketchPresentationDelegate);

defineStaticFunctionTable2(
    "delegates for entity presentation in constrained sketch editor",
    ConstrainedSketchPresentationDelegate, DelegateFactories, delegates );

IQParameterSetModel*
ConstrainedSketchPresentationDelegate::setupSketchEntityParameterSetModel(
    const ConstrainedSketchEntity& e) const
{
    return nullptr;
}


IQParameterSetModel*
ConstrainedSketchPresentationDelegate::setupLayerParameterSetModel(
    const std::string& layerName, const LayerProperties& e) const
{
    return nullptr;
}


void ConstrainedSketchPresentationDelegate::setEntityAppearance(
    const ConstrainedSketchEntity& e, vtkProperty* actprops) const
{}





ConstrainedSketch::ConstrainedSketch(
    DatumPtr pl,
    const ConstrainedSketchParametersDelegate& pd )
    : pl_(pl),
    solverSettings_({rootND, insight::SMALL, 1., 1000})
{
    addLayer( defaultLayerName, pd );
}


ConstrainedSketch::ConstrainedSketch( const ConstrainedSketch& other )
    : pl_(other.pl_),
    solverSettings_(other.solverSettings_)
{
    for (auto &lp: other.layerProperties_)
    {
        layerProperties_.insert({
            lp.first,
            lp.second->cloneLayerProperties()
        });
    }

    std::map<
        std::comparable_weak_ptr<ConstrainedSketchEntity>,  // in original
        std::shared_ptr<ConstrainedSketchEntity> // in cloned
        > cem;

    // clone entities
    for (const auto& g: other.geometry_)
    {
        auto clone = g.second->clone();
        if (propertiesParent_)
        {
            clone->parametersRef().setParent(
                propertiesParent_.get());
        }
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


void ConstrainedSketch::setParentParameter(Parameter *p)
{
    propertiesParent_=p;
    if (propertiesParent_)
    {
        for (auto& g: geometry_)
        {
            g.second->parametersRef().setParent(propertiesParent_.get());
        }
    }
}

Parameter* ConstrainedSketch::parentParameter() const
{
    return const_cast<Parameter*>(propertiesParent_.get());
}


std::shared_ptr<ConstrainedSketch> ConstrainedSketch::createFromStream(
    DatumPtr pl, istream &in,
    const ConstrainedSketchParametersDelegate& pd )
{
    auto sk = ConstrainedSketch::create(pl, pd);
    sk->readFromStream(in, pd);
    return sk;
}




void ConstrainedSketch::readFromStream(
    istream &in,
    const ConstrainedSketchParametersDelegate& pd )
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
        pd );

    bool success = boost::spirit::qi::phrase_parse(
        first,  last,
        grammar, skip
        );

    insight::assertion(
        success,
        "parsing of constrained sketch script was not successful!" );

    // add referenced layers, if not present
    for (auto& ske: geometry_)
    {
        auto & l = ske.second->layerName();
        if (!layerProperties_.count(l))
        {
            addLayer(l, pd);
        }
    }

    if (propertiesParent_)
    {
        for (auto& g: geometry_)
        {
            g.second->parametersRef().setParent(propertiesParent_.get());
        }
    }
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

arma::mat ConstrainedSketch::p3Dto2D(const arma::mat &p3d) const
{
    auto pl=plane()->plane();
    auto p0=vec3(pl.Location());
    auto ex=vec3(pl.XDirection());
    auto ey=vec3(pl.YDirection());
    return vec2(
        arma::dot(p3d-p0, ex),
        arma::dot(p3d-p0, ey)
        );
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

std::set<ConstrainedSketchEntityPtr>
ConstrainedSketch::findConnected(
    ConstrainedSketchEntityPtr theEntity ) const
{
    std::set<ConstrainedSketchEntityPtr> conn;

    auto filterPoints = [](const std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> >& deps)
    {
        std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > fdeps;
        std::copy_if(
            deps.begin(), deps.end(),
            std::inserter(fdeps, fdeps.begin()),
            [](const std::comparable_weak_ptr<ConstrainedSketchEntity>& d)
        {
                return bool(std::dynamic_pointer_cast<insight::cad::SketchPoint>(d.lock()));
        });
        return fdeps;
    };

    auto pts=filterPoints(theEntity->dependencies());

    size_t conn_s;
    do
    {
        conn_s=conn.size();
        for (auto& e: *this)
        {
            auto cpts=filterPoints(e.second->dependencies());

            decltype(pts) common;
            std::set_intersection(
                pts.begin(), pts.end(),
                cpts.begin(), cpts.end(),
                std::inserter(common, common.begin())
                );

            if (common.size())
            {
                if (std::dynamic_pointer_cast<cad::Feature>(e.second))
                {
                    conn.insert(e.second);
                    pts.insert(cpts.begin(), cpts.end());
                }
            }
        }
    } while (conn_s!=conn.size());

    return conn;
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
        if (propertiesParent_)
        {
            geomEntity->parametersRef().setParent(
                propertiesParent_.get());
        }
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
    std::function<bool (const ParameterSet &)> filterFunction ) const
{
    std::set<ConstrainedSketchEntityPtr> ret;
    for (auto& e: *this)
    {
        if (filterFunction(e.second->parameters()))
            ret.insert(e.second);
    }
    return ret;
}




FeaturePtr ConstrainedSketch::layerGeometry(const std::string &layerName) const
{
    if (!pl_->providesPlanarReference())
        throw insight::Exception("Sketch: Planar reference required!");

    std::vector<FeaturePtr> edges;

    for ( auto& sg: geometry_ )
    {
        if (sg.second->layerName()==layerName)
        {
            if ( auto f = std::dynamic_pointer_cast<Feature>(sg.second) )
            {
                edges.push_back(f);
            }
        }
    }

    return cad::Wire::create(edges);
}




void ConstrainedSketch::operator=(const ConstrainedSketch &o)
{
    Feature::operator=(o);
    pl_=o.pl_;

    for (auto &lp: o.layerProperties_)
    {
        layerProperties_.insert({
            lp.first,
            lp.second->cloneLayerProperties()
        });
    }

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

void ConstrainedSketchScriptBuffer::appendLayerProp(const std::string &cmd)
{
    layerProps_.push_back(cmd);
}




void ConstrainedSketchScriptBuffer::write(ostream &os)
{
    for (auto sl=layerProps_.begin(); sl!=layerProps_.end(); ++sl)
    {
        os<<*sl<<std::endl;
    }
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

    auto *rule = new ConstrainedSketchRule(
        '('

        > ruleset.r_datumExpression
                  [ qi::_a = phx::bind(
                   &ConstrainedSketch::create<DatumPtr, const ConstrainedSketchParametersDelegate&>,
                   qi::_1, *noParametersDelegate),
               qi::_b = parser::make_shared_<ConstrainedSketchGrammar>()(
                   qi::_a, *noParametersDelegate,
                   phx::val(&ruleset)),
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

    for (auto &lp: layerProperties_)
    {
        std::string s;
        if (lp.second->size())
        {
            lp.second->saveToString(s, boost::filesystem::current_path()/"outfile");
            s=" "+s;
        }
        sb.appendLayerProp("layer "+lp.first+s);
    }

    // std::cout<<">>>>"<<std::endl;
    // sb.write(std::cout);
    // std::cout<<"<<<<"<<std::endl;
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




std::set<std::string> ConstrainedSketch::usedLayerNames() const
{
    std::set<std::string> l;
    std::transform(
        geometry_.begin(), geometry_.end(),
        std::inserter(l, l.begin()),
        [](const GeometryMap::value_type& gv) { return gv.second->layerName(); }
        );
    return l;
}




std::set<std::string> ConstrainedSketch::layerNames() const
{
    auto l = usedLayerNames();
    std::transform(
        layerProperties_.begin(), layerProperties_.end(),
        std::inserter(l, l.begin()),
        [](const decltype(layerProperties_)::value_type& lp)
        { return lp.first; }
        );
    return l;
}

bool ConstrainedSketch::hasLayer(
    const std::string& layerName) const
{
    return layerNames().count(layerName);
}

bool ConstrainedSketch::layerIsUsed(
    const std::string& layerName) const
{
    return usedLayerNames().count(layerName);
}


void ConstrainedSketch::addLayer(
    const std::string& layerName,
    const ConstrainedSketchParametersDelegate& pd )
{
    layerProperties_.insert({
        layerName,
        pd.createDefaultLayerProperties(layerName)
    });
}

const LayerProperties&
ConstrainedSketch::layerProperties(
    const std::string& layerName ) const
{
    return *layerProperties_.at(layerName);
}

void ConstrainedSketch::setLayerProperties(
    const std::string& layerName,
    const ParameterSet &ps)
{
    layerProperties_.at(layerName)->ParameterSet::operator=(ps);
}


void ConstrainedSketch::parseLayerProperties(
    const std::string& layerName,
    const boost::optional<std::string>& s,
    const ConstrainedSketchParametersDelegate& pd )
{
    if (!hasLayer(layerName))
        addLayer(layerName, pd);

    if (s && !s->empty())
    {
        using namespace rapidxml;
        xml_document<> doc;
        doc.parse<0>(const_cast<char*>(&(*s)[0]));
        xml_node<> *rootnode = doc.first_node("root");

        auto p=layerProperties(layerName).cloneLayerProperties();
        p->readFromNode(std::string(), *rootnode, "." );
        setLayerProperties(layerName, *p);
    }
}

void ConstrainedSketch::removeLayer(const std::string &layerName)
{
    if (!layerIsUsed(layerName))
        layerProperties_.erase(layerName);
    else
        throw insight::Exception(
            "cannot remove layer properties for layer %s, it is still in use",
            layerName.c_str() );
}

std::vector<std::weak_ptr<insight::cad::ConstrainedSketchEntity> >
ConstrainedSketch::entitiesInsideRect(
    double x1, double y1, double x2, double y2
    ) const
{
    ConstrainedSketchEntity::SelectionRect r;
    r.sketch=this;
    r.x1=std::min(x1,x2);
    r.y1=std::min(y1,y2);
    r.x2=std::max(x1,x2);
    r.y2=std::max(y1,y2);

    std::vector<std::weak_ptr<insight::cad::ConstrainedSketchEntity> > es;

    for (auto& e: *this)
    {
        if (e.second->isInside(r))
        {
            es.push_back(e.second);
        }
    }
    return es;
}


void ConstrainedSketch::build()
{
    ExecTimer t("ConstrainedSketch::build() ["+featureSymbolName()+"]");

    if (!cache.contains(hash()))
    {
        if (!pl_->providesPlanarReference())
            throw insight::Exception("Sketch: Planar reference required!");

        /* Should only be solved in constrainedsketcheditor under user supervision!!
         * Otherwise, only the saved DoFs are used.
         */
        // try
        // {
        //     resolveConstraints();
        // }
        // catch (insight::NonConvergenceException& ex)
        // {
        //     insight::Warning(ex);
        // }

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
