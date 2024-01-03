import tkinter as tk
import socket

import numpy as np

class UnixSocketReader:
    def __init__(self, socket_path, tcl_root, notify_callback):
        self.socket_path = socket_path
        self.root = tcl_root
        self.timer_id = None
        self.timer_id = self.root.after(100, self.read_socket)
        self.notify_callback = notify_callback

        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(self.socket_path)
        self.sock.settimeout(0.01)

        self.lr_vectors = None
        self.ir_vectors = [None] * 4

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
            self.timer_id = self.root.after(100, self.read_socket)

    def close(self):
        if self.timer_id is not None:
            self.root.after_cancel(self.timer_id)
        self.sock.close()



def draw_cross(canvas, x, y):
    canvas.create_line(x-10, y, x+10, y, width=2)  # Horizontal line
    canvas.create_line(x, y-10, x, y+10, width=2)  # Vertical line

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

    # Draw four crosses
    draw_cross(canvas, 100, 100)
    draw_cross(canvas, 300, 100)
    draw_cross(canvas, 100, 300)
    draw_cross(canvas, 300, 300)
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

    def new_socket_data():
        if socket.lr_vectors is None:
            canvas.config(bg="red")
            text_box.config(text="Sensor bar not visible")
        elif socket.lr_vectors[0].tolist() == socket.lr_vectors[1].tolist():
            canvas.config(bg="yellow")
            text_box.config(text="Sensor bar only partially visible")
        else:
            canvas.config(bg="green")
            text_box.config(text="Ready to calibrate")

    socket = UnixSocketReader("/tmp/wiimote-mouse.sock", root, new_socket_data)
    try:

        root.mainloop()
    finally:
        socket.close()



if __name__ == "__main__":
    main()
