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
 *
 * The Original Code is Copyright (C) 2019 Blender Foundation.
 * All rights reserved.
 */
#include "testing/testing.h"
#include <pxr/usd/usd/stage.h>

#include <string>

extern "C" {
#include "BLI_utildefines.h"

#include "BKE_appdir.h"

/* Workaround to make it possible to pass a path at runtime to USD. See creator.c. */
void usd_initialise_plugin_path(const char *datafiles_usd_path);
}

class USDStageCreationTest : public testing::Test {
};

TEST_F(USDStageCreationTest, JSONFileLoadingTest)
{
  std::string filename = "usd-stage-creation-test.usdc";

  usd_initialise_plugin_path(BKE_appdir_folder_id(BLENDER_DATAFILES, "usd"));

  /* Simply the ability to create a USD Stage for a specific filename means that the extension has
   * been recognised by the USD library, and that a USD plugin has been loaded to write such files.
   * Practically, this is a test to see whether the USD JSON files can be found and loaded. */
  pxr::UsdStageRefPtr usd_stage = pxr::UsdStage::CreateNew(filename);
  EXPECT_TRUE(usd_stage) << "unable to find suitable USD plugin to write " << filename;
}
