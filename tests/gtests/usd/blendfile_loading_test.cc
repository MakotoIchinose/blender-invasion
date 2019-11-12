#include "blendfile_loading_test.h"

extern "C" {
#include "BKE_main.h"
#include "BKE_node.h"
#include "BKE_scene.h"

#include "BLI_path_util.h"

#include "BLO_readfile.h"

#include "DEG_depsgraph_build.h"
#include "DEG_depsgraph.h"

#include "DNA_genfile.h" /* for DNA_sdna_current_init() */
#include "DNA_windowmanager_types.h"

#include "MEM_guardedalloc.h"

#include "RNA_define.h"
}

DEFINE_string(test_assets_dir,
              "../../lib/tests",
              "lib/tests directory from SVN, containing the test assets.");

BlendfileLoadingTest::BlendfileLoadingTest() : testing::Test(), bfile(nullptr), depsgraph(nullptr)
{
}

BlendfileLoadingTest::~BlendfileLoadingTest()
{
}

void BlendfileLoadingTest::SetUp()
{
  testing::Test::SetUp();

  /* Minimal code to make loading a blendfile and constructing a depsgraph not crash, copied from
   * main() in creator.c. */
  DNA_sdna_current_init();
  DEG_register_node_types();
  RNA_init();
  init_nodesystem();
}

void BlendfileLoadingTest::TearDown()
{
  depsgraph_free();
  blendfile_free();

  testing::Test::TearDown();
}

bool BlendfileLoadingTest::blendfile_load(const char *filepath)
{
  const char *test_assets_dir = FLAGS_test_assets_dir.c_str();
  if (test_assets_dir == nullptr || test_assets_dir[0] == '\0') {
    ADD_FAILURE()
        << "Pass the flag --test_assets_dir and point to the lib/tests directory from SVN.";
    return false;
  }

  char abspath[FILENAME_MAX];
  BLI_path_join(abspath, sizeof(abspath), test_assets_dir, filepath, NULL);

  bfile = BLO_read_from_file(abspath, BLO_READ_SKIP_NONE, NULL /* reports */);
  if (bfile == nullptr) {
    ADD_FAILURE();
    return false;
  }

  /* Create a dummy window manager. Some code would SEGFAULT without it. */
  bfile->main->wm.first = MEM_callocN(sizeof(wmWindowManager), "Dummy Window Manager");
  return true;
}

void BlendfileLoadingTest::blendfile_free()
{
  if (bfile == nullptr) {
    return;
  }
  BLO_blendfiledata_free(bfile);
  bfile = nullptr;
}

void BlendfileLoadingTest::depsgraph_create(eEvaluationMode depsgraph_evaluation_mode)
{
  depsgraph = DEG_graph_new(
      bfile->main, bfile->curscene, bfile->cur_view_layer, depsgraph_evaluation_mode);
  DEG_graph_build_from_view_layer(depsgraph, bfile->main, bfile->curscene, bfile->cur_view_layer);
  BKE_scene_graph_update_tagged(depsgraph, bfile->main);
}

void BlendfileLoadingTest::depsgraph_free()
{
  if (depsgraph == nullptr) {
    return;
  }
  DEG_graph_free(depsgraph);
  depsgraph = nullptr;
}
