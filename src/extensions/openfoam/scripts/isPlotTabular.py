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

from optparse import OptionParser

parser = OptionParser()
parser.add_option("-s", "--skip", dest="skip", metavar='NUM', default=0,
		  type="int",
                  help="number of rows to skip in display")

(opts, args) = parser.parse_args()

class Plot(wx.Panel):
    def __init__(self, parent, id = -1, dpi = None, **kwargs):
        wx.Panel.__init__(self, parent, id=id, **kwargs)
        self.figure = mpl.figure.Figure(dpi=dpi, figsize=(3,2))
        self.canvas = Canvas(self, -1, self.figure)
        self.toolbar = Toolbar(self.canvas)
        self.toolbar.Realize()

        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self.canvas, 1, wx.EXPAND)
        sizer.Add(self.toolbar, 0 , wx.LEFT | wx.EXPAND)
        self.SetSizer(sizer)

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
       return page.figure

lines=open(sys.argv[1]).readlines()
for l in lines:
  if l.strip().startswith('#'):
    lines.remove(l)
    
#headings=lines[0].lstrip(' #').split()
headings=[]

data=np.array([
   map(float, line.replace('(', '').replace(')', '').replace(',', ' ').replace(';', ' ').replace('  ', ' ').split()) 
   for line in lines[1+opts.skip:]
  ])

# moving average
dataavg=np.asarray(
   [
     [ data[i,0] ] + list(np.average(data[i/2:i,1:], axis=0))
     for i in range(1,len(data))
   ]
   )

app = wx.PySimpleApp()
frame = wx.Frame(None, -1, 'Plotter')
plotter = PlotNotebook(frame)

ncols=np.size(data,1)
heads= headings
if len(headings) != ncols:
 heads=[str(i) for i in range(1,ncols)]

for i in range(1, ncols):
  ax = plotter.add(heads[i-1]).gca()
  ax.grid(True)
  ax.plot(data[:,0], data[:,i], 'k--')
  ax.plot(dataavg[:,0], dataavg[:,i], 'k-')

frame.Show()
app.MainLoop()
