from typing import Any, Tuple
import ctypes
import ctypes.util
import tkinter as tk
import socket
from dataclasses import dataclass

import numpy as np
import ctypes
import time

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
        else:
            print(f"Unknown message: {message}")

    def send_message(self, name: str, *params):
        req_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        req_sock.connect(self.socket_path)
        req_sock.settimeout(0.1)

        self.open_commands.append(OpenCommand(name, params, req_sock))

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
    def __init__(self, root: tk.Tk):
        self.root = root

        main_frame = tk.Frame(root)
        main_frame.pack()

        self.canvas = canvas = tk.Canvas(main_frame, width=400, height=400)
        canvas.pack()
        canvas.config(bg="gray")
        self.crosses = LRCrosses(canvas)

        # Add text box
        self.text_box = text_box = tk.Label(main_frame, justify=tk.CENTER, font=("Arial", 12, "italic"))
        text_box.config(text="...")
        text_box.pack()

        # Add space
        space_frame = tk.Frame(main_frame, height=10)
        space_frame.pack()    

        # Add buttons
        button_frame = tk.Frame(main_frame)

        button1 = tk.Button(button_frame, text="Start calibration")
        button1.pack(side=tk.LEFT) 

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

def main():
    root = tk.Tk()
    root.title("Wiimote mouse configurator")

    window = Window(root)
    root.geometry("400x500")

    def new_socket_data():
        nonlocal current_logic

        current_logic.process_socket_data()

    wiimote = WiimoteSocketReader("/tmp/wiimote-mouse.sock", root, new_socket_data)
    wiimote.send_message("mouse", "off")

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

    try:
        root.mainloop()
    finally:
        wiimote.close()

if __name__ == "__main__":
    main()
