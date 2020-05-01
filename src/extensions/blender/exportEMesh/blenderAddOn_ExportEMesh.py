
bl_info = {
    "name": "Export OpenFOAM eMesh",
    "category": "Import-Export",
    "author": "Hannes Kroeger, silentdynamics GmbH",
    "location": "File > Export > OepnFOAM eMesh (.eMesh)",
    "description": "Takes the active object and exports all its edges into an OpenFOAM eMesh file",
}

import bpy, os
from bpy_extras.io_utils import ExportHelper    
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator

class exportEMesh(bpy.types.Operator, ExportHelper):
    bl_idname = "export.export_to_emesh"
    bl_label =  "Export To eMesh"

    filepath = bpy.props.StringProperty(subtype="FILE_PATH")
    filename_ext    = ".eMesh";
    
    filter_glob: StringProperty(
        default="*.eMesh",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )
    
    def execute(self, context):    
        
        fname=self.filepath
        print("save to "+fname)
        
        obdata = bpy.context.active_object.data

        f=open(fname, 'w')
        f.write("""
FoamFile
{
    version     2.0;
    format      ascii;
    class       featureEdgeMesh;
    note        "blender export";
    location    "";
    object      %s;
}""" % os.path.basename(fname))
        f.write("%d\n(\n"%len(obdata.vertices))
        for v in obdata.vertices:
            f.write('({} {} {})\n'.format(v.co.x, v.co.y, v.co.z))
        f.write(")\n\n%d\n(\n"%len(obdata.edges))
        for e in obdata.edges:
            f.write('({} {})\n'.format(e.vertices[0], e.vertices[1]))
        f.write(")\n")
        
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL' }
    
# Define a function to create the menu option for exporting.
def create_menu(self, context):
    self.layout.operator(exportEMesh.bl_idname,text="OpenFOAM eMesh (.eMesh)");

def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(create_menu);

def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(create_menu);


if __name__ == "__main__":
    register()
