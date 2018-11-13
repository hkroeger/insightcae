#!/usr/bin/env python

import os, sys, subprocess
import numpy as np

# Used to guarantee to use at least Wx2.8
import wxversion
wxversion.ensureMinimal('2.8')

import wx
import wx.aui
import matplotlib as mpl
from matplotlib.backends.backend_wxagg import FigureCanvasWxAgg as Canvas
from matplotlib.backends.backend_wxagg import NavigationToolbar2Wx as Toolbar

import Insight.toolkit as itk

from optparse import OptionParser

parser = OptionParser()
parser.add_option("-s", "--skip", dest="skip", metavar='NUM', default=0,
		  type="int",
                  help="number of rows to skip in display")

(opts, args) = parser.parse_args()

class Plot(wx.Panel):
    def __init__(self, parent, id = -1, dpi = None, **kwargs):
        wx.Panel.__init__(self, parent, id=id, **kwargs)

        self.figure = mpl.figure.Figure(dpi=dpi, figsize=(12,10))
        self.canvas = Canvas(self, -1, self.figure)

        self.l0 = wx.StaticText(self, -1, "Start at x=")
        self.t0 = wx.TextCtrl(self, 3, style = wx.TE_PROCESS_ENTER)
        #self.t0.SetRange(-1e10, 1e10)
        self.t0.Bind(wx.EVT_TEXT_ENTER, self.onUpdateT0, id=3)
        self.lv = wx.StaticText(self, -1, "")

        self.toolbar = Toolbar(self.canvas)
        self.toolbar.Realize()

        sizer1 = wx.BoxSizer(wx.HORIZONTAL)
        sizer1.Add(self.l0)
        sizer1.Add(self.t0)
        sizer1.Add(self.lv)

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(sizer1, 1, wx.EXPAND)
        sizer.Add(self.canvas, 1, wx.EXPAND)
        sizer.Add(self.toolbar, 0 , wx.LEFT | wx.EXPAND)
        self.SetSizer(sizer)

    def setLV(self, lv, lvm):
        self.lv.SetLabel(", last value = %g (mean %g)"%(lv, lvm))

    def onUpdateT0(self, event):
        x0=float(self.t0.GetValue())
        self.figure.gca().set_xlim(left=x0)
        self.canvas.draw()

class PlotNotebook(wx.Panel):
    def __init__(self, parent, id = -1):
        wx.Panel.__init__(self, parent, id=id)
        self.nb = wx.aui.AuiNotebook(self)
        sizer = wx.BoxSizer()
        sizer.Add(self.nb, 1, wx.EXPAND)
        self.SetSizer(sizer)

    def add(self,name="plot"):
       page = Plot(self.nb)
       self.nb.AddPage(page,name)
       return page

filename=sys.argv[1]
lines_org=open(filename).readlines()
#for l in lines:
#  if l.strip().startswith('#'):
#    lines.remove(l)
lines=[l for l in lines_org if not l.strip().startswith('#')]
    
#headings=lines[0].lstrip(' #').split()
headings=[]

data_raw=[
   map(float, line.replace('(', '').replace(')', '').replace(',', ' ').replace(';', ' ').replace('  ', ' ').split()) 
   for line in lines[1+opts.skip:]
  ]
data=np.array(data_raw)

# moving average
#dataavg=np.asarray(
#   [
#     [ data[i,0] ] + list(np.average(data[i/2:i,1:], axis=0))
#     for i in range(1,len(data))
#   ]
#   )

#print "data=", data

dataavg=np.asarray(itk.movingAverage( data_raw, 0.33 ));

app = wx.PySimpleApp()
frame = wx.Frame(None, -1, 'Plotter: '+filename)
plotter = PlotNotebook(frame)

ncols=np.size(data,1)
heads= headings
if len(headings) != ncols:
 heads=[str(i) for i in range(1,ncols)]

for i in range(1, ncols):
  page=plotter.add(heads[i-1])
  ax = page.figure.gca()
  ax.grid(True)
  ax.plot([data[0,0],data[-1,0]], [0.,0.], label=None) # include zero
  ax.plot(data[:,0], data[:,i], 'k--')
  ax.plot(dataavg[:,0], dataavg[:,i], 'k-')
  page.setLV(data[-1, i], dataavg[-1,i])

frame.Show()
app.MainLoop()
