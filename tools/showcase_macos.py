# The macOS half of tools/capture_showcase.py: how the game window is found, brought to
# the front and photographed, and how keys and mouse turns are sent to it. The Windows
# half is tools/showcase_windows.py, and both offer the same functions with the same
# names, so capture_showcase.py does not ask which system it runs on.
#
# No Python package is used here either: the system libraries of macOS (CoreGraphics,
# CoreFoundation and the Objective C runtime) are called through ctypes, and the
# picture is taken with screencapture, the screen grabber that comes with macOS.
#
# Two permissions of macOS are needed, both given to the program the script runs in
# (the terminal, for example) under System Settings, Privacy and Security:
#   Screen Recording   for the pictures (without it they show the desktop and no game)
#   Accessibility      for the keys and the mouse turns (without it macOS throws them
#                      away without a word, so the script asks first: input_allowed)
# The pictures that need no input (the menu camera, the places reached by the start
# switches) only need the first one.
import ctypes
import ctypes.util
import os
import platform
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from record_menu_loop import HEIGHT, WIDTH, WINDOW_TITLE  # noqa: E402

# The library functions are declared with their types: without them ctypes would pass
# every argument as a 32 bit number, which cuts the pointers of a 64 bit process.
_core_graphics = ctypes.cdll.LoadLibrary(ctypes.util.find_library("CoreGraphics"))
_core_foundation = ctypes.cdll.LoadLibrary(ctypes.util.find_library("CoreFoundation"))
_objc = ctypes.cdll.LoadLibrary(ctypes.util.find_library("objc"))
# AppKit is loaded for its classes only (NSRunningApplication): no window of its own is
# opened and no Python package is needed for it.
ctypes.cdll.LoadLibrary(ctypes.util.find_library("AppKit"))


class _Point(ctypes.Structure):
    _fields_ = [("x", ctypes.c_double), ("y", ctypes.c_double)]


class _Size(ctypes.Structure):
    _fields_ = [("width", ctypes.c_double), ("height", ctypes.c_double)]


class _Rect(ctypes.Structure):
    _fields_ = [("origin", _Point), ("size", _Size)]


def _declare(library, name, result, *arguments):
    function = getattr(library, name)
    function.restype = result
    function.argtypes = list(arguments)
    return function


_void = ctypes.c_void_p
CGWindowListCopyWindowInfo = _declare(_core_graphics, "CGWindowListCopyWindowInfo", _void,
                                      ctypes.c_uint32, ctypes.c_uint32)
CGRectMakeWithDictionaryRepresentation = _declare(
    _core_graphics, "CGRectMakeWithDictionaryRepresentation", ctypes.c_bool, _void,
    ctypes.POINTER(_Rect))
CGEventCreate = _declare(_core_graphics, "CGEventCreate", _void, _void)
CGEventGetLocation = _declare(_core_graphics, "CGEventGetLocation", _Point, _void)
CGEventCreateKeyboardEvent = _declare(_core_graphics, "CGEventCreateKeyboardEvent", _void, _void,
                                      ctypes.c_uint16, ctypes.c_bool)
CGEventCreateMouseEvent = _declare(_core_graphics, "CGEventCreateMouseEvent", _void, _void,
                                   ctypes.c_uint32, _Point, ctypes.c_uint32)
CGEventSetFlags = _declare(_core_graphics, "CGEventSetFlags", None, _void, ctypes.c_uint64)
CGEventSetIntegerValueField = _declare(_core_graphics, "CGEventSetIntegerValueField", None,
                                       _void, ctypes.c_uint32, ctypes.c_int64)
CGEventPost = _declare(_core_graphics, "CGEventPost", None, ctypes.c_uint32, _void)
CGWarpMouseCursorPosition = _declare(_core_graphics, "CGWarpMouseCursorPosition",
                                     ctypes.c_int32, _Point)
CGAssociateMouseAndMouseCursorPosition = _declare(
    _core_graphics, "CGAssociateMouseAndMouseCursorPosition", ctypes.c_int32, ctypes.c_bool)
CGPreflightPostEventAccess = _declare(_core_graphics, "CGPreflightPostEventAccess", ctypes.c_bool)
CGMainDisplayID = _declare(_core_graphics, "CGMainDisplayID", ctypes.c_uint32)
CGDisplayBounds = _declare(_core_graphics, "CGDisplayBounds", _Rect, ctypes.c_uint32)
CFArrayGetCount = _declare(_core_foundation, "CFArrayGetCount", ctypes.c_long, _void)
CFArrayGetValueAtIndex = _declare(_core_foundation, "CFArrayGetValueAtIndex", _void, _void,
                                  ctypes.c_long)
CFDictionaryGetValue = _declare(_core_foundation, "CFDictionaryGetValue", _void, _void, _void)
CFNumberGetValue = _declare(_core_foundation, "CFNumberGetValue", ctypes.c_bool, _void,
                            ctypes.c_long, _void)
CFStringGetCString = _declare(_core_foundation, "CFStringGetCString", ctypes.c_bool, _void,
                              ctypes.c_char_p, ctypes.c_long, ctypes.c_uint32)
CFRelease = _declare(_core_foundation, "CFRelease", None, _void)
objc_getClass = _declare(_objc, "objc_getClass", _void, ctypes.c_char_p)
sel_registerName = _declare(_objc, "sel_registerName", _void, ctypes.c_char_p)

# The keys of the dictionaries CGWindowListCopyWindowInfo returns, one per window. They
# are constants of the library, so they are read from it and not written out again.
_OWNER_PID = _void.in_dll(_core_graphics, "kCGWindowOwnerPID")
_NUMBER = _void.in_dll(_core_graphics, "kCGWindowNumber")
_NAME = _void.in_dll(_core_graphics, "kCGWindowName")
_LAYER = _void.in_dll(_core_graphics, "kCGWindowLayer")
_BOUNDS = _void.in_dll(_core_graphics, "kCGWindowBounds")

# The numbers of the calls: "only the windows on the screen, without the desktop", the
# type of a whole number in a CFNumber, UTF-8 for a CFString, the place in the event
# stream where a key or a mouse movement enters as if it came from the device, the event
# types, the fields of the mouse movement and the flag of a held shift key.
_ON_SCREEN_ONLY = 1 << 0
_EXCLUDE_DESKTOP = 1 << 4
_NUMBER_SINT64 = 4
_UTF8 = 0x08000100
_HID_EVENT_TAP = 0
_MOUSE_MOVED = 5
_MOUSE_DELTA_X = 4
_MOUSE_DELTA_Y = 5
_SHIFT_FLAG = 1 << 17
# NSApplicationActivateAllWindows: the game has one window, so it changes nothing, but
# the call wants some option.
_ACTIVATE_ALL_WINDOWS = 1 << 0

# The virtual key codes of macOS for the keys the steps use: like the scan codes of
# Windows they name the position of a key on the keyboard, not its letter.
KEY_CODES = {"W": 13, "S": 1, "E": 14, "M": 46, "SHIFT": 56, "TAB": 48, "ENTER": 36}

# How far the camera turns for one unit of mouse movement: the same as on Windows, 10
# units a degree at mouse_sensitivity 4 (tools/showcase_windows.py). GLFW hands the game
# the delta of a mouse event (deltaX of NSEvent, in points, so a Retina screen changes
# nothing), and an event posted here arrives with the numbers it was given, without the
# acceleration of a real mouse. Measured on a Mac with macOS 26: a round started with
# --start-yaw 0 and turned by 900 units gives the same picture as one started with
# --start-yaw 90, whether the 900 units go in pieces of 5, 30 or 100.
UNITS_PER_DEGREE = 10


@dataclass
class Window:
    # The game window: the process it belongs to and its number in the window server,
    # which is what screencapture is given.
    pid: int
    number: int
    # Where the mouse pointer was when the game started, and whether the first mouse
    # movement has been sent yet (send_move says why that matters).
    pointer_at_start: tuple
    moved: bool = False


def prepare():
    # Nothing to switch: CoreGraphics counts in points everywhere, and the picture is
    # made 1280 x 720 again in grab.
    pass


def input_allowed():
    # Whether macOS lets this process post keys and mouse movements (Accessibility).
    # Without that permission CGEventPost drops them silently, and a walk would stand still.
    if CGPreflightPostEventAccess():
        return True, ""
    return False, ("macOS does not let this program send keys and mouse movements: give the "
                   "terminal the Accessibility permission (System Settings, Privacy and "
                   "Security, Accessibility) and run it again")


def _number(dictionary, key):
    value = CFDictionaryGetValue(dictionary, key)
    number = ctypes.c_int64()
    if not value or not CFNumberGetValue(value, _NUMBER_SINT64, ctypes.byref(number)):
        return None
    return number.value


def _text(dictionary, key):
    value = CFDictionaryGetValue(dictionary, key)
    buffer = ctypes.create_string_buffer(512)
    if not value or not CFStringGetCString(value, buffer, len(buffer), _UTF8):
        return ""
    return buffer.value.decode("utf-8")


def _windows():
    # Every window on the screen, the front one first: (pid, number, name, layer, rect).
    found = []
    listing = CGWindowListCopyWindowInfo(_ON_SCREEN_ONLY | _EXCLUDE_DESKTOP, 0)
    if not listing:
        return found
    try:
        for index in range(CFArrayGetCount(listing)):
            entry = CFArrayGetValueAtIndex(listing, index)
            rectangle = _Rect()
            bounds = CFDictionaryGetValue(entry, _BOUNDS)
            if bounds:
                CGRectMakeWithDictionaryRepresentation(bounds, ctypes.byref(rectangle))
            found.append((_number(entry, _OWNER_PID), _number(entry, _NUMBER),
                          _text(entry, _NAME), _number(entry, _LAYER), rectangle))
    finally:
        CFRelease(listing)
    return found


def _rectangle(window):
    # Where the window is, in points, with the top left corner of the main screen as 0, 0
    # (the title bar included). None when it is not on the screen.
    for pid, number, _name, _layer, rectangle in _windows():
        if pid == window.pid and number == window.number:
            return rectangle
    return None


def find_window(process_id, pointer_at_start):
    # The window with the title of the game that belongs to OUR game process, on the
    # normal layer (0) of windows. Another copy of the game is never touched.
    for pid, number, name, layer, _rectangle in _windows():
        if pid == process_id and layer == 0 and name == WINDOW_TITLE:
            return Window(pid, number, pointer_at_start)
    return None


def show(window, _left, _top):
    # Windows moves the window to a fixed place; macOS lets only a program with the
    # Accessibility permission move the window of another one, so it stays where the game
    # put it (the middle of the main screen). It must be on the screen as a whole,
    # though: screencapture would not see a part that is off the screen.
    rectangle = _rectangle(window)
    screen = CGDisplayBounds(CGMainDisplayID())
    if rectangle is None:
        raise RuntimeError("the game window is not on the screen")
    right = rectangle.origin.x + rectangle.size.width
    bottom = rectangle.origin.y + rectangle.size.height
    inside = (rectangle.origin.x >= screen.origin.x and rectangle.origin.y >= screen.origin.y
              and right <= screen.origin.x + screen.size.width
              and bottom <= screen.origin.y + screen.size.height)
    if not inside:
        raise RuntimeError("the game window is not wholly on the main screen")


def _front_pid():
    # The process of the active program. Asked of lsappinfo, the tool of macOS for that
    # question, because NSWorkspace only learns about a change in a program with a run
    # loop, which this script does not have. Two questions, a few milliseconds each:
    # which program is in front (an ASN, the name LaunchServices gives a running
    # program, such as "ASN:0x0-0x297297:"), and which process that program is.
    front = subprocess.run(["lsappinfo", "front"], capture_output=True, text=True,
                           check=False).stdout.strip()
    if not front.startswith("ASN:"):
        return None
    answer = subprocess.run(["lsappinfo", "info", "-only", "pid", front],
                            capture_output=True, text=True, check=False).stdout
    # The answer reads "pid"=12345.
    _, _, number = answer.strip().partition("=")
    return int(number) if number.isdigit() else None


def is_front(window):
    return _front_pid() == window.pid


def _send(receiver, selector, result, *arguments, stret=False):
    # One Objective C message: objc_msgSend with the types of exactly this call.
    send = _objc.objc_msgSend_stret if stret else _objc.objc_msgSend
    send.restype = result
    send.argtypes = [_void, _void] + [type(argument) for argument in arguments]
    return send(receiver, sel_registerName(selector), *arguments)


def bring_to_front(window):
    # A round pauses when its window is not the active one, so the game is asked to the
    # front through NSRunningApplication, as clicking its icon in the Dock would. No key
    # and no click is sent. A game started from the terminal usually comes to the front
    # by itself; this is for the times it does not.
    if is_front(window):
        return
    application = _send(objc_getClass(b"NSRunningApplication"),
                        b"runningApplicationWithProcessIdentifier:", _void,
                        ctypes.c_int32(window.pid))
    if application:
        _send(application, b"activateWithOptions:", ctypes.c_bool,
              ctypes.c_ulong(_ACTIVATE_ALL_WINDOWS))
    waited_from = time.monotonic()
    while not is_front(window) and time.monotonic() - waited_from < 2.0:
        time.sleep(0.05)


def client_size(window):
    # The size of the inside of the window in points. The window server only knows the
    # window with its title bar, which lies above the inside, so the bar is taken off.
    rectangle = _rectangle(window)
    if rectangle is None:
        return 0, 0
    return round(rectangle.size.width), round(rectangle.size.height) - _title_bar_height()


# The style of a GLFW window: a title, the three buttons and a frame that can be resized
# (NSWindowStyleMaskTitled, Closable, Miniaturizable, Resizable).
_GLFW_WINDOW_STYLE = 1 | 2 | 4 | 8


def _title_bar_height():
    # How high AppKit makes the title bar of such a window on this Mac (32 points on
    # macOS 26). AppKit answers that without a window of its own: it is the frame it
    # would give an inside of any size, minus that size.
    inside = _Rect(_Point(0, 0), _Size(100, 100))
    # A message that returns a rectangle (four doubles) goes through objc_msgSend_stret
    # on an Intel Mac and through objc_msgSend on Apple silicon.
    stret = platform.machine() == "x86_64"
    frame = _send(objc_getClass(b"NSWindow"), b"frameRectForContentRect:styleMask:", _Rect,
                  inside, ctypes.c_ulong(_GLFW_WINDOW_STYLE), stret=stret)
    return round(frame.size.height - inside.size.height)


def screen_corners():
    # The window is not moved on macOS (show), so there is only the place it already has.
    return []


def pointer():
    event = CGEventCreate(None)
    location = CGEventGetLocation(event)
    CFRelease(event)
    return location.x, location.y


def put_pointer(x, y):
    # The game hides the pointer and lets it go with the mouse for as long as a round
    # runs (GLFW: CGAssociateMouseAndMouseCursorPosition(false)). After the game has
    # ended the pointer moves with the mouse again in any case; this is only so that it
    # is back in its place at once.
    CGAssociateMouseAndMouseCursorPosition(True)
    CGWarpMouseCursorPosition(_Point(x, y))


def pointer_inside(window):
    rectangle = _rectangle(window)
    if rectangle is None:
        return False
    x, y = pointer()
    return (rectangle.origin.x <= x < rectangle.origin.x + rectangle.size.width and
            rectangle.origin.y <= y < rectangle.origin.y + rectangle.size.height)


def move_pointer_away(window):
    # The menu lights up the button under the pointer. Windows moves the game window away
    # from the pointer (capture_showcase.py, Game.place); macOS cannot move it, so the
    # pointer is put just below the window instead, or above it when there is no room
    # below. Says whether it moved the pointer: Game then puts it back at the end.
    rectangle = _rectangle(window)
    if rectangle is None or not pointer_inside(window):
        return False
    screen = CGDisplayBounds(CGMainDisplayID())
    below = rectangle.origin.y + rectangle.size.height + 10
    x = rectangle.origin.x + rectangle.size.width / 2
    y = below if below < screen.origin.y + screen.size.height else rectangle.origin.y - 10
    CGWarpMouseCursorPosition(_Point(x, max(screen.origin.y, y)))
    return True


def grab(window, path):
    # screencapture photographs the one window (-l, its number), without its shadow (-o)
    # and without the camera sound (-x). It sees the window even when another one lies
    # over it, and never the mouse pointer. On a Retina screen the picture has two pixels
    # per point (2560 wide for a window of 1280), with the title bar on top. That bar is
    # cut off and the inside is scaled to WIDTH x HEIGHT, like the picture of Windows,
    # which has one pixel per pixel of the game.
    width, height = WIDTH, HEIGHT
    whole = os.path.join(tempfile.gettempdir(), f"nm-showcase-window-{os.getpid()}.png")
    subprocess.run(["screencapture", "-x", "-o", "-l", str(window.number), whole], check=True)
    try:
        picture_width, picture_height = _picture_size(whole)
        if picture_width % width != 0:
            raise RuntimeError(f"the picture of the window is {picture_width} wide, which is "
                               f"no whole multiple of {width}")
        scale = picture_width // width
        title_bar = picture_height - height * scale
        if title_bar < 0:
            raise RuntimeError(f"the picture of the window is {picture_width} x "
                               f"{picture_height}, lower than the {height} rows of the game")
        # The picture of screencapture carries the colour profile of the screen (Display
        # P3 on a Mac of these years), and ffmpeg would read the numbers as sRGB, which
        # makes them dull. sips turns them into sRGB first, as the game drew them.
        subprocess.run(["sips", "--matchTo", _SRGB_PROFILE, whole, "--out", whole],
                       check=True, capture_output=True)
        # lanczos: the scaler that keeps the edges of text sharp when halving.
        subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-i", whole,
                        "-vf", f"crop={width * scale}:{height * scale}:0:{title_bar},"
                        f"scale={width}:{height}:flags=lanczos,format=rgb24",
                        "-frames:v", "1", "-update", "1", path], check=True)
    finally:
        if os.path.exists(whole):
            os.remove(whole)
    print("wrote", path)


_SRGB_PROFILE = "/System/Library/ColorSync/Profiles/sRGB Profile.icc"


def _picture_size(path):
    answer = subprocess.run(["sips", "-g", "pixelWidth", "-g", "pixelHeight", path],
                            capture_output=True, text=True, check=True).stdout
    sizes = {}
    for line in answer.splitlines():
        name, _, value = line.strip().partition(": ")
        if name in ("pixelWidth", "pixelHeight"):
            sizes[name] = int(value)
    return sizes["pixelWidth"], sizes["pixelHeight"]


# ---- keys and mouse -----------------------------------------------------------------------

# Which modifier flags are down: a key event carries them, and GLFW reads a held shift
# from the flags of the event, not from its key code alone.
_flags = 0


def send_key(name, up):
    global _flags
    event = CGEventCreateKeyboardEvent(None, KEY_CODES[name], not up)
    if name == "SHIFT":
        _flags = 0 if up else _SHIFT_FLAG
    CGEventSetFlags(event, _flags)
    CGEventPost(_HID_EVENT_TAP, event)
    CFRelease(event)


def _inside_middle(window):
    # The middle of the inside of the window (below the title bar), in points: where
    # GLFW keeps the hidden pointer during a round.
    rectangle = _rectangle(window)
    if rectangle is None:
        raise RuntimeError("the game window is not on the screen")
    bar = _title_bar_height()
    return (rectangle.origin.x + rectangle.size.width / 2,
            rectangle.origin.y + bar + (rectangle.size.height - bar) / 2)


def send_move(window, dx, dy):
    # A mouse movement of dx, dy units. GLFW gives the game the two delta fields of the
    # event; the place of the event is the middle of the window, where the pointer is
    # kept anyway.
    #
    # The first movement of a round carries one more thing. When a round takes the
    # pointer, GLFW puts it in the middle of the window and remembers that jump (from
    # wherever the pointer was), expecting the next movement of the mouse to report it
    # too, and takes it off that movement. A posted event does not report it, so without
    # this the first turn of a run came out short by the way from the pointer to the
    # middle: a turn of 90 degrees ended near 60 when the pointer had been in a corner.
    # So the jump is added to the first event, and GLFW takes it off again.
    middle_x, middle_y = _inside_middle(window)
    if not window.moved:
        start_x, start_y = window.pointer_at_start
        dx += round(middle_x - start_x)
        dy += round(middle_y - start_y)
        window.moved = True
    event = CGEventCreateMouseEvent(None, _MOUSE_MOVED, _Point(middle_x, middle_y), 0)
    CGEventSetIntegerValueField(event, _MOUSE_DELTA_X, dx)
    CGEventSetIntegerValueField(event, _MOUSE_DELTA_Y, dy)
    CGEventSetFlags(event, _flags)
    CGEventPost(_HID_EVENT_TAP, event)
    CFRelease(event)
