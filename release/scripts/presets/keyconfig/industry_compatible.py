import os
import bpy

# ------------------------------------------------------------------------------
# Operators needed by this keymap to function

# Selection Modes

class IC_KEYMAP_OT_mesh_select_mode(bpy.types.Operator):
    bl_idname = "ic_keymap.mesh_select_mode"
    bl_label = "Switch to Vertex, Edge or Face Mode from any mode"
    bl_options = {'UNDO'}

    type: bpy.props.EnumProperty(
        name="Mode",
        items=(
            ('VERT', "Vertex", "Switcth to Vertex Mode From any Mode"),
            ('EDGE', "Edge", "Switcth to Edge Mode From any Mode"),
            ('FACE', "Face", "Switcth to Face Mode From any Mode"),
        ),
    )

    @classmethod
    def poll(cls, context):
        return (context.active_object is not None) and (context.object.type == 'MESH')

    def execute(self, context):
        bpy.ops.object.mode_set(mode='EDIT')
        bpy.ops.mesh.select_mode(type=self.type)

        return{'FINISHED'}

# JKL controls for playback

class IC_KEYMAP_OT_jkl_controls(bpy.types.Operator):
    bl_idname = "ic_keymap.jkl_controls"
    bl_label = "jkl Controls"
    bl_description = "jkl Controls"

    mode: bpy.props.EnumProperty(
        name="JKL Mode",
        items=(
            ('J', "J", "Play Backwards"),
            ('K', "K", "Pause"),
            ('L', "L", "Play Forwards"),
            ('KJ', "L", "Step Frame Back"),
            ('KL', "L", "Step Frame Forward"),
        ),
    )

    def execute(self, context):
        scr = bpy.context.screen
        scops = bpy.ops.screen

        if self.mode == "J":
            if scr.is_animation_playing == True:
                scops.animation_play()
                scops.animation_play(reverse=True)
            else:
                scops.animation_play(reverse=True)
        elif self.mode == "L":
            if scr.is_animation_playing == True:
                scops.animation_play()
                scops.animation_play()
            else:
                scops.animation_play()
        elif self.mode == "K":
            if scr.is_animation_playing == True:
                scops.animation_play()
        elif self.mode == "KJ":
            if scr.is_animation_playing == True:
                scops.animation_play()
            bpy.context.scene.frame_set(bpy.context.scene.frame_current - 1)
        elif self.mode == "KL":
            if scr.is_animation_playing == True:
                scops.animation_play()
            bpy.context.scene.frame_set(bpy.context.scene.frame_current + 1)
        else:
            if scr.is_animation_playing == True:
                scops.animation_play()

        return {'FINISHED'}


# Keyframe

class IC_KEYMAP_OT_insert_key(bpy.types.Operator):
    bl_idname = "ic_keymap.insert_key"
    bl_label = "Insert Location Keyframe"
    bl_options = {'UNDO'}

    mode: bpy.props.EnumProperty(
        name="Keyframe Mode",
        items=(
            ('LOCATION', "Location", "Set location keyframe"),
            ('ROTATION', "Rotation", "Set rotation keyframe"),
            ('SCALING', "Scaling", "Set scaling keyframe"),
            ('LOCROTSCALE', "LocRotScale", "Set location, rotation and scaling keyframes"),
        ),
    )

    def execute(self, context):
        if self.mode == "LOCATION":
            bpy.ops.anim.keyframe_insert_menu(type='Location')
        elif self.mode == "ROTATION":
            bpy.ops.anim.keyframe_insert_menu(type='Rotation')
        elif self.mode == "SCALING":
            bpy.ops.anim.keyframe_insert_menu(type='Scaling')
        elif self.mode == "LOCROTSCALE":
            bpy.ops.anim.keyframe_insert_menu(type='LocRotScale')
        return{'FINISHED'}


classes = (
    IC_KEYMAP_OT_mesh_select_mode,
    IC_KEYMAP_OT_insert_key,
    IC_KEYMAP_OT_jkl_controls,
)



# ------------------------------------------------------------------------------
# Keymap

dirname, filename = os.path.split(__file__)
idname = os.path.splitext(filename)[0]

def update_fn(_self, _context):
    load()


def keyconfig_data_oskey_from_ctrl(keyconfig_data_src):
    # TODO, make into more generic event transforming function.
    keyconfig_data_dst = []
    for km_name, km_parms, km_items_data_src in keyconfig_data_src:
        km_items_data_dst = km_items_data_src.copy()
        items_dst = []
        km_items_data_dst["items"] = items_dst
        for item_src in km_items_data_src["items"]:
            item_op, item_event, item_prop = item_src
            if "ctrl" in item_event:
                item_event = item_event.copy()
                item_event["oskey"] = item_event["ctrl"]
                del item_event["ctrl"]
                items_dst.append((item_op, item_event, item_prop))
            items_dst.append(item_src)
        keyconfig_data_dst.append((km_name, km_parms, km_items_data_dst))
    return keyconfig_data_dst


industry_compatible = bpy.utils.execfile(os.path.join(dirname, "keymap_data", "industry_compatible_data.py"))


def load():
    from sys import platform
    from bl_keymap_utils.io import keyconfig_init_from_data

    prefs = bpy.context.preferences

    kc = bpy.context.window_manager.keyconfigs.new(idname)
    params = industry_compatible.Params(use_mouse_emulate_3_button=prefs.inputs.use_mouse_emulate_3_button)
    keyconfig_data = industry_compatible.generate_keymaps(params)

    if platform == 'darwin':
        keyconfig_data = keyconfig_data_oskey_from_ctrl(keyconfig_data)

    keyconfig_init_from_data(kc, keyconfig_data)

if __name__ == "__main__":
    # XXX, no way to unregister
    for cls in classes:
        bpy.utils.register_class(cls)

    load()
