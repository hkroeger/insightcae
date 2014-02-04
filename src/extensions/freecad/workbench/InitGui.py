#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2009 Yorik van Havre <yorik@uncreated.net>              * 
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

__title__="FreeCAD Insight Workbench - Init file"
__author__ = "Hannes Kroeger <hannes@kroegeronline.net>"
__url__ = ["http://www.kroegeronline.net"]

class InsightWorkbench (Workbench):
    "Insight Workbench"
    Icon = """
        /* XPM */
        static char * draft_xpm[] = {
        "16 16 17 1",
        " 	c None",
        ".	c #5F4A1C",
        "+	c #5A4E36",
        "@	c #8A4D00",
        "#	c #835A04",
        "$	c #7E711F",
        "%	c #847954",
        "&	c #C27400",
        "*	c #817D74",
        "=	c #E79300",
        "-	c #BFAB0C",
        ";	c #ADA791",
        ">	c #B3AE87",
        ",	c #B0B2AE",
        "'	c #ECD200",
        ")	c #D6D8D5",
        "!	c #FCFEFA",
        "   ,!!)!!!!!!!!!",
        "   ,!!>;!!!!!!!!",
        "   ,!!>-,!!!!!!!",
        "   ,!!>'$)!!!!!!",
        "   ,!!>-'%!!!!!!",
        "   ,!!>-$-;!!!!!",
        "   ,!!>-*-$)!!!!",
        " @&+!!>-*;-%!!!!",
        "@&=+)!;'-''-*!!!",
        ".@@.;;%%....+;;!",
        ".&&===========$,",
        ".&&=====&&####.,",
        ".&&.++***,,)))!!",
        "#==+)!!!!!!!!!!!",
        " ##+)!!!!!!!!!!!",
        "   *,,,,,,,,,,,,"};"""

    MenuText = "Insight"
    ToolTip = "CAD Tools for supporting the insight framework"

    def Initialize(self):
        #def QT_TRANSLATE_NOOP(scope, text):
        #    return text

        # import Draft tools, icons and macros menu
        import Insight

        # setup menus
        self.cmdList = ["Insight_ExtractCurve", 'Insight_ExtractedWire']
        self.appendMenu("&Insight", self.cmdList)

FreeCADGui.addWorkbench(InsightWorkbench)
