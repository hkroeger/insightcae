
#include "numericalwindtunnel.h"
#include "openfoam/blockmesh.h"

#include "cadfeatures.h"
#include "occtools.h"

namespace insight
{



NumericalWindtunnel::supplementedInputData::supplementedInputData(
    ParameterSetInput ip,
    const boost::filesystem::path &workDir,
    ActionProgress &ap )
    : supplementedInputDataDerived<Parameters>( ip.forward<Parameters>(), workDir, ap ),
    FOname_allObjects("forcesAllObjects")
{
    CurrentExceptionContext ex("computing further preprocessing informations");

    auto toWindTunnelCS = deferred("coordinate transformation", [this]()
    {
        double L_upw=arma::norm(p().geometry.transformation.upwarddir,2);
        if (L_upw<1e-12)
            throw insight::Exception("Upward direction vector has zero length!");

        double L_fwd=arma::norm(p().geometry.transformation.forwarddir,2);
        if (L_fwd<1e-12)
            throw insight::Exception("Forward direction vector has zero length!");

        if ( fabs(arma::dot(p().geometry.transformation.upwarddir/L_upw,
                           p().geometry.transformation.forwarddir/L_fwd)-1.) < 1e-12 )
        {
            throw insight::Exception("Upward and forward direction are colinear!");
        }

        SpatialTransformation initialMove(-p().geometry.transformation.localOrigin);

        cad::is_gp_Trsf gprot; gprot.SetTransformation
            (
                gp_Ax3(gp_Pnt(0,0,0), gp_Dir(0,0,1), gp_Dir(-1,0,0)),
                gp_Ax3(gp_Pnt(0,0,0),
                       toVec<gp_Dir>(p().geometry.transformation.upwarddir),
                       toVec<gp_Dir>(p().geometry.transformation.forwarddir) )
                );
        auto rot = gprot.toSpatialTransformation()*initialMove;

        return
            SpatialTransformation( p().geometryscale ) // 2. scale
            * rot; // 1. rotate
    });

    SpatialTransformation toAttitude(
        vec3Zero(),
        vec3(
            p().geometry.attitude.roll,
            p().geometry.attitude.trim,
            p().geometry.attitude.yaw) );

    // all objects are loaded in parallel
    for (auto& g: p().geometry.objects)
    {
        const auto& label=g.first;
        transformedObjects_[label] =
            deferred("geometry/"+label, [this,label,toWindTunnelCS,toAttitude](ActionProgress& ap)
            {
                ap.message("Loading geometry file, computing bounding box");

                TransformedObject o;
                auto geom=cad::Transform::create(
                    p().geometry.objects.at(label)->geometry(), toWindTunnelCS );
                o.bb=geom->modelBndBox();

                geom=cad::Transform::create(geom, toAttitude);
                o.bbAtt=geom->modelBndBox();
                o.geometry=geom;
                return o;
            });
    }

    // bounding box in SI, rotated to wind tunnel CS
    auto bb = deferred("bounding box", [this]()
    {
        arma::mat bb=initializedBndBox();
        for (auto& to: transformedObjects_)
            bb=unitedBndBox(bb, to.second->bb);
        return bb;
    });

    Lref_ = deferred("Lref", [bb]()
    {
        double Lref = arma::norm( bb->col(1)-bb->col(0), 2);
        if (Lref < 1e-12)
            throw insight::Exception("Bounding box of object has zero size!");
        return Lref;
    });
    reportSupplementQuantity("Lref", Lref_, "Reference length of object", "m");

    // bounding box in SI, rotated to wind tunnel CS + applied attitude change
    auto bbAtt = deferred("bounding box with attitude", [this]()
    {
        arma::mat bbAtt=initializedBndBox();
        for (auto& to: transformedObjects_)
            bbAtt=unitedBndBox(bbAtt, to.second->bbAtt);
        return bbAtt;
    });

    auto pmin = deferred("pmin", [bbAtt]() -> arma::mat { return bbAtt->col(0); });
    reportSupplementQuantity("pmin", pmin, "Minimum point of bounding box", "m");
    auto pmax = deferred("pmax", [bbAtt]() -> arma::mat { return bbAtt->col(1); });
    reportSupplementQuantity("pmax", pmax, "Maximum point of bounding box", "m");

    l_ = deferred("l", [pmin,pmax]()
    {
        double l=(pmax->at(0)-pmin->at(0));
        if (l<1e-12)
            throw insight::Exception("Length of the object is zero!");
        return l;
    });
    reportSupplementQuantity("l", l_, "object length", "m");

    w_ = deferred("w", [pmin,pmax]()
    {
        double w=(pmax->at(1)-pmin->at(1));
        if (w<1e-12)
            throw insight::Exception("Width of the object is zero!");
        return w;
    });
    reportSupplementQuantity("w", w_, "object width", "m");

    hup_ = deferred("hup", [pmin,pmax]()
    {
        if ((pmax->at(2)-pmin->at(2))<1e-12)
            throw insight::Exception("Height of the object is zero!");
        return pmax->at(2);
    });
    dlo_ = deferred("dlo", [pmin]() { return -pmin->at(2); });
    reportSupplementQuantity("hup", hup_, "object height above local origin", "m");
    reportSupplementQuantity("dlo", dlo_, "object extent below local origin", "m");


    Lupstream_ = deferred("Lupstream", [this]() { return Lref_*p().geometry.LupstreamByL; });
    reportSupplementQuantity("Lupstream", Lupstream_, "Domain extent upstream from object", "m");
    Ldownstream_ = deferred("Ldownstream", [this]() { return Lref_*p().geometry.LdownstreamByL; });
    reportSupplementQuantity("Ldownstream", Ldownstream_, "Domain extent downstream from object", "m");
    Hdom_ = deferred("Hdom", [this]() { return Lref_*p().geometry.LupByL; });
    reportSupplementQuantity("Hdom", Hdom_, "Domain height", "m");
    Laside_ = deferred("Laside", [this]() { return Lref_*p().geometry.LasideByL; });
    reportSupplementQuantity("Laside", Laside_, "Domain sideways from object", "m");


    // vertical location of the object
    auto h = deferred("vertical placement", [this]()
    {
        double h;
        if (boost::get<Parameters::geometry_type::verticalPlacement_onFloor_type>(
                &p().geometry.verticalPlacement))
        {
            // no vertical translation
            h=0.;
        }
        else if (boost::get<Parameters::geometry_type::verticalPlacement_onCeiling_type>(
                     &p().geometry.verticalPlacement))
        {
            // no vertical translation
            h=Hdom_;
        }
        else if (boost::get<Parameters::geometry_type::verticalPlacement_centered_type>(
                     &p().geometry.verticalPlacement))
        {
            h=Hdom_*0.5;
        }
        else if (auto * ah =
                 boost::get<Parameters::geometry_type::verticalPlacement_atHeight_type>(
                     &p().geometry.verticalPlacement))
        {
            if (auto *rdh = boost::get<Parameters::geometry_type
                                       ::verticalPlacement_atHeight_type
                                       ::height_relativeToDomain_type>(
                    &ah->height))
            {
                h=rdh->hByHdomain*Hdom_;
            }
            else if (auto *absh = boost::get<Parameters::geometry_type
                                             ::verticalPlacement_atHeight_type
                                             ::height_absolute_type>(
                         &ah->height))
            {
                h=absh->h;
            }
            else
                throw insight::UnhandledSelection();
        }
        else
            throw insight::UnhandledSelection();

        return std::max<double>(0,std::min<double>(Hdom_,h));
    });

    Lup_ = deferred("Lup", [this,h]() { return std::max<double>(0., Hdom_-h-hup_); });
    reportSupplementQuantity("Lup", Lup_, "Domain height above lower bound of object", "m");

    Ldown_ = deferred("Ldown", [this,h]() { return std::max<double>(0., h-dlo_); });
    reportSupplementQuantity("Ldown", Ldown_, "Domain height below object", "m");


    geometry_ = deferred("placed geometry", [this,pmin,pmax,h]()
    {
        SpatialTransformation windTunnelPlacement(
            vec3(-pmin->at(0),-0.5*(pmin->at(1)+pmax->at(1)),h)
            );

        // apply to geometry
        std::map<std::string,cad::FeaturePtr> geometry;
        for (auto& to: transformedObjects_)
        {
            geometry[to.first]=cad::Transform::create(to.second->geometry, windTunnelPlacement);
        }
        return geometry;
    });


    blocking = deferred("blocking", [this]()
    {
        // inputs
        const double
            Lref_=this->Lref_, l_=this->l_, w_=this->w_, hup_=this->hup_, dlo_=this->dlo_,
            Lupstream_=this->Lupstream_, Ldownstream_=this->Ldownstream_,
            Hdom_=this->Hdom_, Laside_=this->Laside_, Lup_=this->Lup_, Ldown_=this->Ldown_;

        double dx=Lref_/double(p().mesh.nx);

        int nx=std::max(1, int(l_/dx));
        int ny=std::max(1, int(w_/dx));
        int nz=std::max(1, int((hup_+dlo_)/dx));

        int n_upstream=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_upstream).calc_n(dx, Lupstream_));
        int n_downstream=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_downstream).calc_n(dx, Ldownstream_));
        int n_up=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_up).calc_n(dx, Lup_));
        int n_down=std::max(1, bmd::GradingAnalyzer(p().mesh.grad_up).calc_n(dx, Ldown_));
        int n_aside=(Laside_>SMALL)?
                          std::max(1, bmd::GradingAnalyzer(p().mesh.grad_aside).calc_n(dx, Laside_-0.5*w_))
                                        : 0
            ;


        using namespace insight::bmd;
        auto blocking=std::make_shared<blockMeshBlocking>();

        blocking->setScaleFactor(1.0);
        blocking->setDefaultPatch("walls", "patch");


        Patch& inlet = 	blocking->addPatch("inlet", new Patch());
        Patch& outlet = blocking->addPatch("outlet", new Patch());
        Patch& side1 = 	blocking->addPatch("side1", new Patch());
        Patch& side2 = 	blocking->addPatch("side2", new Patch());
        Patch& top = 	blocking->addPatch("top", new Patch("symmetryPlane"));
        Patch& floor = 	blocking->addPatch("floor", new Patch("wall"));

        // points in cross section
        std::map<int, bmd::Point> pts = {
            {100, 	vec3( -Lupstream_, 0, 0)},
            {101, 	vec3( 0, 0, 0)},
            {102, 	vec3( l_, 0, 0)},
            {103, 	vec3( l_+Ldownstream_, 0, 0)},

            {0, 	vec3( -Lupstream_, 0, Ldown_)},
            {1, 	vec3( 0, 0, Ldown_)},
            {2, 	vec3( l_, 0, Ldown_)},
            {3, 	vec3( l_+Ldownstream_, 0,Ldown_)},

            {4, 	vec3( -Lupstream_, 0, Hdom_-Lup_)},
            {5, 	vec3( 0, 0, Hdom_-Lup_)},
            {6, 	vec3( l_, 0, Hdom_-Lup_)},
            {7, 	vec3( l_+Ldownstream_, 0, Hdom_-Lup_)},

            {8, 	vec3( -Lupstream_, 0, Hdom_)},
            {9, 	vec3( 0, 0, Hdom_)},
            {10, 	vec3( l_, 0, Hdom_)},
            {11, 	vec3( l_+Ldownstream_, 0, Hdom_)}
        };
        arma::mat Lv=vec3(0,1,0);

        std::vector<int> nzs;
        std::vector<double> grads;
        std::vector<arma::mat> y0;

        if (p().mesh.longitudinalSymmetry)
        {
            nzs= {n_aside, std::max(1,ny/2)};
            grads = {1./p().mesh.grad_aside, 1};
            y0 = {vec3(0,Laside_,0), vec3(0,0.5*w_,0), vec3(0,0,0)};
        }
        else
        {
            nzs= {n_aside, ny, n_aside};
            grads = {1./p().mesh.grad_aside, 1, p().mesh.grad_aside};
            y0 = {vec3(0,Laside_,0), vec3(0,0.5*w_,0), vec3(0,-0.5*w_,0), vec3(0,-Laside_,0)};
        }

        int iFirst = 0;
        if (nzs[iFirst]<1) iFirst++;

        int iLast = nzs.size()-1;
        if (nzs[iLast]<1) iLast--;

        for (size_t i=0; i<nzs.size(); i++)
        {
            if (nzs[i]>0)
            {
                if (Ldown_>SMALL)
                {
                    if (Lupstream_>SMALL)
                    {
                        Block& bl = blocking->addBlock
                                    (
                                        new Block(P_8(
                                                      pts[100]+y0[i], pts[101]+y0[i], pts[1]+y0[i], pts[0]+y0[i],
                                                      pts[100]+y0[i+1], pts[101]+y0[i+1], pts[1]+y0[i+1], pts[0]+y0[i+1]
                                                      ),
                                                  n_upstream, n_down, nzs[i],
                                                  { 1./p().mesh.grad_upstream, 1./p().mesh.grad_up, grads[i] }
                                                  )
                                        );

                        inlet.addFace(bl.face("0473"));
                        floor.addFace(bl.face("0154"));
                        if (i==iFirst) side1.addFace(bl.face("0321"));
                        if (i==iLast) side2.addFace(bl.face("4567"));
                    }
                    {
                        Block& bl = blocking->addBlock
                                    (
                                        new Block(P_8(
                                                      pts[101]+y0[i], pts[102]+y0[i], pts[2]+y0[i], pts[1]+y0[i],
                                                      pts[101]+y0[i+1], pts[102]+y0[i+1], pts[2]+y0[i+1], pts[1]+y0[i+1]
                                                      ),
                                                  nx, n_down, nzs[i],
                                                  { 1., 1./p().mesh.grad_up, grads[i] }
                                                  )
                                        );
                        floor.addFace(bl.face("0154"));
                        if (i==iFirst) side1.addFace(bl.face("0321"));
                        if (i==iLast) side2.addFace(bl.face("4567"));
                        if (!(Ldownstream_>SMALL))
                            outlet.addFace(bl.face("1265"));
                        if (!(Lupstream_>SMALL))
                            inlet.addFace(bl.face("0473"));
                    }
                    if (Ldownstream_>SMALL)
                    {
                        Block& bl = blocking->addBlock
                                    (
                                        new Block(P_8(
                                                      pts[102]+y0[i], pts[103]+y0[i], pts[3]+y0[i], pts[2]+y0[i],
                                                      pts[102]+y0[i+1], pts[103]+y0[i+1], pts[3]+y0[i+1], pts[2]+y0[i+1]
                                                      ),
                                                  n_downstream, n_down, nzs[i],
                                                  { p().mesh.grad_downstream, 1./p().mesh.grad_up, grads[i] }
                                                  )
                                        );
                        outlet.addFace(bl.face("1265"));
                        floor.addFace(bl.face("0154"));
                        if (i==iFirst) side1.addFace(bl.face("0321"));
                        if (i==iLast) side2.addFace(bl.face("4567"));
                    }
                }

                if (Lupstream_>SMALL)
                {
                    Block& bl = blocking->addBlock
                                (
                                    new Block(P_8(
                                                  pts[0]+y0[i], pts[1]+y0[i], pts[5]+y0[i], pts[4]+y0[i],
                                                  pts[0]+y0[i+1], pts[1]+y0[i+1], pts[5]+y0[i+1], pts[4]+y0[i+1]
                                                  ),
                                              n_upstream, nz, nzs[i],
                                              { 1./p().mesh.grad_upstream, 1., grads[i] }
                                              )
                                    );

                    inlet.addFace(bl.face("0473"));
                    if (!(Ldown_>0.)) floor.addFace(bl.face("0154"));
                    if (!(Lup_>0)) top.addFace(bl.face("2376"));
                    if (i==iFirst) side1.addFace(bl.face("0321"));
                    if (i==iLast) side2.addFace(bl.face("4567"));
                }
                {
                    Block& bl = blocking->addBlock
                                (
                                    new Block(P_8(
                                                  pts[1]+y0[i], pts[2]+y0[i], pts[6]+y0[i], pts[5]+y0[i],
                                                  pts[1]+y0[i+1], pts[2]+y0[i+1], pts[6]+y0[i+1], pts[5]+y0[i+1]
                                                  ),
                                              nx, nz, nzs[i],
                                              { 1., 1., grads[i] }
                                              )
                                    );
                    if (!(Ldown_>SMALL)) floor.addFace(bl.face("0154"));
                    if (!(Lup_>SMALL)) top.addFace(bl.face("2376"));
                    if (i==iFirst) side1.addFace(bl.face("0321"));
                    if (i==iLast) side2.addFace(bl.face("4567"));
                    if (!(Ldownstream_>SMALL))
                        outlet.addFace(bl.face("1265"));
                    if (!(Lupstream_>SMALL))
                        inlet.addFace(bl.face("0473"));
                }
                if (Ldownstream_>SMALL)
                {
                    Block& bl = blocking->addBlock
                                (
                                    new Block(P_8(
                                                  pts[2]+y0[i], pts[3]+y0[i], pts[7]+y0[i], pts[6]+y0[i],
                                                  pts[2]+y0[i+1], pts[3]+y0[i+1], pts[7]+y0[i+1], pts[6]+y0[i+1]
                                                  ),
                                              n_downstream, nz, nzs[i],
                                              { p().mesh.grad_downstream, 1., grads[i] }
                                              )
                                    );
                    outlet.addFace(bl.face("1265"));
                    if (!(Ldown_>0.)) floor.addFace(bl.face("0154"));
                    if (!(Lup_>0)) top.addFace(bl.face("2376"));
                    if (i==iFirst) side1.addFace(bl.face("0321"));
                    if (i==iLast) side2.addFace(bl.face("4567"));
                }

                if (Lup_>SMALL)
                {
                    if (Lupstream_>SMALL)
                    {
                        Block& bl = blocking->addBlock
                                    (
                                        new Block(P_8(
                                                      pts[4]+y0[i], pts[5]+y0[i], pts[9]+y0[i], pts[8]+y0[i],
                                                      pts[4]+y0[i+1], pts[5]+y0[i+1], pts[9]+y0[i+1], pts[8]+y0[i+1]
                                                      ),
                                                  n_upstream, n_up, nzs[i],
                                                  { 1./p().mesh.grad_upstream, p().mesh.grad_up, grads[i] }
                                                  )
                                        );
                        inlet.addFace(bl.face("0473"));
                        top.addFace(bl.face("2376"));
                        if (i==0) side1.addFace(bl.face("0321"));
                        if (i==iLast) side2.addFace(bl.face("4567"));
                    }
                    {
                        Block& bl = blocking->addBlock
                                    (
                                        new Block(P_8(
                                                      pts[5]+y0[i], pts[6]+y0[i], pts[10]+y0[i], pts[9]+y0[i],
                                                      pts[5]+y0[i+1], pts[6]+y0[i+1], pts[10]+y0[i+1], pts[9]+y0[i+1]
                                                      ),
                                                  nx, n_up, nzs[i],
                                                  { 1, p().mesh.grad_up, grads[i] }
                                                  )
                                        );
                        top.addFace(bl.face("2376"));
                        if (i==iFirst) side1.addFace(bl.face("0321"));
                        if (i==iLast) side2.addFace(bl.face("4567"));
                        if (!(Ldownstream_>SMALL))
                            outlet.addFace(bl.face("1265"));
                        if (!(Lupstream_>SMALL))
                            inlet.addFace(bl.face("0473"));
                    }
                    if (Ldownstream_>SMALL)
                    {
                        Block& bl = blocking->addBlock
                                    (
                                        new Block(P_8(
                                                      pts[6]+y0[i], pts[7]+y0[i], pts[11]+y0[i], pts[10]+y0[i],
                                                      pts[6]+y0[i+1], pts[7]+y0[i+1], pts[11]+y0[i+1], pts[10]+y0[i+1]
                                                      ),
                                                  n_downstream, n_up, nzs[i],
                                                  { p().mesh.grad_downstream, p().mesh.grad_up, grads[i] }
                                                  )
                                        );
                        outlet.addFace(bl.face("1265"));
                        top.addFace(bl.face("2376"));
                        if (i==iFirst) side1.addFace(bl.face("0321"));
                        if (i==iLast) side2.addFace(bl.face("4567"));
                    }
                }
            }
        }

        return blocking;
    });


    PiM_ = deferred("PiM", [this]() -> arma::mat
    {
        double zPiM = 0.5*Ldown_;
        if (zPiM<SMALL)
        {
            zPiM=0.5*Lup_;
            if (zPiM<SMALL)
                throw insight::Exception(
                    "no suitable location for mesh seed point found."
                    " Make sure there is space left vertically above or below the object.");
            zPiM=Hdom_-zPiM;
        }
        return vec3(-0.999*Lupstream_,1e-6,zPiM+1e-6);
    });
}



}
