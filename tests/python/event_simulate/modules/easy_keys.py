# Apache License, Version 2.0

# ./blender.bin --debug-events-simulate --python release/scripts/templates_py/debug_events_simulate.py

import bpy
event_types = tuple(
    e.identifier.lower()
    for e in bpy.types.Event.bl_rna.properties["type"].enum_items_static
)
del bpy

# We don't normally care about which one.
event_types_alias = {
    "ctrl": "left_ctrl",
    "shift": "left_shift",
    "alt": "left_alt",
}

import string

# TODO, there are a few more we could add
event_types_text = (
    ('ZERO', "0", False),
    ('ONE', "1", False),
    ('TWO', "2", False),
    ('THREE', "3", False),
    ('FOUR', "4", False),
    ('FIVE', "5", False),
    ('SIX', "6", False),
    ('SEVEN', "7", False),
    ('EIGHT', "8", False),
    ('NINE', "9", False),

    ('ZERO', "=", True),
    ('ONE', "!", True),
    ('TWO', "@", True),
    ('THREE', "#", True),
    ('FOUR', "$", True),
    ('FIVE', "%", True),
    ('SIX', "^", True),
    ('SEVEN', "&", True),
    ('EIGHT', "*", True),
    ('NINE', "-", True),

    ('SEMI_COLON', ";", False),
    ('SEMI_COLON', ":", True),

    ('PERIOD', ".", False),
    ('PERIOD', ">", True),

    ('COMMA', ",", False),
    ('COMMA', "<", True),

    ('QUOTE', "'", False),
    ('QUOTE', '"', True),

    ('MINUS', "-", False),
    ('MINUS', "_", True),

    ('SLASH', "/", False),
    ('SLASH', "?", True),

    ('BACK_SLASH', "\\", False),
    ('BACK_SLASH', "|", True),

    ('EQUAL', "=", False),
    ('EQUAL', "+", True),

    *((ch_upper, ch, False) for (ch_upper, ch) in zip(string.ascii_uppercase, string.ascii_lowercase)),
    *((ch, ch, True) for ch in string.ascii_uppercase),

    ('SPACE', " ", False),
)

event_types_text_from_char = {ch: (ty, is_shift) for (ty, ch, is_shift) in event_types_text}
event_types_text_from_event = {(ty, is_shift): ch for (ty, ch, is_shift) in event_types_text}


class _EventBuilder:
    __slots__ = (
        "_event_gen",
        "_ty",
        "_parent",
        "_event_simulate_kw",
    )
    def __init__(self, event_gen, ty, kw):
        self._event_gen = event_gen
        self._ty = ty
        self._parent = None
        self._event_simulate_kw = kw.copy()

    def __call__(self, count=1):
        for _ in range(count):
            self.tap()
        return self._event_gen

    def tap(self):
        build_keys = []
        e = self
        while e is not None:
            build_keys.append(e._ty.upper())
            e = e._parent
        build_keys.reverse()

        ty_pressed = set()
        for i, value in enumerate(('PRESS', 'RELEASE')):
            if value == 'RELEASE':
                build_keys.reverse()
            for ty in build_keys:
                if value == 'PRESS':
                    ty_pressed.add(ty)
                else:
                    ty_pressed.remove(ty)

                shift = 'LEFT_SHIFT' in ty_pressed or 'RIGHT_SHIFT' in ty_pressed
                ctrl = 'LEFT_CTRL' in ty_pressed or 'RIGHT_CTRL' in ty_pressed
                shift = 'LEFT_SHIFT' in ty_pressed or 'RIGHT_SHIFT' in ty_pressed
                alt = 'LEFT_ALT' in ty_pressed or 'RIGHT_ALT' in ty_pressed
                oskey = 'OSKEY' in ty_pressed

                unicode = None
                if ctrl is False and alt is False and oskey is False:
                    if value == 'PRESS':
                        unicode = event_types_text_from_event.get((ty, shift))
                        if unicode is None and shift:
                            # Some keys don't care about shift
                            unicode = event_types_text_from_event.get((ty, False))
                self._event_gen._window.event_simulate(

                    type=ty,
                    value=value,
                    unicode=unicode,
                    shift=shift,
                    ctrl=ctrl,
                    alt=alt,
                    oskey=oskey,
                    # typically mouse coords.
                    **self._event_simulate_kw,
                )

    def press(self):
        self._key_with_value(value='PRESS')

    def release(self):
        self._key_with_value(value='RELEASE')

    def _key_with_value(self, *, value):
        build_keys = []
        e = self
        while e is not None:
            build_keys.append(e._ty.upper())
            e = e._parent
        build_keys.reverse()

        ty_pressed = set()
        if value == 'RELEASE':
            ty_pressed = set(build_keys)
        for ty in build_keys:
            if value == 'RELEASE':
                build_keys.reverse()
            if value == 'PRESS':
                ty_pressed.add(ty)
            else:
                ty_pressed.remove(ty)
            self._event_gen._window.event_simulate(
                type=ty,
                value=value,
                ctrl='LEFT_CTRL' in ty_pressed or 'RIGHT_CTRL' in ty_pressed,
                shift='LEFT_SHIFT' in ty_pressed or 'RIGHT_SHIFT' in ty_pressed,
                alt='LEFT_ALT' in ty_pressed or 'RIGHT_ALT' in ty_pressed,
                oskey='OSKEY' in ty_pressed,
                # typically mouse coords.
                **self._event_simulate_kw,
            )

    def __getattr__(self, attr):
        attr = event_types_alias.get(attr, attr)
        if attr in event_types:
            e = _EventBuilder(self._event_gen, attr, self._event_simulate_kw)
            e._parent = self
            return e
        raise Exception(f"{attr!r} not found in {event_types!r}")


class EventGenerate:
    __slots__ = (
        "_window",
        "_event_simulate_kw",
    )
    def __init__(self, window):
        self._window = window
        self._event_simulate_kw = {}

        self.cursor_position_set(window.width // 2, window.height // 2)

    def cursor_position_set(self, x, y):
        self._event_simulate_kw["x"] = x
        self._event_simulate_kw["y"] = y

    def text(self, text):
        """ Type in entire phrases. """
        for ch in text:
            ty, shift = event_types_text_from_char[ch]
            ty = ty.lower()
            if shift:
                eb = getattr(_EventBuilder(self, 'left_shift', self._event_simulate_kw), ty)
            else:
                eb = _EventBuilder(self, ty, self._event_simulate_kw)
            eb.tap()
        return self

    def __getattr__(self, attr):
        attr = event_types_alias.get(attr, attr)
        if attr in event_types:
            return _EventBuilder(self, attr, self._event_simulate_kw)
        raise Exception(f"{attr!r} not found in {event_types!r}")


def run(event_iter):

    # 3 works, 4  to be on the safe side.
    TICKS = 4

    def event_step(*args):
        # Run once 'TICKS' is reached.
        if event_step._ticks < TICKS:
            event_step._ticks += 1
            return 0.0
        event_step._ticks = 0

        val = next(event_step.run_events, False)
        if val is not False:
            return 0.0
        print("Success!")

    event_step.run_events = iter(event_iter)
    event_step._ticks = 0
    bpy.app.timers.register(event_step, first_interval=0.0)
