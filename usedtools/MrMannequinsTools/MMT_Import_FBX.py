import bpy
import os

#---------- NOTES -----------------------------------------------------------------------------

# WARNING! If using switchable IK vs FK this script can take longer to run for each animation!!! (i can't figure out why the only difference is like 4 variables getting keyframed lol)
# Currently this only works for animation import, meshes will need to be imported/converted manually for now. I will incorporate character mesh importation in the future!

# What this script does is turns off my constraints and adds constraints to my rig that force the the bones to follow an imported animation armature...
# This might be less hacky to do by generating f-curves BUT i found it was more performance friendly this way as iterating over every channel of every curve on every frame is a damn heavy process...
# Once Mr Mannequins bones are following the imported bones i'm visually keyframing all visible bones every frame, after resetting their transforms so they stay relative to their child of constraints...
# Some imported animations may have keys on partial frames (eg: 1.25) so doing this might change the keyframe numbers BUT it should keep the curves the same as well as converting the keyframes to a more Blender freindly sequence...
# The only big issue i can think of is going to be if you import an animation with keyframes every half a frame... but from what i can tell if your exporting animations from UE4 this should not happen.
# It takes around half the time to convert animation with constraints using this method and it seems more accurate as well.

# And a small cosmetic issue that i can't seem to fix is that once the animations are exported/imported back to UE4 they have an extra 1 or 2 keyframes in them, though the length of the animation and bone transforms seem to be identical.

# There are still some work in progress parts, like fixing offset bones such as the left upperarm twist, balls of the feet and right lowerarm twist, i'm unsure if this issue is only on the default mannequin animations...
# If i find it's NOT a problem for other animations then there will need to be an option to turn the location fixing on/off.

# A stands for Animation

#---------- FUNCTIONS -------------------------------------------------------------------------
        
# uses constraints to apply keyframes based on imported animation to the MMT rig...
def A_Convert_By_Child(armature, target, no_parents, loc_bones, a_name, key_loc):
    bpy.context.view_layer.objects.active = armature
    # get a reference to the imported animation...
    action = armature.animation_data.action
    # hop in and out of pose mode and clear the imported armatures transforms...
    bpy.ops.object.mode_set(mode='POSE')
    bpy.ops.pose.select_all(action='SELECT')
    bpy.ops.pose.transforms_clear()
    bpy.ops.object.mode_set(mode='OBJECT')
    # deselect imported armature, select Mr Mannequins rig, go back into pose mode...
    armature.select_set(False)
    bpy.context.view_layer.objects.active = target
    target.select_set(True)
    bpy.ops.object.mode_set(mode='POSE')
    # save current then show all target layers, we need them all open to set up constraints...
    layers = target.data.layers[:]
    target.data.layers = [True]*32
    bpy.ops.pose.reveal(select=True)
    # create a new action named after the FBX and clear all transforms...
    if target.animation_data is None:
        target.animation_data_create()
    target.animation_data.action = bpy.data.actions.new(name=a_name)    
    bpy.ops.pose.select_all(action='SELECT')
    bpy.ops.pose.transforms_clear()
    # hide the deform and mechanism bones so they dont get processed...
    target.JK_MMT.Hide_deform_bones = True
    bpy.ops.pose.select_all(action='DESELECT')
    # if using switchable IK then set limbs to use FK...    
    if target.JK_MMT.IKvsFK_limbs == '2':
        for data in target.JK_MMT.IK_chain_data:
            if not data.Chain_use_fk:
                data.Chain_use_fk = True
    # if using switchable parenting without switchable IK then set that instead...
    elif target.JK_MMT.IK_parenting == '2':
        for data in target.JK_MMT.IK_chain_data:
            if data.Chain_use_parent:
                data.Chain_use_parent = False
    # if we are importing/converting root motion add a child of with no subtarget to the targets root control bone...
    if "root" in no_parents:
        bpy.context.active_object.data.bones.active = target.pose.bones["CB_root"].bone
        child_of = target.pose.bones["CB_root"].constraints.new(type='CHILD_OF')
        child_of.name = "MMT_CHILD_OF"
        child_of.target = armature
        #child_of.subtarget = a_bone.name   
        context_copy = bpy.context.copy()
        context_copy["constraint"] = target.pose.bones["CB_root"].constraints["MMT_CHILD_OF"]
        bpy.ops.constraint.childof_clear_inverse(context_copy, constraint="MMT_CHILD_OF", owner='BONE')
        bpy.ops.constraint.childof_set_inverse(context_copy, constraint="MMT_CHILD_OF", owner='BONE')   
    # mute the default constraints...
    target.JK_MMT.Mute_default_constraints = True 
    # iterate over imported armatures pose bones...
    for a_bone in armature.pose.bones:
        if "CB_" + a_bone.name in target.pose.bones:
            # setting the target armatures active bone as we loop...
            t_bone = target.pose.bones["CB_" + a_bone.name]
            bpy.context.active_object.data.bones.active = t_bone.bone                                   
            # using limit loc/rots to fix inherited over rotation from using a child of when a parent also has a child of...
            if a_bone.name not in no_parents:
                limit_loc = t_bone.constraints.new(type='LIMIT_LOCATION')
                limit_loc.name = "MMT_LIMIT_LOC"
                limit_loc.use_min_x = True
                limit_loc.use_min_y = True
                limit_loc.use_min_z = True
                limit_loc.use_max_x = True
                limit_loc.use_max_y = True
                limit_loc.use_max_z = True
                limit_loc.use_transform_limit = True
                limit_loc.owner_space = 'LOCAL_WITH_PARENT'
                limit_rot = t_bone.constraints.new(type='LIMIT_ROTATION')
                limit_rot.name = "MMT_LIMIT_ROT"
                limit_rot.use_limit_x = True
                limit_rot.use_limit_y = True
                limit_rot.use_limit_z = True
                limit_rot.use_transform_limit = True
                limit_rot.owner_space = 'LOCAL_WITH_PARENT'
            # adding and setting the child of...
            child_of = t_bone.constraints.new(type='CHILD_OF')
            child_of.name = "MMT_CHILD_OF"
            child_of.target = armature
            child_of.subtarget = a_bone.name   
            context_copy = bpy.context.copy()
            context_copy["constraint"] = t_bone.constraints["MMT_CHILD_OF"]
            bpy.ops.constraint.childof_clear_inverse(context_copy, constraint="MMT_CHILD_OF", owner='BONE')
            bpy.ops.constraint.childof_set_inverse(context_copy, constraint="MMT_CHILD_OF", owner='BONE')
            # some bones need there location locked or they have offset location issues...
            if a_bone.name in loc_bones:
                limit_loc1 = t_bone.constraints.new(type='LIMIT_LOCATION')
                limit_loc1.name = "MMT_LIMIT_LOC_FIX"
                limit_loc1.use_min_x = True
                limit_loc1.use_min_y = True
                limit_loc1.use_min_z = True
                limit_loc1.use_max_x = True
                limit_loc1.use_max_y = True
                limit_loc1.use_max_z = True
                limit_loc1.owner_space = 'LOCAL'         
    bpy.ops.pose.select_all(action='DESELECT')
    # select everything that needs keyframing...
    for p_bone in target.pose.bones:
        if "CB" in p_bone.name or "PB" in p_bone.name or "AT" in p_bone.name or "LT" in p_bone.name:
            p_bone.bone.select = True
    bpy.context.active_object.data.bones.active = target.pose.bones["CB_root"].bone
    # iterate over frame range and insert visual keyframes... (might mess up curves if anim's keyframes are not at integers? seems fine on default mannequin anims though)
    for i in range(int(action.frame_range[0]), int(action.frame_range[1] + 1)):
        bpy.context.scene.frame_set(i)
        bpy.ops.pose.transforms_clear()
        if key_loc:
            bpy.ops.anim.keyframe_insert_menu(type='BUILTIN_KSI_VisualLocRot')
        else:
            bpy.ops.anim.keyframe_insert_menu(type='BUILTIN_KSI_VisualRot')
        # maybe keyframe some things on the first frame...
        if i == int(action.frame_range[0]):
            if bpy.context.scene.JK_MMT.I_key_controls:
                target.JK_MMT.keyframe_insert(data_path="Mute_default_constraints", frame=i)
                if target.JK_MMT.IKvsFK_limbs == '2':
                    for data in target.JK_MMT.IK_chain_data:
                        data.keyframe_insert(data_path='Chain_use_fk', frame=i)
                elif target.JK_MMT.IK_parenting == '2':
                    for data in target.JK_MMT.IK_chain_data:
                        data.keyframe_insert(data_path='Chain_use_parent', frame=i)
    # remove the added constraints once keyframing has been completed...
    if "root" in no_parents:
        target.pose.bones["CB_root"].constraints.remove(target.pose.bones["CB_root"].constraints["MMT_CHILD_OF"])
    extra_bones = []
    for a_bone in armature.pose.bones:
        if "CB_" + a_bone.name in target.pose.bones:
            t_bone = target.pose.bones["CB_" + a_bone.name]
            if t_bone.constraints.get("MMT_LIMIT_LOC") != None:
                t_bone.constraints.remove(t_bone.constraints["MMT_LIMIT_LOC"])
            if t_bone.constraints.get("MMT_LIMIT_LOC_FIX") != None:
                t_bone.constraints.remove(t_bone.constraints["MMT_LIMIT_LOC_FIX"])
            if t_bone.constraints.get("MMT_LIMIT_ROT") != None:
                t_bone.constraints.remove(t_bone.constraints["MMT_LIMIT_ROT"])
            if t_bone.constraints.get("MMT_CHILD_OF") != None:
                t_bone.constraints.remove(t_bone.constraints["MMT_CHILD_OF"])
        else:
            extra_bones.append(a_bone.name)    
    # reset the targets armature layers to what was open when we started...
    target.data.layers = layers
    # remove the imported animation...
    bpy.data.actions.remove(action)
    # return to object mode... (only needed if batch importing?)
    bpy.ops.object.mode_set(mode='OBJECT')
    # print any extra bones...
    if len(extra_bones) > 0:
        print("These bones do not exist in the default mannequin armature: ", extra_bones[:], " Animation has not been imported/converted for them") 

# scales the time/length of the imported animation in the dopesheet...    
def A_Scale_Keyframes(pre_fps, post_fps, offset):
    # get reference to the current area type...
    last_area = bpy.context.area.type
    # switch to the dope sheet and turn of auto snapping...
    bpy.context.area.type = 'DOPESHEET_EDITOR'
    bpy.context.space_data.ui_mode = 'ACTION'
    bpy.context.space_data.auto_snap = 'NONE'
    # set the current frame to the start frame of the animation... (which is going to be the offset used by import)
    bpy.context.scene.frame_current = offset
    # scale the keyframes by pre_fps divided by post fps so the animation has the right length for the desired pre fps... (when animation FBX is imported it seems to automatically set the render fps to what the the FBX was using)
    bpy.ops.transform.transform(mode='TIME_SCALE', value=(pre_fps / post_fps, 0, 0, 0), orient_axis='X', orient_type='VIEW', orient_matrix=((-1, -0, -0), (-0, -1, -0), (-0, -0, -1)), orient_matrix_type='VIEW', mirror=True, use_proportional_edit=False, proportional_edit_falloff='SMOOTH', proportional_size=1, use_proportional_connected=False, use_proportional_projected=False)
    # return the area back to what it was...
    bpy.context.area.type = last_area
    # set the render fps to what it was before importing...
    bpy.context.scene.render.fps = pre_fps


def I_Animation(target, MMT, name):
    # get the render FPS before importing...
    pre_import_fps = bpy.context.scene.render.fps
    # import the fbx...
    bpy.ops.import_scene.fbx(filepath=os.path.join(bpy.path.abspath(MMT.I_path_animations), name), directory="", filter_glob="*.fbx", use_manual_orientation=False, global_scale=1.0, bake_space_transform=False, use_custom_normals=True, use_image_search=True, use_alpha_decals=False, decal_offset=0.0, use_anim=True, anim_offset=MMT.I_frame_offset, use_custom_props=MMT.I_user_props, use_custom_props_enum_as_string=True, ignore_leaf_bones=False, force_connect_children=False, automatic_bone_orientation=False, primary_bone_axis='Y', secondary_bone_axis='X', use_prepost_rot=True, axis_forward='-Z', axis_up='Y')                  
    # once imported all new objects will be selected so save a reference to them...
    new_objects = bpy.context.selected_objects
    # get the render FPS after importing...
    post_import_fps = bpy.context.scene.render.fps
    if target.JK_MMT.Rig_type not in ['NONE', 'TEMPLATE']:
        # get a reference to the imported armature... (There shouldn't be more than one armature, if there is get the first one with animation data)            
        anim_armature = None
        for obj in bpy.context.selected_objects:
            if obj.type == 'ARMATURE' and obj.animation_data:
                anim_armature = obj
                break                 
        # if an armature was imported with animation data...
        if anim_armature != None:
            # if we want to scale the keyframes to fit our fps before conversion to Mr Mannequin...
            if MMT.I_pre_scale_keyframes:
                A_Scale_Keyframes(pre_import_fps, post_import_fps, MMT.I_frame_offset)
            # convert the action on to the Mr Mannequin Rig...
            A_Convert_By_Child(anim_armature, target, [("root" if MMT.I_root_motion else "pelvis")], ["lowerarm_twist_01_l", "lowerarm_twist_01_r", "upperarm_twist_01_l", "upperarm_twist_01_r", "ball_l", "ball_r"], name[:-4], MMT.I_key_location)
            # or if we want to scale the keyframes to fit our fps after conversion to Mr Mannequin...
            if not MMT.I_pre_scale_keyframes:
                A_Scale_Keyframes(pre_import_fps, post_import_fps, MMT.I_frame_offset)
        # if there was no anim data let us know...
        else:
            print("No animation to convert!")
    else:
        print("Animation import only currently supported for the default and re-targeted rigs")        
    # once finished remove all the objects that got imported...
    for obj in new_objects:        
        bpy.data.objects.remove(obj, do_unlink=True)

def I_Mesh(target, MMT, name):    
    # import the .fbx without animation and apply everything...    
    bpy.ops.import_scene.fbx(filepath=os.path.join(bpy.path.abspath(MMT.I_path_meshes), name), directory="", filter_glob="*.fbx", use_manual_orientation=False, global_scale=1.0, bake_space_transform=False, use_custom_normals=True, use_image_search=True, use_alpha_decals=False, decal_offset=0.0, use_anim=False, anim_offset=MMT.I_frame_offset, use_custom_props=MMT.I_user_props, use_custom_props_enum_as_string=True, ignore_leaf_bones=False, force_connect_children=False, automatic_bone_orientation=False, primary_bone_axis='Y', secondary_bone_axis='X', use_prepost_rot=True, axis_forward='-Z', axis_up='Y')                  
    # apply the scale...
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    # if we are trying to rig the mesh to the active MMT rig...
    if MMT.I_rig_to_active:
        imported_objects = bpy.context.selected_objects
        new_meshes = []
        new_armature = None
        # for each new object...
        for obj in bpy.context.selected_objects:
            # if it's a mesh... 
            if obj.type == 'MESH':
                # iterate through modifiers...
                for modifier in obj.modifiers:
                    # if the modifier is an armature one
                    if modifier.type == 'ARMATURE':
                        # assign the new armature if it hasn't been assigned...
                        if new_armature == None:
                            new_armature = modifier.object
                        # if the modifiers target object is the new armature...
                        if modifier.object == new_armature:
                            # make sure the modifier is named to default...
                            modifier.name = "Armature"
                            # append it to new_meshes...
                            new_meshes.append(obj)
                            # switch to deformation from the active MMT armature...
                            modifier.object = target  
        # clean up anything we don't want to keep...    
        for obj in imported_objects:
            if obj not in new_meshes:
                bpy.data.objects.remove(obj, do_unlink=True)
    elif MMT.I_import_to_retarget:
        bpy.ops.jk.c_retargetrig()  
    
#---------- EXECUTION -------------------------------------------------------------------------

def Import(MMT):
    # target will always be the active object...
    target = bpy.context.object
    # if we want to import meshes...        
    if MMT.I_meshes:
        # show deform bones so they can be selected/processed...    
        if target.JK_MMT.Hide_deform_bones:
            target.JK_MMT.Hide_deform_bones = False
        # if batch importing...
        if MMT.I_batch_meshes:
            # for each file in the import folder...
            for name in os.listdir(bpy.path.abspath(MMT.I_path_meshes)):
                # if the file is an FBX...
                if name.upper().endswith(".FBX"):
                    I_Mesh(target, MMT, name)
        else:
            I_Mesh(target, MMT, bpy.path.basename(MMT.I_mesh_fbxs))
    # if we want to import animations...
    if MMT.I_animations:
        # hide deform bones so they dont get selected/processed...    
        if not target.JK_MMT.Hide_deform_bones:
            target.JK_MMT.Hide_deform_bones = True
        # if batch importing...
        if MMT.I_batch_animations:
            # for each file in the import folder...
            for name in os.listdir(bpy.path.abspath(MMT.I_path_animations)):
                # if the file is an FBX...
                if name.upper().endswith(".FBX"):
                    I_Animation(target, MMT, name)
        else:
            I_Animation(target, MMT, bpy.path.basename(MMT.I_animation_fbxs))
            
# used for testing...
#Import(bpy.context.scene.JK_MMT)

# R_Create_Extra_Bones(armature, target)            