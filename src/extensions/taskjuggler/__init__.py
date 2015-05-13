# -*- coding: utf-8 -*-

import os, sys, math, csv
import numpy as np
from datetime import datetime, timedelta
from dateutil.relativedelta import relativedelta


class Accounts:
  def __init__(self, fname=None):
    if not fname is None:
      self.readData(fname)
      
  def writeAccountTJI(self, fname):
    open(fname, "w").write("""\
account in_rev "Umsatzerlöse"
account in_other "Sonstige Erträge"
account in_grant "Zulagen+Zuschüsse"

account cost_prod "Material- und Fertigungskosten"
account cost_staff "Mitarbeiter"
account cost_interest "Zinsen"

account cost_other_rent "Mieten"
account cost_other_services "Dienstleister" { aggregate resources }
account cost_other_facilities "Geschäftsausstattung"
account cost_other_sales "Vertrieb"
account cost_other_fue "FuE"
account cost_other_misc "Allgemein"

account cost_invest "Investitionsausgaben"
account cost_depre "Abschreibungen"
account cost_repay "Tilgungen"

account fund_stock "Stammeinlage"
account fund_loan "Kredit"    

accountreport projectcashflowExport "cashflow" {
  formats csv
  columns id, name, monthly
}
""")
    
  def readData(self, fname="cashflow.csv"):
    self.dates=[]
    self.accounts={}
    self.accountnames={}
    
    fc=2 # first col with data
    
    rows=csv.reader(open(fname, 'r'), delimiter=';')
    dater=rows.next()
    self.dates=map(lambda d: datetime.strptime(d, "%Y-%m-%d"), dater[fc:])

    for r in rows:
      vals=np.array(map(lambda s: float(s.replace(",", ".")) if s!="" else 0.0, r[fc:]))
      self.accounts[r[0]]=vals[1:]-vals[:-1]
      self.accountnames[r[0]]=r[1]
      
    self.cost_other=[]
    for n in self.accounts.keys():
      if n.startswith("cost_other_"):
	self.cost_other.append(n)
    
  def findPeriodIdx(self, date):
    for i in range(0,len(self.dates)-1):
      if (self.dates[i]<=date) and (self.dates[i+1]>date):
	return i
    raise Exception("date outside of time interval"+str(date)+" <> "+str(self.dates))
    return -1
    
  def insertLoan(self, loan):
    start, value, i_p_a, nmonth=loan
    i=self.findPeriodIdx(start)
    self.accounts["fund_loan"][i]+=value
    cd=start
    cv=value
    n=nmonth
    R=value*((1.+i_p_a/12.)**n)*i_p_a/12./(((1.+i_p_a/12.)**n)-1.)
    for j in range(1, nmonth+1):
      if (i+j)<len(self.dates)-1:
	cd=cd+relativedelta(months=1)
	im=i_p_a*cv/12.
	r=R-im
	cv=cv-r
	self.accounts["cost_interest"][i+j]+=im
	self.accounts["cost_repay"][i+j]+=r
	
  def insertInvest(self, investment):
    start, value, depre_years = investment
    i=self.findPeriodIdx(start)
    self.accounts["cost_invest"][i]+=value
    afa=value/depre_years/12.
    cd=start
    cv=value
    for j in range(1, 12*depre_years+1):
      if (i+j)<len(self.dates)-1:
	cd=cd+relativedelta(months=1)
	self.accounts["cost_depre"][i+j]+=afa
	
  def insertDormantEquity(self, doe):
    start, value = doe
    i=self.findPeriodIdx(start)
    self.accounts["fund_stock"][i]+=value
	
  def LiqGuV(self, year, beginliq=0.):
    start=datetime(year,1,1)
    si=self.findPeriodIdx(start)
    
    cols=[]
    cols2=[]
    cols3={}
    
    liq=beginliq
    operrescum=0.0
    for m in range(0,12):
      i=si+m
      
      cci=[self.accounts[n][i] for n in [
	"in_rev",
	"in_other"
	]
      ]
      sumi=sum(cci)
      cco=[self.accounts[n][i] for n in [
	"cost_prod", 
	"cost_staff",
	"cost_depre", 
	"cost_interest"
	]
      ]
      ccoo=[self.accounts[n][i] for n in self.cost_other]
      sumccoo=sum(ccoo)
      sumo=sum(cco)+sumccoo
      operres=sumi-sumo
      operrescum+=operres
      grants=self.accounts["in_grant"][i]
      tax=0.0
      if m+1==12: 
	tax=max(0.0, 0.35*operrescum)
      finalres=operres+grants-tax
      
      cols.append(
	cci
	+[sumi]
	+cco
	+[sumccoo]
	+ccoo
	+[
	  sumo,
	  operres,
	  grants,
	  tax,
	  finalres
	  ]
	)
		  
      cci=[self.accounts[n][i] for n in [
	"in_rev",
	"in_other",
	"in_grant"
	]
      ]
      sumi=sum(cci)
      
      cco=[self.accounts[n][i] for n in [
	"cost_prod",
	"cost_staff", 
	"cost_interest"
	]
      ]
      ccoo=[self.accounts[n][i] for n in self.cost_other]
      sumccoo=sum(ccoo)
      cco2=[
	tax,
	self.accounts["cost_invest"][i], 
	self.accounts["cost_repay"][i]
	]
      sumo=sum(cco)+sumccoo+sum(cco2)
      
      fin=[self.accounts[n][i] for n in [
	"fund_stock",
	"fund_loan"
	]
      ]
      sumfin=sum(fin)
      cashflow=sumi-sumo+sumfin
      liq=liq+cashflow
      
      cols2.append(
	cci
	+[sumi]
	
	+cco
	+[sumccoo]
	+ccoo
	+cco2
	+[sumo]
	
	+[sumfin]
	+fin
	+[cashflow]
	+[liq]
	)
      
      

    res="\\begin{tabular}{lrrrrrrrrrrrr}\\\\\n"
    res+="Monat "+start.strftime("%Y")+" ".join(["& %d"%(m+1) for m in range(0,12)])+"\\\\\n"
    res+="\\hline\n"
    res+="Umsatzerlöse"+" ".join(["& \\E{%.0f}"%cols[m][0] for m in range(0,12)])+"\\\\\n"
    res+="Sonstige Erträge"+" ".join(["& \\E{%.0f}"%cols[m][1] for m in range(0,12)])+"\\\\\n"
    res+="\\textsl{Betriebsertrag}"+" ".join(["& \\E{%.0f}"%cols[m][2] for m in range(0,12)])+"\\\\\n"
    res+="\\hline\n"
    res+="Materialaufwand"+" ".join(["& \\E{%.0f}"%cols[m][3] for m in range(0,12)])+"\\\\\n"
    res+="Personalkosten"+" ".join(["& \\E{%.0f}"%cols[m][4] for m in range(0,12)])+"\\\\\n"
    res+="Abschreibungen"+" ".join(["& \\E{%.0f}"%cols[m][5] for m in range(0,12)])+"\\\\\n"
    res+="Zinsaufwand"+" ".join(["& \\E{%.0f}"%cols[m][6] for m in range(0,12)])+"\\\\\n"
    res+="Sonst. betr. Aufwand"+" ".join(["& \\E{%.0f}"%cols[m][7] for m in range(0,12)])+"\\\\\n"
    for k,n in enumerate(self.cost_other):
      res+="- "+self.accountnames[n]+" ".join(["& \\E{%.0f}"%cols[m][8+k] for m in range(0,12)])+"\\\\\n"
    res+="\\textsl{Betriebsaufwand}"+" ".join(["& \\E{%.0f}"%cols[m][-5] for m in range(0,12)])+"\\\\\n"
    res+="\\hline\n"
    res+="Betriebsergebnis"+" ".join(["& \\E{%.0f}"%cols[m][-4] for m in range(0,12)])+"\\\\\n"
    res+="Öffentl. Zuschüsse"+" ".join(["& \\E{%.0f}"%cols[m][-3] for m in range(0,12)])+"\\\\\n"
    res+="Steuern auf Erträge"+" ".join(["& \\E{%.0f}"%cols[m][-2] for m in range(0,12)])+"\\\\\n"
    res+="\\textbf{Ausgew. Ergebnis}"+" ".join(["& \\E{%.0f}"%cols[m][-1] for m in range(0,12)])+"\\\\\n"
    res+="\\end{tabular}"
    
    res2="\\begin{tabular}{lrrrrrrrrrrrr}\\\\\n"
    res2+="Monat "+start.strftime("%Y")+" ".join(["& %d"%(m+1) for m in range(0,12)])+"\\\\\n"
    res2+="\\hline\n"
    res2+="Umsatzerlöse"+" ".join(["& \\E{%.0f}"%cols2[m][0] for m in range(0,12)])+"\\\\\n"
    res2+="Sonstige Erträge"+" ".join(["& \\E{%.0f}"%cols2[m][1] for m in range(0,12)])+"\\\\\n"
    res2+="Öffentl. Zuschüsse"+" ".join(["& \\E{%.0f}"%cols2[m][2] for m in range(0,12)])+"\\\\\n"
    res2+="\\textsl{$\\sum$ Einnahmen}"+" ".join(["& \\E{%.0f}"%cols2[m][3] for m in range(0,12)])+"\\\\\n"
    res2+="\\hline\n"
    
    res2+="Materialaufwand"+" ".join(["& \\E{%.0f}"%cols2[m][4] for m in range(0,12)])+"\\\\\n"
    res2+="Personalkosten"+" ".join(["& \\E{%.0f}"%cols2[m][5] for m in range(0,12)])+"\\\\\n"
    res2+="Zinsaufwand"+" ".join(["& \\E{%.0f}"%cols2[m][6] for m in range(0,12)])+"\\\\\n"
    res2+="Sonst. betr. Aufwand"+" ".join(["& \\E{%.0f}"%cols2[m][7] for m in range(0,12)])+"\\\\\n"
    for k,n in enumerate(self.cost_other):
      res2+="- "+self.accountnames[n]+" ".join(["& \\E{%.0f}"%cols2[m][8+k] for m in range(0,12)])+"\\\\\n"
    res2+="Steuern auf Erträge"+" ".join(["& \\E{%.0f}"%cols2[m][-9] for m in range(0,12)])+"\\\\\n"
    res2+="Investitionen"+" ".join(["& \\E{%.0f}"%cols2[m][-8] for m in range(0,12)])+"\\\\\n"
    res2+="Tilgungen"+" ".join(["& \\E{%.0f}"%cols2[m][-7] for m in range(0,12)])+"\\\\\n"
    res2+="\\textsl{$\\sum$ Ausgaben}"+" ".join(["& \\E{%.0f}"%cols2[m][-6] for m in range(0,12)])+"\\\\\n"
    res2+="\\hline\n"
    res2+="Finanzierung"+" ".join(["& \\E{%.0f}"%cols2[m][-5] for m in range(0,12)])+"\\\\\n"
    res2+="- Stammeinlagen"+" ".join(["& \\E{%.0f}"%cols2[m][-4] for m in range(0,12)])+"\\\\\n"
    res2+="- Darlehen"+" ".join(["& \\E{%.0f}"%cols2[m][-3] for m in range(0,12)])+"\\\\\n"
    res2+="\\hline\n"
    res2+="\\textbf{Liquidität}"+" ".join(["& \\E{%.0f}"%cols2[m][-2] for m in range(0,12)])+"\\\\\n"
    res2+="\\textbf{Liquidität (kumulativ)}"+" ".join(["& \\E{%.0f}"%cols2[m][-1] for m in range(0,12)])+"\\\\\n"
    res2+="\\end{tabular}"
    
    rev=sum([cols[m][0] for m in range(0,12)])
    otherrev=sum([cols[m][1] for m in range(0,12)])
    totalrev=rev+otherrev
    cost_mat=sum([cols[m][3] for m in range(0,12)])
    cost_staff=sum([cols[m][4] for m in range(0,12)])
    cost_depre=sum([cols[m][5] for m in range(0,12)])
    cost_inter=sum([cols[m][6] for m in range(0,12)])
    cost_other=sum([cols[m][7] for m in range(0,12)])
    cost_tot=cost_mat+cost_staff+cost_depre+cost_inter+cost_other
    result=totalrev-cost_tot
    grants=sum([cols[m][-3] for m in range(0,12)])
    taxes=sum([cols[m][-2] for m in range(0,12)])
    finalres=result+grants-taxes

    cols3["Umsatzerlöse"]="& \\E{%.0f} &"%rev
    cols3["Sonstige Erträge"]="& \\E{%.0f} &"%otherrev
    cols3["Betriebsertrag"]="& \\E{%.0f} &"%totalrev

    def per(nom,denom):
      if abs(denom>1e-10):
	return nom/denom
      else:
	return 0.0
      
    cols3["- Materialaufwand"]="& \\E{%.0f} & %.0f\\%% "%(cost_mat, per(1e2*cost_mat,totalrev))
    cols3["- Personalaufwand"]="& \\E{%.0f} & %.0f\\%% "%(cost_staff, per(1e2*cost_staff,totalrev))
    cols3["- Abschreibungen"]="& \\E{%.0f} & %.0f\\%% "%(cost_depre, per(1e2*cost_depre,totalrev))
    cols3["- Zinsaufwand"]="& \\E{%.0f} & %.0f\\%% "%(cost_inter, per(1e2*cost_inter,totalrev))
    cols3["- sonst. Aufwand"]="& \\E{%.0f} & %.0f\\%% "%(cost_other, per(1e2*cost_other,totalrev))
    cols3["Betriebsaufwand"]="& \\E{%.0f} & %.0f\\%% "%(cost_tot, per(1e2*cost_tot,totalrev))

    cols3["Betriebsergebnis"]="& \\E{%.0f} & %.0f\\%% "%(result, per(1e2*result,totalrev))

    cols3["Öffentl. Zuschüsse"]="& \\E{%.0f} & %.0f\\%% "%(grants, per(1e2*grants,totalrev))
    cols3["Steuern auf Erträge"]="& \\E{%.0f} & %.0f\\%% "%(taxes, per(1e2*taxes,totalrev))

    cols3["Ausgew. Betriebsergebnis"]="& \\E{%.0f} & %.0f\\%% "%(finalres, per(1e2*finalres,totalrev))
    
    return (res, res2, liq, cols, cols2, cols3)




class Milestones(dict):
  
  def writeEventTJI(self, fname="events.tji"):
    open(fname, "w").write("""\
flags eventmilestone

taskreport "milestones" {
  formats csv
  columns id, name, start, note
  hidetask ~ ( eventmilestone & isleaf() )
}
""")
    
  def __init__(self):
    pass
  
  def readData(self, fname="milestones.csv"):
    self.investments={}
    self.loans={}
    self.does={}
    
    rows=csv.reader(open(fname, 'r'), delimiter=';')
    dater=rows.next()
    
    for r in rows:
      try:
	start=datetime.strptime(r[2], "%Y-%m-%d")
	vals=r[3].split(';')
	if (vals[0]=='invest'):
	  value=float(vals[1])
	  depre_years=int(vals[2])
	  self.investments[r[0]]=(start,value,depre_years)
	elif (vals[0]=='loan'):
	  value=float(vals[1])
	  i_p_a=float(vals[2])
	  nmonth=int(vals[3])
	  self.loans[r[0]]=(start,value,i_p_a,nmonth)
	elif (vals[0]=='doe'):
	  value=float(vals[1])
	  self.does[r[0]]=(start,value)
      except:
	pass
    print self



def runTJ(tjpfile, m4files=[], skipTJ=False):
  acc=Accounts()
  events=Milestones()

  ok=True
  
  if not skipTJ:
    acc.writeAccountTJI("accounts.tji")
    events.writeEventTJI("events.tji")

    for m4file,tjifile in m4files:
      #tjifile=os.path.splitext(m4file)[0]+".tji"
      ok=ok and (os.system("m4 %s > %s"%(m4file, tjifile))==0)
    ok=ok and (os.system("tj3 "+tjpfile)==0)
  
  if ok:
    acc.readData()
    events.readData()
    

  return ok, acc, events

def postprocTJ(acc, events, fromyear, nyear):
  
  for name, lo in events.loans.items():
    print "Inserting loan ", name
    acc.insertLoan(lo)
  for name, inv in events.investments.items():
    print "Inserting investment ", name
    acc.insertInvest(inv)
  for name, doe in events.does.items():
    print "Inserting dormant equity ", name
    acc.insertDormantEquity(doe)

  colsguv=[list()]*nyear
  colsliq=[list()]*nyear
  sumcols=[dict()]*nyear
  liq=0
  for iy in range(0,nyear):
    y=fromyear+iy
    r1, r2, liq, colsguv[iy], colsliq[iy], sumcols[iy]=acc.LiqGuV(y, liq)
    open("guv%d.tex"%y, "w").write(r1)
    open("liq%d.tex"%y, "w").write(r2)
    
  import matplotlib.pyplot as plt
  cl=['r','g','b']
  
  def plot1(cols, row, fname):
    plt.figure()
    for iy in range(0,nyear):
      plt.bar([12*iy+1], [sum([1e-3*cols[iy][j][row] for j in range(0,12)])], label="Jahr %d"%(iy+1), color=cl[iy], width=12)
      plt.bar(range(12*iy+1, 12*(iy+1)+1), [1e-3*cols[iy][j][row] for j in range(0,12)], color=cl[iy])
    plt.legend()
    plt.xlim((1,36))
    plt.xlabel("Monat")
    plt.ylabel("Tsd.EUR")
    plt.savefig(fname)
    
  plot1(colsguv, 2, "umsaetze.png")    
  plot1(colsguv, 3, "materialausgaben.png")    
  plot1(colsguv, 4, "personalausgaben.png")    
  plot1(colsguv, 5, "abschreibungen.png")    
  plot1(colsguv, 6, "zinsausgaben.png")    
  plot1(colsguv, 7, "sonstigeausgaben.png")    
  plot1(colsguv, -4, "betriebsergebnis.png")
  plot1(colsguv, -3, "zuschuesse.png")

  plot1(colsliq, -8, "investitionen.png")    
  plot1(colsliq, -7, "tilgungen.png")    
  
  plt.figure()
  for iy in range(0,nyear):
    plt.plot(range(12*iy+1, 12*(iy+1)+1), [1e-3*colsliq[iy][j][-1] for j in range(0,12)], cl[iy]+'o-', label="Jahr %d"%(iy+1))
  plt.legend()
  plt.grid()
  plt.xlim((1,36))
  plt.xlabel("Monat")
  plt.ylabel("Liquiditaet [Tsd.EUR]")
  plt.savefig("liquiditaet.png")

  unisumcols={}
  for sc in sumcols:
    for n,v in sc.items():
      if n in unisumcols:
	unisumcols[n]+=sc[n]
      else:
	unisumcols[n]=sc[n]

  nc=len(sumcols)
  f=open("summary.tex", "w")
  f.write("\\begin{tabular}{l%s}\n"%('|rr'*nc))
  f.write("%s\\\\\n" % "".join(['& \\multicolumn{2}{c}{%d}'%(fromyear+i) for i in range(0,nc)]))
  f.write("%s\\\\\n"%('& Betrag & \\% v. Ertrag'*nc))
  f.write("\\hline\n")
  for rn in [
    "Umsatzerlöse",
    "Sonstige Erträge",
    "Betriebsertrag",
    None,
    "- Materialaufwand",
    "- Personalaufwand",
    "- Abschreibungen",
    "- Zinsaufwand",
    "- sonst. Aufwand",
    "Betriebsaufwand",
    None,
    "Betriebsergebnis",
    None,
    "Öffentl. Zuschüsse",
    "Steuern auf Erträge",
    None,
    "Ausgew. Betriebsergebnis"    
    ]:
    if rn is None:
      f.write("\\hline\n")
    else:
      f.write("%s %s\\\\\n"%(rn, unisumcols[rn]))
  f.write("\\end{tabular}\n")
