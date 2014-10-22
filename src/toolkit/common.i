%include <std_string.i>
%include <std_vector.i>


%typemap(typecheck) arma::mat& {
    $1 = PySequence_Check($input) ? 1 : 0;
}

%typemap(in) arma::mat& (arma::mat vIn) {
    size_t iLen = PySequence_Length($input); 
    vIn.set_size(iLen);
    for(unsigned int i = 0; i < iLen; i++) {
        PyObject *o = PySequence_GetItem($input, i);
        if (PyNumber_Check(o)) {
            vIn(i)=(float)PyFloat_AsDouble(o);
        }
    }
    $1 = &vIn;
}

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

%typemap(typecheck) boost::filesystem::path& {
    $1 = PyString_Check($input) ? 1 : 0;
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
