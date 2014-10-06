%module(directors="1") cad
%{
#include "solidmodel.h"
#include "datum.h"
#include "sketch.h"
%}

%include <std_string.i>

%typemap(typecheck) arma::mat& {
    $1 = PySequence_Check($input) ? 1 : 0;
}

%typemap(in) arma::mat& (arma::mat vIn) {
    int iLen = PySequence_Length($input); 
    vIn.set_size(iLen);
    for(unsigned int i = 0; i < iLen; i++) {
        PyObject *o = PySequence_GetItem($input, i);
        if (PyNumber_Check(o)) {
            vIn(i)=(float)PyFloat_AsDouble(o);
        }
    }
    $1 = &vIn;
}

%typemap(typecheck) boost::filesystem::path& {
    $1 = PyString_Check($input) ? 1 : 0;
}

%typemap(in) boost::filesystem::path& (boost::filesystem::path vIn) {
    vIn=PyString_AsString($input);
    $1 = &vIn;
}

%include "solidmodel.h"
%include "datum.h"
%include "sketch.h"
