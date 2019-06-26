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
from bl_ui.space_view3d import (
    VIEW3D_PT_shading_lighting,
    VIEW3D_PT_shading_color,
    VIEW3D_PT_shading_options,
)
from bpy.types import (
    Panel,
    UIList,
)


class RenderButtonsPanel:
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "render"
    # COMPAT_ENGINES must be defined in each subclass, external engines can add themselves here

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)


class RENDER_PT_context(Panel):
    bl_space_type = 'PROPERTIES'
    bl_region_type = 'WINDOW'
    bl_context = "render"
    bl_options = {'HIDE_HEADER'}
    bl_label = ""

    @classmethod
    def poll(cls, context):
        return context.scene

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False

        scene = context.scene
        rd = scene.render

        if rd.has_multiple_engines:
            layout.prop(rd, "engine", text="Render Engine")


class RENDER_PT_color_management(RenderButtonsPanel, Panel):
    bl_label = "Color Management"
    bl_options = {'DEFAULT_CLOSED'}
    bl_order = 100
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_EEVEE', 'BLENDER_WORKBENCH'}

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False  # No animation.

        scene = context.scene
        view = scene.view_settings

        flow = layout.grid_flow(row_major=True, columns=0, even_columns=False, even_rows=False, align=True)

        col = flow.column()
        col.prop(scene.display_settings, "display_device")

        col.separator()

        col.prop(view, "view_transform")
        col.prop(view, "look")

        col = flow.column()
        col.prop(view, "exposure")
        col.prop(view, "gamma")

        col.separator()

        col.prop(scene.sequencer_colorspace_settings, "name", text="Sequencer")


class RENDER_PT_color_management_curves(RenderButtonsPanel, Panel):
    bl_label = "Use Curves"
    bl_parent_id = "RENDER_PT_color_management"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_EEVEE', 'BLENDER_WORKBENCH'}

    def draw_header(self, context):

        scene = context.scene
        view = scene.view_settings

        self.layout.prop(view, "use_curve_mapping", text="")

    def draw(self, context):
        layout = self.layout

        scene = context.scene
        view = scene.view_settings

        layout.use_property_split = False
        layout.use_property_decorate = False  # No animation.

        layout.enabled = view.use_curve_mapping

        layout.template_curve_mapping(view, "curve_mapping", type='COLOR', levels=True)


class RENDER_PT_eevee_ambient_occlusion(RenderButtonsPanel, Panel):
    bl_label = "Ambient Occlusion"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw_header(self, context):
        scene = context.scene
        props = scene.eevee
        self.layout.prop(props, "use_gtao", text="")

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        scene = context.scene
        props = scene.eevee

        layout.active = props.use_gtao
        col = layout.column()
        col.prop(props, "gtao_distance")
        col.prop(props, "gtao_factor")
        col.prop(props, "gtao_quality")
        col.prop(props, "use_gtao_bent_normals")
        col.prop(props, "use_gtao_bounce")


class RENDER_PT_eevee_motion_blur(RenderButtonsPanel, Panel):
    bl_label = "Motion Blur"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw_header(self, context):
        scene = context.scene
        props = scene.eevee
        self.layout.prop(props, "use_motion_blur", text="")

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        scene = context.scene
        props = scene.eevee

        layout.active = props.use_motion_blur
        col = layout.column()
        col.prop(props, "motion_blur_samples")
        col.prop(props, "motion_blur_shutter")


class RENDER_PT_eevee_depth_of_field(RenderButtonsPanel, Panel):
    bl_label = "Depth of Field"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        scene = context.scene
        props = scene.eevee

        col = layout.column()
        col.prop(props, "bokeh_max_size")
        # Not supported yet
        # col.prop(props, "bokeh_threshold")


class RENDER_PT_eevee_bloom(RenderButtonsPanel, Panel):
    bl_label = "Bloom"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw_header(self, context):
        scene = context.scene
        props = scene.eevee
        self.layout.prop(props, "use_bloom", text="")

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True

        scene = context.scene
        props = scene.eevee

        layout.active = props.use_bloom
        col = layout.column()
        col.prop(props, "bloom_threshold")
        col.prop(props, "bloom_knee")
        col.prop(props, "bloom_radius")
        col.prop(props, "bloom_color")
        col.prop(props, "bloom_intensity")
        col.prop(props, "bloom_clamp")


class RENDER_PT_eevee_volumetric(RenderButtonsPanel, Panel):
    bl_label = "Volumetric"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True

        scene = context.scene
        props = scene.eevee

        col = layout.column(align=True)
        col.prop(props, "volumetric_start")
        col.prop(props, "volumetric_end")

        col = layout.column()
        col.prop(props, "volumetric_tile_size")
        col.prop(props, "volumetric_samples")
        col.prop(props, "volumetric_sample_distribution", text="Distribution")


class RENDER_PT_eevee_volumetric_lighting(RenderButtonsPanel, Panel):
    bl_label = "Volumetric Lighting"
    bl_parent_id = "RENDER_PT_eevee_volumetric"
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    def draw_header(self, context):
        scene = context.scene
        props = scene.eevee
        self.layout.prop(props, "use_volumetric_lights", text="")

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True

        scene = context.scene
        props = scene.eevee

        layout.active = props.use_volumetric_lights
        layout.prop(props, "volumetric_light_clamp", text="Light Clamping")


class RENDER_PT_eevee_volumetric_shadows(RenderButtonsPanel, Panel):
    bl_label = "Volumetric Shadows"
    bl_parent_id = "RENDER_PT_eevee_volumetric"
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    def draw_header(self, context):
        scene = context.scene
        props = scene.eevee
        self.layout.prop(props, "use_volumetric_shadows", text="")

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True

        scene = context.scene
        props = scene.eevee

        layout.active = props.use_volumetric_shadows
        layout.prop(props, "volumetric_shadow_samples", text="Shadow Samples")


class RENDER_PT_eevee_subsurface_scattering(RenderButtonsPanel, Panel):
    bl_label = "Subsurface Scattering"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True

        scene = context.scene
        props = scene.eevee

        col = layout.column()
        col.prop(props, "sss_samples")
        col.prop(props, "sss_jitter_threshold")
        col.prop(props, "use_sss_separate_albedo")


class RENDER_PT_eevee_screen_space_reflections(RenderButtonsPanel, Panel):
    bl_label = "Screen Space Reflections"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw_header(self, context):
        scene = context.scene
        props = scene.eevee
        self.layout.prop(props, "use_ssr", text="")

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True

        scene = context.scene
        props = scene.eevee

        col = layout.column()
        col.active = props.use_ssr
        col.prop(props, "use_ssr_refraction", text="Refraction")
        col.prop(props, "use_ssr_halfres")
        col.prop(props, "ssr_quality")
        col.prop(props, "ssr_max_roughness")
        col.prop(props, "ssr_thickness")
        col.prop(props, "ssr_border_fade")
        col.prop(props, "ssr_firefly_fac")


class RENDER_PT_eevee_shadows(RenderButtonsPanel, Panel):
    bl_label = "Shadows"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True

        scene = context.scene
        props = scene.eevee

        col = layout.column()
        col.prop(props, "shadow_method")
        col.prop(props, "shadow_cube_size", text="Cube Size")
        col.prop(props, "shadow_cascade_size", text="Cascade Size")
        col.prop(props, "use_shadow_high_bitdepth")
        col.prop(props, "use_soft_shadows")
        col.prop(props, "light_threshold")


class RENDER_PT_eevee_sampling(RenderButtonsPanel, Panel):
    bl_label = "Sampling"
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False  # No animation.

        scene = context.scene
        props = scene.eevee

        col = layout.column(align=True)
        col.prop(props, "taa_render_samples", text="Render")
        col.prop(props, "taa_samples", text="Viewport")

        col = layout.column()
        col.prop(props, "use_taa_reprojection")


class RENDER_PT_eevee_indirect_lighting(RenderButtonsPanel, Panel):
    bl_label = "Indirect Lighting"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False  # No animation.

        scene = context.scene
        props = scene.eevee

        col = layout.column()
        col.operator("scene.light_cache_bake", text="Bake Indirect Lighting", icon='RENDER_STILL')
        col.operator("scene.light_cache_bake", text="Bake Cubemap Only", icon='LIGHTPROBE_CUBEMAP').subset = 'CUBEMAPS'
        col.operator("scene.light_cache_free", text="Delete Lighting Cache")

        cache_info = scene.eevee.gi_cache_info
        if cache_info:
            col.label(text=cache_info)

        col.prop(props, "gi_auto_bake")

        col.prop(props, "gi_diffuse_bounces")
        col.prop(props, "gi_cubemap_resolution")
        col.prop(props, "gi_visibility_resolution", text="Diffuse Occlusion")
        col.prop(props, "gi_irradiance_smoothing")
        col.prop(props, "gi_glossy_clamp")
        col.prop(props, "gi_filter_quality")


class RENDER_PT_eevee_indirect_lighting_display(RenderButtonsPanel, Panel):
    bl_label = "Display"
    bl_parent_id = "RENDER_PT_eevee_indirect_lighting"
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False  # No animation.

        scene = context.scene
        props = scene.eevee

        row = layout.row(align=True)
        row.prop(props, "gi_cubemap_display_size", text="Cubemap Size")
        row.prop(props, "gi_show_cubemaps", text="", toggle=True)

        row = layout.row(align=True)
        row.prop(props, "gi_irradiance_display_size", text="Irradiance Size")
        row.prop(props, "gi_show_irradiance", text="", toggle=True)


class RENDER_PT_eevee_film(RenderButtonsPanel, Panel):
    bl_label = "Film"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True

        scene = context.scene
        rd = scene.render

        col = layout.column()
        col.prop(rd, "filter_size")
        col.prop(rd, "film_transparent", text="Transparent")


class RENDER_PT_eevee_film_overscan(RenderButtonsPanel, Panel):
    bl_label = "Overscan"
    bl_parent_id = "RENDER_PT_eevee_film"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    def draw_header(self, context):

        scene = context.scene
        props = scene.eevee

        self.layout.prop(props, "use_overscan", text="")

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        scene = context.scene
        props = scene.eevee

        layout.active = props.use_overscan
        layout.prop(props, "overscan_size", text="Size")


class RENDER_PT_eevee_hair(RenderButtonsPanel, Panel):
    bl_label = "Hair"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        rd = scene.render

        layout.use_property_split = True

        layout.prop(rd, "hair_type", expand=True)
        layout.prop(rd, "hair_subdiv")


class RENDER_PT_opengl_sampling(RenderButtonsPanel, Panel):
    bl_label = "Sampling"
    COMPAT_ENGINES = {'BLENDER_WORKBENCH'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False  # No animation.

        scene = context.scene
        props = scene.display

        col = layout.column()
        col.prop(props, "render_aa", text="Render")
        col.prop(props, "viewport_aa", text="Viewport Render")


class RENDER_PT_opengl_film(RenderButtonsPanel, Panel):
    bl_label = "Film"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_WORKBENCH'}

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False  # No animation.

        rd = context.scene.render
        layout.prop(rd, "film_transparent", text="Transparent")


class RENDER_PT_opengl_lighting(RenderButtonsPanel, Panel):
    bl_label = "Lighting"
    COMPAT_ENGINES = {'BLENDER_WORKBENCH'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        VIEW3D_PT_shading_lighting.draw(self, context)


class RENDER_PT_opengl_color(RenderButtonsPanel, Panel):
    bl_label = "Color"
    COMPAT_ENGINES = {'BLENDER_WORKBENCH'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        VIEW3D_PT_shading_color._draw_color_type(self, context)


class RENDER_PT_opengl_options(RenderButtonsPanel, Panel):
    bl_label = "Options"
    COMPAT_ENGINES = {'BLENDER_WORKBENCH'}

    @classmethod
    def poll(cls, context):
        return (context.engine in cls.COMPAT_ENGINES)

    def draw(self, context):
        VIEW3D_PT_shading_options.draw(self, context)


class RENDER_PT_simplify(RenderButtonsPanel, Panel):
    bl_label = "Simplify"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_EEVEE', 'BLENDER_WORKBENCH'}

    def draw_header(self, context):
        rd = context.scene.render
        self.layout.prop(rd, "use_simplify", text="")

    def draw(self, context):
        pass


class RENDER_PT_simplify_viewport(RenderButtonsPanel, Panel):
    bl_label = "Viewport"
    bl_parent_id = "RENDER_PT_simplify"
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_EEVEE', 'BLENDER_WORKBENCH'}

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True

        rd = context.scene.render

        layout.active = rd.use_simplify

        flow = layout.grid_flow(row_major=True, columns=0, even_columns=False, even_rows=False, align=True)

        col = flow.column()
        col.prop(rd, "simplify_subdivision", text="Max Subdivision")

        col = flow.column()
        col.prop(rd, "simplify_child_particles", text="Max Child Particles")

        col = flow.column()
        col.prop(rd, "use_simplify_smoke_highres", text="High-resolution Smoke")


class RENDER_PT_simplify_render(RenderButtonsPanel, Panel):
    bl_label = "Render"
    bl_parent_id = "RENDER_PT_simplify"
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_EEVEE', 'BLENDER_WORKBENCH'}

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True

        rd = context.scene.render

        layout.active = rd.use_simplify

        flow = layout.grid_flow(row_major=True, columns=0, even_columns=False, even_rows=False, align=True)

        col = flow.column()
        col.prop(rd, "simplify_subdivision_render", text="Max Subdivision")

        col = flow.column()
        col.prop(rd, "simplify_child_particles_render", text="Max Child Particles")


class RENDER_PT_simplify_greasepencil(RenderButtonsPanel, Panel):
    bl_label = "Grease Pencil"
    bl_parent_id = "RENDER_PT_simplify"
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_GAME', 'BLENDER_CLAY', 'BLENDER_EEVEE'}
    bl_options = {'DEFAULT_CLOSED'}

    def draw_header(self, context):
        rd = context.scene.render
        self.layout.prop(rd, "simplify_gpencil", text="")

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False

        rd = context.scene.render

        layout.active = rd.simplify_gpencil

        col = layout.column()
        col.prop(rd, "simplify_gpencil_onplay", text="Playback Only")
        col.prop(rd, "simplify_gpencil_view_modifier", text="Modifiers")
        col.prop(rd, "simplify_gpencil_shader_fx", text="ShaderFX")
        col.prop(rd, "simplify_gpencil_blend", text="Layers Blending")

        col.prop(rd, "simplify_gpencil_view_fill")
        sub = col.column()
        sub.active = rd.simplify_gpencil_view_fill
        sub.prop(rd, "simplify_gpencil_remove_lines", text="Lines")


class LANPR_linesets(UIList):
    def draw_item(self, context, layout, data, item, icon, active_data, active_propname, index):
        lineset = item
        if self.layout_type in {'DEFAULT', 'COMPACT'}:
            split = layout.split(factor=0.4)
            t=''
            if not lineset.use_multiple_levels:
                t='Level %d'%lineset.qi_begin
            else:
                t='Level %d - %d'%(lineset.qi_begin,lineset.qi_end)
            split.label(text=t)
            row = split.row(align=True)
            row.prop(lineset, "color", text="", icon_value=icon)
            row.prop(lineset, "thickness", text="", icon_value=icon)
        elif self.layout_type == 'GRID':
            layout.alignment = 'CENTER'
            layout.label("", icon_value=icon)

class RENDER_PT_lanpr(RenderButtonsPanel, Panel):
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_LANPR', 'BLENDER_OPENGL', 'BLENDER_EEVEE'}
    bl_label = "LANPR"
    bl_options = {'DEFAULT_CLOSED'}
    
    @classmethod
    def poll(cls, context):
        return True

    def draw_header(self, context):
        self.layout.prop(context.scene.lanpr, "enabled", text="")

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        lanpr = scene.lanpr
        active_layer = lanpr.layers.active_layer 
        mode = lanpr.master_mode


        
        if scene.render.engine!='BLENDER_LANPR':
            layout.prop(lanpr, "master_mode", expand=True) 
        else:
            layout.label(text='Only Software mode result is used to generate GP stroke.')

        if mode == "SOFTWARE":
            row=layout.row(align=True)
            row.prop(lanpr,'auto_update',toggle=True,text='Auto')
            if not lanpr.auto_update:
                row.operator("scene.lanpr_calculate", icon='RENDER_STILL', text='Update')

        if mode == "DPIX" or mode == "SOFTWARE":
            
            layout.prop(lanpr, "background_color")
            layout.prop(lanpr, "crease_threshold")

            if lanpr.master_mode == "SOFTWARE":
                row = layout.row()
                row.prop(lanpr,"enable_intersections", text = "Intersection Lines")
                row.prop(lanpr,"enable_chaining", text = "Chaining")
                
                split = layout.split(factor=0.4)
                col = split.column()
                col.label(text="Line Layers:")
                col = split.column()
                col.operator("scene.lanpr_auto_create_line_layer", text = "Default", icon = "ADD")
                row=layout.row()
                row.template_list("LANPR_linesets", "", lanpr, "layers", lanpr.layers, "active_layer_index", rows=4)
                col=row.column(align=True)
                if active_layer:
                    col.operator("scene.lanpr_add_line_layer", icon="ADD", text='')
                    col.operator("scene.lanpr_delete_line_layer", icon="REMOVE", text='')
                    col.separator()
                    col.operator("scene.lanpr_move_line_layer",icon='TRIA_UP', text='').direction = "UP"
                    col.operator("scene.lanpr_move_line_layer",icon='TRIA_DOWN', text='').direction = "DOWN"
                    col.separator()
                    col.operator("scene.lanpr_rebuild_all_commands",icon="FILE_REFRESH", text='')
                else:
                    col.operator("scene.lanpr_add_line_layer", icon="ADD", text='')
        else:
            layout.label(text="Vectorization:")
            layout.prop(lanpr, "enable_vector_trace", expand = True)


class RENDER_PT_lanpr_line_types(RenderButtonsPanel, Panel):
    bl_label = "Layer Settings"
    bl_parent_id = "RENDER_PT_lanpr"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_LANPR', 'BLENDER_OPENGL', 'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        scene = context.scene
        lanpr = scene.lanpr
        active_layer = lanpr.layers.active_layer
        return active_layer and lanpr.master_mode != "SNAKE"

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        lanpr = scene.lanpr
        active_layer = lanpr.layers.active_layer
        if active_layer and lanpr.master_mode == "DPIX":
            active_layer = lanpr.layers[0]

        if lanpr.master_mode == "SOFTWARE":
            row = layout.row(align=True)
            row.prop(active_layer, "use_multiple_levels", icon='GP_MULTIFRAME_EDITING', icon_only=True)
            row.prop(active_layer, "qi_begin", text='Level')
            if active_layer.use_multiple_levels:
                row.prop(active_layer, "qi_end", text='To')
        
        row = layout.row(align=True)
        row.prop(active_layer,"use_same_style")
        if active_layer.use_same_style:
            row = layout.row(align=True)
            row.prop(active_layer, "color", text="")
            row.prop(active_layer, "thickness", text="")
            row = layout.row(align=True)
            row.prop(active_layer, "enable_contour", text="Contour", toggle=True)
            row.prop(active_layer, "enable_crease", text="Crease", toggle=True)
            row.prop(active_layer, "enable_edge_mark", text="Mark", toggle=True)
            row.prop(active_layer, "enable_material_seperate", text="Material", toggle=True)
            row.prop(active_layer, "enable_intersection", text="Intersection", toggle=True)
        else:
            row.prop(active_layer, "thickness", text="")
            split = layout.split(factor=0.3)
            col = split.column()
            col.prop(active_layer, "enable_contour", text="Contour", toggle=True)
            col.prop(active_layer, "enable_crease", text="Crease", toggle=True)
            col.prop(active_layer, "enable_edge_mark", text="Mark", toggle=True)
            col.prop(active_layer, "enable_material_seperate", text="Material", toggle=True)
            col.prop(active_layer, "enable_intersection", text="Intersection", toggle=True)
            col = split.column()
            row = col.row(align = True)
            #row.enabled = active_layer.enable_contour this is always enabled now
            row.prop(active_layer, "contour_color", text="")
            row.prop(active_layer, "thickness_contour", text="", slider=True)
            row = col.row(align = True)
            row.enabled = active_layer.enable_crease
            row.prop(active_layer, "crease_color", text="")
            row.prop(active_layer, "thickness_crease", text="", slider=True)
            row = col.row(align = True)
            row.enabled = active_layer.enable_edge_mark
            row.prop(active_layer, "edge_mark_color", text="")
            row.prop(active_layer, "thickness_edge_mark", text="", slider=True)
            row = col.row(align = True)
            row.enabled = active_layer.enable_material_seperate
            row.prop(active_layer, "material_color", text="")
            row.prop(active_layer, "thickness_material", text="", slider=True)
            row = col.row(align = True)
            if lanpr.enable_intersections:
                row.enabled = active_layer.enable_intersection
                row.prop(active_layer, "intersection_color", text="")
                row.prop(active_layer, "thickness_intersection", text="", slider=True)
            else:
                row.label(text= "Intersection Calculation Disabled")

        if lanpr.master_mode == "DPIX" and active_layer.enable_intersection:
            row = col.row(align = True)
            row.prop(lanpr,"enable_intersections", toggle = True, text = "Enable")
            if lanpr.enable_intersections:
                row.operator("scene.lanpr_calculate", text= "Recalculate")

class RENDER_PT_lanpr_line_components(RenderButtonsPanel, Panel):
    bl_label = "Including"
    bl_parent_id = "RENDER_PT_lanpr"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_LANPR', 'BLENDER_OPENGL', 'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        scene = context.scene
        lanpr = scene.lanpr
        active_layer = lanpr.layers.active_layer
        return active_layer and lanpr.master_mode == "SOFTWARE" and not lanpr.enable_chaining

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        lanpr = scene.lanpr
        active_layer = lanpr.layers.active_layer

        layout.operator("scene.lanpr_add_line_component")#, icon = "ZOOMIN")
        
        i=0
        for c in active_layer.components:
            split = layout.split(factor=0.85)
            col = split.column()
            sp2 = col.split(factor=0.4)
            cl = sp2.column()
            cl.prop(c,"component_mode", text = "")
            cl = sp2.column()
            if c.component_mode == "OBJECT":
                cl.prop(c,"object_select", text = "")
            elif c.component_mode == "MATERIAL":
                cl.prop(c,"material_select", text = "")
            elif c.component_mode == "COLLECTION":
                cl.prop(c,"collection_select", text = "")
            col = split.column()
            col.operator("scene.lanpr_delete_line_component", text="").index=i
            i=i+1


class RENDER_PT_lanpr_line_effects(RenderButtonsPanel, Panel):
    bl_label = "Effects"
    bl_parent_id = "RENDER_PT_lanpr"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_LANPR', 'BLENDER_OPENGL', 'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        scene = context.scene
        lanpr = scene.lanpr
        return lanpr.master_mode == "DPIX" or lanpr.master_mode == "SOFTWARE"

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        lanpr = scene.lanpr
        active_layer = lanpr.layers.active_layer
        
        if lanpr.master_mode == "DPIX":
            row = layout.row(align = True)
            row.prop(lanpr, "crease_threshold")
            row.prop(lanpr, "crease_fade_threshold")
            row = layout.row(align = True)
            row.prop(lanpr, "depth_width_influence")
            row.prop(lanpr, "depth_width_curve")
            row = layout.row(align = True)
            row.prop(lanpr, "depth_alpha_influence")
            row.prop(lanpr, "depth_alpha_curve")
        elif lanpr.master_mode == "SOFTWARE":
            if active_layer:
                layout.label(text= "Normal based line weight:")
                layout.prop(active_layer,"normal_mode", expand = True)
                if active_layer.normal_mode != "DISABLED":
                    layout.prop(active_layer,"normal_control_object")
                    layout.prop(active_layer,"normal_effect_inverse", toggle = True)
                    layout.prop(active_layer,"normal_ramp_begin")
                    layout.prop(active_layer,"normal_ramp_end")
                    layout.prop(active_layer,"normal_thickness_begin", slider=True)
                    layout.prop(active_layer,"normal_thickness_end", slider=True)


class RENDER_PT_lanpr_snake_sobel_parameters(RenderButtonsPanel, Panel):
    bl_label = "Sobel Parameters"
    bl_parent_id = "RENDER_PT_lanpr"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_LANPR', 'BLENDER_OPENGL', 'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        scene = context.scene
        lanpr = scene.lanpr
        return lanpr.master_mode == "SNAKE"

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        lanpr = scene.lanpr
        layout.prop(lanpr, "depth_clamp")
        layout.prop(lanpr, "depth_strength")
        layout.prop(lanpr, "normal_clamp")
        layout.prop(lanpr, "normal_strength")
        if lanpr.enable_vector_trace == "DISABLED":
            layout.prop(lanpr, "display_thinning_result")

class RENDER_PT_lanpr_snake_settings(RenderButtonsPanel, Panel):
    bl_label = "Snake Settings"
    bl_parent_id = "RENDER_PT_lanpr"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_LANPR', 'BLENDER_OPENGL', 'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        scene = context.scene
        lanpr = scene.lanpr
        return lanpr.master_mode == "SNAKE" and lanpr.enable_vector_trace == "ENABLED"

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        lanpr = scene.lanpr

        split = layout.split()
        col = split.column()
        col.prop(lanpr, "background_color")
        col = split.column()
        col.prop(lanpr, "line_color")
        
        layout.prop(lanpr, "line_thickness")

        split = layout.split()
        col = split.column()
        col.prop(lanpr, "depth_width_influence")
        col.prop(lanpr, "depth_alpha_influence")
        col = split.column()
        col.prop(lanpr, "depth_width_curve")
        col.prop(lanpr, "depth_alpha_curve")
        
        layout.label(text="Taper:")
        layout.prop(lanpr, "use_same_taper", expand = True)
        if lanpr.use_same_taper == "DISABLED":
            split = layout.split()
            col = split.column(align = True)
            col.label(text="Left:")
            col.prop(lanpr,"taper_left_distance")
            col.prop(lanpr,"taper_left_strength")
            col = split.column(align = True)
            col.label(text="Right:")
            col.prop(lanpr,"taper_right_distance")
            col.prop(lanpr,"taper_right_strength")
        else:
            split = layout.split()
            col = split.column(align = True)
            col.prop(lanpr,"taper_left_distance")
            col.prop(lanpr,"taper_left_strength") 

        layout.label(text="Tip Extend:")
        layout.prop(lanpr, "enable_tip_extend",  expand = True)
        if lanpr.enable_tip_extend == "ENABLED":
            layout.label(text="---INOP---")
            layout.prop(lanpr,"extend_length")

class RENDER_PT_lanpr_software_chain_styles(RenderButtonsPanel, Panel):
    bl_label = "Chaining Options"
    bl_parent_id = "RENDER_PT_lanpr"
    bl_options = {'DEFAULT_CLOSED'}
    COMPAT_ENGINES = {'BLENDER_RENDER', 'BLENDER_LANPR', 'BLENDER_OPENGL', 'BLENDER_EEVEE'}

    @classmethod
    def poll(cls, context):
        scene = context.scene
        lanpr = scene.lanpr
        return lanpr.master_mode == "SOFTWARE" and lanpr.enable_chaining

    def draw(self, context):
        layout = self.layout
        scene = context.scene
        lanpr = scene.lanpr
        
        layout.label(text="Threasholds:")
        row = layout.row(align=True)
        row.prop(lanpr, "chaining_geometry_threshold", text="Geometry")
        row.prop(lanpr, "chaining_image_threshold", text="Image")

        layout.label(text="Taper:")
        layout.prop(lanpr, "use_same_taper", expand = True)
        if lanpr.use_same_taper == "DISABLED":
            split = layout.split()
            col = split.column(align = True)
            col.label(text="Left:")
            col.prop(lanpr,"taper_left_distance")
            col.prop(lanpr,"taper_left_strength")
            col = split.column(align = True)
            col.label(text="Right:")
            col.prop(lanpr,"taper_right_distance")
            col.prop(lanpr,"taper_right_strength")
        else:
            split = layout.split()
            col = split.column(align = True)
            col.prop(lanpr,"taper_left_distance")
            col.prop(lanpr,"taper_left_strength") 


classes = (
    RENDER_PT_context,
    RENDER_PT_eevee_sampling,
    RENDER_PT_eevee_ambient_occlusion,
    RENDER_PT_eevee_bloom,
    RENDER_PT_eevee_depth_of_field,
    RENDER_PT_eevee_subsurface_scattering,
    RENDER_PT_eevee_screen_space_reflections,
    RENDER_PT_eevee_motion_blur,
    RENDER_PT_eevee_volumetric,
    RENDER_PT_eevee_volumetric_lighting,
    RENDER_PT_eevee_volumetric_shadows,
    RENDER_PT_eevee_hair,
    RENDER_PT_eevee_shadows,
    RENDER_PT_eevee_indirect_lighting,
    RENDER_PT_eevee_indirect_lighting_display,
    RENDER_PT_eevee_film,
    RENDER_PT_eevee_film_overscan,
    RENDER_PT_opengl_sampling,
    RENDER_PT_opengl_lighting,
    RENDER_PT_opengl_color,
    RENDER_PT_opengl_options,
    RENDER_PT_opengl_film,
    RENDER_PT_color_management,
    RENDER_PT_color_management_curves,
    RENDER_PT_simplify,
    RENDER_PT_simplify_viewport,
    RENDER_PT_simplify_render,
    RENDER_PT_simplify_greasepencil,
    RENDER_PT_lanpr,
    RENDER_PT_lanpr_line_types,
    RENDER_PT_lanpr_line_components,
    RENDER_PT_lanpr_line_effects,
    RENDER_PT_lanpr_snake_sobel_parameters,
    RENDER_PT_lanpr_snake_settings,
    RENDER_PT_lanpr_software_chain_styles,
    LANPR_linesets,
)

if __name__ == "__main__":  # only for live edit.
    from bpy.utils import register_class
    for cls in classes:
        register_class(cls)
