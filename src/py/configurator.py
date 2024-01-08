from typing import Any, Tuple
import ctypes
import ctypes.util
import tkinter as tk
import socket
from dataclasses import dataclass

import numpy as np
import ctypes
import argparse

_libc_path = ctypes.util.find_library("c")
_libc = ctypes.CDLL(_libc_path)

def fflush(file):
    libc = ctypes.CDLL(ctypes.util.find_library("c"))
    libc.fflush(file)


@dataclass
class OpenCommand:
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
        if message_parts[0] == "lr":
            if message_parts[1] == "invalid":
                self.lr_vectors = None
            else:
                self.lr_vectors = np.array(
                    [int(x) for x in message_parts[1:]]
                ).reshape((2, 2))
        elif message_parts[0] == "ir":
            index = int(message_parts[1])
            valid = bool(int(message_parts[2]))
            if not valid:
                self.ir_vectors[index] = None
            else:
                self.ir_vectors[index] = np.array(
                    [int(x) for x in message_parts[3:5]]
                )
        elif message_parts[0] == "b":
            self.pressed_buttons = [m for m in message_parts[1:] if m]
        else:
            print(f"Unknown message: {message}")

    def send_message(self, name: str, *params, callback=None):
        req_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        req_sock.connect(self.socket_path)
        req_sock.settimeout(0.1)

        self.open_commands.append(
            OpenCommand(name, params, req_sock, callback)
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

        self.open_commands.clear()
        self.closed_commands = self.closed_commands[-self.MAX_CLOSED_COMMANDS:]

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
        canvas.create_line(x-10, y, x+10, y, width=2),
        canvas.create_line(x, y-10, x, y+10, width=2),
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
            x, y = lr_vectors[i] * [self.canvas.winfo_width(), self.canvas.winfo_height()] / 1024
            self.crosses[i] = draw_cross(self.canvas, x, y)

def get_screen_resolution(root):
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()
    return screen_width, screen_height


class Window:
    on_screen_area_updated = None

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

        main_frame = tk.Frame(root)
        main_frame.pack()

        # Screen subregion controls
        self.screen_area_box = screen_area_box = tk.Frame(main_frame)
        screen_area_box.pack()

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
                "write",
                lambda *args: self.parse_screen_area_values()
            )
        
        left_entry = tk.Entry(screen_area_box, width=5, textvariable=self.screen_area_text_values[0])
        left_entry.grid(row=2, column=0)

        # Right edit box
        self.right_entry = tk.Entry(screen_area_box, width=5, textvariable=self.screen_area_text_values[2])
        self.right_entry.grid(row=2, column=2)

        # Top edit box
        self.top_entry = tk.Entry(screen_area_box, width=5, textvariable=self.screen_area_text_values[1])
        self.top_entry.grid(row=1, column=1)

        # Bottom edit box
        self.bottom_entry = tk.Entry(screen_area_box, width=5, textvariable=self.screen_area_text_values[3])
        self.bottom_entry.grid(row=3, column=1)

        # "IR canvas"
        self.canvas = canvas = tk.Canvas(main_frame, width=400, height=400)
        canvas.pack()
        canvas.config(bg="gray")
        self.crosses = LRCrosses(canvas)

        # Add text box
        self.text_box = text_box = tk.Label(main_frame, justify=tk.CENTER, font=("Arial", 12, "italic"))
        text_box.config(text="...", wraplength=400)
        text_box.pack()

        # Add space
        space_frame = tk.Frame(main_frame, height=10)
        space_frame.pack()    

        # Add buttons
        button_frame = tk.Frame(main_frame)

        button1 = tk.Button(button_frame, text="Start calibration")
        button1.pack(side=tk.LEFT) 
        self.btn_start_calibration = button1

        button2 = tk.Button(button_frame, text="Button 2")
        button2.pack(side=tk.LEFT)

        button3 = tk.Button(button_frame, text="Button 3")
        button3.pack(side=tk.LEFT)

        button_frame.pack(side=tk.BOTTOM)

        screen_width, screen_height = get_screen_resolution(root)
        print(f"Screen resolution: {screen_width}x{screen_height}")


class Logic:
    def __init__(self, win: Window, wiimote: WiimoteSocketReader):
        self.win = win
        self.wiimote = wiimote

    def start(self):
        pass

    def process_socket_data(self):
        pass

    def stop(self):
        pass


class IdleLogic(Logic):
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
    def start(self):
        self.wiimote.send_message("mouse", "on")

    def process_socket_data(self):
        print("Waiting for mouse to be turned on again...")
        if not self.wiimote.open_commands:
            self.win.root.destroy()


class CalibrationLogic(Logic):
    on_completed = None
    on_exit = None

    STEPS = [
        "Point the wiimote to the center of the screen, then press A; press B to cancel",
        "Point the wiimote to the top-left corner of the screen, then press A; press B to cancel",
        "Point the wiimote to the top-right corner of the screen, then press A; press B to cancel",
        "Point the wiimote to the bottom-left corner of the screen, then press A; press B to cancel",
        "Point the wiimote to the bottom-right corner of the screen, then press A; press B to cancel",
        "Calibration completed",
    ]

    def start(self):
        self.btn_was_pressed = False
        self.step_data = []
        self.win.btn_start_calibration.config(highlightcolor="blue", highlightthickness=5)

    def process_socket_data(self):
        self.win.text_box.config(text=self.STEPS[len(self.step_data)])
        if len(self.step_data) == len(self.STEPS) - 1:
            if self.on_completed:
                self.on_completed()
            if self.on_exit:
                self.on_exit()
            return

        if not self.btn_was_pressed:
            if "a" in self.wiimote.pressed_buttons:
                self.step_data.append(self.wiimote.lr_vectors)
            elif "b" in self.wiimote.pressed_buttons:
                if self.on_exit:
                    self.on_exit()
        self.btn_was_pressed = bool(len(self.wiimote.pressed_buttons))

        if self.wiimote.lr_vectors is None:
            self.win.canvas.config(bg="red")
        elif self.wiimote.lr_vectors[0].tolist() == self.wiimote.lr_vectors[1].tolist():
            self.win.canvas.config(bg="yellow")
        else:
            self.win.canvas.config(bg="green")
        self.win.crosses.update(self.wiimote.lr_vectors)

    def stop(self):
        self.win.btn_start_calibration.config(highlightthickness=0)


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

    root = tk.Tk()
    root.title("Wiimote mouse configurator")

    window = Window(root)
    root.geometry("800x600")

    def new_socket_data():
        nonlocal current_logic

        current_logic.process_socket_data()

    wiimote = WiimoteSocketReader(args.socket_path, root, new_socket_data)
    wiimote.send_message("mouse", "off")

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

    def send_calibration_data(calibration_logic):
        data = calibration_logic.step_data[1:5]

        wii_corners = [
            np.mean(d, axis=0) for d in data
        ]
        wii_corner_mat = np.hstack(
            [wii_corners, np.ones([4, 1])]
        )

        corners = np.array([
            [0, 0],
            [10000, 0],
            [0, 10000],
            [10000, 10000],
        ])

        calmat, _, __, ___ = np.linalg.lstsq(wii_corner_mat, corners, rcond=None)

        int64_calmat = (calmat * 100).astype(np.int64).T
        wiimote.send_message("cal100", *int64_calmat.flatten().tolist())

    def on_start_calibration():
        calibration_logic = CalibrationLogic(window, wiimote)
        calibration_logic.on_exit = lambda: switch_logic(idle_logic)
        calibration_logic.on_completed = lambda: send_calibration_data(calibration_logic)

        switch_logic(calibration_logic)

    window.btn_start_calibration.config(command=on_start_calibration)

    def on_screen_area_updated(values):
        wiimote.send_message("screenarea100", *[v * 10000 for v in values])

    window.on_screen_area_updated = on_screen_area_updated

    try:
        root.mainloop()
    finally:
        wiimote.close()

if __name__ == "__main__":
    main()
