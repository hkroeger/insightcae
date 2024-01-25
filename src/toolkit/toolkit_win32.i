%include "toolkit.i"

%pythonbegin %{
import re, os
liblist=list(filter(re.compile('libvtkCommon.*so.*').search, [l[73:].strip() for l in open('/proc/%d/maps'%os.getpid(), 'r').readlines()]))
if len(liblist)==0:
  import Insight.vtkPyOffscreen
%}
