#ifndef __BLENDFILE_LOADING_TEST_H__
#define __BLENDFILE_LOADING_TEST_H__

#include "testing/testing.h"
#include <DEG_depsgraph.h>

struct BlendFileData;
struct Depsgraph;

class BlendfileLoadingAbstractTest : public testing::Test {
 protected:
  struct BlendFileData *bfile;
  struct Depsgraph *depsgraph;

 public:
  BlendfileLoadingAbstractTest();
  virtual ~BlendfileLoadingAbstractTest();

  /* Sets up Blender just enough to not crash on loading
   * a blendfile and constructing a depsgraph. */
  static void SetUpTestCase();

 protected:
  /* Frees the depsgraph & blendfile. */
  virtual void TearDown();

  /* Loads a blend file from the lib/tests directory from SVN.
   * Returns 'ok' flag (true=good, false=bad) and sets this->bfile.
   * Fails the test if the file cannot be loaded (still returns though).
   * Requires the CLI argument --test-asset-dir to point to ../../lib/tests.
   *
   * WARNING: only files saved with Blender 2.80+ can be loaded. Since Blender
   * is only partially initialised (most importantly, without window manager),
   * the space types are not registered, so any versioning code that handles
   * those will SEGFAULT.
   */
  bool blendfile_load(const char *filepath);
  /* Free bfile if it is not nullptr */
  void blendfile_free();

  /* Create a depsgraph. Assumes a blend file has been loaded to this->bfile. */
  void depsgraph_create(eEvaluationMode depsgraph_evaluation_mode);
  /* Free the depsgraph if it's not nullptr. */
  void depsgraph_free();
};

#endif /* __BLENDFILE_LOADING_TEST_H__ */
