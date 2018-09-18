#!/bin/bash

OPTIND=1         # Reset in case getopts has been used previously in the shell.
while getopts "h?isa" opt; do
    case "$opt" in
    h|\?)
        echo "<object mesh file> <tool mesh file> <result mesh file>"
        echo "-s : subtract tool from object"
        echo "-a : add (unify) tool and object"
        echo "-i : intersect tool and object"
        exit 0
        ;;
    i)  OP=i
        ;;
    s)  OP=s
        ;;
    a)  OP=a
        ;;
    esac
done

OBJ=${@:$OPTIND:1}
TOOL=${@:$OPTIND+1:1}
RES=${@:$OPTIND+2:1}

if [ "$OP" == "i" ] || [ "$OP" == "s" ] || [ "$OP" == "a" ]; then
 echo "operation = $OP"
 echo "object = $OBJ"
 echo "tool = $TOOL"
 echo "result = $RES"
else
 echo "Error: you need to specifiy the operation!"
 exit -1
fi

/usr/bin/blender --background --python-console << ende

import bpy
from mathutils import Vector, Matrix, Quaternion, Euler, Color

bpy.ops.import_mesh.stl(filepath="$TOOL")
tool = bpy.context.object
print(tool.name)

bpy.ops.import_mesh.stl(filepath="$OBJ")
obj = bpy.context.object
print(obj.name)

opname="bool"
# set the object to active
bpy.context.scene.objects.active = obj
obj.modifiers.new(opname, 'BOOLEAN')
opnames={
    'i': 'INTERSECT',
    'a': 'UNION',
    's': 'DIFFERENCE'
}
print("opname=", opnames["$OP"])
obj.modifiers[opname].operation=opnames["$OP"]
obj.modifiers[opname].object = tool

bpy.ops.object.modifier_apply(apply_as='DATA', modifier=opname)

bpy.ops.export_mesh.stl(filepath="$RES", use_selection=True)

ende
