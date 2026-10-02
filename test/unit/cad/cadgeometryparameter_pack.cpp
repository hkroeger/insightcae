#include "base/exception.h"
#include "base/rapidxml.h"
#include "cadgeometryparameter.h"

#include <fstream>

using namespace insight;

int main(int argc, char* argv[])
{
    try
    {
        auto dir = boost::filesystem::temp_directory_path()
                   / boost::filesystem::unique_path("cadgeometryparameter_pack-%%%%%%");
        boost::filesystem::create_directories(dir);

        auto geomFile = dir / "geometry.stl";
        {
            std::ofstream f(geomFile.string());
            f << "solid dummy\nendsolid dummy\n";
        }

        CADGeometryParameter p(geomFile, "test geometry");
        p.resolveRelativePaths(dir);
        insight::assertion(!p.isPacked(), "parameter must not be packed initially");

        // packing a clone must not affect the original
        {
            auto c = p.cloneAs<CADGeometryParameter>();
            c->pack();
            insight::assertion(c->isPacked(), "clone must be packed");
            insight::assertion(!p.isPacked(), "original must not be packed by clone");
        }

        // clearing packed data of a clone must not affect the original
        p.pack();
        insight::assertion(p.isPacked(), "original must be packed");
        {
            auto c = p.cloneAs<CADGeometryParameter>();
            c->clearPackedData();
            insight::assertion(!c->isPacked(), "clone must be unpacked");
            insight::assertion(p.isPacked(), "original must stay packed");
        }

        // write packed, remove external file, read back
        {
            rapidxml::xml_document<> doc;
            auto *root = doc.allocate_node(rapidxml::node_element, "root");
            doc.append_node(root);
            p.appendToNode("geom", doc, *root, hierarchicalData::Element::OutputProperties());

            boost::filesystem::remove(geomFile);

            CADGeometryParameter r("read back");
            r.resolveRelativePaths(dir);
            r.readFromNode("geom", *root);
            insight::assertion(r.isPacked(), "read parameter must be packed");
            insight::assertion(
                r.baseDirectory() && *r.baseDirectory()==dir,
                "base directory must be preserved on read");
            r.pack(); // must not throw (relative path with base directory)
            insight::assertion(r.isPacked(), "read parameter must remain packed");

            auto afp = r.accessibleFilePath();
            insight::assertion(bool(afp), "expected file path");
            insight::assertion(
                boost::filesystem::exists(*afp),
                "unpacked file must exist: "+afp->string());
        }

        // assignment across source types
        {
            {
                std::ofstream f(geomFile.string());
                f << "solid dummy\nendsolid dummy\n";
            }

            CADGeometryParameter s("script");
            s.setScript("dummy script");
            CADGeometryParameter f(geomFile, "file");

            f.assignFrom(s);
            insight::assertion(f.isEqual(s), "file <- script assignment failed");

            CADGeometryParameter f2(geomFile, "file");
            f2.resolveRelativePaths(dir);
            s.assignFrom(f2);
            insight::assertion(s.isEqual(f2), "script <- file assignment failed");
            s.pack();
            insight::assertion(s.isPacked(), "assigned parameter must be packable");
            insight::assertion(!f2.isPacked(), "assigned parameter must not share container");
        }

        // default constructed parameter (null file container)
        {
            CADGeometryParameter d("default");
            insight::assertion(!d.isPacked(), "default must not be packed");
            d.pack();
            d.clearPackedData();
            insight::assertion(!d.filePath(), "default must have no file path");
            auto c = d.cloneAs<CADGeometryParameter>();
            insight::assertion(c->isEqual(d), "default clone must be equal");
        }

        boost::filesystem::remove_all(dir);
    }
    catch (const std::exception& e)
    {
        std::cerr<<e.what()<<std::endl;
        return -1;
    }

    return 0;
}
