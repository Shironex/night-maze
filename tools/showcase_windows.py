# The Windows half of tools/capture_showcase.py: how the game window is found, put on
# top and photographed, and how keys and mouse turns are sent to it. The macOS half is
# tools/showcase_macos.py, and both offer the same functions with the same names, so
# capture_showcase.py does not ask which system it runs on.
#
# The window code and the screen grabber (ffmpeg gdigrab) are the ones of
# tools/record_menu_loop.py; the keys and the mouse go through SendInput.
import ctypes
import ctypes.wintypes
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from record_menu_loop import (WINDOW_LEFT, WINDOW_TOP, client_size,  # noqa: E402,F401
                              grab_arguments, put_on_top, run, use_real_pixels, user32)
from record_menu_loop import find_window as find_window_of  # noqa: E402

# The settings file writes mouse_sensitivity = 4, which is 0.1 degrees per unit of mouse
# movement (game/Settings.hpp), so a turn of 90 degrees is 900 units.
UNITS_PER_DEGREE = 10


def prepare():
    # Real pixels, not the scaled sizes of a display with scaling (record_menu_loop.py).
    use_real_pixels()


def input_allowed():
    # Windows lets any program send keys to the active window.
    return True, ""


def find_window(process_id, _pointer_at_start):
    # Where the pointer was when the game started is only needed on macOS.
    return find_window_of(process_id)


def show(window, left, top):
    put_on_top(window, left, top)


def pointer():
    point = ctypes.wintypes.POINT()
    user32.GetCursorPos(ctypes.byref(point))
    return point.x, point.y


def put_pointer(x, y):
    user32.SetCursorPos(x, y)


def is_front(window):
    return user32.GetForegroundWindow() == window


def bring_to_front(window):
    # A game that a script starts is not always the active window: Windows keeps another
    # program's window in front when the person is working in it. A round pauses when
    # its window is not the active one, so the window is asked for the front: the thread
    # of the window in front lends its right to do that. No key and no click is sent.
    if user32.GetForegroundWindow() == window:
        return
    front_thread = user32.GetWindowThreadProcessId(user32.GetForegroundWindow(), None)
    own_thread = ctypes.windll.kernel32.GetCurrentThreadId()
    user32.AttachThreadInput(own_thread, front_thread, True)
    user32.BringWindowToTop(window)
    user32.SetForegroundWindow(window)
    user32.AttachThreadInput(own_thread, front_thread, False)
    time.sleep(0.3)


def screen_corners():
    # The top left corner of every screen, moved in by as much as the usual place is.
    corners = []

    @ctypes.WINFUNCTYPE(ctypes.c_int, ctypes.wintypes.HMONITOR, ctypes.wintypes.HDC,
                        ctypes.POINTER(ctypes.wintypes.RECT), ctypes.wintypes.LPARAM)
    def visit(_monitor, _context, rectangle, _data):
        screen = rectangle.contents
        corners.append((screen.left + WINDOW_LEFT, screen.top + WINDOW_TOP))
        return 1

    user32.EnumDisplayMonitors(0, 0, visit, 0)
    return corners


def pointer_inside(window):
    point = ctypes.wintypes.POINT()
    user32.GetCursorPos(ctypes.byref(point))
    rectangle = ctypes.wintypes.RECT()
    user32.GetWindowRect(window, ctypes.byref(rectangle))
    return (rectangle.left <= point.x < rectangle.right and
            rectangle.top <= point.y < rectangle.bottom)


def move_pointer_away(_window):
    # Nothing to do: on Windows the window was put where the pointer is not
    # (capture_showcase.py, Game.place), and the pointer is never moved by the script.
    return False


def grab(window, path):
    run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", *grab_arguments(window),
         "-frames:v", "1", "-update", "1", path])
    print("wrote", path)


# ---- keys and mouse -----------------------------------------------------------------------

# Scan codes of the keys the steps use (the position of a key on the keyboard, so the
# layout of the keyboard does not matter).
SCAN_CODES = {"W": 0x11, "S": 0x1F, "E": 0x12, "M": 0x32, "SHIFT": 0x2A, "TAB": 0x0F,
              "ENTER": 0x1C}

INPUT_MOUSE = 0
INPUT_KEYBOARD = 1
KEYEVENTF_KEYUP = 0x0002
KEYEVENTF_SCANCODE = 0x0008
MOUSEEVENTF_MOVE = 0x0001


class MouseInput(ctypes.Structure):
    _fields_ = [("dx", ctypes.c_long), ("dy", ctypes.c_long), ("mouseData", ctypes.c_ulong),
                ("dwFlags", ctypes.c_ulong), ("time", ctypes.c_ulong),
                ("dwExtraInfo", ctypes.c_void_p)]


class KeyboardInput(ctypes.Structure):
    _fields_ = [("wVk", ctypes.c_ushort), ("wScan", ctypes.c_ushort),
                ("dwFlags", ctypes.c_ulong), ("time", ctypes.c_ulong),
                ("dwExtraInfo", ctypes.c_void_p)]


class InputUnion(ctypes.Union):
    _fields_ = [("mouse", MouseInput), ("keyboard", KeyboardInput)]


class Input(ctypes.Structure):
    _fields_ = [("type", ctypes.c_ulong), ("union", InputUnion)]


def send_key(name, up):
    flags = KEYEVENTF_SCANCODE | (KEYEVENTF_KEYUP if up else 0)
    entry = Input(INPUT_KEYBOARD,
                  InputUnion(keyboard=KeyboardInput(0, SCAN_CODES[name], flags, 0, None)))
    user32.SendInput(1, ctypes.byref(entry), ctypes.sizeof(Input))


def send_move(_window, dx, dy):
    entry = Input(INPUT_MOUSE, InputUnion(mouse=MouseInput(dx, dy, 0, MOUSEEVENTF_MOVE, 0, None)))
    user32.SendInput(1, ctypes.byref(entry), ctypes.sizeof(Input))
