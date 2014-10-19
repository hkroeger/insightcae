%include "common.i"

%module(directors="1") cad
%{
#include "feature.h"
#include "featurefilter.h"
#include "solidmodel.h"
#include "datum.h"
#include "sketch.h"
#include "meshing.h"
%}

%include "feature.h"
%include "featurefilter.h"
%include "solidmodel.h"
%include "datum.h"
%include "sketch.h"
%include "meshing.h"