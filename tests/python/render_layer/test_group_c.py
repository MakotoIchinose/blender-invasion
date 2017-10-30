# ############################################################
# Importing - Same For All Render Layer Tests
# ############################################################

import unittest
import os
import sys

from render_layer_common import *


# ############################################################
# Testing
# ############################################################

class UnitTesting(RenderLayerTesting):
    def test_group_create_basic(self):
        """
        See if the creation of new groups is working
        """
        import bpy
        scene = bpy.context.scene

        # clean slate
        self.cleanup_tree()

        child = bpy.data.objects.new("Child", None)
        master_collection = scene.master_collection
        scene_collection = master_collection.collections.new('Collection')
        scene_collection.objects.link(child)

        layer_collection_one = scene.render_layers[0].collections.link(scene_collection)
        layer_collection_two = scene.render_layers[0].collections.link(scene_collection)

        # update depsgraph
        scene.update()

        # create group
        group = layer_collection_one.create_group()

        # update depsgraph
        scene.update()
        scene.depsgraph.debug_graphviz("/tmp/a.dot")


# ############################################################
# Main - Same For All Render Layer Tests
# ############################################################

if __name__ == '__main__':
    UnitTesting._extra_arguments = setup_extra_arguments(__file__)
    unittest.main()
