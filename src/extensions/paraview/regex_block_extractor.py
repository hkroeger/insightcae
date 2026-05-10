"""
RegexBlockExtractor – ParaView Plugin
======================================
Selektiert Blöcke aus einem vtkMultiBlockDataSet anhand von
Regular Expressions, die der User in der GUI eingeben kann.

Installation:
  Tools → Manage Plugins → Load New → diese Datei auswählen
  Optional: "Auto Load" aktivieren

Danach erscheint der Filter unter:
  Filters → Alphabetical → Regex Block Extractor
"""

from paraview.util.vtkAlgorithm import (
    smdomain,
    smhint,
    smproperty,
    smproxy,
)
from vtkmodules.vtkCommonDataModel import vtkMultiBlockDataSet
from vtkmodules.util.vtkAlgorithm import VTKPythonAlgorithmBase
import vtk
import re


@smproxy.filter(name="RegexBlockExtractor", label="Regex Extract Block")
@smhint.xml('<ShowInMenu category="Filters"/>')
@smproperty.input(name="Input", port_index=0)
@smdomain.datatype(dataTypes=["vtkMultiBlockDataSet"], composite_data_supported=True)
class RegexBlockExtractor(VTKPythonAlgorithmBase):
    """
    Extrahiert Blöcke aus einem MultiBlockDataSet, deren Namen
    auf mindestens einen der angegebenen Regular Expressions passen.
    """

    def __init__(self):
        super().__init__(
            nInputPorts=1,
            nOutputPorts=1,
            outputType="vtkMultiBlockDataSet",
        )
        # Standardwert: alles matchen
        self._patterns = ".*"
        self._case_sensitive = False
        self._maintain_structure = False

    # ------------------------------------------------------------------
    # GUI-Properties
    # ------------------------------------------------------------------

    @smproperty.stringvector(
        name="Patterns",
        label="Regex Patterns (kommagetrennt)",
        default_values=".*",
        number_of_elements=1,
    )
    @smdomain.xml("""
        <Documentation>
          Kommagetrennte Liste von Regular Expressions.
          Ein Block wird selektiert, wenn sein Name auf
          mindestens einen Pattern passt.
          Beispiele:
            wall.*, .*inlet.*, patch_\\d+
        </Documentation>
    """)
    def SetPatterns(self, value):
        if self._patterns != value:
            self._patterns = value
            self.Modified()

    def GetPatterns(self):
        return self._patterns

    @smproperty.intvector(
        name="CaseSensitive",
        label="Groß-/Kleinschreibung beachten",
        default_values=0,
        number_of_elements=1,
    )
    @smdomain.xml('<BooleanDomain name="bool"/>')
    def SetCaseSensitive(self, value):
        if self._case_sensitive != bool(value):
            self._case_sensitive = bool(value)
            self.Modified()

    def GetCaseSensitive(self):
        return int(self._case_sensitive)

    @smproperty.intvector(
        name="MaintainStructure",
        label="Blockstruktur erhalten (leere Elternknoten behalten)",
        default_values=0,
        number_of_elements=1,
    )
    @smdomain.xml('<BooleanDomain name="bool"/>')
    def SetMaintainStructure(self, value):
        if self._maintain_structure != bool(value):
            self._maintain_structure = bool(value)
            self.Modified()

    def GetMaintainStructure(self):
        return int(self._maintain_structure)

    # ------------------------------------------------------------------
    # Hilfsmethoden
    # ------------------------------------------------------------------

    def _parse_patterns(self):
        """Kommagetrennte Pattern-Zeichenkette → Liste kompilierter Regex."""
        flags = 0 if self._case_sensitive else re.IGNORECASE
        compiled = []
        for raw in self._patterns.split(","):
            p = raw.strip()
            if p:
                try:
                    compiled.append(re.compile(p, flags))
                except re.error as e:
                    print(f"[RegexBlockExtractor] Ungültiger Pattern '{p}': {e}")
        return compiled

    def _matches(self, name, compiled_patterns):
        return any(p.search(name) for p in compiled_patterns)

    def _collect_leaves(self, mb, parent_path=""):
        """
        Rekursiv alle Blatt-Blöcke sammeln als Liste von Dicts:
          index, name, path, parent_mb, block
        """
        results = []
        name_key = vtkMultiBlockDataSet.NAME()
        n = mb.GetNumberOfBlocks()
        for i in range(n):
            block = mb.GetBlock(i)
            name = ""
            if mb.HasMetaData(i) and mb.GetMetaData(i).Has(name_key):
                name = mb.GetMetaData(i).Get(name_key)
            current_path = f"{parent_path}/{name or f'block_{i}'}"

            if block is not None and block.IsA("vtkMultiBlockDataSet"):
                results.extend(self._collect_leaves(block, current_path))
            else:
                results.append(
                    dict(index=i, name=name, path=current_path,
                         parent_mb=mb, block=block)
                )
        return results

    def _collect_with_structure(self, mb, compiled_patterns, parent_path=""):
        """
        Baut einen neuen MB-Baum, der nur gematchte Blöcke enthält,
        aber die ursprüngliche Hierarchie beibehält.
        Gibt (neuer_MB_oder_None, match_count) zurück.
        """
        name_key = vtkMultiBlockDataSet.NAME()
        new_mb = vtkMultiBlockDataSet()
        out_idx = 0
        match_count = 0

        n = mb.GetNumberOfBlocks()
        for i in range(n):
            block = mb.GetBlock(i)
            name = ""
            if mb.HasMetaData(i) and mb.GetMetaData(i).Has(name_key):
                name = mb.GetMetaData(i).Get(name_key)
            current_path = f"{parent_path}/{name or f'block_{i}'}"

            if block is not None and block.IsA("vtkMultiBlockDataSet"):
                child_mb, child_count = self._collect_with_structure(
                    block, compiled_patterns, current_path
                )
                if child_count > 0:
                    new_mb.SetBlock(out_idx, child_mb)
                    if name:
                        new_mb.GetMetaData(out_idx).Set(name_key, name)
                    out_idx += 1
                    match_count += child_count
            else:
                if self._matches(name, compiled_patterns):
                    new_mb.SetBlock(out_idx, block)
                    if name:
                        new_mb.GetMetaData(out_idx).Set(name_key, name)
                    out_idx += 1
                    match_count += 1
                    print(f"[RegexBlockExtractor]  + '{name}'  ({current_path})")

        new_mb.SetNumberOfBlocks(out_idx)
        return new_mb, match_count

    # ------------------------------------------------------------------
    # VTK Pipeline
    # ------------------------------------------------------------------

    def FillInputPortInformation(self, port, info):
        info.Set(self.INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet")
        return 1

    def RequestData(self, request, inInfo, outInfo):
        inp = vtkMultiBlockDataSet.GetData(inInfo[0], 0)
        out = vtkMultiBlockDataSet.GetData(outInfo, 0)

        compiled_patterns = self._parse_patterns()
        if not compiled_patterns:
            print("[RegexBlockExtractor] Keine gültigen Patterns – Output ist leer.")
            return 1

        print(f"\n[RegexBlockExtractor] Patterns : {self._patterns}")
        print(f"[RegexBlockExtractor] Case-sensitiv: {self._case_sensitive}")

        if self._maintain_structure:
            result_mb, match_count = self._collect_with_structure(inp, compiled_patterns)
            out.ShallowCopy(result_mb)
        else:
            leaves = self._collect_leaves(inp)
            matched = [b for b in leaves if self._matches(b["name"], compiled_patterns)]
            print(f"[RegexBlockExtractor] Blöcke gesamt : {len(leaves)}")
            print(f"[RegexBlockExtractor] Blöcke matched: {len(matched)}")
            out.SetNumberOfBlocks(len(matched))
            name_key = vtkMultiBlockDataSet.NAME()
            for out_idx, b in enumerate(matched):
                out.SetBlock(out_idx, b["block"])
                if b["name"]:
                    out.GetMetaData(out_idx).Set(name_key, b["name"])
                print(f"[RegexBlockExtractor]  + [{b['index']}] '{b['name']}'  ({b['path']})")

        return 1
