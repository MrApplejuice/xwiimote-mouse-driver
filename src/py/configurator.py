from typing import Tuple

import tkinter as tk
import socket

import numpy as np

class UnixSocketReader:
    def __init__(self, socket_path, tcl_root, notify_callback):
        self.poll_interval = 10

        self.socket_path = socket_path
        self.root = tcl_root
        self.notify_callback = notify_callback

        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(self.socket_path)
        self.sock.settimeout(0.01)

        self.lr_vectors = None
        self.ir_vectors = [None] * 4

        self.timer_id = self.root.after(self.poll_interval, self.read_socket)

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

    def read_socket(self):
        try:
            data = self.sock.recv(1024)
            if not data:
                return
            messages = data.decode().split("\n")
            for message in messages:
                if message:
                    self.process_message(message)
            self.notify_callback()
        except IOError as e:
            print(f"Error reading socket: {e}")
        finally:
            self.timer_id = self.root.after(self.poll_interval, self.read_socket)

    def close(self):
        if self.timer_id is not None:
            self.root.after_cancel(self.timer_id)
        self.sock.close()

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

def main():
    root = tk.Tk()
    root.title("Wiimote mouse configurator")

    main_frame = tk.Frame(root)
    main_frame.pack()

    canvas = tk.Canvas(main_frame, width=400, height=400)
    canvas.pack()

    canvas.config(bg="gray")

    # Add text box
    text_box = tk.Label(main_frame, justify=tk.CENTER, font=("Arial", 12, "italic"))
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

    root.geometry("400x500")

    crosses = LRCrosses(canvas)

    def new_socket_data():
        if wiimote.lr_vectors is None:
            canvas.config(bg="red")
            text_box.config(text="Sensor bar not visible")
        elif wiimote.lr_vectors[0].tolist() == wiimote.lr_vectors[1].tolist():
            canvas.config(bg="yellow")
            text_box.config(text="Sensor bar only partially visible")
        else:
            canvas.config(bg="green")
            text_box.config(text="Ready to calibrate")
        crosses.update(wiimote.lr_vectors)

    wiimote = UnixSocketReader("/tmp/wiimote-mouse.sock", root, new_socket_data)
    try:

        root.mainloop()
    finally:
        wiimote.close()



if __name__ == "__main__":
    main()
