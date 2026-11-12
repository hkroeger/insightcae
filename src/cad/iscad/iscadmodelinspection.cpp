#include "iscadmodelinspection.h"

#include "cadtypes.h"
#include "cadfeature.h"
#include "cadmodel.h"
#include "cadparameter.h"

#include "TopExp.hxx"
#include "TopTools_IndexedMapOfShape.hxx"

#include <QFile>


using namespace insight::assistant;
typedef ISCADAssistantHost H;


namespace {


H::Vec3 v3(const arma::mat& v)
{
    if (v.n_elem<3)
        return H::Vec3();
    return H::Vec3{v(0), v(1), v(2)};
}


QStringList sortedNames(QStringList l)
{
    l.sort(Qt::CaseInsensitive);
    return l;
}


/**
 * evaluate a property, failures leave it empty
 */
template<class T, class F>
std::optional<T> tryGet(F f)
{
    try
    {
        return f();
    }
    catch (...)
    {
        return std::optional<T>();
    }
}


}




namespace ISCADModelInspection
{


std::shared_ptr<insight::cad::Feature> builtFeature(
    const insight::cad::Model& model,
    const QString& name,
    QString& error )
{
    using insight::cad::FeaturePtr;

    QStringList parts=name.split('.', QString::SkipEmptyParts);
    if (parts.isEmpty())
    {
        error="No feature name given.";
        return FeaturePtr();
    }

    const auto& ms=model.modelsteps();
    auto i=ms.find(parts[0].toStdString());
    if (i==ms.end())
    {
        QStringList known;
        for (const auto& m: ms)
            known<<QString::fromStdString(m.first);
        error=QString("Unknown feature \"%1\". Features of the model: %2")
                .arg(parts[0], known.isEmpty() ? QString("(none)") : sortedNames(known).join(", "));
        return FeaturePtr();
    }

    FeaturePtr f=i->second;
    QString path=parts[0];
    for (int k=1; k<=parts.size(); ++k)
    {
        // accessing an unbuilt feature would build it here
        if (!f || !f->valid())
        {
            error=QString("Feature \"%1\" has not been built. "
                          "Call rebuild first (it might also be beyond the range of a partial rebuild).")
                    .arg(path);
            return FeaturePtr();
        }
        if (k==parts.size())
            break;

        const auto& subs=f->providedSubshapes();
        auto j=subs.find(parts[k].toStdString());
        if (j==subs.end())
        {
            QStringList known;
            for (const auto& sf: subs)
                known<<QString::fromStdString(sf.first);
            error=QString("Feature \"%1\" has no sub-feature \"%2\". Sub-features: %3")
                    .arg(path, parts[k], known.isEmpty() ? QString("(none)") : sortedNames(known).join(", "));
            return FeaturePtr();
        }
        f=j->second;
        path+="."+parts[k];
    }
    return f;
}


H::SymbolTable symbols(const insight::cad::Model& m)
{
    H::SymbolTable t;
    t.ok=true;

    for (const auto& s: m.scalars())
    {
        std::optional<double> v;
        if (s.second && s.second->valid())
            v=tryGet<double>([&]() { return s.second->value(); });
        t.scalars.append({QString::fromStdString(s.first), v});
    }

    auto vecs = [](const insight::cad::Model::VectorTableContents& tab,
                   QList<QPair<QString, std::optional<H::Vec3> > >& out)
    {
        for (const auto& s: tab)
        {
            std::optional<H::Vec3> v;
            if (s.second && s.second->valid())
                v=tryGet<H::Vec3>([&]() { return v3(s.second->value()); });
            out.append({QString::fromStdString(s.first), v});
        }
    };
    vecs(m.points(), t.points);
    vecs(m.directions(), t.directions);

    for (const auto& d: m.datums())
        t.datums<<QString::fromStdString(d.first);
    t.datums=sortedNames(t.datums);

    for (const auto& f: m.modelsteps())
    {
        H::SymbolTable::FeatureInfo fi;
        fi.name=QString::fromStdString(f.first);
        fi.isComponent=m.isComponent(f.first);
        fi.built=f.second && f.second->valid();
        t.features.append(fi);
    }

    return t;
}


H::FeatureProperties featureProperties(const insight::cad::Model& model, const QString& name)
{
    H::FeatureProperties p;

    auto f=builtFeature(model, name, p.error);
    if (!f)
        return p;

    p.ok=true;
    if (!name.contains('.'))
        p.isComponent=model.isComponent(name.toStdString());

    try
    {
        // count each shared entity once
        // (Feature::topologicalProperties counts edges per face)
        auto count = [&f](TopAbs_ShapeEnum t)
        {
            TopTools_IndexedMapOfShape m;
            TopExp::MapShapes(f->shape(), t, m);
            return m.Extent();
        };
        p.nSolids=count(TopAbs_SOLID);
        p.nShells=count(TopAbs_SHELL);
        p.nFaces=count(TopAbs_FACE);
        p.nEdges=count(TopAbs_EDGE);
        p.nVertices=count(TopAbs_VERTEX);
    }
    catch (...) {}

    p.volume=tryGet<double>([&]() { return f->modelVolume(); });
    p.surfaceArea=tryGet<double>([&]() { return f->modelSurfaceArea(); });

    try
    {
        arma::mat bb=f->modelBndBox();
        p.bbMin=v3(bb.col(0));
        p.bbMax=v3(bb.col(1));
    }
    catch (...) {}

    if (p.volume && *p.volume>0)
    {
        p.cog=tryGet<H::Vec3>([&]() { return v3(f->modelCoG()); });
    }
    else
    {
        p.cog=tryGet<H::Vec3>([&]() { return v3(f->surfaceCoG()); });
        p.cogIsSurface=true;
    }

    try
    {
        for (const auto& s: f->providedSubshapes())
            p.subfeatures<<QString::fromStdString(s.first);
        p.subfeatures=sortedNames(p.subfeatures);
        for (const auto& s: f->getDatumScalars())
            p.scalars.append({QString::fromStdString(s.first), s.second});
        for (const auto& s: f->getDatumPoints())
            p.points.append({QString::fromStdString(s.first), v3(s.second)});
        for (const auto& s: f->getDatumVectors())
            p.vectors.append({QString::fromStdString(s.first), v3(s.second)});
        for (const auto& s: f->providedDatums())
            p.datums<<QString::fromStdString(s.first);
        p.datums=sortedNames(p.datums);
    }
    catch (...) {}

    return p;
}


QList<CommandDoc> commandDocumentation()
{
    QList<CommandDoc> docs;
    if (insight::cad::Feature::ruleDocumentationFunctions_)
    {
        for (const auto& rd: *insight::cad::Feature::ruleDocumentationFunctions_)
        {
            for (const auto& info: rd.second())
            {
                docs.append({
                    QString::fromStdString(info.command_),
                    QString::fromStdString(info.signature_),
                    QString::fromStdString(info.documentation_)
                });
            }
        }
    }
    return docs;
}


QStringList librarySearchPaths()
{
    QStringList paths;
    insight::cad::sharedModelLocations locs;
    for (const auto& p: locs)
    {
        if (p==boost::filesystem::path("."))
            continue;
        paths<<QString::fromStdString(p.string());
    }
    return paths;
}


QString languageReference(ReferenceLevel level)
{
    QStringList files;
    if (level==FullReference || level==BasicReference)
        files<<":/iscad_assistant/insightcad.md";
    if (level==FullReference)
        files<<":/iscad_assistant/iscad-reference.md";

    QString ref;
    for (const auto& fn: files)
    {
        QFile f(fn);
        if (f.open(QIODevice::ReadOnly|QIODevice::Text))
        {
            if (!ref.isEmpty())
                ref+="\n\n";
            ref+=QString::fromUtf8(f.readAll());
        }
    }
    return ref;
}


}
