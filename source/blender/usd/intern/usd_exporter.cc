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
 * \ingroup USD
 */

#include "usd_exporter.h"
#include "usd_writer_mesh.h"
#include "usd_writer_transform.h"

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/tokens.h>

extern "C" {
#include "BKE_anim.h"
#include "BKE_mesh_runtime.h"
#include "BKE_scene.h"

#include "BLI_iterator.h"

#include "DNA_layer_types.h"
}

USDExporter::USDExporter(const char *filename, ExportSettings &settings)
    : m_settings(settings), m_filename(filename)
{
}

USDExporter::~USDExporter()
{
}

void USDExporter::operator()(float &r_progress, bool &r_was_canceled)
{
  static const pxr::SdfPath root("/");
  Timer timer_("Writing to USD");

  r_progress = 0.0;
  r_was_canceled = false;

  // Create a stage with the Only Correct up-axis.
  m_stage = pxr::UsdStage::CreateNew(m_filename);
  m_stage->SetMetadata(pxr::UsdGeomTokens->upAxis, pxr::VtValue(pxr::UsdGeomTokens->z));

  for (Base *base = static_cast<Base *>(m_settings.view_layer->object_bases.first); base;
       base = base->next) {
    Object *ob = base->object;
    Object *ob_eval = DEG_get_evaluated_object(m_settings.depsgraph, ob);

    export_or_queue(ob_eval, NULL, root);
  }

  // DEG_OBJECT_ITER_BEGIN (m_settings.depsgraph,
  //                        ob_eval,
  //                        DEG_ITER_OBJECT_FLAG_LINKED_DIRECTLY |
  //                            DEG_ITER_OBJECT_FLAG_LINKED_VIA_SET | DEG_ITER_OBJECT_FLAG_VISIBLE
  //                            | DEG_ITER_OBJECT_FLAG_DUPLI) {
  //   export_or_queue(ob_eval, data_.dupli_parent);
  // }
  // DEG_OBJECT_ITER_END;

  printf("====== Dealing with deferred exports\n");
  // for (DeferredExportSet::iterator it : deferred_exports) {

  // }

  printf("Impossible exports: %lu \n", deferred_exports.size());

  while (!deferred_exports.empty()) {
    DeferredExportSet::iterator it = deferred_exports.begin();

    Object *missing_ob = it->first;
    Object *missing_ob_eval = DEG_get_evaluated_object(m_settings.depsgraph, missing_ob);

    printf("   - %s  referred by %lu objects\n", missing_ob->id.name, it->second.size());
    printf(
        "     type = %d  addr = %p   orig = %p   eval = %p  orig->parent = %p (%s at path %s)\n",
        missing_ob->type,
        missing_ob,
        DEG_get_original_object(missing_ob),
        missing_ob_eval,
        missing_ob->parent,
        missing_ob->parent ? missing_ob->parent->id.name : "-null-",
        usd_object_paths[missing_ob->parent].GetString().c_str());

    for (const auto &deferred_ctx : it->second) {
      printf("     - %s (%p) instanced by %s (%p)\n",
             deferred_ctx.ob_eval->id.name,
             deferred_ctx.ob_eval,
             deferred_ctx.instanced_by ? deferred_ctx.instanced_by->id.name : "-none-",
             deferred_ctx.instanced_by);
    }

    deferred_exports.erase(it);

    // export_or_queue(missing_ob_eval, NULL);
  }

  m_stage->GetRootLayer()->Save();

  r_progress = 1.0;
}

void USDExporter::export_or_queue(Object *ob_eval,
                                  Object *instanced_by,
                                  const pxr::SdfPath &anchor)
{
  Object *missing_ob = NULL;

  pxr::SdfPath parent_path = parent_usd_path(ob_eval, instanced_by, anchor, &missing_ob);
  USDExporterContext ctx = {m_stage, parent_path, ob_eval, instanced_by};

  if (parent_path.IsEmpty()) {
    // Object path couldn't be found, so queue this object until it is ok.
    Object *missing_orig = DEG_get_original_object(missing_ob);
    deferred_exports[missing_orig].push_back(ctx);
    printf("Deferred, %lu exports now waiting for this object %p\n",
           deferred_exports[missing_orig].size(),
           missing_orig);
    return;
  }

  printf("              XForm %s (%p) inside parent %s anchored at %s\n",
         ob_eval->id.name,
         ob_eval,
         parent_path.GetString().c_str(),
         anchor.GetString().c_str());

  pxr::SdfPath usd_path = export_object(ctx);

  // Export objects duplicated by this object.
  ListBase *lb = object_duplilist(m_settings.depsgraph, m_settings.scene, ob_eval);

  if (lb) {
    DupliObject *link = static_cast<DupliObject *>(lb->first);
    Object *dupli_ob = NULL;
    Object *dupli_parent = NULL;

    for (; link; link = link->next) {
      /* This skips things like custom bone shapes. */
      if (link->no_draw) {
        continue;
      }

      if (link->type != OB_DUPLICOLLECTION) {
        continue;
      }

      Object *instance_eval_ob = DEG_get_evaluated_object(m_settings.depsgraph, link->ob);
      export_or_queue(instance_eval_ob, ob_eval, usd_path);
    }

    free_object_duplilist(lb);
  }

  // Perform export of objects that were waiting for this one.
  Object *ob_orig = DEG_get_original_object(ob_eval);
  DeferredExportSet::const_iterator deferred_it = deferred_exports.find(ob_orig);
  if (deferred_it == deferred_exports.end())
    return;

  const std::vector<USDExporterContext> deferred = deferred_it->second;
  deferred_exports.erase(deferred_it);

  printf("   Now free to handle %lu deferred exports\n", deferred.size());

  for (USDExporterContext ctx : deferred) {
    export_or_queue(ctx.ob_eval, ctx.instanced_by, pxr::SdfPath("/"));
  }
}

/* Return the parent USD path. If an empty path is returned, r_missing is set to the Object that is
 * required to determine that path (and is not available in usd_object_paths). */
pxr::SdfPath USDExporter::parent_usd_path(Object *ob_eval,
                                          Object *instanced_by,
                                          const pxr::SdfPath &anchor,
                                          Object **r_missing)
{
  if (ob_eval->parent == NULL) {
    return anchor;
  }

  // Append the parent object's USD path.
  USDPathMap::iterator path_it = usd_object_paths.find(ob_eval->parent);
  if (path_it == usd_object_paths.end()) {
    printf(
        "USD-\033[31mSKIPPING\033[0m object %s (%p) instanced by %p because parent %s (%p) "
        "hasn't been seen yet\n",
        ob_eval->id.name,
        ob_eval,
        instanced_by,
        ob_eval->parent->id.name,
        ob_eval->parent);
    *r_missing = ob_eval->parent;
    return pxr::SdfPath();
  }

  return path_it->second;
}

pxr::SdfPath USDExporter::export_object(const USDExporterContext &ctx)
{
  USDAbstractWriter *xform_writer = export_object_xform(ctx);

  USDExporterContext data_ctx = ctx;
  data_ctx.parent_path = xform_writer->usd_path();
  export_object_data(data_ctx);

  return xform_writer->usd_path();
}

/* Write the transform. This is always done, even when we don't write the data, as it makes it
 * possible to reference collection-instantiating empties. */
USDAbstractWriter *USDExporter::export_object_xform(const USDExporterContext &ctx)
{
  USDAbstractWriter *xform_writer = new USDTransformWriter(ctx);

  const pxr::SdfPath &xform_usd_path = xform_writer->usd_path();
  usd_object_paths[ctx.ob_eval] = xform_usd_path;
  usd_writers[xform_usd_path] = xform_writer;
  xform_writer->write();

  return xform_writer;
}

/* Write the object data, if we know how. */
USDAbstractWriter *USDExporter::export_object_data(const USDExporterContext &ctx)
{
  USDAbstractWriter *data_writer = NULL;

  switch (ctx.ob_eval->type) {
    case OB_MESH:
      data_writer = new USDMeshWriter(ctx);
      break;
    default:
      printf("USD-\033[34mXFORM-ONLY\033[0m object %s  type=%d (no data writer)\n",
             ctx.ob_eval->id.name,
             ctx.ob_eval->type);
      return NULL;
  }

  if (!data_writer->is_supported()) {
    printf("USD-\033[34mXFORM-ONLY\033[0m object %s  type=%d (data writer rejects the data)\n",
           ctx.ob_eval->id.name,
           ctx.ob_eval->type);
    delete data_writer;
    return NULL;
  }

  usd_writers[data_writer->usd_path()] = data_writer;
  data_writer->write();

  return data_writer;
}
