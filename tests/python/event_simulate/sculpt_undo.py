# Apache License, Version 2.0
# ./blender.bin --debug-events-simulate --python tests/python/event_simulate/sculpt_undo.py

import os
import sys
sys.path.append(os.path.join(os.path.dirname(__file__), "modules"))

import easy_keys

# Callback doesn't have context.
import bpy
from bpy import context
window = context.window

def play_keys():
    e = easy_keys.EventGenerate(window)

    yield e.esc()                     # Kick splash screen away
    yield e.shift.f5()                # 3D View
    yield e.ctrl.space()              # Full-screen.
    yield e.a().x().ret()             # Delete all.
    yield e.shift.a().m().t()         # Add torus.
    yield e.r().y().text("45").ret()  # Rotate Y 45.
    yield e.ctrl.a().r()              # Apply rotation.
    yield e.ctrl.tab().s()            # Sculpt via pie menu.
    yield e.ctrl.d().ret()            # Dynamic topology.
    yield e.f3()                      # Symmetrize via search.
    yield e.text("Symmetrize").ret()  # ...
    yield e.ctrl.tab().o()            # Object mode.
    yield e.x().ret()                 # Delete the object.
    yield e.ctrl.z()                  # Undo...
    yield e.ctrl.z()                  # Undo crash.

    # Allow human interaction.
    bpy.app.debug_events_simulate = False
    yield False

easy_keys.run(play_keys())
