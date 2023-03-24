#include "vtkrendering.h"

#include <vtkTransformPolyDataFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkTransform.h>

#include "vtkSmartPointer.h"
#include "vtkOpenFOAMReader.h"
#include "vtkArrowSource.h"
#include "vtkGlyphSource2D.h"
#include "vtkCutter.h"
#include "vtkPlane.h"
#include "vtkX3DExporter.h"
#include "vtkRenderer.h"
#include "vtkPolyDataMapper.h"
#include "vtkDataSetMapper.h"
#include "vtkWindowToImageFilter.h"
#include "vtkPNGWriter.h"
#include "vtkProperty.h"
#include "vtkAutoInit.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"
#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkTextProperty.h"
#include "vtkLineSource.h"
#include "vtkRendererCollection.h"
#include "vtkActor2DCollection.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkCleanPolyData.h"
#include "vtkLogLookupTable.h"

#include "vtkPointData.h"
#include "vtkExtractBlock.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkAlgorithmOutput.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkInformation.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkAxesActor.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkLightKit.h"

#include "base/exception.h"
#include "base/spatialtransformation.h"

void vtkRenderingOpenGL2_AutoInit_Construct();
void vtkRenderingFreeType_AutoInit_Construct();
void vtkInteractionStyle_AutoInit_Construct();

using namespace std;
using namespace boost;

namespace insight
{



vtkSmartPointer<vtkPolyData> createArrows(
    std::vector<std::pair<arma::mat, arma::mat> > from_to,
    bool glyph2d
)
{
  auto af = vtkSmartPointer<vtkAppendPolyData>::New();

  for (const auto& ft: from_to)
  {
    const arma::mat& from = ft.first;
    const arma::mat& to = ft.second;

    if (glyph2d)
    {
        auto l = vtkSmartPointer<vtkLineSource>::New();
        l->SetPoint1( toArray(from) );
        l->SetPoint2( toArray(to) );
        l->Update();

        af->AddInputData(l->GetOutput());
    }
    else
    {
        double r=arma::norm(from-to, 2);

        insight::CoordinateSystem cs(from, to-from);

        arma::mat R=arma::zeros(3,3);
        R.col(0)=cs.ex;
        R.col(1)=cs.ey;
        R.col(2)=cs.ez;
        insight::SpatialTransformation st1, st2;
        st1.setScale(r);
        st2.setRotationMatrix(R);

        auto a0 = vtkSmartPointer<vtkArrowSource>::New();
        a0->SetTipRadius(0.025);
        a0->SetTipLength(0.1);
        a0->SetShaftRadius(0.0075);

        auto tf= vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        tf->SetInputConnection(a0->GetOutputPort());
        tf->SetTransform( st1.appended(st2).toVTKTransform() );
        tf->Update();

        af->AddInputData(tf->GetOutput());
    }
  }
  auto cleanFilter = vtkSmartPointer<vtkCleanPolyData>::New();
  cleanFilter->SetInputConnection(af->GetOutputPort());
  cleanFilter->Update();

  return cleanFilter->GetOutput();
}




const std::vector<double> colorMapData_SD = {
    0.0,    0.0,    0.333333,    1,
    0.2003664868,    0.576471,    1,    0.988235,
    0.4172793444,    0.0,    0.870588,    0.0862745,
    0.6102942075,    0.976471,    0.94902,    0.0980392,
    0.8180138451,    1.0,    0.0,    0.0,
    1.0,    1.0,    0.0,    1.0
    };


vtkSmartPointer<vtkLookupTable> createColorMap(
        const std::vector<double>& cb,
        int nc,
        bool logscale
        )
{
    auto gen = vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();

    gen->DiscretizeOn();

    int n=cb.size()/4;
    gen->SetNumberOfIndexedColors(n);
    for (int i=0; i<n; i++)
    {
        int j=4*i;
        gen->AddRGBPoint(cb[j], cb[j+1], cb[j+2], cb[j+3]);
    }
    gen->SetNumberOfValues(nc);
    gen->SetUseLogScale(logscale);
    gen->Build();

    vtkSmartPointer<vtkLookupTable> lut;
    if (logscale)
    {
        lut = vtkSmartPointer<vtkLogLookupTable>::New();
        lut->SetScaleToLog10();
    }
    else
    {
        lut = vtkSmartPointer<vtkLookupTable>::New();
    }
    lut->SetNumberOfTableValues(nc);
    for (int i=0; i<nc; i++)
    {
        double rgb[4];
        gen->GetColor( double(i)/double(nc-1), rgb);
        lut->SetTableValue(i, rgb[0], rgb[1], rgb[2]);
    }

    lut->Build();

    return lut;
}



MinMax calcRange(
    FieldSelection fsel,
    const std::vector<vtkDataSet*> datasets,
    const std::vector<vtkAlgorithm*> algorithms
    )
{
    insight::CurrentExceptionContext ex("determining range of field "+fsel.fieldName());

    auto fieldname = boost::fusion::get<0>(fsel);
    auto fs = boost::fusion::get<1>(fsel);
    auto cmpt = boost::fusion::get<2>(fsel);

    MinMax mm(DBL_MAX, -DBL_MAX);

    auto evaluate = [&](vtkDataSet* ds)
    {
        double mima[2] = {0,1};
        vtkDataArray *arr;
        switch (fs)
        {
          case Point:
            arr=ds->GetPointData()->GetArray(fieldname.c_str());
            insight::assertion(
                        arr!=nullptr,
                        "could not lookup point field "+fieldname );
            break;
          case Cell:
            arr=ds->GetCellData()->GetArray(fieldname.c_str()); //->GetRange(mima, cmpt);
            insight::assertion(
                        arr!=nullptr,
                        "could not lookup cell field "+fieldname );
            break;
        }
        arr->GetRange(mima, cmpt); // cmpt==-1 => L2 norm
        mm.first=std::min(mm.first, mima[0]);
        mm.second=std::max(mm.second, mima[1]);
    };

    for (const auto d: datasets)
    {
        evaluate(d);
    }
    for (const auto a: algorithms)
    {
        a->Update();
        if (auto * d = vtkDataSet::SafeDownCast(a->GetOutputDataObject(0)))
            evaluate(d);
//        else
//            throw insight::Exception("invalid input algorithm provided (does not return vtkDataSet)");
    }
    return mm;
}



void
MultiBlockDataSetExtractor::findObjectsBelowNode(
        std::vector<std::string> name_pattern,
        vtkDataObject* input,
        std::set<int>& res) const
{
  insight::CurrentExceptionContext ex("searching for groups with names matching \""+name_pattern.front()+"\"");

  vtkMultiBlockDataSet *mbds=vtkMultiBlockDataSet::SafeDownCast(input);
  insight::assertion(mbds!=nullptr, "valid vtkMultiBlockDataset expected!");

  boost::regex repat(name_pattern.front());

  auto iter = mbds->NewTreeIterator();
  iter->VisitOnlyLeavesOff();
  iter->TraverseSubTreeOff();
  for ( iter->InitTraversal();
       !iter->IsDoneWithTraversal();
        iter->GoToNextItem() )
  {
    std::string name(
                iter->GetCurrentMetaData()->Get(
                    vtkCompositeDataSet::NAME() ) );

    if (boost::regex_match(name, repat))
    {
        auto sub = vtkMultiBlockDataSet::SafeDownCast(iter->GetCurrentDataObject());

        if (name_pattern.size()>1)
        {
            insight::assertion(
                        sub!=nullptr,
                        "node "+name+" (selected by pattern \""+name_pattern.front()+"\") was not a vtkMultiBlockDataSet!" );
            findObjectsBelowNode(
                        std::vector<std::string>(name_pattern.begin()+1, name_pattern.end()),
                        sub, res);
        }
        else
        {
            if (sub) // add all childs
            {
                auto k = sub->NewTreeIterator();
                k->VisitOnlyLeavesOff();
                k->TraverseSubTreeOn();
                for ( k->InitTraversal();
                     !k->IsDoneWithTraversal();
                      k->GoToNextItem() )
                {
                    res.insert(flatIndices_.at(k->GetCurrentDataObject()));
                }
            }
            else // add leaf itself
            {
                res.insert(flatIndices_.at(iter->GetCurrentDataObject()));
            }
        }
    }
  }
}



MultiBlockDataSetExtractor::MultiBlockDataSetExtractor(vtkMultiBlockDataSet* mbds)
  : mbds_(mbds)
{
  CurrentExceptionContext ex("generating flat index list of vtkMultiBlockDataSet");

  insight::assertion(mbds_!=nullptr, "a non-null pointer to the MultiBlockDataSet is expected!");

  // traverse over all leafs to get flat index
  // GetCurrentFlatIndex() only return the right value,
  // if the loop is over the entire structure
  auto iter = mbds_->NewTreeIterator();
  iter->VisitOnlyLeavesOff();
  iter->SkipEmptyNodesOff();

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    auto o=iter->GetCurrentDataObject();
    auto j=iter->GetCurrentFlatIndex();
    flatIndices_[o]=j;
  }
}




std::set<int> MultiBlockDataSetExtractor::flatIndices(const std::vector<std::string>& groupNamePatterns) const
{
  insight::CurrentExceptionContext ex("determining block indices for pattern ["+boost::join(groupNamePatterns, ", ")+"]");

  insight::assertion(
              groupNamePatterns.size()>0,
              "specify at least one input element" );

  std::set<int> res;
  findObjectsBelowNode(groupNamePatterns, mbds_, res);

  return res;
}




VTKOffscreenScene::VTKOffscreenScene()
{
  vtkRenderingOpenGL2_AutoInit_Construct();
  vtkRenderingFreeType_AutoInit_Construct();
  vtkInteractionStyle_AutoInit_Construct();

  renderer_ = vtkSmartPointer<vtkRenderer>::New();
  renderWindow_ = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow_->AddRenderer(renderer_);
  renderWindow_->OffScreenRenderingOn();
  renderWindow_->SetSize(1920, 1440);
  renderWindow_->SetMultiSamples(8);
  renderer_->SetBackground(1, 1, 1); // Background color white

  auto renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  renderWindowInteractor->SetRenderWindow(renderWindow_);
  auto axes = vtkSmartPointer<vtkAxesActor>::New();
  auto widget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
  widget->SetOutlineColor( 0.9300, 0.5700, 0.1300 );
  widget->SetOrientationMarker( axes );
  widget->SetInteractor( renderWindowInteractor );
  widget->SetViewport( 0.0, 0.0, 0.4, 0.4 );
  widget->SetEnabled( 1 );
  widget->InteractiveOn();

  auto light_kit = vtkSmartPointer<vtkLightKit>::New();
  light_kit->SetKeyLightIntensity(1.0);
  light_kit->AddLightsToRenderer(renderer_);
}




VTKOffscreenScene::~VTKOffscreenScene()
{
}




void VTKOffscreenScene::addActor2D(vtkSmartPointer<vtkActor2D> actor)
{
  renderer_->AddActor2D(actor);
}


vtkSmartPointer<vtkScalarBarActor> VTKOffscreenScene::addColorBar(
      const std::string& title,
      vtkSmartPointer<vtkLookupTable> lut,
      double x, double y,
      bool horiz,
      double w, double len,
      double fontmult
      )
{
  auto cb = vtkSmartPointer<vtkScalarBarActor>::New();
  cb->SetLookupTable(lut);
  if (horiz)
  {
    cb->SetOrientationToHorizontal();
    cb->SetHeight(w);
    cb->SetWidth(len);
  }
  else
  {
    cb->SetHeight(len);
    cb->SetWidth(w);
  }
  cb->SetPosition(x, y);
  cb->SetLabelFormat("%g");
  cb->SetNumberOfLabels(5);
  cb->VisibilityOn();
  cb->SetTitle(title.c_str());
  cb->SetBarRatio(0.15);
  cb->SetUnconstrainedFontSize(true);
  cb->GetTitleTextProperty()->SetColor(0,0,0);
  cb->GetTitleTextProperty()->SetFontSize(fontmult*double(cb->GetTitleTextProperty()->GetFontSize()));
  cb->GetLabelTextProperty()->SetColor(0,0,0);
  cb->GetLabelTextProperty()->SetItalic(false);
  cb->GetLabelTextProperty()->SetFontSize(fontmult*double(cb->GetLabelTextProperty()->GetFontSize()));
  cb->GetAnnotationTextProperty()->SetColor(0,0,0);
  cb->GetAnnotationTextProperty()->SetItalic(false);
  cb->GetAnnotationTextProperty()->SetFontSize(fontmult*double(cb->GetAnnotationTextProperty()->GetFontSize()));

  renderer_->AddActor(cb);

  return cb;
}

void VTKOffscreenScene::exportX3D(const boost::filesystem::path& file)
{
  auto x3d=vtkSmartPointer<vtkX3DExporter>::New();
  x3d->SetInput(renderWindow_);
  x3d->SetFileName(file.string().c_str());
  x3d->BinaryOff();
  x3d->Update();
  x3d->Write();
}

void VTKOffscreenScene::exportImage(const boost::filesystem::path& pngfile)
{
  renderWindow_->Render();
  auto windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
  windowToImageFilter->SetInput(renderWindow_);
  windowToImageFilter->Update();
  auto writer = vtkSmartPointer<vtkPNGWriter>::New();
  writer->SetFileName(pngfile.string().c_str());
  writer->SetInputConnection(windowToImageFilter->GetOutputPort());
  writer->Write();
}


vtkCamera* VTKOffscreenScene::activeCamera()
{
  return renderer_->GetActiveCamera();
}


void VTKOffscreenScene::setParallelScale(
    boost::variant<
      double, // scale
      std::pair<double,double> // Lh, Lv
    > scaleOrSize
    )
{
  if (const auto* sz = boost::get<std::pair<double,double> >(&scaleOrSize))
  {
      double w = sz->first;
      double h = sz->second;
      double W = renderWindow_->GetSize()[0];
      double H = renderWindow_->GetSize()[1];
      double HbyW=H/W;
      double scale=0.5*std::max(w*HbyW, h);
      //print W, H, w, h, scale
      cout<<"setParallelScale: W, H, w, h, scale = "<<W<<", "<<H<<", "<<w<<", "<<h<<", "<<scale<<endl;
      activeCamera()->SetParallelScale(scale);
  }
  else if (const auto* sc = boost::get<double>(&scaleOrSize))
  {
      activeCamera()->SetParallelScale(*sc);
  }
}

void VTKOffscreenScene::fitAll(double mult)
{
  double bnds[6] = {DBL_MAX, -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX, -DBL_MAX};

  renderWindow_->Render();

  auto aa = renderer_->GetActors();
  aa->InitTraversal();
  for (vtkActor*a = aa->GetNextItem(); a!=0; a = aa->GetNextItem())
  {
    double abnds[6]={0};
    a->GetBounds(abnds);
    for (int i=0;i<3;i++)
    {
      bnds[2*i]=std::min(bnds[2*i], abnds[2*i]);
      bnds[2*i+1]=std::max(bnds[2*i+1], abnds[2*i+1]);
    }
  }

  arma::mat L=vec3(bnds[1]-bnds[0], bnds[3]-bnds[2], bnds[5]-bnds[4]);
  arma::mat ctr=vec3(
      0.5*(bnds[1]+bnds[0]),
      0.5*(bnds[3]+bnds[2]),
      0.5*(bnds[5]+bnds[4])
      );

  cout<<"fitAll:"<<endl;
  cout<<"L="<<L<<", ctr="<<ctr<<endl;

  arma::mat p, fp, ey;
  p=fp=ey=vec3(0,0,0);
  activeCamera()->GetPosition(p.memptr());
  activeCamera()->GetFocalPoint(fp.memptr());
  activeCamera()->GetViewUp(ey.memptr());
  cout<<"camera: pos="<<p<<", focus="<<fp<<", up="<<ey<<endl;
  arma::mat n=p-fp; n/=norm(n,2);
  ey/=norm(ey,2);
  arma::mat ex=-arma::cross(n,ey); ex/=norm(ex,2);
  ey = arma::cross(n, ex);
  cout<<"n="<<n<<", ex="<<ex<<", ey="<<ey<<endl;


  arma::mat diff=ctr-p; diff-=dot(diff,n)*n;
  arma::mat np=p+diff, nfp=fp+diff;
  activeCamera()->SetPosition( np.memptr() );
  activeCamera()->SetFocalPoint( nfp.memptr() );
  cout<<"set camera to pos="<<np<<", focus="<<nfp<<endl;

  double w= fabs(arma::dot(vec3(L[0],0,0), ex))+fabs(arma::dot(vec3(0,L[1],0), ex))+fabs(arma::dot(vec3(0,0,L[2]), ex));
  double h= fabs(arma::dot(vec3(L[0],0,0), ey))+fabs(arma::dot(vec3(0,L[1],0), ey))+fabs(arma::dot(vec3(0,0,L[2]), ey));
  cout<<"calculated w="<<w<<", h="<<h<<endl;

  setParallelScale(std::pair<double,double>(mult*w, mult*h));
}

void VTKOffscreenScene::clearScene()
{
  auto acts = renderer_->GetActors();
  vtkActor *act;
  for( acts->InitTraversal(); (act = acts->GetNextItem())!=nullptr; )
  {
      renderer_->RemoveActor( act );
  }

  auto acts2 = renderer_->GetActors2D();
  vtkActor2D *act2;
  for( acts2->InitTraversal(); (act2 = acts2->GetNextItem())!=nullptr; )
  {
      renderer_->RemoveActor2D( act2 );
  }

//  renderer_->Clear();
}

void VTKOffscreenScene::removeActor(vtkActor *act)
{
  renderer_->RemoveActor(act);
}

void VTKOffscreenScene::removeActor2D(vtkActor2D *act)
{
  renderer_->RemoveActor2D(act);
}






OpenFOAMCaseScene::OpenFOAMCaseScene(const boost::filesystem::path& casepath)
  : VTKOffscreenScene(),
    ofcase_(vtkSmartPointer<vtkOpenFOAMReader>::New())
{
  ofcase_->SetFileName( casepath.string().c_str() );
  ofcase_->SetSkipZeroTime(false);
  ofcase_->Update();

  ofcase_->CreateCellToPointOn();
  ofcase_->ReadZonesOn();
  ofcase_->DecomposePolyhedraOn();
  ofcase_->EnableAllCellArrays();
  ofcase_->EnableAllPointArrays();
  ofcase_->EnableAllPatchArrays();

  times_ = ofcase_->GetTimeValues();
  cout<<"VTK OpenFOAM Reader: available times = (";
  for (int i=0; i<times_->GetNumberOfTuples(); i++)
  {
    cout<<" "<<times_->GetTuple1(i)<<":"<<i<<endl;
  }
  cout<<" )"<<endl;

  setTimeIndex( times_->GetNumberOfTuples()-1 );

  for (int i=0; i<ofcase_->GetNumberOfPatchArrays(); i++)
  {
    patches_[ofcase_->GetPatchArrayName(i)]=i;
  }

}

vtkDoubleArray *OpenFOAMCaseScene::times() const
{
  return times_;
}

void OpenFOAMCaseScene::setTimeValue(double t)
{
  ofcase_->SetTimeValue( t );
  ofcase_->Modified();
  ofcase_->Update();
}

void OpenFOAMCaseScene::setTimeIndex(vtkIdType timeId)
{
  setTimeValue( times_->GetTuple1(timeId) );
}

vtkUnstructuredGrid* OpenFOAMCaseScene::internalMesh() const
{
  auto oo=ofcase_->GetOutput();
  for (vtkIdType i=0; i<oo->GetNumberOfBlocks(); i++)
  {
    auto md=oo->GetMetaData(i);
    if ( std::string(md->Get(vtkCompositeDataSet::NAME()))=="internalMesh" )
    {
      return vtkUnstructuredGrid::SafeDownCast(oo->GetBlock(i));
    }
  }

  return nullptr;
}

std::vector<std::string> OpenFOAMCaseScene::matchingPatchNames(const std::string& patchNamePattern) const
{
  std::vector<string> patchNames;

  regex pattern(patchNamePattern);
  for (const auto& p: patches_)
  {
    if (regex_match(p.first, pattern))
    {
      patchNames.push_back(p.first);
      cout<<"selecting for "<<patchNamePattern<<": "<<p.first<<" (id "<<p.second<<")"<<endl;
    }
  }

  return patchNames;
}

vtkPolyData* OpenFOAMCaseScene::patch(const std::string& name) const
{
  auto oo=ofcase_->GetOutput();
  for (vtkIdType i=0; i<oo->GetNumberOfBlocks(); i++)
  {
    auto md=oo->GetMetaData(i);
    if ( std::string(md->Get(vtkCompositeDataSet::NAME()))=="Patches" )
    {
      auto pm = vtkMultiBlockDataSet::SafeDownCast(oo->GetBlock(i));
      for (vtkIdType j=0; j<pm->GetNumberOfBlocks(); j++)
      {
        if ( std::string(pm->GetMetaData(j)->Get(vtkCompositeDataSet::NAME()))==name )
        {
          auto pmesh=vtkPolyData::SafeDownCast(pm->GetBlock(j));
          return pmesh;
        }
      }
    }
  }

  return nullptr;
}

vtkSmartPointer<vtkPolyData> OpenFOAMCaseScene::patches(const std::string& namePattern) const
{
  auto names = matchingPatchNames(namePattern);

  auto af = vtkSmartPointer<vtkAppendPolyData>::New();
  for (const auto& pn: names)
  {
    af->AddInputData(patch(pn));
  }
  af->Update();

  return af->GetOutput();
}

vtkSmartPointer<vtkOpenFOAMReader> OpenFOAMCaseScene::ofcase() const
{
  return ofcase_;
}

vtkSmartPointer<vtkCompositeDataGeometryFilter> OpenFOAMCaseScene::extractBlock(const std::string& name) const
{
  std::set<int> blkIdx;
  if (patches_.find(name)!=patches_.end())
  {
    int i=patches_.at(name);
    blkIdx.insert(i);
  }
  else
  {
    auto names = matchingPatchNames(name);
    for (const auto& pn: names)
    {
      blkIdx.insert(patches_.at(pn));
    }
    if (blkIdx.size()==0)
      throw insight::Exception(
          str(format("No patch matches expression \"%s\"!")%name)
          );
  }
  return extractBlock(blkIdx);
}

vtkSmartPointer<vtkCompositeDataGeometryFilter> OpenFOAMCaseScene::extractBlock(int blockIdx) const
{
  return extractBlock(std::set<int>({blockIdx}));
}

vtkSmartPointer<vtkCompositeDataGeometryFilter> OpenFOAMCaseScene::extractBlock(const std::set<int>& blockIdxs) const
{
    insight::assertion(
                blockIdxs.size()>0,
                "no block indices to extract were provided!" );

  auto eb = vtkSmartPointer<vtkExtractBlock>::New();
  eb->SetInputConnection(ofcase_->GetOutputPort());
  for (int i: blockIdxs) eb->AddIndex( i );
  auto eb2 = vtkSmartPointer<vtkCompositeDataGeometryFilter>::New();
  eb2->SetInputConnection(eb->GetOutputPort());
  return eb2;
}

FieldSelection::FieldSelection(string fieldName, FieldSupport fieldSupport, int component)
  : boost::fusion::tuple
    <
     std::string,
     FieldSupport,
     int
    > ( fieldName, fieldSupport, component)
{}

string FieldSelection::fieldName() const
{
  return boost::fusion::at_c<0>(*this);
}


FieldColor::FieldColor(FieldSelection fs,
           vtkSmartPointer<vtkLookupTable> lut,
           RangeSelection rs)
  : boost::fusion::tuple
    <
     FieldSelection,
     vtkSmartPointer<vtkLookupTable>,
     RangeSelection
    >(fs, lut, rs)
{}

vtkSmartPointer<vtkLookupTable> FieldColor::lookupTable() const
{
  return boost::fusion::at_c<1>(*this);
}


}
