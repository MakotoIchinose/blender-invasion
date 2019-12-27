"""
Copyright (c) 2018 iCyP
Released under the MIT license
https://opensource.org/licenses/mit-license.php

"""
VRM="VRM"

class VRM_pydata(object):
    def __init__(
            self,
            filepath = None,json = None,decoded_binary = None,
            image_propaties = None,meshes =None,materials = None,
            nodes_dict = None,origine_nodes_dict = None,
            skins_joints_list = None , skins_root_node_list = None
            ):
        self.filepath = filepath
        self.json = json
        self.decoded_binary = decoded_binary

        self.image_propaties = image_propaties if image_propaties is not None else []
        self.meshes = meshes if meshes is not None else []
        self.materials = materials if materials is not None else []
        self.nodes_dict = nodes_dict if nodes_dict is not None else {}
        self.origine_nodes_dict = origine_nodes_dict if origine_nodes_dict is not None else {}
        self.skins_joints_list = skins_joints_list if skins_joints_list is not None else []
        self.skins_root_node_list = skins_root_node_list if skins_root_node_list is not None else []


class Mesh(object):
    def __init__(self):
        self.name = ""
        self.face_indices = []
        self.skin_id = None
        self.object_id = None



class Node(object):
    def __init__(self):
        self.name = ""
        self.position = None
        self.rotation = None
        self.scale = None
        self.children = None
        self.blend_bone = None
        self.mesh_id = None
        self.skin_id = None


class HumanBones(object):
    center_req = ["hips","spine","chest","neck","head"]
    left_leg_req = ["leftUpperLeg","leftLowerLeg","leftFoot"]
    left_arm_req = ["leftUpperArm","leftLowerArm","leftHand"]
    right_leg_req = ["rightUpperLeg","rightLowerLeg","rightFoot"]
    right_arm_req = ["rightUpperArm","rightLowerArm","rightHand"]

    requires= [
        *center_req[:],
        *left_leg_req[:],*right_leg_req[:],
        *left_arm_req[:],*right_arm_req[:]   
    ]

    left_arm_def = ["leftShoulder",
                    "leftThumbProximal","leftThumbIntermediate","leftThumbDistal","leftIndexProximal",
                    "leftIndexIntermediate","leftIndexDistal","leftMiddleProximal","leftMiddleIntermediate",
                    "leftMiddleDistal","leftRingProximal","leftRingIntermediate","leftRingDistal",
                    "leftLittleProximal","leftLittleIntermediate","leftLittleDistal"]

    right_arm_def = ["rightShoulder",
                        "rightThumbProximal","rightThumbIntermediate","rightThumbDistal",
                        "rightIndexProximal","rightIndexIntermediate","rightIndexDistal",
                        "rightMiddleProximal","rightMiddleIntermediate","rightMiddleDistal",
                        "rightRingProximal","rightRingIntermediate","rightRingDistal",
                        "rightLittleProximal","rightLittleIntermediate","rightLittleDistal"]
    center_def = ["upperChest","jaw"]
    left_leg_def = ["leftToes"]
    right_leg_def = ["rightToes"]
    defines =  [
        "leftEye","rightEye",
        *center_def[:],
        *left_leg_def[:],
        *right_leg_def[:],
        *left_arm_def[:],
        *right_arm_def[:]
                ]
    #child:parent
    hierarchy = {
        #体幹
        "leftEye":"head",
        "rightEye":"head",
        "jaw":"head",
        "head":"neck",
        "neck":"upperChest",
        "upperChest":"chest",
        "chest":"spine",
        "spine":"hips", #root
        #右上
        "rightShoulder":"chest",
        "rightUpperArm":"rightShoulder",
        "rightLowerArm":"rightUpperArm",
        "rightHand":"rightLowerArm",
        "rightThumbProximal":"rightHand",
        "rightThumbIntermediate":"rightThumbProximal",
        "rightThumbDistal":"rightThumbIntermediate",
        "rightIndexProximal":"rightHand",
        "rightIndexIntermediate":"rightIndexProximal",
        "rightIndexDistal":"rightIndexIntermediate",
        "rightMiddleProximal":"rightHand",
        "rightMiddleIntermediate":"rightMiddleProximal",
        "rightMiddleDistal":"rightMiddleIntermediate",
        "rightRingProximal":"rightHand",
        "rightRingIntermediate":"rightRingProximal",
        "rightRingDistal":"rightRingIntermediate",
        "rightLittleProximal":"rightHand",
        "rightLittleIntermediate":"rightLittleProximal",
        "rightLittleDistal":"rightLittleIntermediate",
        #左上
        "leftShoulder":"chest",
        "leftUpperArm":"leftShoulder",
        "leftLowerArm":"leftUpperArm",
        "leftHand":"leftLowerArm",
        "leftThumbProximal":"leftHand",
        "leftThumbIntermediate":"leftThumbProximal",
        "leftThumbDistal":"leftThumbIntermediate",
        "leftIndexProximal":"leftHand",
        "leftIndexIntermediate":"leftIndexProximal",
        "leftIndexDistal":"leftIndexIntermediate",
        "leftMiddleProximal":"leftHand",
        "leftMiddleIntermediate":"leftMiddleProximal",
        "leftMiddleDistal":"leftMiddleIntermediate",
        "leftRingProximal":"leftHand",
        "leftRingIntermediate":"leftRingProximal",
        "leftRingDistal":"leftRingIntermediate",
        "leftLittleProximal":"leftHand",
        "leftLittleIntermediate":"leftLittleProximal",
        "leftLittleDistal":"leftLittleIntermediate",

        #左足
        "leftUpperLeg":"hips",
        "leftLowerLeg":"leftUpperLeg",
        "leftFoot":"leftLowerLeg",
        "leftToes":"leftFoot",
        #右足
        "rightUpperLeg":"hips",
        "rightLowerLeg":"rightUpperLeg",
        "rightFoot":"rightLowerLeg",
        "rightToes":"rightFoot"
    }


class Image_props(object):
    def __init__(self,name,filepath,fileType):
        self.name = name
        self.filePath = filepath
        self.fileType = fileType




class Material(object):
    def __init__(self):
        self.name = ""
        self.shader_name = ""



class Material_GLTF(Material):
    def __init__(self):
        super().__init__()

        self.base_color = [1,1,1,1]
        self.metallic_factor = 1
        self.roughness_factor = 1
        self.emissive_factor = [0,0,0]

        self.color_texture_index = None
        self.color_texcood_index = None
        self.metallic_roughness_texture_index = None
        self.metallic_roughness_texture_texcood = None
        self.normal_texture_index = None
        self.normal_texture_texcoord_index = None
        self.emissive_texture_index = None
        self.emissive_texture_texcoord_index = None
        self.occlusion_texture_index = None
        self.occlusion_texture_texcood_index = None
        
        self.double_sided = False
        self.alphaMode = "OPAQUE"
        self.shadeless = 0 #0 is shade ,1 is shadeless


class Material_Transparent_Z_write(Material):
    float_props = [
            "_MainTex",
            "_Cutoff",
            "_BlendMode",
            "_CullMode",
            "_VColBlendMode",
            "_SrcBlend",
            "_DstBlend",
            "_ZWrite",
            ]
    texture_index_list = [
        "_MainTex"
        ]
    vector_props = [
        "_Color"
        ]

    def __init__(self):
        super().__init__()
        self.float_prop_dic = {prop: None for prop in self.float_props}
        self.vector_props_dic = {prop: None for prop in self.vector_props}
        self.texture_index_dic = {tex:None for tex in self.texture_index_list}



class Material_MToon(Material):
    #{key = MToonProp, val = ShaderNodeGroup_member_name}
    version = 32
    float_props_exchange_dic = {
        "_MToonVersion":None,
        "_Cutoff":"CutoffRate",
        "_BumpScale":"BumpScale",
        "_ReceiveShadowRate":"ReceiveShadowRate",
        "_ShadeShift":"ShadeShift",
        "_ShadeToony":"ShadeToony",
        "_RimLightingMix":"RimLightingMix",
        "_RimFresnelPower":"RimFresnelPower",
        "_RimLift":"RimLift",
        "_ShadingGradeRate":"ShadingGradeRate",
        "_LightColorAttenuation":"LightColorAttenuation",
        "_IndirectLightIntensity":"IndirectLightIntensity",
        "_OutlineWidth":"OutlineWidth",
        "_OutlineScaledMaxDistance":"OutlineScaleMaxDistance",
        "_OutlineLightingMix":"OutlineLightingMix",
        "_UvAnimScrollX":"UV_Scroll_X", ######
        "_UvAnimScrollY":"UV_Scroll_Y", ######
        "_UvAnimRotation":"UV_Scroll_Rotation",######
        "_DebugMode":None,
        "_BlendMode":None,
        "_OutlineWidthMode":"OutlineWidthMode",
        "_OutlineColorMode":"OutlineColorMode",
        "_CullMode":None,
        "_OutlineCullMode":None,
        "_SrcBlend":None,
        "_DstBlend":None,
        "_ZWrite":None,
        "_IsFirstSetup":None,
    }

    texture_kind_exchange_dic = {
        "_MainTex":"MainTexture",
        "_ShadeTexture":"ShadeTexture",
        "_BumpMap":"NomalmapTexture",
        "_ReceiveShadowTexture":"ReceiveShadow_Texture",
        "_ShadingGradeTexture":"ShadingGradeTexture",
        "_EmissionMap":"Emission_Texture",
        "_SphereAdd":"SphereAddTexture",
        "_RimTexture":"RimTexture",
        "_OutlineWidthTexture":"OutlineWidthTexture",
        "_UvAnimMaskTexture":"UV_Animation_Mask_Texture" #####
        }
    vector_base_props_exchange_dic = {
            "_Color":"DiffuseColor",
            "_ShadeColor":"ShadeColor",
            "_EmissionColor":"EmissionColor",
            "_RimColor":"RimColor",
            "_OutlineColor":"OutlineColor"
            
        }
    #texture offset and scaling props by texture
    vector_props_exchange_dic = {}
    vector_props_exchange_dic.update(vector_base_props_exchange_dic)
    vector_props_exchange_dic.update(texture_kind_exchange_dic)
    
    keyword_list = [
        "_NORMALMAP",
        "_ALPHATEST_ON",
        "_ALPHABLEND_ON",
        "_ALPHAPREMULTIPLY_ON",
        "MTOON_OUTLINE_WIDTH_WORLD",
        "MTOON_OUTLINE_WIDTH_SCREEN",
        "MTOON_OUTLINE_COLOR_FIXED",
        "MTOON_OUTLINE_COLOR_MIXED",
        "MTOON_DEBUG_NORMAL",
        "MTOON_DEBUG_LITSHADERATE"
    ]
    tagmap_list = [
        "RenderType"
    ]

    def __init__(self):
        super().__init__()
        self.float_props_dic = {prop:None for prop in self.float_props_exchange_dic.keys()}
        self.vector_props_dic = {prop:None for prop in self.vector_props_exchange_dic.keys()}
        self.texture_index_dic = {prop: None for prop in self.texture_kind_exchange_dic.keys()}
        self.keyword_dic = {kw:False for kw in self.keyword_list}
        self.tag_dic = {tag:None for tag in self.tagmap_list}
        


def nested_json_value_getter(target_dict,attr_list,default=None):
    _result = default
    if isinstance(target_dict,(dict,list)):
        _attr = attr_list.pop(0)
        if (isinstance(target_dict,list) and len(target_dict) > abs(_attr)) \
            or (isinstance(target_dict,dict) and target_dict.get(_attr) is not None):
                if attr_list:
                    _result = nested_json_value_getter(target_dict[_attr],attr_list)
                else:
                    _result =  target_dict[_attr]  
    return _result

    
if "__main__" == __name__:
    pass
