# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8 compliant>

import bpy
import string

class LANPR_reset_object_transfromations(bpy.types.Operator):
    """Reset Transformations"""
    bl_idname = "lanpr.reset_object_transfromations"
    bl_label = "Reset Transformations"

    obj = bpy.props.StringProperty(name="Target Object")
    
    def execute(self, context):
        print(self.obj)
        ob = bpy.data.objects[self.obj]
        ob.location.zero()
        ob.rotation_euler.zero()
        ob.scale.xyz=[1,1,1]
        return {'FINISHED'}

classes=(
    LANPR_reset_object_transfromations,
)
