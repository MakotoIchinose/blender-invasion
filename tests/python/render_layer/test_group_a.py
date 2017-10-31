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
        See if the creation of new groups is not crashing anything.
        """
        import bpy
        scene = bpy.context.scene
        layer_collection = bpy.context.layer_collection

        # Cleanup Viewport render layer
        # technically this shouldn't be needed but
        # for now we need it because depsgraph build all the scenelayers
        # at once.

        while len(scene.render_layers) > 1:
            scene.render_layers.remove(scene.render_layers[1])

        # create group
        group = layer_collection.create_group()

        # update depsgraph
        scene.update()


# ############################################################
# Main - Same For All Render Layer Tests
# ############################################################

if __name__ == '__main__':
    UnitTesting._extra_arguments = setup_extra_arguments(__file__)
    unittest.main()
