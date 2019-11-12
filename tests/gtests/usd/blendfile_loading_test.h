#ifndef __BLENDFILE_LOADING_TEST_H__
#define __BLENDFILE_LOADING_TEST_H__

#include "testing/testing.h"
#include <DEG_depsgraph.h>

struct BlendFileData;
struct Depsgraph;

class BlendfileLoadingTest : public testing::Test {
 protected:
  struct BlendFileData *bfile;
  struct Depsgraph *depsgraph;

 public:
  BlendfileLoadingTest();
  virtual ~BlendfileLoadingTest();

  virtual void SetUp();
  virtual void TearDown();

 protected:
  /* Load a blend file, return 'ok' (true=good, false=bad) and set bfile.
   * Fails the test if the file cannot be loaded (still returns though).
   * Requires the environment variable TEST_ASSETS_DIR to point to ../lib/tests.
   */
  bool blendfile_load(const char *filepath);
  /* Free bfile if it is not nullptr */
  void blendfile_free();

  /* Create a depsgraph. Assumes a blend file has been loaded. */
  void depsgraph_create(eEvaluationMode depsgraph_evaluation_mode);
  /* Free the depsgraph if it's not nullptr. */
  void depsgraph_free();
};

#endif /* __BLENDFILE_LOADING_TEST_H__ */
