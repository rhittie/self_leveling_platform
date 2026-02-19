#!/usr/bin/env python3
"""Motor Limits Setup GUI.

Move motors to their physical extents and set IN/OUT limits.
Connects via serial in test mode.
"""
import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import threading
import re
import time


class MotorLimitsGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Motor Limits Setup")
        self.root.resizable(True, True)
        self.root.minsize(500, 600)
        self.ser = None
        self.connected = False
        self.busy = False
        self.serial_lock = threading.Lock()

        # Limits storage
        self.m1_pos = tk.IntVar(value=0)
        self.m2_pos = tk.IntVar(value=0)
        self.m1_in = tk.StringVar(value="--")
        self.m1_out = tk.StringVar(value="--")
        self.m2_in = tk.StringVar(value="--")
        self.m2_out = tk.StringVar(value="--")
        self.step_amount = tk.IntVar(value=100)
        self.status_msg = tk.StringVar(value="Disconnected")

        self.root.columnconfigure(0, weight=1)
        self.root.rowconfigure(6, weight=1)  # Log panel row stretches

        self._build_ui()

    def _build_ui(self):
        # Connection frame
        conn = ttk.LabelFrame(self.root, text="Connection", padding=8)
        conn.grid(row=0, column=0, columnspan=2, padx=8, pady=(8, 4), sticky="ew")

        ttk.Label(conn, text="Port:").grid(row=0, column=0)
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(conn, textvariable=self.port_var, width=12)
        self.port_combo.grid(row=0, column=1, padx=4)
        self._refresh_ports()

        ttk.Button(conn, text="Refresh", command=self._refresh_ports).grid(row=0, column=2, padx=2)
        self.connect_btn = ttk.Button(conn, text="Connect", command=self._toggle_connect)
        self.connect_btn.grid(row=0, column=3, padx=4)
        self.status_label = ttk.Label(conn, textvariable=self.status_msg, foreground="red")
        self.status_label.grid(row=0, column=4, padx=8)

        # Step amount
        step_frame = ttk.LabelFrame(self.root, text="Step Amount", padding=8)
        step_frame.grid(row=1, column=0, columnspan=2, padx=8, pady=4, sticky="ew")

        for i, val in enumerate([100, 500, 1000, 5000, 10000]):
            ttk.Radiobutton(step_frame, text=str(val), variable=self.step_amount,
                            value=val).grid(row=0, column=i, padx=6)

        ttk.Label(step_frame, text="Custom:").grid(row=0, column=5, padx=(12, 2))
        self.custom_steps = ttk.Entry(step_frame, width=8)
        self.custom_steps.grid(row=0, column=6)
        ttk.Button(step_frame, text="Set", command=self._set_custom_steps).grid(row=0, column=7, padx=2)

        # Both Motors
        both_frame = ttk.LabelFrame(self.root, text="Both Motors", padding=8)
        both_frame.grid(row=2, column=0, columnspan=2, padx=8, pady=4, sticky="ew")

        both_btn_frame = ttk.Frame(both_frame)
        both_btn_frame.pack()

        tk.Button(both_btn_frame, text="  -  ", font=("", 14, "bold"), bg="#ff6b6b", fg="white",
                  command=lambda: self._move_both(-1)).grid(row=0, column=0, padx=8)
        tk.Button(both_btn_frame, text="  +  ", font=("", 14, "bold"), bg="#51cf66", fg="white",
                  command=lambda: self._move_both(1)).grid(row=0, column=1, padx=8)

        # Motor 1
        self._build_motor_frame("Motor 1 (Left Back)", 3, self.m1_pos, self.m1_in, self.m1_out, 1)

        # Motor 2
        self._build_motor_frame("Motor 2 (Right Back)", 4, self.m2_pos, self.m2_in, self.m2_out, 2)

        # Summary
        summary = ttk.LabelFrame(self.root, text="Config Values", padding=8)
        summary.grid(row=5, column=0, columnspan=2, padx=8, pady=4, sticky="ew")

        self.summary_text = tk.Text(summary, height=4, width=55, font=("Consolas", 10), state="disabled")
        self.summary_text.grid(row=0, column=0, columnspan=2)
        ttk.Button(summary, text="Copy to Clipboard", command=self._copy_summary).grid(row=1, column=0, pady=(4, 0))

        # Log panel
        log_frame = ttk.LabelFrame(self.root, text="Serial Log", padding=4)
        log_frame.grid(row=6, column=0, columnspan=2, padx=8, pady=4, sticky="nsew")
        log_frame.columnconfigure(0, weight=1)
        log_frame.rowconfigure(0, weight=1)

        self.log_text = tk.Text(log_frame, height=8, width=55, font=("Consolas", 9), state="disabled")
        self.log_text.grid(row=0, column=0, sticky="nsew")
        scrollbar = ttk.Scrollbar(log_frame, orient="vertical", command=self.log_text.yview)
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.log_text.config(yscrollcommand=scrollbar.set)

        # Reset button
        ttk.Button(self.root, text="Reset Positions to 0", command=self._reset_positions).grid(
            row=7, column=0, columnspan=2, pady=8)

    def _build_motor_frame(self, title, row, pos_var, in_var, out_var, motor_num):
        frame = ttk.LabelFrame(self.root, text=title, padding=10)
        frame.grid(row=row, column=0, columnspan=2, padx=8, pady=4, sticky="ew")

        # Position display
        ttk.Label(frame, text="Position:", font=("", 10, "bold")).grid(row=0, column=0, columnspan=2)
        pos_label = ttk.Label(frame, textvariable=pos_var, font=("Consolas", 18, "bold"), width=8, anchor="center")
        pos_label.grid(row=1, column=0, columnspan=2, pady=4)

        # + / - buttons
        btn_frame = ttk.Frame(frame)
        btn_frame.grid(row=2, column=0, columnspan=2, pady=4)

        minus_btn = tk.Button(btn_frame, text="  -  ", font=("", 14, "bold"), bg="#ff6b6b", fg="white",
                              command=lambda: self._move_motor(motor_num, -1))
        minus_btn.grid(row=0, column=0, padx=8)

        tk.Button(btn_frame, text="Set Zero", font=("", 10), bg="#868e96", fg="white",
                  command=lambda: self._reset_single_motor(motor_num)).grid(row=0, column=1, padx=8)

        plus_btn = tk.Button(btn_frame, text="  +  ", font=("", 14, "bold"), bg="#51cf66", fg="white",
                             command=lambda: self._move_motor(motor_num, 1))
        plus_btn.grid(row=0, column=2, padx=8)

        # Limit buttons
        limit_frame = ttk.Frame(frame)
        limit_frame.grid(row=3, column=0, columnspan=2, pady=(8, 0))

        tk.Button(limit_frame, text="Set IN (min)", font=("", 10), bg="#339af0", fg="white",
                  command=lambda: self._set_limit(motor_num, "in")).grid(row=0, column=0, padx=8)
        tk.Button(limit_frame, text="Set OUT (max)", font=("", 10), bg="#f59f00", fg="white",
                  command=lambda: self._set_limit(motor_num, "out")).grid(row=0, column=1, padx=8)

        # Limit display
        info_frame = ttk.Frame(frame)
        info_frame.grid(row=4, column=0, columnspan=2, pady=(6, 0))

        ttk.Label(info_frame, text="IN:").grid(row=0, column=0)
        ttk.Label(info_frame, textvariable=in_var, font=("Consolas", 11), width=7, anchor="center").grid(row=0, column=1)
        ttk.Label(info_frame, text="OUT:").grid(row=0, column=2, padx=(12, 0))
        ttk.Label(info_frame, textvariable=out_var, font=("Consolas", 11), width=7, anchor="center").grid(row=0, column=3)

    # ==================== Logging ====================

    def _log(self, msg):
        """Append a message to the log panel. Thread-safe via root.after."""
        def do_log():
            self.log_text.config(state="normal")
            self.log_text.insert(tk.END, msg + "\n")
            self.log_text.see(tk.END)
            self.log_text.config(state="disabled")
        self.root.after(0, do_log)

    # ==================== Serial ====================

    def _on_serial_error(self, msg):
        self.status_msg.set(f"Error: {msg}")
        self.status_label.config(foreground="red")
        self.busy = False

    # ==================== Connection ====================

    def _refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_combo["values"] = ports
        if ports:
            if "COM4" in ports:
                self.port_var.set("COM4")
            else:
                self.port_var.set(ports[0])

    def _toggle_connect(self):
        if self.connected:
            self._disconnect()
        else:
            self._connect()

    def _send_direct(self, cmd, wait=0.5):
        """Send a command directly and return response lines. Thread-safe via lock."""
        if not self.ser or not self.ser.is_open:
            return []
        with self.serial_lock:
            self.ser.reset_input_buffer()
            self._log(f">> {cmd}")
            self.ser.write(f"{cmd}\n".encode())
            time.sleep(wait)
            lines = []
            while self.ser.in_waiting:
                line = self.ser.readline().decode("utf-8", errors="replace").strip()
                if line:
                    lines.append(line)
                    self._log(f"<< {line}")
            return lines

    def _connect(self):
        port = self.port_var.get()
        if not port:
            messagebox.showerror("Error", "Select a port first")
            return

        self.status_msg.set("Connecting...")
        self.status_label.config(foreground="orange")
        self.root.update()

        def do_connect():
            try:
                self.ser = serial.Serial(port, 115200, timeout=0.5)
                self._log(f"Opened {port}, waiting for boot...")
                time.sleep(3)
                # Drain boot messages
                while self.ser.in_waiting:
                    line = self.ser.readline().decode("utf-8", errors="replace").strip()
                    if line:
                        self._log(f"boot: {line}")
                time.sleep(0.5)
                while self.ser.in_waiting:
                    line = self.ser.readline().decode("utf-8", errors="replace").strip()
                    if line:
                        self._log(f"boot: {line}")

                # All setup commands sent directly - no worker thread
                self._send_direct("r", 0.5)
                self._send_direct("admin", 1.0)
                self._send_direct("munlock", 0.5)
                lines = self._send_direct("mpos", 0.5)

                # Parse mpos response
                found = False
                for l in lines:
                    m = re.match(r'\[MPOS\]\s+M1:([-\d]+)\s+M2:([-\d]+)', l)
                    if m:
                        def update(m1=int(m.group(1)), m2=int(m.group(2))):
                            self.m1_pos.set(m1)
                            self.m2_pos.set(m2)
                            self.status_msg.set(f"Connected  |  M1:{m1}  M2:{m2}")
                            self.status_label.config(foreground="green")
                            self.connected = True
                            self.connect_btn.config(text="Disconnect")
                            self._log("Connection verified - test mode active")
                        self.root.after(0, update)
                        found = True

                if not found:
                    def warn():
                        self.status_msg.set("Connected (no MPOS response)")
                        self.status_label.config(foreground="orange")
                        self.connected = True
                        self.connect_btn.config(text="Disconnect")
                    self.root.after(0, warn)

            except Exception as e:
                self._log(f"Connection failed: {e}")
                def show_err():
                    messagebox.showerror("Connection Error", str(e))
                    self.status_msg.set("Disconnected")
                    self.status_label.config(foreground="red")
                self.root.after(0, show_err)

        threading.Thread(target=do_connect, daemon=True).start()

    def _on_connect_mpos(self, lines):
        """Verify connection by checking mpos response."""
        found = False
        for l in lines:
            m = re.match(r'\[MPOS\]\s+M1:([-\d]+)\s+M2:([-\d]+)', l)
            if m:
                self.m1_pos.set(int(m.group(1)))
                self.m2_pos.set(int(m.group(2)))
                found = True
        if found:
            self.status_msg.set(f"Connected  |  M1:{self.m1_pos.get()}  M2:{self.m2_pos.get()}")
            self.status_label.config(foreground="green")
            self._log("Connection verified - test mode active")
        else:
            self.status_msg.set("Connected but no MPOS response!")
            self.status_label.config(foreground="orange")
            self._log("WARNING: No [MPOS] in response - may not be in test mode")

    def _disconnect(self):
        if self.ser:
            try:
                self.ser.write(b"exit\n")
                time.sleep(0.5)
                self.ser.close()
            except:
                pass
        self.ser = None
        self.connected = False
        self.connect_btn.config(text="Connect")
        self.status_msg.set("Disconnected")
        self.status_label.config(foreground="red")
        self._log("Disconnected")

    # ==================== Response Handlers (main thread) ====================

    def _on_mpos_response(self, lines):
        for l in lines:
            m = re.match(r'\[MPOS\]\s+M1:([-\d]+)\s+M2:([-\d]+)', l)
            if m:
                self.m1_pos.set(int(m.group(1)))
                self.m2_pos.set(int(m.group(2)))
        self.busy = False
        self.status_msg.set(f"Ready  |  M1:{self.m1_pos.get()}  M2:{self.m2_pos.get()}")
        self.status_label.config(foreground="green")

    def _on_move_done(self, motor_num, steps):
        """Update position client-side instead of querying serial."""
        def handler(lines):
            if motor_num == 1:
                self.m1_pos.set(self.m1_pos.get() + steps)
            else:
                self.m2_pos.set(self.m2_pos.get() + steps)
            self.busy = False
            self.status_msg.set(f"Ready  |  M1:{self.m1_pos.get()}  M2:{self.m2_pos.get()}")
            self.status_label.config(foreground="green")
        return handler

    # ==================== Commands ====================

    def _move_both(self, direction):
        if not self.connected:
            messagebox.showwarning("Not Connected", "Connect first")
            return
        if self.busy:
            return
        self.busy = True
        steps = self.step_amount.get() * direction
        # Motor 2 is physically reversed
        steps_m2 = -steps
        wait = max(0.5, abs(steps) * 0.003)
        self.status_msg.set(f"Moving both motors {steps:+d} steps...")
        self.status_label.config(foreground="orange")

        def do_move():
            try:
                self._send_direct(f"m1 {steps}", wait)
                self._send_direct(f"m2 {steps_m2}", wait)
                def update_ui():
                    self.m1_pos.set(self.m1_pos.get() + steps)
                    self.m2_pos.set(self.m2_pos.get() + steps)  # Display matches M1 direction
                    self.busy = False
                    self.status_msg.set(f"Ready  |  M1:{self.m1_pos.get()}  M2:{self.m2_pos.get()}")
                    self.status_label.config(foreground="green")
                self.root.after(0, update_ui)
            except Exception as e:
                self._log(f"ERROR: {e}")
                self.root.after(0, lambda: self._on_serial_error(str(e)))

        threading.Thread(target=do_move, daemon=True).start()

    def _move_motor(self, motor_num, direction):
        if not self.connected:
            messagebox.showwarning("Not Connected", "Connect first")
            return
        if self.busy:
            return
        self.busy = True
        # Motor 2 direction is physically reversed (lead screw orientation)
        display_direction = direction
        if motor_num == 2:
            direction = -direction
        steps = self.step_amount.get() * direction
        display_steps = self.step_amount.get() * display_direction
        wait = max(0.5, abs(steps) * 0.003)
        self.status_msg.set(f"Moving M{motor_num} {steps:+d} steps...")
        self.status_label.config(foreground="orange")

        def do_move():
            try:
                cmd = f"m{motor_num} {steps}"
                self._log(f">> {cmd}")
                self.ser.reset_input_buffer()
                self.ser.write(f"{cmd}\n".encode())
                time.sleep(wait)
                while self.ser.in_waiting:
                    line = self.ser.readline().decode("utf-8", errors="replace").strip()
                    if line:
                        self._log(f"<< {line}")
                # Update position client-side
                def update_ui():
                    if motor_num == 1:
                        self.m1_pos.set(self.m1_pos.get() + display_steps)
                    else:
                        self.m2_pos.set(self.m2_pos.get() + display_steps)
                    self.busy = False
                    self.status_msg.set(f"Ready  |  M1:{self.m1_pos.get()}  M2:{self.m2_pos.get()}")
                    self.status_label.config(foreground="green")
                self.root.after(0, update_ui)
            except Exception as e:
                self._log(f"ERROR: {e}")
                self.root.after(0, lambda: self._on_serial_error(str(e)))

        threading.Thread(target=do_move, daemon=True).start()

    def _set_limit(self, motor_num, which):
        if motor_num == 1:
            pos = self.m1_pos.get()
            if which == "in":
                self.m1_in.set(str(pos))
            else:
                self.m1_out.set(str(pos))
        else:
            pos = self.m2_pos.get()
            if which == "in":
                self.m2_in.set(str(pos))
            else:
                self.m2_out.set(str(pos))
        self._update_summary()

    def _set_custom_steps(self):
        try:
            val = int(self.custom_steps.get())
            if 1 <= val <= 100000:
                self.step_amount.set(val)
        except ValueError:
            pass

    def _reset_single_motor(self, motor_num):
        if not self.connected or self.busy:
            return
        def do_reset():
            self._send_direct(f"mreset{motor_num}", 0.5)
            def update_ui():
                if motor_num == 1:
                    self.m1_pos.set(0)
                else:
                    self.m2_pos.set(0)
                self.status_msg.set(f"Ready  |  M1:{self.m1_pos.get()}  M2:{self.m2_pos.get()}")
            self.root.after(0, update_ui)
        threading.Thread(target=do_reset, daemon=True).start()

    def _reset_positions(self):
        if not self.connected or self.busy:
            return
        def do_reset():
            self._send_direct("mreset", 0.5)
            def update_ui():
                self.m1_pos.set(0)
                self.m2_pos.set(0)
                self.status_msg.set("Ready  |  M1:0  M2:0")
            self.root.after(0, update_ui)
        threading.Thread(target=do_reset, daemon=True).start()

    # ==================== Summary ====================

    def _update_summary(self):
        m1_in = self.m1_in.get()
        m1_out = self.m1_out.get()
        m2_in = self.m2_in.get()
        m2_out = self.m2_out.get()

        vals = []
        for v in [m1_in, m1_out, m2_in, m2_out]:
            if v != "--":
                vals.append(int(v))

        lines = []
        lines.append(f"M1: IN={m1_in}  OUT={m1_out}")
        lines.append(f"M2: IN={m2_in}  OUT={m2_out}")

        if vals:
            min_val = min(vals)
            max_val = max(vals)
            lines.append("")
            lines.append(f"// config.h values:")
            lines.append(f"#define MOTOR_MIN_POSITION {min_val}")
            lines.append(f"#define MOTOR_MAX_POSITION {max_val}")

        self.summary_text.config(state="normal")
        self.summary_text.delete("1.0", tk.END)
        self.summary_text.insert("1.0", "\n".join(lines))
        self.summary_text.config(state="disabled")

    def _copy_summary(self):
        self.root.clipboard_clear()
        self.summary_text.config(state="normal")
        text = self.summary_text.get("1.0", tk.END).strip()
        self.summary_text.config(state="disabled")
        self.root.clipboard_append(text)

    def on_close(self):
        self._disconnect()
        self.root.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = MotorLimitsGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()
