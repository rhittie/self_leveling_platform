#!/usr/bin/env python3
"""
Self-Leveling Platform - Admin Test Mode GUI

A graphical interface for testing hardware components via serial connection.
Requires: pip install pyserial
"""

import tkinter as tk
from tkinter import ttk, scrolledtext, messagebox
import serial
import serial.tools.list_ports
import threading
import queue
import time


class TestModeGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Self-Leveling Platform - Test Mode")
        self.root.geometry("900x700+0+0")
        self.root.minsize(800, 600)
        self.root.deiconify()
        self.root.lift()
        self.root.focus_force()
        self.root.attributes('-topmost', True)
        self.root.after(2000, lambda: self.root.attributes('-topmost', False))

        # Serial connection
        self.serial_port = None
        self.is_connected = False
        self.read_thread = None
        self.stop_thread = False
        self.message_queue = queue.Queue()

        # Track toggle states
        self.imu_streaming = False
        self.button_test = False
        self.motor1_continuous = False
        self.motor2_continuous = False
        self.led_cycling = False

        self.setup_ui()
        self.refresh_ports()

        # Start queue processor
        self.process_queue()

    def setup_ui(self):
        """Create the main UI layout."""
        # Main container with padding
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky="nsew")
        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(0, weight=1)

        # Connection frame at top
        self.setup_connection_frame(main_frame)

        # Create notebook for organized tabs
        notebook = ttk.Notebook(main_frame)
        notebook.grid(row=1, column=0, sticky="nsew", pady=(10, 0))
        main_frame.rowconfigure(1, weight=1)
        main_frame.columnconfigure(0, weight=1)

        # Create tabs
        motors_frame = ttk.Frame(notebook, padding="10")
        imu_frame = ttk.Frame(notebook, padding="10")
        led_button_frame = ttk.Frame(notebook, padding="10")
        system_frame = ttk.Frame(notebook, padding="10")

        notebook.add(motors_frame, text="Motors")
        notebook.add(imu_frame, text="IMU / MPU6050")
        notebook.add(led_button_frame, text="LED & Button")
        notebook.add(system_frame, text="System")

        self.setup_motors_tab(motors_frame)
        self.setup_imu_tab(imu_frame)
        self.setup_led_button_tab(led_button_frame)
        self.setup_system_tab(system_frame)

        # Serial output at bottom
        self.setup_output_frame(main_frame)

    def setup_connection_frame(self, parent):
        """Setup the serial connection controls."""
        conn_frame = ttk.LabelFrame(parent, text="Serial Connection", padding="5")
        conn_frame.grid(row=0, column=0, sticky="ew")

        # Port selection
        ttk.Label(conn_frame, text="Port:").grid(row=0, column=0, padx=5)
        self.port_combo = ttk.Combobox(conn_frame, width=20, state="readonly")
        self.port_combo.grid(row=0, column=1, padx=5)

        # Refresh button
        ttk.Button(conn_frame, text="Refresh", command=self.refresh_ports).grid(row=0, column=2, padx=5)

        # Baud rate
        ttk.Label(conn_frame, text="Baud:").grid(row=0, column=3, padx=5)
        self.baud_combo = ttk.Combobox(conn_frame, width=10, state="readonly",
                                        values=["9600", "115200", "230400", "460800"])
        self.baud_combo.set("115200")
        self.baud_combo.grid(row=0, column=4, padx=5)

        # Connect button
        self.connect_btn = ttk.Button(conn_frame, text="Connect", command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=5, padx=10)

        # Status indicator
        self.status_label = ttk.Label(conn_frame, text="● Disconnected", foreground="red")
        self.status_label.grid(row=0, column=6, padx=10)

        # Enter/Exit test mode buttons
        ttk.Separator(conn_frame, orient="vertical").grid(row=0, column=7, sticky="ns", padx=10)
        self.test_mode_btn = ttk.Button(conn_frame, text="Enter Test Mode",
                                         command=lambda: self.send_command("admin"))
        self.test_mode_btn.grid(row=0, column=8, padx=5)
        ttk.Button(conn_frame, text="Exit Test Mode",
                   command=lambda: self.send_command("exit")).grid(row=0, column=9, padx=5)

    def setup_motors_tab(self, parent):
        """Setup motor control tab."""
        parent.columnconfigure(0, weight=1)
        parent.columnconfigure(1, weight=1)

        # Motor 1 frame
        m1_frame = ttk.LabelFrame(parent, text="Motor 1 (Left Back)", padding="10")
        m1_frame.grid(row=0, column=0, sticky="nsew", padx=5, pady=5)

        ttk.Label(m1_frame, text="Steps:").grid(row=0, column=0, pady=5)
        self.m1_steps = ttk.Entry(m1_frame, width=10)
        self.m1_steps.insert(0, "100")
        self.m1_steps.grid(row=0, column=1, pady=5)

        btn_frame1 = ttk.Frame(m1_frame)
        btn_frame1.grid(row=1, column=0, columnspan=2, pady=5)
        ttk.Button(btn_frame1, text="Move +", width=10,
                   command=lambda: self.send_command(f"m1 {self.m1_steps.get()}")).pack(side="left", padx=2)
        ttk.Button(btn_frame1, text="Move -", width=10,
                   command=lambda: self.send_command(f"m1 -{self.m1_steps.get()}")).pack(side="left", padx=2)

        self.m1c_btn = ttk.Button(m1_frame, text="Continuous: OFF", width=20,
                                   command=self.toggle_m1_continuous)
        self.m1c_btn.grid(row=2, column=0, columnspan=2, pady=5)

        # Motor 2 frame
        m2_frame = ttk.LabelFrame(parent, text="Motor 2 (Right Back)", padding="10")
        m2_frame.grid(row=0, column=1, sticky="nsew", padx=5, pady=5)

        ttk.Label(m2_frame, text="Steps:").grid(row=0, column=0, pady=5)
        self.m2_steps = ttk.Entry(m2_frame, width=10)
        self.m2_steps.insert(0, "100")
        self.m2_steps.grid(row=0, column=1, pady=5)

        btn_frame2 = ttk.Frame(m2_frame)
        btn_frame2.grid(row=1, column=0, columnspan=2, pady=5)
        ttk.Button(btn_frame2, text="Move +", width=10,
                   command=lambda: self.send_command(f"m2 {self.m2_steps.get()}")).pack(side="left", padx=2)
        ttk.Button(btn_frame2, text="Move -", width=10,
                   command=lambda: self.send_command(f"m2 -{self.m2_steps.get()}")).pack(side="left", padx=2)

        self.m2c_btn = ttk.Button(m2_frame, text="Continuous: OFF", width=20,
                                   command=self.toggle_m2_continuous)
        self.m2c_btn.grid(row=2, column=0, columnspan=2, pady=5)

        # Global motor controls
        global_frame = ttk.LabelFrame(parent, text="Global Motor Controls", padding="10")
        global_frame.grid(row=1, column=0, columnspan=2, sticky="ew", padx=5, pady=5)

        ttk.Button(global_frame, text="STOP ALL MOTORS", width=20,
                   command=self.stop_all_motors).pack(side="left", padx=10)

        ttk.Label(global_frame, text="Speed (RPM):").pack(side="left", padx=(20, 5))
        self.speed_entry = ttk.Entry(global_frame, width=5)
        self.speed_entry.insert(0, "10")
        self.speed_entry.pack(side="left")
        ttk.Button(global_frame, text="Set Speed",
                   command=lambda: self.send_command(f"mspeed {self.speed_entry.get()}")).pack(side="left", padx=5)

        # Quick move buttons
        quick_frame = ttk.LabelFrame(parent, text="Quick Moves", padding="10")
        quick_frame.grid(row=2, column=0, columnspan=2, sticky="ew", padx=5, pady=5)

        for steps in [10, 50, 100, 200, 500]:
            btn_container = ttk.Frame(quick_frame)
            btn_container.pack(side="left", padx=5)
            ttk.Label(btn_container, text=f"{steps} steps").pack()
            btn_row = ttk.Frame(btn_container)
            btn_row.pack()
            ttk.Button(btn_row, text="M1", width=4,
                       command=lambda s=steps: self.send_command(f"m1 {s}")).pack(side="left")
            ttk.Button(btn_row, text="M2", width=4,
                       command=lambda s=steps: self.send_command(f"m2 {s}")).pack(side="left")

    def setup_imu_tab(self, parent):
        """Setup IMU/MPU6050 testing tab."""
        parent.columnconfigure(0, weight=1)
        parent.columnconfigure(1, weight=1)

        # I2C / Connection frame
        i2c_frame = ttk.LabelFrame(parent, text="I2C / Connection", padding="10")
        i2c_frame.grid(row=0, column=0, sticky="nsew", padx=5, pady=5)

        ttk.Button(i2c_frame, text="Scan I2C Bus", width=20,
                   command=lambda: self.send_command("scan")).pack(pady=5)
        ttk.Button(i2c_frame, text="Initialize IMU", width=20,
                   command=lambda: self.send_command("imu")).pack(pady=5)
        ttk.Label(i2c_frame, text="Expected: 0x68 (MPU6050)",
                  foreground="gray").pack(pady=5)

        # Reading frame
        read_frame = ttk.LabelFrame(parent, text="Read Data", padding="10")
        read_frame.grid(row=0, column=1, sticky="nsew", padx=5, pady=5)

        ttk.Button(read_frame, text="Single Reading", width=20,
                   command=lambda: self.send_command("read")).pack(pady=5)
        ttk.Button(read_frame, text="Raw Values", width=20,
                   command=lambda: self.send_command("raw")).pack(pady=5)

        self.stream_btn = ttk.Button(read_frame, text="Stream: OFF", width=20,
                                      command=self.toggle_streaming)
        self.stream_btn.pack(pady=5)

        # Calibration frame
        cal_frame = ttk.LabelFrame(parent, text="Calibration", padding="10")
        cal_frame.grid(row=1, column=0, columnspan=2, sticky="ew", padx=5, pady=5)

        ttk.Label(cal_frame, text="Keep platform STILL and LEVEL during calibration!",
                  foreground="orange").pack(pady=5)
        ttk.Button(cal_frame, text="Run Calibration", width=20,
                   command=lambda: self.send_command("cal")).pack(pady=5)

    def setup_led_button_tab(self, parent):
        """Setup LED and button testing tab."""
        parent.columnconfigure(0, weight=1)
        parent.columnconfigure(1, weight=1)

        # LED frame
        led_frame = ttk.LabelFrame(parent, text="LED Control", padding="10")
        led_frame.grid(row=0, column=0, sticky="nsew", padx=5, pady=5)

        # Basic controls
        basic_frame = ttk.Frame(led_frame)
        basic_frame.pack(pady=5)
        ttk.Button(basic_frame, text="ON", width=8,
                   command=lambda: self.send_command("led on")).pack(side="left", padx=2)
        ttk.Button(basic_frame, text="OFF", width=8,
                   command=lambda: self.send_command("led off")).pack(side="left", padx=2)

        # Patterns
        ttk.Label(led_frame, text="Patterns:", font=("", 9, "bold")).pack(pady=(10, 5))
        patterns_frame = ttk.Frame(led_frame)
        patterns_frame.pack()

        patterns = [
            ("Slow Blink (1Hz)", "led slow"),
            ("Fast Blink (4Hz)", "led fast"),
            ("Double Pulse", "led pulse"),
            ("Error (10Hz)", "led error"),
        ]

        for i, (label, cmd) in enumerate(patterns):
            ttk.Button(patterns_frame, text=label, width=18,
                       command=lambda c=cmd: self.send_command(c)).grid(row=i//2, column=i%2, padx=2, pady=2)

        # Cycle button
        self.led_cycle_btn = ttk.Button(led_frame, text="Cycle All Patterns", width=20,
                                         command=self.toggle_led_cycle)
        self.led_cycle_btn.pack(pady=10)

        # Button frame
        btn_frame = ttk.LabelFrame(parent, text="Button Testing", padding="10")
        btn_frame.grid(row=0, column=1, sticky="nsew", padx=5, pady=5)

        ttk.Label(btn_frame, text="Enable button test mode to see\nbutton press events in the output.",
                  justify="center").pack(pady=10)

        self.btn_test_btn = ttk.Button(btn_frame, text="Button Test: OFF", width=20,
                                        command=self.toggle_button_test)
        self.btn_test_btn.pack(pady=10)

        ttk.Label(btn_frame, text="Events:\n• SHORT_PRESS\n• LONG_PRESS (2+ seconds)",
                  justify="left", foreground="gray").pack(pady=10)

    def setup_system_tab(self, parent):
        """Setup system info tab."""
        parent.columnconfigure(0, weight=1)

        # Info frame
        info_frame = ttk.LabelFrame(parent, text="System Information", padding="10")
        info_frame.grid(row=0, column=0, sticky="ew", padx=5, pady=5)

        btn_frame = ttk.Frame(info_frame)
        btn_frame.pack(pady=10)
        ttk.Button(btn_frame, text="Show Pin Info", width=20,
                   command=lambda: self.send_command("info")).pack(side="left", padx=5)
        ttk.Button(btn_frame, text="Show Status", width=20,
                   command=lambda: self.send_command("s")).pack(side="left", padx=5)
        ttk.Button(btn_frame, text="Show Help", width=20,
                   command=lambda: self.send_command("help")).pack(side="left", padx=5)

        # Manual command frame
        manual_frame = ttk.LabelFrame(parent, text="Manual Command", padding="10")
        manual_frame.grid(row=1, column=0, sticky="ew", padx=5, pady=5)

        cmd_frame = ttk.Frame(manual_frame)
        cmd_frame.pack(fill="x", pady=5)

        self.manual_cmd = ttk.Entry(cmd_frame)
        self.manual_cmd.pack(side="left", fill="x", expand=True, padx=(0, 5))
        self.manual_cmd.bind("<Return>", lambda e: self.send_manual_command())

        ttk.Button(cmd_frame, text="Send", command=self.send_manual_command).pack(side="left")

        # Normal mode commands
        normal_frame = ttk.LabelFrame(parent, text="Normal Mode Commands", padding="10")
        normal_frame.grid(row=2, column=0, sticky="ew", padx=5, pady=5)

        ttk.Label(normal_frame, text="These commands work outside of test mode:",
                  foreground="gray").pack(pady=5)

        normal_btn_frame = ttk.Frame(normal_frame)
        normal_btn_frame.pack(pady=5)

        ttk.Button(normal_btn_frame, text="Reset to IDLE",
                   command=lambda: self.send_command("r")).pack(side="left", padx=5)
        ttk.Button(normal_btn_frame, text="Toggle Logging",
                   command=lambda: self.send_command("l")).pack(side="left", padx=5)

    def setup_output_frame(self, parent):
        """Setup the serial output display."""
        output_frame = ttk.LabelFrame(parent, text="Serial Output", padding="5")
        output_frame.grid(row=2, column=0, sticky="nsew", pady=(10, 0))
        parent.rowconfigure(2, weight=1)

        # Output text area
        self.output_text = scrolledtext.ScrolledText(output_frame, height=12,
                                                      font=("Consolas", 9),
                                                      state="disabled")
        self.output_text.pack(fill="both", expand=True)

        # Control buttons
        ctrl_frame = ttk.Frame(output_frame)
        ctrl_frame.pack(fill="x", pady=(5, 0))

        ttk.Button(ctrl_frame, text="Clear", command=self.clear_output).pack(side="left")

        self.autoscroll_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(ctrl_frame, text="Auto-scroll",
                        variable=self.autoscroll_var).pack(side="left", padx=10)

    def refresh_ports(self):
        """Refresh the list of available serial ports."""
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_combo["values"] = ports
        if ports:
            self.port_combo.set(ports[0])

    def toggle_connection(self):
        """Connect or disconnect from the serial port."""
        if self.is_connected:
            self.disconnect()
        else:
            self.connect()

    def connect(self):
        """Establish serial connection."""
        port = self.port_combo.get()
        baud = int(self.baud_combo.get())

        if not port:
            messagebox.showerror("Error", "Please select a port")
            return

        try:
            self.serial_port = serial.Serial(port, baud, timeout=0.1)
            self.is_connected = True
            self.stop_thread = False

            # Start read thread
            self.read_thread = threading.Thread(target=self.read_serial, daemon=True)
            self.read_thread.start()

            # Update UI
            self.connect_btn.config(text="Disconnect")
            self.status_label.config(text="● Connected", foreground="green")
            self.log_output(f"Connected to {port} at {baud} baud\n")

        except Exception as e:
            messagebox.showerror("Connection Error", str(e))

    def disconnect(self):
        """Close serial connection."""
        self.stop_thread = True
        self.is_connected = False

        if self.serial_port:
            self.serial_port.close()
            self.serial_port = None

        # Reset toggle states
        self.imu_streaming = False
        self.button_test = False
        self.motor1_continuous = False
        self.motor2_continuous = False
        self.led_cycling = False
        self.update_toggle_buttons()

        # Update UI
        self.connect_btn.config(text="Connect")
        self.status_label.config(text="● Disconnected", foreground="red")
        self.log_output("Disconnected\n")

    def read_serial(self):
        """Background thread to read serial data."""
        while not self.stop_thread:
            try:
                if self.serial_port and self.serial_port.in_waiting:
                    data = self.serial_port.readline().decode("utf-8", errors="replace")
                    if data:
                        self.message_queue.put(data)
            except Exception as e:
                if not self.stop_thread:
                    self.message_queue.put(f"[Error: {e}]\n")
            time.sleep(0.01)

    def process_queue(self):
        """Process messages from the read thread."""
        try:
            while True:
                message = self.message_queue.get_nowait()
                self.log_output(message)
        except queue.Empty:
            pass

        # Schedule next check
        self.root.after(50, self.process_queue)

    def log_output(self, text):
        """Add text to the output display."""
        self.output_text.config(state="normal")
        self.output_text.insert("end", text)
        if self.autoscroll_var.get():
            self.output_text.see("end")
        self.output_text.config(state="disabled")

    def clear_output(self):
        """Clear the output display."""
        self.output_text.config(state="normal")
        self.output_text.delete("1.0", "end")
        self.output_text.config(state="disabled")

    def send_command(self, cmd):
        """Send a command to the device."""
        if not self.is_connected:
            messagebox.showwarning("Not Connected", "Please connect to a device first")
            return

        try:
            self.serial_port.write(f"{cmd}\n".encode())
            self.log_output(f"> {cmd}\n")
        except Exception as e:
            messagebox.showerror("Send Error", str(e))

    def send_manual_command(self):
        """Send the manual command entry."""
        cmd = self.manual_cmd.get().strip()
        if cmd:
            self.send_command(cmd)
            self.manual_cmd.delete(0, "end")

    # Toggle button handlers
    def toggle_m1_continuous(self):
        self.motor1_continuous = not self.motor1_continuous
        self.send_command("m1c")
        self.update_toggle_buttons()

    def toggle_m2_continuous(self):
        self.motor2_continuous = not self.motor2_continuous
        self.send_command("m2c")
        self.update_toggle_buttons()

    def toggle_streaming(self):
        self.imu_streaming = not self.imu_streaming
        self.send_command("stream")
        self.update_toggle_buttons()

    def toggle_button_test(self):
        self.button_test = not self.button_test
        self.send_command("btn")
        self.update_toggle_buttons()

    def toggle_led_cycle(self):
        self.led_cycling = not self.led_cycling
        self.send_command("led cycle" if self.led_cycling else "led off")
        self.update_toggle_buttons()

    def stop_all_motors(self):
        self.motor1_continuous = False
        self.motor2_continuous = False
        self.send_command("mstop")
        self.update_toggle_buttons()

    def update_toggle_buttons(self):
        """Update toggle button text based on state."""
        self.m1c_btn.config(text=f"Continuous: {'ON' if self.motor1_continuous else 'OFF'}")
        self.m2c_btn.config(text=f"Continuous: {'ON' if self.motor2_continuous else 'OFF'}")
        self.stream_btn.config(text=f"Stream: {'ON' if self.imu_streaming else 'OFF'}")
        self.btn_test_btn.config(text=f"Button Test: {'ON' if self.button_test else 'OFF'}")
        self.led_cycle_btn.config(text=f"{'Stop Cycling' if self.led_cycling else 'Cycle All Patterns'}")

    def on_closing(self):
        """Handle window close."""
        if self.is_connected:
            self.disconnect()
        self.root.destroy()


def main():
    root = tk.Tk()
    app = TestModeGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    root.mainloop()


if __name__ == "__main__":
    main()
