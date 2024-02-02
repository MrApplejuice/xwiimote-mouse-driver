"""
This file is part of xwiimote-mouse-driver.

xwiimote-mouse-driver is free software: you can redistribute it and/or modify it under 
the terms of the GNU General Public License as published by the Free 
Software Foundation, either version 3 of the License, or (at your option) 
any later version.

xwiimote-mouse-driver is distributed in the hope that it will be useful, but WITHOUT 
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with 
xwiimote-mouse-driver. If not, see <https://www.gnu.org/licenses/>. 
"""

import sys
from typing import Any, Tuple, Optional, List, Union, Callable
import tkinter as tk
import tkinter.font as tkf
from tkinter import ttk
import socket
from dataclasses import dataclass
from collections import OrderedDict

import numpy as np
import argparse

import lzma
import base64
import struct
import time
import webbrowser


WIIMOTE_BUTTON_READABLE_NAMES = OrderedDict(
    [
        ("a", "A"),
        ("b", "B"),
        ("+", "Plus"),
        ("-", "Minus"),
        ("h", "Home"),
        ("1", "1"),
        ("2", "2"),
        ("u", "Up"),
        ("d", "Down"),
        ("l", "Left"),
        ("r", "Right"),
    ]
)


@dataclass
class OpenCommand:
    start_time: int
    name: str
    params: Tuple[Any]
    sock: socket.socket
    callback: Any = None


class WiimoteSocketReader:
    MAX_CLOSED_COMMANDS = 128

    def __init__(self, socket_path, tcl_root, notify_callback):
        self.poll_interval = 10

        self.socket_path = socket_path
        self.root = tcl_root
        self.notify_callback = notify_callback

        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(self.socket_path)
        self.sock.settimeout(1)

        self.lr_vectors = None
        self.ir_vectors = [None] * 4
        self.pressed_buttons = []

        self.open_commands = []
        self.closed_commands = []

        self.timer_id = self.root.after(self.poll_interval, self.process)

    def process_message(self, message: str):
        message_parts = message.strip().split(":")
        if message_parts[0] == "flr":
            if message_parts[1] == "invalid":
                self.lr_vectors = None
            else:
                self.lr_vectors = np.array([int(x) for x in message_parts[1:]]).reshape(
                    (2, 2)
                )
        elif message_parts[0] in ["lr", "flr"]:
            pass
        elif message_parts[0] == "ir":
            index = int(message_parts[1])
            valid = bool(int(message_parts[2]))
            if not valid:
                self.ir_vectors[index] = None
            else:
                self.ir_vectors[index] = np.array([int(x) for x in message_parts[3:5]])
        elif message_parts[0] == "b":
            self.pressed_buttons = [m for m in message_parts[1:] if m]
        else:
            print(f"Unknown message: {message}")

    def send_message(self, name: str, *params, callback=None):
        req_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        req_sock.connect(self.socket_path)
        req_sock.settimeout(0.2)

        self.open_commands.append(
            OpenCommand(time.time(), name, params, req_sock, callback)
        )

        data = (":".join([name] + [str(x) for x in params]) + "\n").encode()
        padding = b"\0" * 1024
        req_sock.sendall(data + padding)

    def decode_in_buffer(self, buffer):
        for line in buffer.decode().split("\n"):
            line = line.strip()
            if line:
                yield line

    def read_input_socket(self):
        try:
            data = self.sock.recv(1024)
            if not data:
                return
            for message in self.decode_in_buffer(data):
                self.process_message(message)
        except IOError as e:
            print(f"Error reading main socket: {e}")

    def process_response_message(self, cmd: OpenCommand, message: str):
        message_parts = message.strip().split(":")
        if message_parts[0] in ("ERROR", "OK"):
            self.open_commands.remove(cmd)
            cmd.sock.close()
            self.closed_commands.insert(0, (cmd, message_parts[0]))
            while len(self.closed_commands) > self.MAX_CLOSED_COMMANDS:
                self.closed_commands.pop()

            if cmd.callback:
                cmd.callback(message_parts[0], message_parts[1:])

    def receive_open_command_responses(self):
        for cmd in list(self.open_commands):
            try:
                data = cmd.sock.recv(1024)
                if not data:
                    continue
                for message in self.decode_in_buffer(data):
                    self.process_response_message(cmd, message)
            except IOError as e:
                print(f"Error reading response for: {cmd.name}: {e}")

        self.closed_commands = self.closed_commands[-self.MAX_CLOSED_COMMANDS :]

    def process(self):
        try:
            self.read_input_socket()
            self.receive_open_command_responses()
            self.notify_callback()
        finally:
            self.timer_id = self.root.after(self.poll_interval, self.process)

    def close(self):
        if self.timer_id is not None:
            self.root.after_cancel(self.timer_id)
            self.timer_id = None

            self.sock.close()
            for cmd in self.open_commands:
                cmd.sock.close()
            self.open_commands.clear()


def draw_cross(canvas, x, y) -> Tuple[int, int]:
    return (
        canvas.create_line(x - 10, y, x + 10, y, width=2),
        canvas.create_line(x, y - 10, x, y + 10, width=2),
    )


class LRCrosses:
    def __init__(self, canvas):
        self.canvas = canvas
        self.crosses = [None] * 2

    def update(self, lr_vectors):
        for drawelements in self.crosses:
            if drawelements is not None:
                for d in drawelements:
                    self.canvas.delete(d)

        if lr_vectors is None:
            self.crosses = [None] * 2
            return

        for i in range(2):
            # [1024, 768] seems to be the (logical) measured size of the
            # wiimote sensor, actually measured was 760 vertical.
            # Not strictly needed here, but makes for a nicer
            # visualization
            x, y = (
                lr_vectors[i]
                * [self.canvas.winfo_width(), self.canvas.winfo_height()]
                / [1024, 768]
            )
            self.crosses[i] = draw_cross(self.canvas, x, self.canvas.winfo_height() - y)


def get_screen_resolution(root):
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()
    return screen_width, screen_height


def parse_color_string(color_str: str) -> Tuple[int, int, int]:
    if color_str.startswith("#"):
        color_str = color_str[1:]

    r = int(color_str[0:2], 16)
    g = int(color_str[2:4], 16)
    b = int(color_str[4:6], 16)

    return r, g, b


def embedded_image(decompressed_embedded_image, root_bg_color):
    img = tk.PhotoImage(
        width=len(decompressed_embedded_image[0]),
        height=len(decompressed_embedded_image),
    )

    try:

        def setalpha(x, y, alpha):
            img.tk.call(img.name, "transparency", "set", x, y, alpha, "-alpha")

        def to_hexcolor(r, g, b, a):
            return f"#{r:02x}{g:02x}{b:02x}"

        setalpha(0, 0, 0)
    except tk.TclError:
        # We are using a tk version that does not support setting linear alpha
        # ... we accept this and use binary alpha
        def setalpha(x, y, alpha):
            img.transparency_set(x, y, alpha <= 0)

        def to_hexcolor(r, g, b, a):
            blend_value = sum(root_bg_color) // len(root_bg_color) * (255 - a)
            r = (r * a + blend_value) // 255
            g = (g * a + blend_value) // 255
            b = (b * a + blend_value) // 255
            return f"#{r:02x}{g:02x}{b:02x}"

    for y, row in enumerate(decompressed_embedded_image):
        for x, pixel in enumerate(row):
            img.put(to_hexcolor(*pixel), (x, y))
            setalpha(x, y, pixel[3])

    return img


@dataclass
class KeybindingOption:
    raw_key_name: str
    name: Optional[str]
    category: str

    @property
    def display_name(self):
        name = self.name or self.raw_key_name
        if name == "None":
            return "[Unassigned]"
        return f"{name} ({self.category})"


class KeybindingsComboboxes:
    values: List[KeybindingOption]
    on_new_mapping_selected: Optional[Callable[[str, str, bool], None]] = None

    __internal_update = 0

    def __init__(self):
        self.values = [KeybindingOption("None", None, "None")]
        self.on_screen_boxes = {}
        self.off_screen_boxes = {}

        self.on_screen_selections = {
            k: self.values[0] for k in WIIMOTE_BUTTON_READABLE_NAMES.keys()
        }
        self.off_screen_selections = dict(self.on_screen_selections)

    def __wrap_internal(self, func, *args, **kwargs):
        self.__internal_update += 1
        try:
            return func(*args, **kwargs)
        finally:
            self.__internal_update -= 1

    @property
    def __combobox_options(self):
        return [v.display_name for v in self.values]

    def __update_selections(self):
        for btn in WIIMOTE_BUTTON_READABLE_NAMES.keys():
            if btn in self.on_screen_boxes:
                self.on_screen_boxes[btn].current(
                    self.values.index(self.on_screen_selections[btn])
                )
            if btn in self.off_screen_boxes:
                self.off_screen_boxes[btn].current(
                    self.values.index(self.off_screen_selections[btn])
                )

    def __find_closest_match(self, combo_value: str) -> Optional[KeybindingOption]:
        combo_value = combo_value.strip().lower()
        found = None
        if combo_value == "":
            found = self.values[0]
        else:
            for keyValue in self.values:
                if keyValue.name and keyValue.name.lower() == combo_value:
                    found = keyValue
                if (
                    keyValue.raw_key_name
                    and keyValue.raw_key_name.lower() == combo_value
                ):
                    found = keyValue
                if keyValue.display_name.lower() == combo_value:
                    found = keyValue
                if found:
                    break
        return found

    def __selection_changed(
        self, wii_button: str, on_screen: bool, combobox: ttk.Combobox, event
    ):
        found = self.__find_closest_match(combobox.get())
        if not found:
            pass
        if on_screen:
            self.on_screen_selections[wii_button] = found
        else:
            self.off_screen_selections[wii_button] = found

        if self.__internal_update == 0:
            if self.on_new_mapping_selected:
                self.on_new_mapping_selected(wii_button, found.raw_key_name, on_screen)

    def __validate_match(self, wii_button: str, on_screen: bool, combobox):
        found = self.__find_closest_match(combobox.get())
        if found is None:
            return False

        if on_screen:
            self.on_screen_selections[wii_button] = found
        else:
            self.off_screen_selections[wii_button] = found
        self.__update_selections()

        if self.__internal_update == 0:
            if self.on_new_mapping_selected:
                self.on_new_mapping_selected(wii_button, found.raw_key_name, on_screen)
        return True

    def add_combobox(self, wii_button: str, on_screen: bool, combobox: ttk.Combobox):
        combobox.config(
            values=self.__combobox_options,
            validate="focusout",
            validatecommand=lambda: self.__validate_match(
                wii_button, on_screen, combobox
            ),
        )

        combobox.bind(
            "<<ComboboxSelected>>",
            lambda event: self.__selection_changed(
                wii_button, on_screen, combobox, event
            ),
        )
        combobox.bind(
            "<Return>",
            lambda event: self.__validate_match(wii_button, on_screen, combobox),
        )

        def deferred_select_all(_):
            combobox.after_idle(lambda: combobox.select_range(0, tk.END))

        combobox.bind("<FocusIn>", deferred_select_all)
        combobox.bind("<Control-a>", deferred_select_all)

        if on_screen:
            self.on_screen_boxes[wii_button] = combobox
        else:
            self.off_screen_boxes[wii_button] = combobox

        self.__wrap_internal(self.__update_selections)

    def add_value(self, option: KeybindingOption):
        self.values.append(option)

        for combobox in self.on_screen_boxes.values():
            combobox.config(values=self.__combobox_options)
        for combobox in self.off_screen_boxes.values():
            combobox.config(values=self.__combobox_options)

        self.__wrap_internal(self.__update_selections)

    def set_mapping(
        self,
        wii_button: str,
        on_screen: bool,
        option: Union[KeybindingOption, str, None],
    ):
        if isinstance(option, str):
            option = next((v for v in self.values if v.raw_key_name == option), None)
            if option is None:
                print(f"Unknown key name: {option}", file=sys.stderr)
        if option is None:
            option = self.values[0]

        if on_screen:
            self.on_screen_selections[wii_button] = option
        else:
            self.off_screen_selections[wii_button] = option

        self.__wrap_internal(self.__update_selections)


class Window:
    on_screen_area_updated = None
    on_smoothing_parameters_edited = None
    smoothing_parameters_enabled = False

    screen_area_values = (0, 0, 100, 100)

    def parse_screen_area_values(self):
        values = []
        for v in self.screen_area_text_values:
            try:
                value = int(v.get().strip("%"))
            except ValueError:
                self.screen_area_box.config(bg="red")
                return
            values.append(value)

        self.screen_area_box.config(bg=self.root.cget("bg"))

        self.screen_area_values = tuple(values)
        if self.on_screen_area_updated:
            self.on_screen_area_updated(self.screen_area_values)

    def __init__(self, root: tk.Tk):
        self.root = root
        root_background_color = parse_color_string(root.cget("background"))

        bold_font = tkf.Font(weight="bold")

        self.refs = []

        notebook = ttk.Notebook(root)
        notebook.pack(fill=tk.BOTH, expand=True)

        calibration_main = tk.Frame(notebook)
        calibration_main.pack()
        notebook.add(calibration_main, text="Calibration")

        # Frame for IR canvas and buttons
        calibration_frame = tk.Frame(calibration_main)
        calibration_frame.pack(fill=tk.BOTH, expand=True)

        # "IR canvas"
        ir_frame = tk.Frame(calibration_frame)
        ir_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=False)

        self.canvas = canvas = tk.Canvas(ir_frame, width=400, height=400 * 768 // 1024)
        canvas.config(bg="gray")
        canvas.pack()
        self.crosses = LRCrosses(canvas)

        # Add text box
        self.text_box = text_box = tk.Label(
            ir_frame, justify=tk.CENTER, font=("Arial", 12, "italic")
        )
        text_box.config(text="...", wraplength=400)
        text_box.pack()

        # Add buttons
        button_frame = tk.Frame(calibration_frame)
        button_frame.pack(pady=10, padx=10, fill=tk.X)

        button1 = tk.Button(button_frame, text="Start calibration (1)")
        button1.pack(side=tk.TOP, fill=tk.X, pady=5, padx=0)
        self.btn_start_calibration = button1

        button2 = tk.Button(button_frame, text="Enable mouse (2)")
        button2.pack(side=tk.TOP, fill=tk.X, pady=5, padx=0)
        self.btn_enable_mouse = button2

        button3 = tk.Button(button_frame, text="Help")
        button3.pack(side=tk.TOP, fill=tk.X, pady=5, padx=0)
        self.btn_help = button3

        # Screen subregion controls
        self.screen_area_box = screen_area_box = tk.Frame(calibration_frame)
        screen_area_box.pack(side=tk.TOP, pady=20)

        # Add label
        label = tk.Label(screen_area_box, text="Configure screen subregion")
        label.grid(row=0, columnspan=3)

        # Left edit box
        self.screen_area_text_values = [
            tk.StringVar(),
            tk.StringVar(),
            tk.StringVar(),
            tk.StringVar(),
        ]
        for i, v in enumerate(self.screen_area_values):
            self.screen_area_text_values[i].set(str(v) + "%")
            self.screen_area_text_values[i].trace_add(
                "write", lambda *args: self.parse_screen_area_values()
            )

        left_entry = tk.Entry(
            screen_area_box, width=5, textvariable=self.screen_area_text_values[0]
        )
        left_entry.grid(row=2, column=0)

        # Right edit box
        self.right_entry = tk.Entry(
            screen_area_box, width=5, textvariable=self.screen_area_text_values[2]
        )
        self.right_entry.grid(row=2, column=2)

        # Top edit box
        self.top_entry = tk.Entry(
            screen_area_box, width=5, textvariable=self.screen_area_text_values[1]
        )
        self.top_entry.grid(row=1, column=1)

        # Bottom edit box
        self.bottom_entry = tk.Entry(
            screen_area_box, width=5, textvariable=self.screen_area_text_values[3]
        )
        self.bottom_entry.grid(row=3, column=1)

        keybindings_main = tk.Frame(notebook)
        keybindings_main.pack()
        notebook.add(keybindings_main, text="Keybindings")

        key_canvas = tk.Canvas(
            keybindings_main, width=len(WIIMOTE_IMG[0]), height=len(WIIMOTE_IMG)
        )
        key_canvas.pack(side=tk.LEFT, fill=tk.Y, expand=False, padx=10)

        img = embedded_image(WIIMOTE_IMG, root_background_color)
        self.refs.append(img)
        key_canvas.create_image(0, 0, anchor=tk.NW, image=img)

        button_assignments_grid = tk.Frame(keybindings_main)
        button_assignments_grid.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, pady=10)

        label = tk.Label(button_assignments_grid, text="On screen")
        label.config(font=bold_font)
        label.grid(row=0, column=1, padx=20, pady=5)

        label = tk.Label(button_assignments_grid, text="Off screen")
        label.config(font=bold_font)
        label.grid(row=0, column=2, padx=20, pady=5)

        self.keybindings = keybindings = KeybindingsComboboxes()
        for i, wii_btn in enumerate(WIIMOTE_BUTTON_READABLE_NAMES.keys()):
            label = tk.Label(
                button_assignments_grid, text=WIIMOTE_BUTTON_READABLE_NAMES[wii_btn]
            )
            label.grid(row=i + 1, column=0, sticky=tk.W, padx=20, pady=3)

            for col, on_screen in ((1, True), (2, False)):
                combobox = ttk.Combobox(button_assignments_grid, values=[""])
                combobox.grid(row=i + 1, column=col, padx=3, pady=3)
                keybindings.add_combobox(wii_btn, on_screen, combobox)

        advanced_main = tk.Frame(notebook)
        advanced_main.pack()
        notebook.add(advanced_main, text="Advanced")

        advanced_col = tk.Frame(advanced_main)
        advanced_col.grid(row=0, column=0, sticky=tk.NW, padx=10, pady=10)
        row_i = 0

        def make_row(label, control):
            nonlocal row_i

            label = tk.Label(advanced_col, text=label)
            label.grid(row=row_i, column=0, sticky=tk.W)
            control.grid(row=row_i, column=1, sticky=tk.W, padx=10)
            row_i += 1
            return control

        subcaption_font = ("Arial", 12, "italic")
        label = tk.Label(advanced_col, text="Movement smoothing", font=subcaption_font)
        label.grid(row=row_i, columnspan=2, sticky=tk.W, pady=10)
        row_i += 1

        self.smoothing_pressed_var = tk.DoubleVar(value=1.0)
        make_row(
            "log10 pressed",
            tk.Entry(advanced_col, textvariable=self.smoothing_pressed_var),
        )

        self.smoothing_released_var = tk.DoubleVar(value=1.0)
        make_row(
            "log10 released",
            tk.Entry(advanced_col, textvariable=self.smoothing_released_var),
        )

        self.smoothing_click_freeze_var = tk.DoubleVar(value=1.0)
        make_row(
            "Click freeze (s)",
            tk.Entry(advanced_col, textvariable=self.smoothing_click_freeze_var),
        )

        def smoothing_callback():
            if (
                self.on_smoothing_parameters_edited
                and self.smoothing_parameters_enabled
            ):
                self.on_smoothing_parameters_edited(
                    self.smoothing_pressed_var.get(),
                    self.smoothing_released_var.get(),
                    self.smoothing_click_freeze_var.get(),
                )

        for v in [
            self.smoothing_released_var,
            self.smoothing_pressed_var,
            self.smoothing_click_freeze_var,
        ]:
            v.trace_add("write", lambda *args: smoothing_callback())

        label = tk.Label(advanced_col, text="Towed Circle", font=subcaption_font)
        label.grid(row=row_i, columnspan=2, sticky=tk.W, pady=10)
        row_i += 1

        self.towed_circle_radius_var = tk.DoubleVar(value=0.0)
        make_row(
            "Radius",
            tk.Entry(advanced_col, textvariable=self.towed_circle_radius_var),
        )


class Logic:
    def __init__(self, win: Window, wiimote: WiimoteSocketReader):
        self.win = win
        self.wiimote = wiimote

    def on_wii_button_pressed(self, button_name: str):
        pass

    def start(self):
        pass

    def process_socket_data(self):
        pass

    def stop(self):
        pass


class IdleLogic(Logic):
    on_start_calibration = None
    on_toggle_mouse = None

    def start(self):
        self.wiimote.send_message("calibration", "off")

    def on_wii_button_pressed(self, button_name: str):
        if button_name == "1":
            if self.on_start_calibration:
                self.on_start_calibration()
        if button_name == "2":
            if self.on_toggle_mouse:
                self.on_toggle_mouse()

    def process_socket_data(self):
        if self.wiimote.lr_vectors is None:
            self.win.canvas.config(bg="red")
            self.win.text_box.config(text="Sensor bar not visible")
        elif self.wiimote.lr_vectors[0].tolist() == self.wiimote.lr_vectors[1].tolist():
            self.win.canvas.config(bg="yellow")
            self.win.text_box.config(text="Sensor bar only partially visible")
        else:
            self.win.canvas.config(bg="green")
            self.win.text_box.config(text="Ready to calibrate")
        self.win.crosses.update(self.wiimote.lr_vectors)


class ClosingLogic(Logic):
    start_time = None

    def start(self):
        self.start_time = time.time()
        self.wiimote.send_message("mouse", "on")
        self.wiimote.send_message("calibration", "off")

    def process_socket_data(self):
        print("Waiting for mouse to be turned on again...")
        if (not self.wiimote.open_commands) or (time.time() - self.start_time > 20):
            self.win.root.destroy()


class CalibrationLogic(Logic):
    on_completed = None
    on_exit = None

    STEPS = [
        "Point the wiimote directly at the sensorbar, then press A; press 1 to cancel",
        "Point the wiimote to the top-left corner of the screen, then press A; press 1 to cancel",
        "Point the wiimote to the top-right corner of the screen, then press A; press 1 to cancel",
        "Point the wiimote to the bottom-left corner of the screen, then press A; press 1 to cancel",
        "Point the wiimote to the bottom-right corner of the screen, then press A; press 1 to cancel",
        "Calibration completed",
    ]

    def start(self):
        self.error_start_time = None
        self.step_data = []
        self.win.btn_start_calibration.config(
            highlightthickness=5,
            highlightcolor="blue",
            highlightbackground="blue",
            pady=0,
            padx=0,
        )
        self.win.btn_start_calibration.pack(side=tk.TOP, fill=tk.X, pady=5, padx=0)

    def on_wii_button_pressed(self, button_name: str):
        if button_name == "1":
            if self.on_exit:
                self.on_exit()

        if self.wiimote.lr_vectors is None:
            msg = (
                "ERROR: Sensor bar not visible! Make sure that the sensor "
                "bar is turned on and in view of the wiimote."
            )
            self.error_start_time = time.time()
            self.win.text_box.config(text=msg)
            return

        if button_name == "a":
            self.step_data.append(self.wiimote.lr_vectors)

    def process_socket_data(self):
        if self.error_start_time is not None:
            if time.time() - self.error_start_time > 5:
                self.error_start_time = None
        else:
            self.win.text_box.config(text=self.STEPS[len(self.step_data)])
        if len(self.step_data) == len(self.STEPS) - 1:
            if self.on_completed:
                self.on_completed()
            if self.on_exit:
                self.on_exit()
            return

        if self.wiimote.lr_vectors is None:
            self.win.canvas.config(bg="red")
        elif self.wiimote.lr_vectors[0].tolist() == self.wiimote.lr_vectors[1].tolist():
            self.win.canvas.config(bg="yellow")
        else:
            self.win.canvas.config(bg="green")
        self.win.crosses.update(self.wiimote.lr_vectors)

    def stop(self):
        self.win.btn_start_calibration.config(highlightthickness=0, pady=5, padx=5)
        self.win.btn_start_calibration.pack(side=tk.TOP, fill=tk.X, pady=5, padx=0)


def query_keybindings(window: Window, wiimote: WiimoteSocketReader):
    def query_keymap():
        def received(reps, args):
            if reps != "OK":
                print("ERROR getting keymap: ", args)
                return

            pairs = zip(args[::3], args[1::3], args[2::3])
            for wiiButtonName, irState, targetButtonName in pairs:
                irState = bool(int(irState))
                window.keybindings.set_mapping(
                    wiiButtonName,
                    irState,
                    targetButtonName,
                )

        wiimote.send_message("keymapget", callback=received)

    def query_keys(wiimote: WiimoteSocketReader):
        BATCH_SIZE = 10

        current_batch = []
        batch_index = 0
        key_count = 0

        def query_key(i: int):
            current_batch.append(i)

            def received(reps, args):
                current_batch.remove(i)
                query_next_batch()
                if reps != "OK":
                    print("ERROR getting key index: ", i)
                    return

                def filter_none(x):
                    if x == "":
                        return None
                    return x

                kb = KeybindingOption(*(filter_none(x) for x in args))
                window.keybindings.add_value(kb)

            wiimote.send_message("keyget", i, callback=received)

        def query_next_batch():
            nonlocal batch_index

            if current_batch:
                return

            while (batch_index < key_count) and (len(current_batch) < BATCH_SIZE):
                query_key(batch_index)
                batch_index += 1

            if len(current_batch) == 0:
                query_keymap()

        def count_received(resp, args):
            nonlocal key_count

            if resp != "OK":
                print("error getting key count: ", args)
                return

            key_count = int(args[0])
            query_next_batch()

        wiimote.send_message("keycount", callback=count_received)

    query_keys(wiimote)


def query_smoothing_factors(window: Window, wiimote: WiimoteSocketReader):
    def received(reps, args):
        if reps != "OK":
            print("ERROR getting smoothing factors: ", args)
            return

        window.smoothing_pressed_var.set(int(args[0]) / 100)
        window.smoothing_released_var.set(int(args[1]) / 100)
        window.smoothing_click_freeze_var.set(int(args[2]) / 100000)
        window.smoothing_parameters_enabled = True

    wiimote.send_message("getsmoothing100", callback=received)


def query_towed_circle_radius(window: Window, wiimote: WiimoteSocketReader):
    def received(reps, args):
        if reps != "OK":
            print("ERROR getting towed circle radius: ", args)
            return

        window.towed_circle_radius_var.set(int(args[0]) / 10000)

        def update_value():
            wiimote.send_message(
                "settcradius10000", int(window.towed_circle_radius_var.get() * 10000)
            )

        window.towed_circle_radius_var.trace_add("write", lambda *args: update_value())

    wiimote.send_message("gettcradius10000", callback=received)


FREE_SORFTWARE_COPTYRIGHT_NOTICE = """
Copyright (C) 2024  Paul Konstantin Gerke

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
""".lstrip()


def main():
    parser = argparse.ArgumentParser(
        prog="Wiimote mouse configurator",
        description="Configuration tool for the xwiimote-mouse-driver",
    )

    parser.add_argument(
        "--socket-path",
        type=str,
        default="wiimote-mouse.sock",
        help="Path to the socket to use for communication with the driver",
    )

    args = parser.parse_args()

    print(FREE_SORFTWARE_COPTYRIGHT_NOTICE)

    root = tk.Tk()
    root.title("Wiimote mouse configurator")
    root.resizable(False, False)

    current_logic = None

    window = Window(root)
    root.geometry("600x400")

    last_pressed_buttons = []

    def new_socket_data():
        nonlocal current_logic, last_pressed_buttons

        current_logic.process_socket_data()

        if current_logic:
            for b_name in wiimote.pressed_buttons:
                if b_name not in last_pressed_buttons:
                    current_logic.on_wii_button_pressed(b_name)
        last_pressed_buttons = tuple(wiimote.pressed_buttons)

    wiimote = WiimoteSocketReader(args.socket_path, root, new_socket_data)
    wiimote.send_message("mouse", "off")

    query_keybindings(window, wiimote)
    query_smoothing_factors(window, wiimote)
    query_towed_circle_radius(window, wiimote)

    def assign_screenarea(cmd, topLeftBottomRight100):
        if cmd == "ERROR":
            print("Error getting screen area")
            return

        for sv, text in zip(window.screen_area_text_values, topLeftBottomRight100):
            sv.set(str(int(text) // 10000) + "%")

    wiimote.send_message("getscreenarea100", callback=assign_screenarea)

    idle_logic = IdleLogic(window, wiimote)
    current_logic = idle_logic

    def switch_logic(new_logic):
        nonlocal current_logic

        current_logic.stop()
        current_logic = new_logic
        current_logic.start()

    def on_close():
        switch_logic(ClosingLogic(window, wiimote))

    root.protocol("WM_DELETE_WINDOW", on_close)

    def bind_new_key(wii_button: str, target_button: str, on_screen: bool):
        if target_button in ("None", None):
            target_button = ""
        ir = "1" if on_screen else "0"
        wiimote.send_message("bindkey", wii_button, ir, target_button)

    window.keybindings.on_new_mapping_selected = bind_new_key

    def send_calibration_data(calibration_logic):
        lr = np.array(calibration_logic.step_data[0])
        distance = np.linalg.norm(lr[0] - lr[1])
        wiimote.send_message("irdist100", int(distance * 100))

        data = calibration_logic.step_data[1:5]

        wii_corners = [np.mean(d, axis=0) for d in data]
        wii_corner_mat = np.hstack([wii_corners, np.ones([4, 1])])

        corners = np.array(
            [
                [0, 0],
                [10000, 0],
                [0, 10000],
                [10000, 10000],
            ]
        )

        calmat, _, __, ___ = np.linalg.lstsq(wii_corner_mat, corners, rcond=None)

        int64_calmat = (calmat * 100).astype(np.int64).T
        wiimote.send_message("cal100", *int64_calmat.flatten().tolist())

    def on_start_calibration():
        wiimote.send_message("mouse", "off")
        wiimote.send_message("calibration", "on")
        window.btn_enable_mouse.config(relief=tk.RAISED)

        calibration_logic = CalibrationLogic(window, wiimote)
        calibration_logic.on_exit = lambda: switch_logic(idle_logic)
        calibration_logic.on_completed = lambda: send_calibration_data(
            calibration_logic
        )

        switch_logic(calibration_logic)

    window.btn_start_calibration.config(command=on_start_calibration)
    idle_logic.on_start_calibration = on_start_calibration

    def open_help_website():
        website = "https://mrapplejuice.github.io/xwiimote-mouse-driver/#"
        webbrowser.open(website)

    window.btn_help.config(command=open_help_website)

    def on_screen_area_updated(values):
        wiimote.send_message("screenarea100", *[v * 10000 for v in values])

    window.on_screen_area_updated = on_screen_area_updated

    def toggle_mouse_activated():
        new_pressed_state = window.btn_enable_mouse["relief"] == tk.RAISED

        if new_pressed_state:
            wiimote.send_message("mouse", "on")
            window.btn_enable_mouse.config(relief=tk.SUNKEN)
        else:
            wiimote.send_message("mouse", "off")
            window.btn_enable_mouse.config(relief=tk.RAISED)

    window.btn_enable_mouse.config(command=toggle_mouse_activated)
    idle_logic.on_toggle_mouse = toggle_mouse_activated

    def on_smoothing_parameters_edited(pressed, released, click_freeze):
        wiimote.send_message(
            "setsmoothing100",
            int(pressed * 100),
            int(released * 100),
            int(click_freeze * 100000),
        )

    window.on_smoothing_parameters_edited = on_smoothing_parameters_edited

    try:
        root.mainloop()
    finally:
        wiimote.close()


def decompress_embedded_image(data):
    rle_data = lzma.decompress(base64.b85decode(data.strip().encode("ascii")))
    width, height, ncolors = struct.unpack("<HHH", rle_data[:6])
    offset = struct.calcsize("<HHH")

    sample_unpack_str = "B" * ncolors
    sample_size = struct.calcsize("<" + sample_unpack_str)

    flat_image_data = []

    backrefs = []
    while offset < len(rle_data):
        (reps,) = struct.unpack_from("<l", rle_data, offset)
        offset += 4
        if reps < 0:
            reps = -reps
            (ref_index,) = struct.unpack_from("<H", rle_data, offset)
            offset += 2
            sample = backrefs[ref_index]
        else:
            sample = list(struct.unpack_from("<" + sample_unpack_str, rle_data, offset))
            offset += sample_size
            backrefs.append(sample)
        flat_image_data.extend([sample] * reps)

    rows = []
    for y in range(height):
        row = flat_image_data[y * width : (y + 1) * width]
        if len(row) == width:
            rows.append(row)

    return rows


WIIMOTE_IMG = decompress_embedded_image(
    r"{Wp48S^xk9=GL@E0stWa8~^|S5YJf5;D1XMd|d!IfCt+co!^P^Z_C>)-U6$C=!x8-%x>`Q77q"
    r"z@MKwwi!;QE~psJQw0FOQT6c3pqA{?Fg9EW@#m}jqi$B?v}qJ**%5cQXM!!M10UFsc0_L^09?"
    r"T5M;qC)5unLXoK=j&mWy+S4g)Jee?KWz@79j%X4WK3euEJdGcb0RZjPxY<)w5w;sN%@mj^CIK"
    r"hIZ+h}OenkT8w+U90DCk(T1_uwoU&8}+Ls<)orxC;tAH1WY#*5%BABKF9~uV}XYl;4WA81x9i"
    r"#G;(<AGusbpC(--p&H<o8Ou9LX}^!oQm>Py{EpLne7&OcBLlW13Xo<};pv^tZPlSx~i7vY~N9"
    r"9e7<2BHo?r6Fv?$^$$eBA1XQX3_$5<i(5JK&!a`NL#;|5FtY**$sWUjSJR?*Sj)!;V<o*MZ}w"
    r"EM9zaem`-ARoB4YlI3YmyC46phGBTQ&(oA2ZF0pGofk~J>RK`Re6q7p->(SPcc*k6)PEaR~h_"
    r"<owR!WH)^;a?W2Ry;vDey=_~IKgBAd_wBA=l&&NqfY&vpQ`#OU3;;3Gp~YsM7X^1DDqs~{dKt"
    r"&uE{_46)&1kjO0YDyRjms;3*LQuKU$avyp)3)Xl5Z^A7trFndz(K*`Ao>R)D~K>85ihKh~C0~"
    r"2sg5w%hx&R@mf0)Vj_0i!81<9!wp#s*V%E;cGbA9VPbgd^&B;IxCAq^`X}a5YQWuHKCKozc?t"
    r"pmlaWuZ4d}r^~qzgRUTgZ`!_LLN&n#jZMY_Pw&_Kh{J}FyQ4&JRf|B=letM<Cjo`)B6#N$$^a"
    r"lra&e<#qwY8O87tW`z%20aF4^)4snV&B&~W4~9C;NghS}7Ngm(nOTGjRYj+1B!!{AzH6yYhQ^"
    r"QSYdaDmj?5@R}jO>D$^4hvev)!c=_(suj!IhiqEjC!XqoKI??O11Ax>l&u#?iu_}KoN9>|B|y"
    r"8IPI}fpUtHnyB+25w@4!3e?>4th%+jrByuUasx{|9?kfNm9><d`VGiL53A)iqy7KqRk##N4E7"
    r"-_$S}cP3jXGj3m>^-_b=8_+Lw^*8UfZddXb)=WcZm_x-qa15ty4|GjxA*Fp1-!xp!8kgnhJAK"
    r"x<-t_W&nBbE-LC+3yHa7@~xPQEng|QpRGXh>vi_aTwJPujbp7(ZH`FIid=Pe;Thm;Y`osO{vm"
    r"3(ZEQB+!KqFt{OG+x=aE8UrIfE1pwJe+!HS%(jda7RUggc4e%rmd0WwM;*<{CwV#<Q%-T-Of7"
    r"WwUhiy;gzezE;<W2!#1(xjU4)v}HMDO<<lA>FSB{D|H9(-amLUriUfWwftqJB*F2^add@OL7O"
    r"8a)P@X#uMgV5dF3khq)+Sad|>DsittL^Ik3o+X8yI(-O92$+2%`S1BK%Rh-~>zJ;py;0#GSL*"
    r"fyh?t^bH3<K7hNDx^9zY*wYefLgt3Z+AFaLU|2q6Cu1o`;0=#!sazPzn}cgSC}+@~=hh{W70B"
    r"dm;x;XPcq?;}?`sG%D1Iz2Dvwl$YPI^(<pZP=qvmR0LXvQjJ2yze6wd0j|HhuM}6~+OY?LknJ"
    r"`7?Gld+3jVXC72a?2B=v0)=ZqB^b|n)0X&D|6J0NjI>Ey$~(#H_8o4(j@V`RlfMzWB_qxsF^e"
    r"*=z+bbG2o75)7&{Z6unuwikZ-E@_AzP}1)EriP9uynzLuS7mc<Y}WMeZVDf6MU}!_Tfju?}gC"
    r"bylqAPW*v)j#QQ-~O=lkT#-9jke8fLYpscqXey&(>mYrD`dD9X55ymF|ex8`@^xR`XP9JqVow"
    r"_xPk9w8M9iSd`!r!gUA**gJZ?9Mos@k8QPb=Ng5Ln7^2~^xm2LuAUhKsn)R{7_29Pr1fAK0u^"
    r">ar_Qm3`q2Ad!uMdEVrL!#byi3Vr*4b9f=_?30a=SV>7xOof6q%?chhO?%T!0;?#K>e?h|*=n"
    r">_KE9p%24CTxt~9WHA}SFbqho&MgTBaT$S=Bfmwr3o1J%7PI+=rslg7LaFp#la@3nu3`~qE-8"
    r"`zo?d$jj6gA)_&EOTI}s}&Gj{=$Wn4GM(?jmR(ak<lbjkHBTFmV#A-f>m?4h#a^lqPl1(dX@8"
    r"%JK{tpn^B3KCFtNMhz`{CP^C-)*<tX>`aF@^5-d9cGKnKclh~i*JcqQ_Qd@E`VFoVT1P=>An0"
    r"AE&Sp5Ry0q0lLo<>yANMi{bp9>gg45FQ}P2aut$`Bsiqp2;cCxMr=j7o&&P=b?}YUyvx;L4yi"
    r"5^@n8j7w(1=K&$2Fc}_fZ}j#<W)nt0cI(s)d<B3DCi2$k{!_epFyZ_774CVs#86h^#4gQiY#T"
    r"3RYPV1gmc)X^cT{|xGe8{^iR$q6veWfG>};&#4~jKHp=Z=torh2nCT7Vp5-z2!NJ{F>;}XR-%"
    r"jZSLU!1FBLglyzqhi+IQHk~m6c%C)Huo6#p4ExG+&cwfj;PkYeaxXp$6FVFKL|AI>G0GTi{-}"
    r"C17%CslnCgEmBmfF_r#FT81d~Kz>9a7=4Caa*P$xq(tJ<xO?z?8VBWl$AuV%++rL@HUMG3W^Z"
    r"MfuqDawkXsdg`A$!hEnSEKq0vh^2b7Z7Jvkv|(2+4`75IQ*1wwOy}B^SBR`%|Kkk6bVV#+-`S"
    r"a9_+pTS05xOS_WP&d@7I|F$ZWuB7#g8;)e&%STl!%z0psX|&o)^95TC+F#^DW6nM88QdRw35-"
    r"T0l-{0B!w`_~T?}=ZYo}M#O52QeOCJScjNn?2J{JUFk=4}%ny{9P)}VB!`T-K3I`f%5%5nsA>"
    r"CLckGR+*%sUy&Jcu+W^NCdCh?-*k6t$Vh{68X<`jEMF<gA4_c@-LbfvCSS$;7Fb_v9@>))4>@"
    r"h?4Vwvxm;jtyQrl%npcmGTYzJ`G+bZ<jnC>~?z1DdB~wwUbmuM+nri>Y4sG<#sx3!QpnFHl*|"
    r"?G${7B-)qA7lqVpUK%BNzT+9HJzS^<-m$kUuBA+g=|xY8#J<w3aSh{MUPfXHZt)XuXuwS-KWw"
    r"@Hkv#|Jw3X?6Uk25@m%zL%)kGx!=aW6UM+Kd9RfQPbBza2nrYoVJO7I?P)$yoh{0ROhLU$iqy"
    r"S`8s;5EZQ1>gA-J8$@1$I2ZeZ8VLUBkMc8>8K@*|tCvQJSQz_|&2YT|VAJ)<md`U_Mqh8dT{#"
    r"E!x)jm8A^PiN1J<uf{fLN=bmvH!Ee7@zL#S8=?eC?S(jo)9Qu$SNz=xFcW3MK8|f@jWd82)WZ"
    r"i4-cCobXeN92iCFCIXxJY3<2a@O7>^$oMmlV5DzupErTi_UV>xB2pY0_DDU`ilUe)Et{r*jBr"
    r"@O!w=YlcvaIVDX~SEUm6KB@k>eM?RH#Y^oY%yhhnIROF4x${QL6f)@E{{S%t+(+JmW4k^PE>>"
    r"n|?#eXG*mO>?7H@c9m=L*Uu^SfOJkRl#tByWa__PaX?C>_m<^>PGA2Xr>K+pEOZdxLyWc1Fom"
    r"*nNX$uUQ(4DmCdG6w!L50VcgEI3OmC&Y{n)Ulfq`xW)^0H)UtLd>JgP+PMU&5SY8Lhr`92rmB"
    r"RP?{_QYt2^!VG1ikBkBOhpkBlRc5By6jW?EI{+nm6>Aa_f0jnNGjY#rEQ3QW&nwf`t7#=to!#"
    r"@cWTDv((>?fa7hW()rsc8`Wrs2jCF_ga$Ro9fwhICO@UoCQ2Vc+r-G@>hJfZQjmDI}JJA{l6f"
    r"vUw52&4iMqp>79vM2n{Qa^Nd(|G?kXmpzU&3sgDO$py3VJkVwAVg3YHn0Yap{vrxhWC%oRXk}"
    r"p`@jWmKWoBAALVuSH&GXYio4Z$sH^2GY1zCx9+(VLA}!#N7&;FK_#^5(wdRXfGY0xoyJ|E!x|"
    r"Hc(XMq#9<U{UOZ{4aK;PHid`ICCZnI-)b=i|@)-l~8SOWfnrU5d(#J%IE_3AbOgz(rk;a^Y<U"
    r"bv%Ay)Ml&dH@N*q5mzR+GtV!KfHW`C3UJ3Jw<C&n?Ub`kJF<UG~R62Xb|y(#*d3UD*M;s>CWl"
    r"~V|mK={&3{z1>lzCmQ<RoM&N{7E3fFDv*bpJtnMhoLw0yjYY60ju;=yLjV76E%$wlMQ)oHf1E"
    r"-kTR6z^5o;hZe{*FM&y6C`PsGcX1Me~SmsdY}l@eKb&j@rkE658U{X<AlgRWz8mjMMGke8}6F"
    r"LJtPS9K!9(T{4xtCN0sJ;pl@?68!03a7BrzaXxBDUCj-$t=3>EQR2>^^Py^PT|#)J(k1n30+y"
    r"(Ag6*p67$3Okx5#!`j2o6sV=AyPR5G-l6`0v6=z;PfPro(!rth708mLuQh*7^=Nyi=6jnhgp)"
    r"FglU3k>%VxeLa#9`TikYoO_z8On*xEjpF5kok<1|G(Y3qE;F+3zJJC5@bRDrhkTv<Ov$VE<$V"
    r"aJLQxeqUIgkWk$gb+7(8nS^H2~sh*b#)H4{rUotYQ`Hrf5(aLqck2%FPm1q)Ryf>u&oYe+p!7"
    r"a8NbWw6m0@s<tp^$!|A+3Jvd}((m0Y=#1ZjvU<*3Z_aNS=W?uOvkf-sSF}V<edNSa209z{6k%"
    r"SC?j*;z~$FgM}&SPx(BFEWMue*C)H6C%B?i{r1#6oCt=8Ow?$$6`2UeX3&Vr7!M6|X=QmOevU"
    r"!o9;V28bVJc&I@b8!=v1TM=(B<}5%9d~lQLq?MO-l(LA8m(6>^N`Nw%j1CfTc?4i-opoV$dad"
    r"JB-#q1AjGr9PDe9IiQe;agi;^as@qwH|IhTZhsCXroqx^Wf~!mj1`0|LagbiEiDZPd~x!T!qf"
    r"ZXI4jhr~hH8nj#iX4scr+C1Dnap%=Q<uZnrY-o*nyw&lj+s#)>*SSx3=c;ag*No;3S6LvJB|F"
    r"bZ)J*?zS<B+>cBgz3132giK#<HSD^<UT#7xj`EMxX+wucg{8mou>7BF$c7jmitIcTs#NZ)$f8"
    r"5CY5jZ0jINEqBqM$(@js1CuLK9il;aXIogVFxit^xS9q{{>YHt2*yBie@Qr<_6B36)_s!m+l>"
    r"S>nX71VX1qaPKYFl*3O;xH^jARM)P7W9xb3Y4kmLT+eS6zHg+2F=zX(&*JdQwvbZy(*YS48@A"
    r"i936Xdxd56|XP<^;m_%>qnINetJy7c=w57t0{9!-D+&{8YPXwT70RU81t5iv%kkUBlpr>2f&h"
    r";3Yi^U;a#oYSq}aQ+N3lp_8wl9wSf&ts3}g!S0u;dfa&GRe~GCoU35F(TQxh3J<w3q959*rwv"
    r"^G$*iRCn4>U6o)(CWu=`p{T)7@COn_W?3ERzzTJ?73AZug-hWsB$nAldhW01s`!brl9f%{1`n"
    r";zdyJHsVv@R_%BDp+^C76e!!dWgYq$fM~5BhRJ<OFQwDNeWgGdj~hRdz5tI&bI<cD0~T}kN|C"
    r"BnS+_^}|4Z-|vfwVk3zE~Y$?hf6@6`9#07M)9qJn`?1EnKyT}?LEd6Su8m~>tkI~jT^%C~9W?"
    r"2`%XnTacuAS;+~VHQSiYn!wWWUw&$+SccKgqtCZYpyXN1Y`uvlfBucBS56KskKqjgO$LNNAM7"
    r"xF%bH22Hg{bIqSTsdLWib$nA2J5pVmM$y(Q)mQANRY={Z`_7i&MiIsTOM2{CL>vHU(=<f?hzY"
    r"bzOsnN#?<M&Ti4H}dh?qE1-knsU<+axOW2T}Rd=NBO15)|u0`<+5X*Jg$yeaOnU_tcG8?`|Wg"
    r";U(W-B2YDPWDEt0R$tFHRh~~izwiG~Pn#Lh?u;&GJ;hA&zv_<ljG+(c1o(2SNmxdv>)kj5I>*"
    r"oKDh8xAn#}wB<G$<55-p~Ugj`LiLz+NV`CKqikwK(6VUZ5TU&+FEOxS(S1?l`pyqj7IMZ`2pn"
    r")mW`S4%YQYv`5<hZVulFKbS5>eS$Hu-60e-FaM`m2e8T<W9*_CDT##EPqW(C=I|_?|#trHT;+"
    r"EqSZbJ9J-nMhM4J9f+wta<k)8V{Og_aBh9a5T8Mw69FeN|t(%-lxc_1WK29{NLO16xb0HJcoB"
    r"l@Ovx3QWHurzE8M6@p`LxaC7a6~}(v>-FKY%**Qn{{cUrT{#dclIgx%<>2O`>8`J6STZgCHhp"
    r"ygb2`s&QXcoW~^D$c`aIFdU5n=To^YgDOr~S>-!sZ-1TsxjRMJyR7MflAuA4V_%2>q{fLD5Xx"
    r"5V8I-hXe}29e);O6!*wlh!!_u@EAqXE_DFnP!bYK8~TJs|GX)ukZ18V>6H3Q&0Q;{v7OyXsrC"
    r"wb<26BOBd2awB^-|RyF12(mTc(+}J^)157>0JE{0fbm?hmDfHuBO|w1X*^)fk_$tZ80bj<!}R"
    r"Ypbnmj0+f?N)cptUP@v&|7mk#+IzGjZWUeonqG1YCw1tD|`)ZNp1uLBK%f9U%icrY7AM=tiu@"
    r"WK9ZvPkdZ6j2c0>W4mm4mEoR?%O>?5nK0uAryv77r+~@N*ei`v@RCXJezt+>p_!4b)Ts=dcaQ"
    r"QH(`xUhn-~3V}`U4g!VEa$Il3Y7L5bOOl51b*Ojf3rP}I3NH`R&20GqZkk*1=;`*6X1P>lSBz"
    r"85+e;1VHofh%xZj@KGg9Y>&IlLlEoyAc%2Mj|^qLvC(b+BvWos?FNGg$Z0}GIY0>I`klu>{oQ"
    r"d}R)Yv~!TBVM;~Dj2p~MF+p9#BLpz(NUyn7y|;;dj4y}k~;h|WkuT`{I*paFhRy9Dd7{OfUCi"
    r"$*jSPDtnb^pwUeD*mm82gAW{&zZF$J-lE9j*fNubFB1xcO9|R$*$8gocnp!HeEk#$4{~!MhD~"
    r"G@*9d67SgG<&xj@9SW5Tfs`u87N$+jlO;eg9fiM<TQT5$+h$lKz*N5>i8#SN8@sfq8(2!p24H"
    r"G}n`QW2yJ$-Cj@+SsQ-x`IX!gyHfdyql;z09dq_iRh7~@Phn$T<OtK);|Y|ju{21v4+flUy-l"
    r"ueg3bo)93~l^|9?iIxkbdsuPRbjruIyR5HcPO90+DhP->yW58HlcVY`MUZ5)Tb$Bbr353jjMr"
    r"hmrZc(678$%Fu&Gn(1+_}n%z9+^l$JF5;PBP@p>89@hUPX;6-%GC`Vj(KpSV$^K$J9*2zNG~7"
    r"IyfP_9!^kzlzhI!`m{Mn62tq!&0U&3f_c7C@ZHQ!jh?@`cyC_sG9$}MXOS$UIHxf6Z^S3;1tW"
    r">+E(A>bP<eR2vfDP@8TCXA+o86cwykXTgCVcyuRKyuHQKr-otCa!(2(eie^gisE00EdQ%>DrY"
    r"!(gi(vBYQl0ssI200dcD"
)

if __name__ == "__main__":
    main()
