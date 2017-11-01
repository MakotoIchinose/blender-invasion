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
    def test_group_write_load(self):
        """
        See if saving/loading is working for groups
        """
        import bpy
        scene = bpy.context.scene
        layer_collection = bpy.context.layer_collection

        while len(scene.render_layers) > 1:
            scene.render_layers.remove(scene.render_layers[1])

        # create group
        group = layer_collection.create_group()

        # update depsgraph
        scene.update()

        import os
        import tempfile
        with tempfile.TemporaryDirectory() as dirpath:
            filepath = os.path.join(dirpath, 'layers.blend')

            # save file
            bpy.ops.wm.save_mainfile('EXEC_DEFAULT', filepath=filepath)

            # open file
            bpy.ops.wm.open_mainfile('EXEC_DEFAULT', filepath=filepath)


# ############################################################
# Main - Same For All Render Layer Tests
# ############################################################

if __name__ == '__main__':
    UnitTesting._extra_arguments = setup_extra_arguments(__file__)
    unittest.main()
