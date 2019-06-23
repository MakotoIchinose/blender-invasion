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

import os

import bpy
from mathutils import Matrix, Vector, Color
from bpy_extras import io_utils, node_shader_utils

def write_mtl(filepath, materials):
    C = bpy.context
    scene = C.scene
    world = scene.world
    world_amb = Color((0.8, 0.8, 0.8))

    source_dir = os.path.dirname(bpy.data.filepath)
    dest_dir = os.path.dirname(filepath)

    with open(filepath, "w", encoding="utf8", newline="\n") as f:
        fw = f.write

        fw('# Blender MTL File: %r\n' % (os.path.basename(bpy.data.filepath) or "None"))
        fw('# Material Count: %i\n' % len(materials))

        for material in materials:
            fw('newmtl %s\n' % material)
            mat = bpy.data.materials[material]
            mat_wrap = node_shader_utils.PrincipledBSDFWrapper(mat) if mat else None

            if mat_wrap:
                use_mirror = mat_wrap.metallic != 0.0
                use_transparency = mat_wrap.alpha != 1.0

                # XXX Totally empirical conversion, trying to adapt it
                #     (from 1.0 - 0.0 Principled BSDF range to 0.0 - 900.0 OBJ specular exponent range)...
                spec = (1.0 - mat_wrap.roughness) * 30
                spec *= spec
                fw('Ns %.6f\n' % spec)

                # Ambient
                if use_mirror:
                    fw('Ka %.6f %.6f %.6f\n' % (mat_wrap.metallic, mat_wrap.metallic, mat_wrap.metallic))
                else:
                    fw('Ka %.6f %.6f %.6f\n' % (1.0, 1.0, 1.0))
                    fw('Kd %.6f %.6f %.6f\n' % mat_wrap.base_color[:3])  # Diffuse
                    # XXX TODO Find a way to handle tint and diffuse color, in a consistent way with import...
                fw('Ks %.6f %.6f %.6f\n' % (mat_wrap.specular, mat_wrap.specular, mat_wrap.specular))  # Specular
                # Emission, not in original MTL standard but seems pretty common, see T45766.
                fw('Ke 0.0 0.0 0.0\n')           # XXX Not supported by current Principled-based shader.
                fw('Ni %.6f\n' % mat_wrap.ior)   # Refraction index
                fw('d %.6f\n' % mat_wrap.alpha)  # Alpha (obj uses 'd' for dissolve)

                # See http://en.wikipedia.org/wiki/Wavefront_.obj_file for whole list of values...
                # Note that mapping is rather fuzzy sometimes, trying to do our best here.
                if mat_wrap.specular == 0:
                    fw('illum 1\n')  # no specular.
                elif use_mirror:
                    if use_transparency:
                        fw('illum 6\n')  # Reflection, Transparency, Ray trace
                    else:
                        fw('illum 3\n')  # Reflection and Ray trace
                elif use_transparency:
                    fw('illum 9\n')  # 'Glass' transparency and no Ray trace reflection... fuzzy matching, but...
                else:
                    fw('illum 2\n')  # light normally

                #### And now, the image textures...
                image_map = {
                    "map_Kd": "base_color_texture",
                    "map_Ka": None,  # ambient...
                    "map_Ks": "specular_texture",
                    "map_Ns": "roughness_texture",
                    "map_d": "alpha_texture",
                    "map_Tr": None,  # transmission roughness?
                    "map_Bump": "normalmap_texture",
                    "disp": None,  # displacement...
                    "refl": "metallic_texture",
                    "map_Ke": None  # emission...
                }

                for key, mat_wrap_key in sorted(image_map.items()):
                    if mat_wrap_key is None:
                        continue
                    tex_wrap = getattr(mat_wrap, mat_wrap_key, None)
                    if tex_wrap is None:
                        continue
                    image = tex_wrap.image
                    if image is None:
                        continue

                    filepath = io_utils.path_reference(image.filepath, source_dir, dest_dir,
                                                       'AUTO', "", None, image.library)
                    options = []
                    if key == "map_Bump":
                        if mat_wrap.normalmap_strength != 1.0:
                            options.append('-bm %.6f' % mat_wrap.normalmap_strength)
                    if tex_wrap.translation != Vector((0.0, 0.0, 0.0)):
                        options.append('-o %.6f %.6f %.6f' % tex_wrap.translation[:])
                    if tex_wrap.scale != Vector((1.0, 1.0, 1.0)):
                        options.append('-s %.6f %.6f %.6f' % tex_wrap.scale[:])
                    if options:
                        fw('%s %s %s\n' % (key, " ".join(options), repr(filepath)[1:-1]))
                    else:
                        fw('%s %s\n' % (key, repr(filepath)[1:-1]))

            else:
                # Write a dummy material here?
                fw('Ns 500\n')
                fw('Ka 0.8 0.8 0.8\n')  # Ambient
                fw('Kd 0.8 0.8 0.8\n')  # Diffuse
                fw('Ks 0.8 0.8 0.8\n')  # Specular
                fw('d 1\n')             # No alpha
                fw('illum 2\n')         # light normally
