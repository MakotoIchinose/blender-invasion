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
from bpy.types import Panel
from bpy import data


class CollectionButtonsPanel:
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "collection"
    # COMPAT_ENGINES must be defined in each subclass, external engines can add themselves here

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

class COLLECTION_PT_collection_flags(CollectionButtonsPanel, Panel):
    bl_label = "Collection Flags"

    def draw(self, context):
        layout=self.layout
        collection=context.collection
        vl = context.view_layer
        vlc = vl.active_layer_collection
        if vlc.name == 'Master Collection':
            row = layout.row()
            row.label(text="This is the master collection")
            return
        
        row = layout.row()
        col = row.column(align=True)
        col.prop(vlc,"hide_viewport")
        col.prop(vlc,"holdout")
        col.prop(vlc,"indirect_only")
        row = layout.row()
        col = row.column(align=True)
        col.prop(collection,"hide_select")
        col.prop(collection,"hide_viewport")
        col.prop(collection,"hide_render")
            

class COLLECTION_PT_lanpr_collection(CollectionButtonsPanel, Panel):
    bl_label = "Collection LANPR"

    @classmethod
    def poll(cls, context):
        return context.scene.lanpr.enabled

    def draw(self,context):
        layout = self.layout
        collection = context.collection
        row = layout.row()
        row.prop(collection.lanpr,"usage",expand=True)
        if collection.lanpr.usage!='INCLUDE':
            layout.prop(collection.lanpr,"force")

classes = (
    COLLECTION_PT_collection_flags,
    COLLECTION_PT_lanpr_collection,
)

if __name__ == "__main__":  # only for live edit.
    from bpy.utils import register_class
    for cls in classes:
        register_class(cls)
