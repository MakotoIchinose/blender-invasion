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
#include "intern/abstract_hierarchy_iterator.h"

#include "testing/testing.h"

extern "C" {
#include "BLI_math.h"
#include "DEG_depsgraph.h"
#include "DNA_object_types.h"
}

class HierarchyContextOrderTest : public testing::Test {
};

static Object *fake_pointer(int value)
{
  return static_cast<Object *>(POINTER_FROM_INT(value));
}

TEST_F(HierarchyContextOrderTest, ObjectPointerTest)
{
  HierarchyContext ctx_a = {.object = fake_pointer(1), .duplicator = nullptr};
  HierarchyContext ctx_b = {.object = fake_pointer(2), .duplicator = nullptr};
  EXPECT_EQ(true, ctx_a < ctx_b);
  EXPECT_EQ(false, ctx_b < ctx_a);
  EXPECT_EQ(false, ctx_a < ctx_a);
}

TEST_F(HierarchyContextOrderTest, DuplicatorPointerTest)
{
  HierarchyContext ctx_a = {
      .object = fake_pointer(1), .duplicator = fake_pointer(1), .export_name = "A"};
  HierarchyContext ctx_b = {
      .object = fake_pointer(1), .duplicator = fake_pointer(1), .export_name = "B"};
  EXPECT_EQ(true, ctx_a < ctx_b);
  EXPECT_EQ(false, ctx_b < ctx_a);
  EXPECT_EQ(false, ctx_a < ctx_a);
}

TEST_F(HierarchyContextOrderTest, ExportParentTest)
{
  HierarchyContext ctx_a = {.object = fake_pointer(1), .export_parent = fake_pointer(1)};
  HierarchyContext ctx_b = {.object = fake_pointer(1), .export_parent = fake_pointer(2)};
  EXPECT_EQ(true, ctx_a < ctx_b);
  EXPECT_EQ(false, ctx_b < ctx_a);
  EXPECT_EQ(false, ctx_a < ctx_a);
}

TEST_F(HierarchyContextOrderTest, TransitiveTest)
{
  HierarchyContext ctx_a = {
      .object = fake_pointer(1),
      .export_parent = fake_pointer(1),
      .duplicator = nullptr,
      .export_name = "A",
  };
  HierarchyContext ctx_b = {
      .object = fake_pointer(2),
      .export_parent = nullptr,
      .duplicator = fake_pointer(1),
      .export_name = "B",
  };
  HierarchyContext ctx_c = {
      .object = fake_pointer(2),
      .export_parent = fake_pointer(2),
      .duplicator = fake_pointer(1),
      .export_name = "C",
  };
  HierarchyContext ctx_d = {
      .object = fake_pointer(2),
      .export_parent = fake_pointer(3),
      .duplicator = nullptr,
      .export_name = "D",
  };
  EXPECT_EQ(true, ctx_a < ctx_b);
  EXPECT_EQ(true, ctx_a < ctx_c);
  EXPECT_EQ(true, ctx_a < ctx_d);
  EXPECT_EQ(true, ctx_b < ctx_c);
  EXPECT_EQ(true, ctx_b < ctx_d);
  EXPECT_EQ(true, ctx_c < ctx_d);

  EXPECT_EQ(false, ctx_b < ctx_a);
  EXPECT_EQ(false, ctx_c < ctx_a);
  EXPECT_EQ(false, ctx_d < ctx_a);
  EXPECT_EQ(false, ctx_c < ctx_b);
  EXPECT_EQ(false, ctx_d < ctx_b);
  EXPECT_EQ(false, ctx_d < ctx_c);
}
