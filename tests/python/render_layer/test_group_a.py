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
        layer_collection = bpy.context.layer_collection

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
