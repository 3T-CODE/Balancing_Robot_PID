import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog, scrolledtext, Button
import os
import serial
import threading
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.animation import FuncAnimation

data_buffer = []
paused = False
show_P = True
show_I = True
show_D = True
show_angleError = True

def new_file():
    messagebox.showinfo("New File", "Creating a new file...")

def open_file():
    file_path = filedialog.askopenfilename(filetypes=[("Text files", "*.txt"), ("All files", "*.*")])
    if file_path:
        messagebox.showinfo("Open File", f"File opened: {file_path}")

def save_file():
    file_path = filedialog.asksaveasfilename(defaultextension=".txt", filetypes=[("Text files", "*.txt")])
    if file_path:
        try:
            with open(file_path, 'w') as file:
                for entry in data_buffer:
                    file.write(entry + "\n")
            messagebox.showinfo("Save File", f"File saved as {file_path}")
        except IOError as e:
            messagebox.showerror("Error", f"Failed to save file: {e}")

def read_from_port(serial_port, text_widget):
    while True:
        if serial_port.in_waiting > 0:
            try:
                data = serial_port.readline().decode('utf-8').strip()
                if data:
                    data_buffer.append(data)
                    text_widget.insert(tk.END, data + "\n")
                    text_widget.see(tk.END)  # Auto-scroll to the end
            except serial.SerialException as e:
                messagebox.showerror("Error", f"Error reading from the COM Port: {e}")
                break

def clear_text_widget(text_widget):
    text_widget.delete(1.0, tk.END)
    data_buffer.clear()

def clear_plot(ax):
    ax.cla()
    ax.set_xlabel('time (s)')
    ax.set_ylabel('value')
    ax.set_title('Data Plot')
    ax.legend()

def toggle_pause():
    global paused
    paused = not paused
    if paused:
        pause_button.config(text="Resume")
    else:
        pause_button.config(text="Pause")

def update_plot(frame, P_data, I_data, D_data, angleError_data, time_data, ax):
    if paused:
        return

    while data_buffer:
        line = data_buffer.pop(0)
        parts = line.split()
        if len(parts) == 5:
            P_data.append(float(parts[0]))
            I_data.append(float(parts[1]))
            D_data.append(float(parts[2]))
            angleError_data.append(float(parts[3]))
            time_data.append(float(parts[4]))

    ax.cla()
    if show_P:
        ax.plot(time_data, P_data, label='P', color='blue')
    if show_I:
        ax.plot(time_data, I_data, label='I', color='green')
    if show_D:
        ax.plot(time_data, D_data, label='D', color='orange')
    if show_angleError:
        ax.plot(time_data, angleError_data, label='angleError', color='red')
    ax.set_xlabel('time (s)')
    ax.set_ylabel('value')
    ax.set_title('Data Plot')
    ax.legend()

def plot_data():
    global show_P, show_I, show_D, show_angleError
    P_data = []
    I_data = []
    D_data = []
    angleError_data = []
    time_data = []

    plot_window = tk.Toplevel(root)
    plot_window.title("Data Plot")

    fig, ax = plt.subplots()

    canvas = FigureCanvasTkAgg(fig, master=plot_window)
    canvas.draw()
    canvas.get_tk_widget().pack()

    # Add a refresh button
    Button(plot_window, text="Refresh", command=lambda: clear_plot(ax)).pack(side=tk.LEFT)
    
    # Add a pause button
    global pause_button
    pause_button = Button(plot_window, text="Pause", command=toggle_pause)
    pause_button.pack(side=tk.LEFT)

    # Add hide/show buttons for P, I, D, and angleError
    Button(plot_window, text="P-hide", command=lambda: toggle_line('P')).pack(side=tk.LEFT)
    Button(plot_window, text="I-hide", command=lambda: toggle_line('I')).pack(side=tk.LEFT)
    Button(plot_window, text="D-hide", command=lambda: toggle_line('D')).pack(side=tk.LEFT)
    Button(plot_window, text="angleError-hide", command=lambda: toggle_line('angleError')).pack(side=tk.LEFT)

    # Add a quit button
    Button(plot_window, text="Quit", command=root.quit).pack(side=tk.LEFT)

    # Start continuous plotting
    ani = FuncAnimation(fig, update_plot, fargs=(P_data, I_data, D_data, angleError_data, time_data, ax), interval=1000)
    canvas.draw()

def toggle_line(line_type):
    global show_P, show_I, show_D, show_angleError
    if line_type == 'P':
        show_P = not show_P
    elif line_type == 'I':
        show_I = not show_I
    elif line_type == 'D':
        show_D = not show_D
    elif line_type == 'angleError':
        show_angleError = not show_angleError

def select_com_port():
    com_ports = [f"/dev/{f}" for f in os.listdir('/dev') if f.startswith('tty')]

    selected_port = None
    if com_ports:
        selected_port = simpledialog.askstring("Select COM Port", f"Available COM Ports:\n{', '.join(com_ports)}\n\nEnter the full path of the COM port:")

        # If no port is selected, default to /dev/rfcomm0
        if not selected_port:
            selected_port = "/dev/rfcomm0"
    else:
        # If no COM ports are found, default to /dev/rfcomm0
        selected_port = "/dev/rfcomm0"

    try:
        serial_port = serial.Serial(selected_port, baudrate=9600, timeout=1)
        messagebox.showinfo("Com Port Selection", f"COM Port selected: {selected_port}")

        # Create a new window to display the data
        data_window = tk.Toplevel(root)
        data_window.title(f"Data from {selected_port}")
        text_widget = scrolledtext.ScrolledText(data_window, width=60, height=20)
        text_widget.pack()

        # Add Refresh and Plot buttons
        refresh_button = Button(data_window, text="Refresh", command=lambda: clear_text_widget(text_widget))
        refresh_button.pack(side=tk.LEFT)
        
        plot_button = Button(data_window, text="Plot", command=plot_data)
        plot_button.pack(side=tk.LEFT)

        # Add a quit button
        Button(data_window, text="Quit", command=root.quit).pack(side=tk.LEFT)

        # Start a thread to continuously read data from the serial port
        threading.Thread(target=read_from_port, args=(serial_port, text_widget), daemon=True).start()

    except serial.SerialException as e:
        messagebox.showerror("Error", f"Failed to open COM Port: {e}")

def quit_app():
    root.quit()

# Creating the main window
root = tk.Tk()
root.title("Menu Example")

# Creating a menu bar
menu_bar = tk.Menu(root)

# Adding File menu
file_menu = tk.Menu(menu_bar, tearoff=0)
file_menu.add_command(label="New File", command=new_file)
file_menu.add_command(label="Open File", command=open_file)
file_menu.add_command(label="Save File", command=save_file)
file_menu.add_command(label="Select COM Port", command=select_com_port)
file_menu.add_separator()  # Adds a separator line
file_menu.add_command(label="Quit", command=quit_app)
menu_bar.add_cascade(label="File", menu=file_menu)

# Configuring the menu bar
root.config(menu=menu_bar)

# Running the application
root.mainloop()

