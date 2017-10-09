%include <std_string.i>
%include <std_vector.i>
%include <std_map.i>
%include <boost_shared_ptr.i>

%shared_ptr(insight::ResultSet);

%typemap(typecheck) insight::ParameterSet::EntryList& 
{
    $1 = PySequence_Check($input) ? 1 : 0;
}


%typemap(in) insight::ParameterSet::EntryList& (insight::ParameterSet::EntryList vIn) 
{
    size_t iLen = PySequence_Length($input); 
    vIn.clear();
    for(unsigned int i = 0; i < iLen; i++) 
    {
        PyObject *o = PySequence_GetItem($input, i);
        
        std::string n;
        insight::Parameter* p;
        
        n = PyString_AsString
        (
            PyTuple_GetItem(o, 0)
        );
        int res1 = SWIG_ConvertPtr
        ( 
            PyTuple_GetItem(o, 1), 
            &p,
            SWIGTYPE_p_insight__Parameter, 1 
        );
        
        if (res1!=-1) //(!SWIG_IsOK(res1)) 
        {
            vIn.push_back( insight::ParameterSet::SingleEntry(n, p->clone()) );
        }
    }
    $1 = &vIn;
}


%typemap(typecheck) boost::ptr_vector<insight::sampleOps::set>& 
{
    $1 = PySequence_Check($input) ? 1 : 0;
}


%typemap(in) boost::ptr_vector<insight::sampleOps::set>& (boost::ptr_vector<insight::sampleOps::set> vIn) 
{
    size_t iLen = PySequence_Length($input); 
    vIn.clear();
    for(unsigned int i = 0; i < iLen; i++) 
    {
        PyObject *o = PySequence_GetItem($input, i);
        
        std::string n;
        insight::sampleOps::set* p;
        
        int res1 = SWIG_ConvertPtr
        ( 
            o, 
            &p,
            SWIGTYPE_p_insight__sampleOps__set, 1 
        );
        
        if (res1!=-1) //(!SWIG_IsOK(res1)) 
        {
            vIn.push_back( p->clone() );
        }
    }
    $1 = &vIn;
}

%typemap(typecheck) insight::PlotCurveList& 
{
    $1 = PySequence_Check($input) ? 1 : 0;
}


%typemap(in) insight::PlotCurveList& (insight::PlotCurveList vIn) 
{
    size_t iLen = PySequence_Length($input); 
    vIn.clear();
    for(unsigned int i = 0; i < iLen; i++) 
    {
        PyObject *o = PySequence_GetItem($input, i);
        
        insight::PlotCurve* c;
        
        int res1 = SWIG_ConvertPtr
        ( 
            o, 
            &c,
            SWIGTYPE_p_insight__PlotCurve, 1 
        );
        
        if (res1!=-1) //(!SWIG_IsOK(res1)) 
        {
            vIn.push_back( insight::PlotCurve(*c) );
        }
    }
    $1 = &vIn;
}

%typemap(typecheck) arma::mat& {
    $1 = PySequence_Check($input) ? 1 : 0;
}


// Python => C++
%typemap(in) arma::mat& (arma::mat vIn) {
    size_t jLen=-1, iLen = PySequence_Length($input); 
    if (iLen>0)
    {
     // 2D-array?
     if (PySequence_Check(PySequence_GetItem($input, 0)))
     {
        jLen = PySequence_Length(PySequence_GetItem($input, 0)); 
        vIn.set_size(iLen);
     }
     else
     {
        vIn.set_size(iLen, jLen);
     }
     
     for(unsigned int i = 0; i < iLen; i++) 
     {
        PyObject *e1 = PySequence_GetItem($input, i);
        if (PyNumber_Check(e1)) 
        {
            vIn(i)=(float)PyFloat_AsDouble(e1);
        } else if (PySequence_Check(e1))
        {
            if (PySequence_Length(e1) != jLen)
             throw insight::Exception("error during conversion from 2D python array into arma::mat! Only rectangular matrices are supported.");
             
            for(unsigned int j = 0; j < jLen; j++) 
            {
                PyObject *e2 = PySequence_GetItem(e1, j);
                if (PyNumber_Check(e2)) 
                {
                    vIn(i,j)=(float)PyFloat_AsDouble(e2);
                }
                else
                {
                    throw insight::Exception("unexpected data type in conversion from python array into arma::mat! Only numbers are supported!");
                }
            }
        } else
        {
            throw insight::Exception("unexpected row data type in conversion from python array into arma::mat! Only numbers or lists are supported!");
        }
     }
    }
    $1 = &vIn;
}



// C++ => Python
%typemap(out) arma::mat {
    PyObject *o = PyList_New($1.n_rows);
    for (unsigned int i=0; i<$1.n_rows; i++)
    {
      if ($1.n_cols==1)
      {
        PyList_SetItem(o, i, PyFloat_FromDouble($1(i)));
      }
      else
      {
        PyObject *o2=PyList_New($1.n_cols);
        for (unsigned int j=0; j<$1.n_cols; j++)
        {
            PyList_SetItem(o2, j, PyFloat_FromDouble($1(i,j)));
        }
        PyList_SetItem(o, i, o2);
      }
    }
    $result=o;
}

%typemap(out) arma::mat& {
    PyObject *o = PyList_New($1->n_rows);
    for (unsigned int i=0; i<$1->n_rows; i++)
    {
      if ($1->n_cols==1)
      {
        PyList_SetItem(o, i, PyFloat_FromDouble((*$1)(i)));
      }
      else
      {
        PyObject *o2=PyList_New($1->n_cols);
        for (unsigned int j=0; j<$1->n_cols; j++)
        {
            PyList_SetItem(o2, j, PyFloat_FromDouble((*$1)(i,j)));
        }
        PyList_SetItem(o, i, o2);
      }
    }
    $result=o;
}





%typemap(typecheck) arma::cube& {
    $1 = PySequence_Check($input) ? 1 : 0;
}



// C++ => Python
%typemap(out) arma::cube {
    PyObject *o = PyList_New($1.n_rows);
    for (unsigned int i=0; i<$1.n_rows; i++)
    {
        PyObject *o2=PyList_New($1.n_cols);
        for (unsigned int j=0; j<$1.n_cols; j++)
        {
            PyObject *o3=PyList_New($1.n_slices);
            for (unsigned int k=0; k<$1.n_slices; k++)
            {
                PyList_SetItem(o3, k, PyFloat_FromDouble($1(i,j,k)));
            }
            PyList_SetItem(o2, j, o3);
        }
        PyList_SetItem(o, i, o2);
    }
    $result=o;
}

%typemap(out) arma::cube& {
    PyObject *o = PyList_New($1->n_rows);
    for (unsigned int i=0; i<$1->n_rows; i++)
    {
        PyObject *o2=PyList_New($1->n_cols);
        for (unsigned int j=0; j<$1->n_cols; j++)
        {
            PyObject *o3=PyList_New($1->n_slices);
            for (unsigned int k=0; k<$1->n_slices; k++)
            {
                PyList_SetItem(o3, k, PyFloat_FromDouble((*$1)(i,j,k)));
            }
            PyList_SetItem(o2, j, o3);
        }
        PyList_SetItem(o, i, o2);
    }
    $result=o;
}



%typemap(typecheck) boost::filesystem::path& {
    $1 = PyString_Check($input) ? 1 : 0;
}

%typemap(out) const boost::filesystem::path& {
    $result = PyString_FromString($1->c_str());
}
%typemap(in) boost::filesystem::path& (boost::filesystem::path vIn) {
    vIn=PyString_AsString($input);
    $1 = &vIn;
}

%typemap(typecheck) GeomAbs_CurveType {
    $1 = PyString_Check($input) ? 1 : 0;
}

%typemap(in) GeomAbs_CurveType {
    std::string name=PyString_AsString($input);
    GeomAbs_CurveType ct;
    
    if (name=="Line") ct=GeomAbs_Line;
    else if (name=="Circle") ct=GeomAbs_Circle;
    else if (name=="Ellipse") ct=GeomAbs_Ellipse;
    else if (name=="Hyperbola") ct=GeomAbs_Hyperbola;
    else if (name=="Parabola") ct=GeomAbs_Parabola;
    else if (name=="BezierCurve") ct=GeomAbs_BezierCurve;
    else if (name=="BSplineCurve") ct=GeomAbs_BSplineCurve;
    else if (name=="OtherCurve") ct=GeomAbs_OtherCurve;
    else throw insight::Exception("unrecognized curve type: "+name);
    
    $1 = ct;
}

%typemap(typecheck) GeomAbs_SurfaceType {
    $1 = PyString_Check($input) ? 1 : 0;
}

%typemap(in) GeomAbs_SurfaceType {
    std::string name=PyString_AsString($input);
    GeomAbs_SurfaceType ct;
    
    if (name=="Plane") ct=GeomAbs_Plane;
    else if (name=="Cylinder") ct=GeomAbs_Cylinder;
    else if (name=="Cone") ct=GeomAbs_Cone;
    else if (name=="Sphere") ct=GeomAbs_Sphere;
    else if (name=="Torus") ct=GeomAbs_Torus;
    else if (name=="BezierSurface") ct=GeomAbs_BezierSurface;
    else if (name=="BSplineSurface") ct=GeomAbs_BSplineSurface;
    else if (name=="SurfaceOfRevolution") ct=GeomAbs_SurfaceOfRevolution;
    else if (name=="SurfaceOfExtrusion") ct=GeomAbs_SurfaceOfExtrusion;
    else if (name=="OffsetSurface") ct=GeomAbs_OffsetSurface;
    else if (name=="OtherSurface")  ct=GeomAbs_OtherSurface;
    else throw insight::Exception("unrecognized surface type: "+name);
    
    $1 = ct;
}

%template(StringList) std::vector<std::string>;
