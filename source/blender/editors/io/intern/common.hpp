#ifndef __io_intern_common__
#  define __io_intern_common__

/* #include "BLI_listbase.h" */

/* #include "DNA_mesh_types.h" */
/* #include "DNA_object_types.h" */

extern "C" {

// Clang-format is adding these spaces, but it's not allowed by the standard

#  include "BKE_global.h"
#  include "BKE_mesh.h"
#  include "BKE_mesh_runtime.h"
#  include "BKE_modifier.h"
#  include "BKE_library.h"
#  include "BKE_customdata.h"
#  include "BKE_scene.h"

#  include "MEM_guardedalloc.h"

/* SpaceType struct has a member called 'new' which obviously conflicts with C++
 * so temporarily redefining the new keyword to make it compile. */
#  define new extern_new
#  include "BKE_screen.h"
#  undef new

#  include "BLI_listbase.h"
#  include "BLI_math_matrix.h"
#  include "BLI_math_vector.h"
#  include "BLI_utildefines.h"

#  include "bmesh.h"
#  include "bmesh_tools.h"

#  include "DNA_layer_types.h"
#  include "DNA_meshdata_types.h"
#  include "DNA_mesh_types.h"
#  include "DNA_modifier_types.h"
#  include "DNA_object_types.h"

#  include "DEG_depsgraph_build.h"
#  include "DEG_depsgraph_query.h"

#  include "../io_common.h"
}

#  include <utility>
#  include <string>
#  include <vector>
#  include <set>
#  include <array>
#  include <typeinfo>
#  include <iterator>

namespace common {
using ulong = unsigned long;

// --- PROTOTYPES ---

bool object_is_smoke_sim(const Object *const ob);

bool object_type_is_exportable(const Object *const ob);

bool should_export_object(const ExportSettings *const settings, const Object *const ob);

void change_orientation(float (&mat)[4][4], int forward, int up);

bool get_final_mesh(const ExportSettings *const settings,
                    const Scene *const escene,
                    const Object *eob,
                    Mesh **mesh /* out */,
                    float (*mat)[4][4] /* out */);

void free_mesh(Mesh *mesh, bool needs_free);

void name_compat(std::string &ob_name, const std::string &mesh_name);
std::string get_version_string();

float get_unit_scale(const Scene *const scene);

void export_start(bContext *C, ExportSettings *const settings);
bool export_end(bContext *C, ExportSettings *const settings);

// Execute `start` and `end` and time it. Those functions should be
// specific to each exportter, but have the same signature as the two above
bool time_export(bContext *C,
                 ExportSettings *const settings,
                 void (*start)(bContext *C, ExportSettings *const settings),
                 bool (*end)(bContext *C, ExportSettings *const settings));

const std::array<float, 3> calculate_normal(const Mesh *const mesh, const MPoly &mp);

std::vector<std::array<float, 3>> get_normals(const Mesh *const mesh);
std::vector<std::array<float, 2>> get_uv(const Mesh *const mesh);
std::vector<std::array<float, 3>> get_vertices(const Mesh *const mesh);

// --- TEMPLATES ---

// T should be something with an ID, like Mesh or Curve
template<typename T> std::string get_object_name(const Object *const ob, const T *const data)
{
  std::string name{ob->id.name + 2};
  std::string data_name{data->id.name + 2};
  name_compat(name /* modifies */, data_name);
  return name;
}

// --- Deduplication ---
// TODO someone How to properly compare similarity of vectors?
struct threshold_comparator {
  threshold_comparator(const float th) : th(th)
  {
  }
  template<typename key_t> bool operator()(const key_t &lhs, const key_t &rhs) const
  {
    return  // too_similar(lhs.first, rhs.first, th) &&
        lhs.first < rhs.first;
  }
  const float th;
};

// The set used to deduplicate UVs and normals
template<typename key_t> using set_t = std::set<key_t, threshold_comparator>;
// Provides the mapping from the deduplicated to the original index
template<typename key_t> using set_mapping_t = std::vector<typename set_t<key_t>::iterator>;
// A pair of the two above, to tie them together
template<typename key_t> using dedup_pair_t = std::pair<set_t<key_t>, set_mapping_t<key_t>>;
// The set key for UV and normal. ulong is the original index
using uv_key_t = std::pair<std::array<float, 2>, ulong>;
using no_key_t = std::pair<std::array<float, 3>, ulong>;

// Construct a new deduplicated set for either normals or UVs, with the given similarity threshold
// C++14/17 would be useful here...
template<typename key_t> dedup_pair_t<key_t> make_deduplicate_set(const float threshold)
{
  return {set_t<key_t>{threshold_comparator(threshold)} /* set */, {} /* set_mapping */};
}
}  // namespace common

#endif  // __io_intern_common_
