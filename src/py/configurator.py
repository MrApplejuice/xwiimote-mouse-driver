import tkinter as tk
import socket


class UnixSocketReader:
    def __init__(self, socket_path, tcl_root):
        self.socket_path = socket_path
        self.root = tcl_root
        self.timer_id = None
        self.timer_id = self.root.after(100, self.read_socket)

        self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.sock.connect(self.socket_path)
        self.sock.settimeout(0.01)

    def read_socket(self):
        try:
            data = self.sock.recv(1024)
            if not data:
                return
            messages = data.decode().split("\n")
            for message in messages:
                if message:
                    print(f"Received data: {message}")
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

    socket = UnixSocketReader("/tmp/wiimote-mouse.sock", root)

    main_frame = tk.Frame(root)
    main_frame.pack()

    canvas = tk.Canvas(main_frame, width=400, height=400)

    # Draw four crosses
    draw_cross(canvas, 100, 100)
    draw_cross(canvas, 300, 100)
    draw_cross(canvas, 100, 300)
    draw_cross(canvas, 300, 300)

    # Add buttons
    button_frame = tk.Frame(main_frame)

    button1 = tk.Button(button_frame, text="Start calibration")
    button1.pack(side=tk.LEFT) 

    button2 = tk.Button(button_frame, text="Button 2")
    button2.pack(side=tk.LEFT)

    button3 = tk.Button(button_frame, text="Button 3")
    button3.pack(side=tk.LEFT)

    canvas.pack()
    button_frame.pack(side=tk.BOTTOM)

    screen_width, screen_height = get_screen_resolution(root)
    print(f"Screen resolution: {screen_width}x{screen_height}")

    root.geometry("400x500")
    try:
        root.mainloop()
    finally:
        socket.close()



if __name__ == "__main__":
    main()
