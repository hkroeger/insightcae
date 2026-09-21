#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# PythonAnalysis smoke test: build a ParameterSet containing one instance of
# every concrete, SWIG-exposed Parameter type from plain toolkit, and a
# ResultSet containing one instance of every concrete, SWIG-exposed
# ResultElement type from plain toolkit.
#
# Excluded (not reachable from Python at all, verified against toolkit.i /
# common.i / base/parameters.h):
#   - SpatialTransformationParameter, PropertyLibrarySelectionParameter
#     (their headers are never %include'd)
#   - scalarLengthParameter / scalarVelocityParameter (SimpleDimensionedParameter
#     is a template with no %template instantiation, so it's never wrapped)
#   - FileResult, Video (never %include'd; FileResult is only reachable as
#     Image's base class)
#
# Note: uses explicit "import ... as insight" + qualified access throughout,
# not "from ... import *" - a chained "from Insight.toolkitOffscreen import *"
# (which itself does "from Insight.toolkit import *") does not reliably expose
# every SWIG-generated name to a second star-import.

import sys
import os
import traceback

_LOGFILE = "/tmp/kitchensink_debug.log"


def _log(msg):
    with open(_LOGFILE, "a") as f:
        f.write(msg + "\n")


_log("=== top-level: before import Insight.toolkitOffscreen ===")
try:
    import Insight.toolkitOffscreen as insight
    _log("=== top-level: import succeeded ===")
except BaseException:
    _log("=== top-level: import FAILED ===\n" + traceback.format_exc())
    raise

EXPECTED_PARAM_COUNT = 17
EXPECTED_RESULT_COUNT = 10


def defaultParameters():
    try:
        _log("defaultParameters: enter")
        ps = insight.ParameterSet.create("kitchen sink parameter set")
        _log("defaultParameters: created empty ParameterSet")

        ps.insert("double", insight.DoubleParameter(3.14, "a double parameter"))
        _log("inserted double")
        ps.insert("int", insight.IntParameter(42, "an int parameter"))
        _log("inserted int")
        ps.insert("bool", insight.BoolParameter(True, "a bool parameter"))
        _log("inserted bool")
        ps.insert("string", insight.StringParameter("hello world", "a string parameter"))
        _log("inserted string")
        ps.insert("date", insight.DateParameter("a date parameter"))
        _log("inserted date")
        ps.insert("datetime", insight.DateTimeParameter("a date-time parameter"))
        _log("inserted datetime")
        ps.insert("vector", insight.VectorParameter("a vector parameter"))
        _log("inserted vector")
        ps.insert("doublerange", insight.DoubleRangeParameter(0.0, 10.0, 5, "a double range parameter"))
        _log("inserted doublerange")

        items = insight.StringList()
        items.push_back("OptionA")
        items.push_back("OptionB")
        _log("built StringList for selection")
        ps.insert("selection", insight.SelectionParameter("OptionA", items, "a selection parameter"))
        _log("inserted selection")

        ps.insert("matrix", insight.MatrixParameter("a matrix parameter"))
        _log("inserted matrix")
        ps.insert("path", insight.PathParameter("/tmp", "a path parameter"))
        _log("inserted path")
        ps.insert("directory", insight.DirectoryParameter("/tmp", "a directory parameter"))
        _log("inserted directory")

        ps.insert("array", insight.ArrayParameter(
            insight.DoubleParameter(1.0, "array element"), 3, "an array parameter"))
        _log("inserted array")

        ps.insert("labeledarray", insight.LabeledArrayParameter(
            insight.DoubleParameter(1.0, "labeled array element"), 2, "a labeled array parameter"))
        _log("inserted labeledarray")

        ps.insert("labeledarraykeyselection", insight.LabeledArrayKeySelectionParameter(
            "a labeled array key selection parameter"))
        _log("inserted labeledarraykeyselection")

        subset = insight.SelectableSubsetParameter("a selectable subset parameter")
        option_a = insight.ParameterSet.create("option A parameters")
        option_a.insert("value", insight.DoubleParameter(1.0, "a value"))
        subset.addItem("optionA", option_a)
        subset.setSelection("optionA")
        ps.insert("selectablesubset", subset)
        _log("inserted selectablesubset")

        ps.insert("nestedsubset", insight.ParameterSet.create("a nested parameter set"))
        _log("inserted nestedsubset")

        n = ps.nChildren()
        _log("defaultParameters: nChildren=%d" % n)
        if n != EXPECTED_PARAM_COUNT:
            raise Exception(
                "expected %d parameters, got %d" % (EXPECTED_PARAM_COUNT, n))

        _log("defaultParameters: returning successfully")
        return ps
    except BaseException:
        _log("defaultParameters: FAILED\n" + traceback.format_exc())
        raise


def category():
    return "Test"


def executeAnalysis(parameters, workdir):
    try:
        _log("executeAnalysis: enter")

        n = parameters.nChildren()
        _log("executeAnalysis: parameters.nChildren()=%d" % n)
        if n != EXPECTED_PARAM_COUNT:
            raise Exception(
                "expected %d parameters at execution time, got %d"
                % (EXPECTED_PARAM_COUNT, n))

        result = insight.ResultSet(None, "Kitchen Sink Results",
                                    "covers all toolkit result element types")
        _log("created empty ResultSet")

        result.insert("scalar", insight.ScalarResult(1.23, "a scalar result", "", "m"))
        _log("inserted scalar")
        result.insert("vector", insight.VectorResult([1.0, 2.0, 3.0], "a vector result", "", "m/s"))
        _log("inserted vector result")
        result.insert("tabular", insight.TabularResult("a tabular result", "", ""))
        _log("inserted tabular")
        result.insert("attributetable", insight.AttributeTableResult("an attribute table result", "", ""))
        _log("inserted attributetable")
        result.insert("polarchart", insight.PolarChart("a polar chart", "", ""))
        _log("inserted polarchart")
        result.insert("image", insight.Image("an image result", "", ""))
        _log("inserted image")
        result.insert("polarcontourchart", insight.PolarContourChart("a polar contour chart", "", ""))
        _log("inserted polarcontourchart")
        result.insert("comment", insight.Comment("this is a comment", "a comment result"))
        _log("inserted comment")
        result.insert("chart", insight.Chart("a chart", "", ""))
        _log("inserted chart")

        section = insight.ResultSection("a nested section", "exercises ResultSection nesting")
        _log("created section")
        section.insert("nestedscalar", insight.ScalarResult(4.56, "a nested scalar result", "", "m"))
        _log("inserted nestedscalar")
        section.insert("nestedcomment", insight.Comment("nested comment text", "a nested comment"))
        _log("inserted nestedcomment")
        result.insert("section", section)
        _log("inserted section")

        rn = result.nChildren()
        _log("executeAnalysis: result.nChildren()=%d" % rn)
        if rn != EXPECTED_RESULT_COUNT:
            raise Exception(
                "expected %d results, got %d" % (EXPECTED_RESULT_COUNT, rn))

        with open(os.path.join(workdir, "SUCCESS"), "w") as f:
            f.write("parameters=%d\nresults=%d\n" % (n, rn))
        _log("wrote SUCCESS sentinel")

        return result
    except BaseException:
        _log("executeAnalysis: FAILED\n" + traceback.format_exc())
        raise
