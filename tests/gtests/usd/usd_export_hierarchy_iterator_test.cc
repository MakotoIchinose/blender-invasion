#include "testing/testing.h"

// Keep first since utildefines defines AT which conflicts with STL
#include "intern/abstract_hierarchy_iterator.h"

extern "C" {
#include "BKE_blender.h"
#include "BKE_main.h"
#include "BKE_modifier.h"
#include "BKE_node.h"
#include "BKE_scene.h"

#include "BLI_utildefines.h"
#include "BLI_math.h"

#include "BLO_readfile.h"

#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph.h"

#include "DNA_genfile.h"
#include "DNA_scene_types.h"
#include "DNA_windowmanager_types.h"

#include "RNA_define.h"

#include "MEM_guardedalloc.h"
}

#include <map>
#include <set>

/* Mapping from ID.name to set of export hierarchy path. Duplicated objects can be exported
 * multiple times, hence the set. */
typedef std::map<std::string, std::set<std::string>> created_writers;

class TestHierarchyWriter : public AbstractHierarchyWriter {
 public:
  created_writers &writers_map;

  TestHierarchyWriter(created_writers &writers_map) : writers_map(writers_map)
  {
  }

  void write(HierarchyContext &context) override
  {
    const char *id_name = context.object->id.name;
    created_writers::mapped_type &writers = writers_map[id_name];

    BLI_assert(writers.find(context.export_path) == writers.end());
    writers.insert(context.export_path);
  }
};

void debug_print_writers(const char *label, const created_writers &writers_map)
{
  printf("%s:\n", label);
  for (auto idname_writers : writers_map) {
    printf("    %s:\n", idname_writers.first.c_str());
    for (const std::string &export_path : idname_writers.second) {
      printf("      - %s\n", export_path.c_str());
    }
  }
}

class TestingHierarchyIterator : public AbstractHierarchyIterator {
 public: /* Public so that the test cases can directly inspect the created writers. */
  created_writers xform_writers;
  created_writers data_writers;
  created_writers hair_writers;
  created_writers particle_writers;

 public:
  explicit TestingHierarchyIterator(Depsgraph *depsgraph) : AbstractHierarchyIterator(depsgraph)
  {
  }
  virtual ~TestingHierarchyIterator()
  {
  }

 protected:
  AbstractHierarchyWriter *create_xform_writer(const HierarchyContext *context) override
  {
    return new TestHierarchyWriter(xform_writers);
  }
  AbstractHierarchyWriter *create_data_writer(const HierarchyContext *context) override
  {
    return new TestHierarchyWriter(data_writers);
  }
  AbstractHierarchyWriter *create_hair_writer(const HierarchyContext *context) override
  {
    return new TestHierarchyWriter(hair_writers);
  }
  AbstractHierarchyWriter *create_particle_writer(const HierarchyContext *context) override
  {
    return new TestHierarchyWriter(particle_writers);
  }

  void delete_object_writer(AbstractHierarchyWriter *writer) override
  {
    delete writer;
  }
};

class USDHierarchyIteratorTest : public testing::Test {
 protected:
  BlendFileData *bfile;
  Depsgraph *depsgraph;
  TestingHierarchyIterator *iterator;

  virtual void SetUp()
  {
    bfile = nullptr;
    depsgraph = nullptr;
    iterator = nullptr;

    /* Minimal code to make the exporter not crash, copied from main() in creator.c. */
    DNA_sdna_current_init();
    DEG_register_node_types();
    RNA_init();
    init_nodesystem();
  }

  virtual void TearDown()
  {
    iterator_free();
    depsgraph_free();
    blendfile_free();
  }

  /* Load a blend file, return 'ok' (true=good, false=bad) and set bfile.
   * Fails the test if the file cannot be loaded.
   */
  bool blendfile_load(const char *filepath)
  {
    bfile = BLO_read_from_file(filepath, BLO_READ_SKIP_NONE, NULL /* reports */);
    if (bfile == nullptr) {
      ADD_FAILURE();
      return false;
    }

    /* Create a dummy window manager. Some code would SEGFAULT without it. */
    bfile->main->wm.first = MEM_callocN(sizeof(wmWindowManager), "Dummy Window Manager");
    return true;
  }
  /* Free bfile if it is not nullptr */
  void blendfile_free()
  {
    if (bfile == nullptr) {
      return;
    }
    BLO_blendfiledata_free(bfile);
    bfile = nullptr;
  }

  /* Create a depsgraph. Assumes a blend file has been loaded. */
  void depsgraph_create()
  {
    /* TODO(sergey): Pass scene layer somehow? */
    depsgraph = DEG_graph_new(
        bfile->main, bfile->curscene, bfile->cur_view_layer, DAG_EVAL_RENDER);

    DEG_graph_build_from_view_layer(
        depsgraph, bfile->main, bfile->curscene, bfile->cur_view_layer);
    BKE_scene_graph_update_tagged(depsgraph, bfile->main);
  }
  /* Free the depsgraph if it's not nullptr. */
  void depsgraph_free()
  {
    if (depsgraph == nullptr) {
      return;
    }
    DEG_graph_free(depsgraph);
    depsgraph = nullptr;
  }

  /* Create a test iterator. */
  void iterator_create()
  {
    iterator = new TestingHierarchyIterator(depsgraph);
  }
  /* Free the test iterator if it is not nullptr. */
  void iterator_free()
  {
    if (iterator == nullptr) {
      return;
    }
    delete iterator;
    iterator = nullptr;
  }
};

TEST_F(USDHierarchyIteratorTest, ExportHierarchyTest)
{
  /* Load the test blend file. */
  if (!blendfile_load("../../lib/tests/usd/usd_hierarchy_export_test.blend")) {
    return;
  }
  depsgraph_create();
  iterator_create();

  iterator->iterate();

  // Mapping from object name to set of export paths.
  created_writers expected_xforms = {
      {"OBCamera", {"/Camera"}},
      {"OBDupli1", {"/Dupli1"}},
      {"OBDupli2", {"/ParentOfDupli2/Dupli2"}},
      {"OBGEO_Ear_L",
       {"/Dupli1/GEO_Head-0/GEO_Ear_L-1",
        "/Ground plane/OutsideDupliGrandParent/OutsideDupliParent/GEO_Head/GEO_Ear_L",
        "/ParentOfDupli2/Dupli2/GEO_Head-0/GEO_Ear_L-1"}},
      {"OBGEO_Ear_R",
       {"/Dupli1/GEO_Head-0/GEO_Ear_R-2",
        "/Ground plane/OutsideDupliGrandParent/OutsideDupliParent/GEO_Head/GEO_Ear_R",
        "/ParentOfDupli2/Dupli2/GEO_Head-0/GEO_Ear_R-2"}},
      {"OBGEO_Head",
       {"/Dupli1/GEO_Head-0",
        "/Ground plane/OutsideDupliGrandParent/OutsideDupliParent/GEO_Head",
        "/ParentOfDupli2/Dupli2/GEO_Head-0"}},
      {"OBGEO_Nose",
       {"/Dupli1/GEO_Head-0/GEO_Nose-3",
        "/Ground plane/OutsideDupliGrandParent/OutsideDupliParent/GEO_Head/GEO_Nose",
        "/ParentOfDupli2/Dupli2/GEO_Head-0/GEO_Nose-3"}},
      {"OBGround plane", {"/Ground plane"}},
      {"OBOutsideDupliGrandParent", {"/Ground plane/OutsideDupliGrandParent"}},
      {"OBOutsideDupliParent", {"/Ground plane/OutsideDupliGrandParent/OutsideDupliParent"}},
      {"OBParentOfDupli2", {"/ParentOfDupli2"}}};
  EXPECT_EQ(expected_xforms, iterator->xform_writers);

  created_writers expected_data = {
      {"OBCamera", {"/Camera/Camera"}},
      {"OBGEO_Ear_L",
       {"/Dupli1/GEO_Head-0/GEO_Ear_L-1/Ear",
        "/Ground plane/OutsideDupliGrandParent/OutsideDupliParent/GEO_Head/GEO_Ear_L/Ear",
        "/ParentOfDupli2/Dupli2/GEO_Head-0/GEO_Ear_L-1/Ear"}},
      {"OBGEO_Ear_R",
       {"/Dupli1/GEO_Head-0/GEO_Ear_R-2/Ear",
        "/Ground plane/OutsideDupliGrandParent/OutsideDupliParent/GEO_Head/GEO_Ear_R/Ear",
        "/ParentOfDupli2/Dupli2/GEO_Head-0/GEO_Ear_R-2/Ear"}},
      {"OBGEO_Head",
       {"/Dupli1/GEO_Head-0/Face",
        "/Ground plane/OutsideDupliGrandParent/OutsideDupliParent/GEO_Head/Face",
        "/ParentOfDupli2/Dupli2/GEO_Head-0/Face"}},
      {"OBGEO_Nose",
       {"/Dupli1/GEO_Head-0/GEO_Nose-3/Nose",
        "/Ground plane/OutsideDupliGrandParent/OutsideDupliParent/GEO_Head/GEO_Nose/Nose",
        "/ParentOfDupli2/Dupli2/GEO_Head-0/GEO_Nose-3/Nose"}},
      {"OBGround plane", {"/Ground plane/Plane"}},
      {"OBParentOfDupli2", {"/ParentOfDupli2/Icosphere"}},
  };

  EXPECT_EQ(expected_data, iterator->data_writers);

  // The scene has no hair or particle systems.
  EXPECT_EQ(0, iterator->hair_writers.size());
  EXPECT_EQ(0, iterator->particle_writers.size());
}
