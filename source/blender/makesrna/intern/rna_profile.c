/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/** \file
 * \ingroup RNA
 */

#include <stdlib.h>
#include <stdio.h>

#include "DNA_profilewidget_types.h"
#include "DNA_texture_types.h"

#include "BLI_utildefines.h"

#include "RNA_define.h"
#include "rna_internal.h"

#include "WM_api.h"
#include "WM_types.h"

#ifdef RNA_RUNTIME

#  include "RNA_access.h"

#  include "DNA_image_types.h"
#  include "DNA_material_types.h"
#  include "DNA_movieclip_types.h"
#  include "DNA_node_types.h"
#  include "DNA_object_types.h"
#  include "DNA_particle_types.h"
#  include "DNA_sequence_types.h"

#  include "MEM_guardedalloc.h"

#  include "BKE_colorband.h"
#  include "BKE_profile_widget.h"
#  include "BKE_image.h"
#  include "BKE_movieclip.h"
#  include "BKE_node.h"
#  include "BKE_sequencer.h"
#  include "BKE_linestyle.h"

#  include "DEG_depsgraph.h"

#  include "ED_node.h"

#  include "IMB_colormanagement.h"
#  include "IMB_imbuf.h"

static void rna_ProfileWidget_clip_set(PointerRNA *ptr, bool value)
{
  ProfileWidget *prwdgt = (ProfileWidget *)ptr->data;

  if (value) {
    prwdgt->flag |= PROF_USE_CLIP;
  }
  else {
    prwdgt->flag &= ~PROF_USE_CLIP ;
  }

  BKE_profilewidget_changed(prwdgt, false);
}

static void rna_ProfileWidget_sample_straight_set(PointerRNA *ptr, bool value)
{
  ProfileWidget *prwdgt = (ProfileWidget *)ptr->data;

  if (value) {
    prwdgt->flag |= PROF_SAMPLE_STRAIGHT_EDGES;
  }
  else {
    prwdgt->flag &= ~PROF_SAMPLE_STRAIGHT_EDGES;
  }

  BKE_profilewidget_changed(prwdgt, false);
}

static void rna_ProfileWidget_remove_point(ProfileWidget *prwdgt,
                                           ReportList *reports,
                                           PointerRNA *point_ptr)
{
  ProfilePoint *point = point_ptr->data;
  if (BKE_profilewidget_remove_point(prwdgt, point) == false) {
    BKE_report(reports, RPT_ERROR, "Unable to remove path point");
    return;
  }

  RNA_POINTER_INVALIDATE(point_ptr);
}

static void rna_ProfileWidget_evaluate(struct ProfileWidget *prwdgt,
                                       ReportList *reports,
                                       float length_portion,
                                       float *location)
{
  if (!prwdgt->table) {
    BKE_report(reports, RPT_ERROR,"ProfileWidget table not initialized, call initialize()");
  }
  BKE_profilewidget_evaluate_length_portion(prwdgt, length_portion, &location[0], &location[1]);
}

static void rna_ProfileWidget_initialize(struct ProfileWidget *prwdgt, int totsegments)
{
  BKE_profilewidget_initialize(prwdgt, (short)totsegments);
}

static void rna_ProfileWidget_changed(struct ProfileWidget *prwdgt)
{
  BKE_profilewidget_changed(prwdgt, false);
}

#else

static void rna_def_profilepoint(BlenderRNA *brna)
{
  StructRNA *srna;
  PropertyRNA *prop;

  static const EnumPropertyItem prop_handle_type_items[] = {
      {PROF_HANDLE_AUTO, "AUTO_CLAMPED", 0, "Auto Clamped Handle", ""},
      {PROF_HANDLE_VECTOR, "VECTOR", 0, "Vector Handle", ""},
      {0, NULL, 0, NULL, NULL},
  };

  srna = RNA_def_struct(brna, "ProfilePoint", NULL);
  RNA_def_struct_ui_text(srna, "ProfilePoint", "Point of a path used to define a profile");

  prop = RNA_def_property(srna, "location", PROP_FLOAT, PROP_XYZ);
  RNA_def_property_float_sdna(prop, NULL, "x");
  RNA_def_property_array(prop, 2);
  RNA_def_property_ui_text(prop, "Location", "X/Y coordinates of the path point");

  prop = RNA_def_property(srna, "handle_type", PROP_ENUM, PROP_NONE);
  RNA_def_property_enum_bitflag_sdna(prop, NULL, "flag");
  RNA_def_property_enum_items(prop, prop_handle_type_items);
  RNA_def_property_ui_text(prop, "Handle Type",
                           "Path interpolation at this point: Bezier or vector");

  prop = RNA_def_property(srna, "select", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, NULL, "flag", PROF_SELECT);
  RNA_def_property_ui_text(prop, "Select", "Selection state of the path point");
}

static void rna_def_profilewidget_points_api(BlenderRNA *brna, PropertyRNA *cprop)
{
  StructRNA *srna;
  PropertyRNA *parm;
  FunctionRNA *func;

  RNA_def_property_srna(cprop, "ProfilePoints");
  srna = RNA_def_struct(brna, "ProfilePoints", NULL);
  RNA_def_struct_sdna(srna, "ProfileWidget");
  RNA_def_struct_ui_text(srna, "Profile Point", "Collection of Profile Points");

  func = RNA_def_function(srna, "add", "BKE_profilewidget_insert");
  RNA_def_function_ui_description(func, "Add point to the profile widget");
  parm = RNA_def_float(func, "x", 0.0f, -FLT_MAX, FLT_MAX, "X Position",
                       "X Position for new point", -FLT_MAX, FLT_MAX);
  RNA_def_parameter_flags(parm, 0, PARM_REQUIRED);
  parm = RNA_def_float(func, "y", 0.0f, -FLT_MAX, FLT_MAX, "Y Position",
                       "Y Position for new point", -FLT_MAX, FLT_MAX);
  RNA_def_parameter_flags(parm, 0, PARM_REQUIRED);
  parm = RNA_def_pointer(func, "point", "ProfilePoint", "", "New point");
  RNA_def_function_return(func, parm);

  func = RNA_def_function(srna, "remove", "rna_ProfileWidget_remove_point");
  RNA_def_function_flag(func, FUNC_USE_REPORTS);
  RNA_def_function_ui_description(func, "Delete point from profile widget");
  parm = RNA_def_pointer(func, "point", "ProfilePoint", "", "Point to remove");
  RNA_def_parameter_flags(parm, PROP_NEVER_NULL, PARM_REQUIRED | PARM_RNAPTR);
  RNA_def_parameter_clear_flags(parm, PROP_THICK_WRAP, 0);
}


static void rna_def_profilewidget(BlenderRNA *brna)
{
  StructRNA *srna;
  PropertyRNA *prop;
  PropertyRNA *parm;
  FunctionRNA *func;

  static const EnumPropertyItem rna_enum_profilewidget_preset_items[] = {
      {PROF_PRESET_LINE, "LINE", 0, "Line", "Default"},
      {PROF_PRESET_SUPPORTS, "SUPPORTS", 0, "Support Loops",
       "Loops on either side of the profile"},
      {PROF_PRESET_EXAMPLE1, "EXAMPLE1", 0, "Molding Example", "An example use case"},
      {0, NULL, 0, NULL, NULL},
  };

  srna = RNA_def_struct(brna, "ProfileWidget", NULL);
  RNA_def_struct_ui_text(srna,"ProfileWidget", "Profile Path editor used to build a profile path");

  prop = RNA_def_property(srna, "preset", PROP_ENUM, PROP_NONE);
  RNA_def_property_enum_sdna(prop, NULL, "preset");
  RNA_def_property_enum_items(prop, rna_enum_profilewidget_preset_items);
  RNA_def_property_ui_text(prop, "Preset", "");

  prop = RNA_def_property(srna, "use_clip", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, NULL, "flag", PROF_USE_CLIP);
  RNA_def_property_ui_text(prop, "Clip", "Force the path view to fit a defined boundary");
  RNA_def_property_boolean_funcs(prop, NULL, "rna_ProfileWidget_clip_set");

  prop = RNA_def_property(srna, "sample_straight_edges", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_boolean_sdna(prop, NULL, "flag", PROF_SAMPLE_STRAIGHT_EDGES);
  RNA_def_property_ui_text(prop, "Sample Straight Edges", "Sample edges with vector handles");
  RNA_def_property_boolean_funcs(prop, NULL, "rna_ProfileWidget_sample_straight_set");

  func = RNA_def_function(srna, "update", "rna_ProfileWidget_changed");
  RNA_def_function_ui_description(func, "Update profile widget after making changes");

  func = RNA_def_function(srna, "initialize", "rna_ProfileWidget_initialize");
  parm = RNA_def_int(func, "totsegments", 1, 1, 1000, "", "The number of segment values to"
                     " initialize the segments table with", 1, 100);
  RNA_def_parameter_flags(parm, PROP_NEVER_NULL, PARM_REQUIRED);
  RNA_def_function_ui_description(func, "Set the number of display segments and fill tables");

  prop = RNA_def_property(srna, "points", PROP_COLLECTION, PROP_NONE);
  RNA_def_property_collection_sdna(prop, NULL, "path", "totpoint");
  RNA_def_property_struct_type(prop, "ProfilePoint");
  RNA_def_property_ui_text(prop, "Points", "Profile widget control points");
  rna_def_profilewidget_points_api(brna, prop);

  prop = RNA_def_property(srna, "totsegments", PROP_INT, PROP_NONE);
  RNA_def_property_int_sdna(prop, NULL, "totsegments");
  RNA_def_property_ui_text(prop, "Number of Segments", "The number of segments to sample from"
                           " control points");

  prop = RNA_def_property(srna, "segments", PROP_COLLECTION, PROP_NONE);
  RNA_def_property_collection_sdna(prop, NULL, "segments", "totsegments");
  RNA_def_property_struct_type(prop, "ProfilePoint");
  RNA_def_property_ui_text(prop, "Segments", "Segments sampled from control points");

  func = RNA_def_function(srna, "evaluate", "rna_ProfileWidget_evaluate");
  RNA_def_function_flag(func, FUNC_USE_REPORTS);
  RNA_def_function_ui_description(func, "Evaluate the at the given portion of the path length");
  parm = RNA_def_float(func, "length_portion", 0.0f, 0.0f, 1.0f, "Length Portion",
                       "Portion of the path length to travel before evaluation", 0.0f, 1.0f);
  RNA_def_parameter_flags(parm, 0, PARM_REQUIRED);
  parm = RNA_def_float_vector(func, "location", 2, NULL, -100.0f, 100.0f, "Location",
                              "The location at the given portion of the profile", -100.0f, 100.0f);
  RNA_def_function_output(func, parm);
}

void RNA_def_profile(BlenderRNA *brna)
{
  rna_def_profilepoint(brna);
  rna_def_profilewidget(brna);
}

#endif
