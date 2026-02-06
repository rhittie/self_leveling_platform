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
import queue
import re
import time


class MotorLimitsGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Motor Limits Setup")
        self.root.resizable(False, False)
        self.ser = None
        self.connected = False
        self.busy = False

        # Serial command queue - all serial ops go through worker thread
        self.cmd_queue = queue.Queue()
        self.worker_thread = None
        self.worker_running = False

        # Limits storage
        self.m1_pos = tk.IntVar(value=0)
        self.m2_pos = tk.IntVar(value=0)
        self.m1_in = tk.StringVar(value="--")
        self.m1_out = tk.StringVar(value="--")
        self.m2_in = tk.StringVar(value="--")
        self.m2_out = tk.StringVar(value="--")
        self.step_amount = tk.IntVar(value=100)
        self.status_msg = tk.StringVar(value="Disconnected")

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

        for i, val in enumerate([10, 50, 100, 500, 1000]):
            ttk.Radiobutton(step_frame, text=str(val), variable=self.step_amount,
                            value=val).grid(row=0, column=i, padx=6)

        ttk.Label(step_frame, text="Custom:").grid(row=0, column=5, padx=(12, 2))
        self.custom_steps = ttk.Entry(step_frame, width=6)
        self.custom_steps.grid(row=0, column=6)
        ttk.Button(step_frame, text="Set", command=self._set_custom_steps).grid(row=0, column=7, padx=2)

        # Motor 1
        self._build_motor_frame("Motor 1 (Left Back)", 2, self.m1_pos, self.m1_in, self.m1_out, 1)

        # Motor 2
        self._build_motor_frame("Motor 2 (Right Back)", 3, self.m2_pos, self.m2_in, self.m2_out, 2)

        # Summary
        summary = ttk.LabelFrame(self.root, text="Config Values", padding=8)
        summary.grid(row=4, column=0, columnspan=2, padx=8, pady=4, sticky="ew")

        self.summary_text = tk.Text(summary, height=4, width=55, font=("Consolas", 10), state="disabled")
        self.summary_text.grid(row=0, column=0, columnspan=2)
        ttk.Button(summary, text="Copy to Clipboard", command=self._copy_summary).grid(row=1, column=0, pady=(4, 0))

        # Log panel
        log_frame = ttk.LabelFrame(self.root, text="Serial Log", padding=4)
        log_frame.grid(row=5, column=0, columnspan=2, padx=8, pady=4, sticky="ew")

        self.log_text = tk.Text(log_frame, height=8, width=55, font=("Consolas", 9), state="disabled")
        self.log_text.grid(row=0, column=0, sticky="ew")
        scrollbar = ttk.Scrollbar(log_frame, orient="vertical", command=self.log_text.yview)
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.log_text.config(yscrollcommand=scrollbar.set)

        # Reset button
        ttk.Button(self.root, text="Reset Positions to 0", command=self._reset_positions).grid(
            row=6, column=0, columnspan=2, pady=8)

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

        plus_btn = tk.Button(btn_frame, text="  +  ", font=("", 14, "bold"), bg="#51cf66", fg="white",
                             command=lambda: self._move_motor(motor_num, 1))
        plus_btn.grid(row=0, column=1, padx=8)

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

    # ==================== Serial Worker ====================

    def _start_worker(self):
        self.worker_running = True
        self.worker_thread = threading.Thread(target=self._worker_loop, daemon=True)
        self.worker_thread.start()

    def _stop_worker(self):
        self.worker_running = False
        self.cmd_queue.put(None)

    def _worker_loop(self):
        while self.worker_running:
            try:
                item = self.cmd_queue.get(timeout=0.5)
            except queue.Empty:
                continue
            if item is None:
                break
            cmd, wait, callback = item
            try:
                self._log(f">> {cmd}")
                lines = self._send_raw(cmd, wait)
                for l in lines:
                    self._log(f"<< {l}")
                if callback:
                    self.root.after(0, callback, lines)
            except Exception as e:
                self._log(f"ERROR: {e}")
                self.root.after(0, self._on_serial_error, str(e))

    def _send_raw(self, cmd, wait=0.5):
        if not self.ser or not self.ser.is_open:
            return []
        # Drain stale data
        while self.ser.in_waiting:
            self.ser.readline()
        self.ser.write(f"{cmd}\n".encode())
        time.sleep(wait)
        lines = []
        while self.ser.in_waiting:
            line = self.ser.readline().decode("utf-8", errors="replace").strip()
            if line:
                lines.append(line)
        return lines

    def _queue_cmd(self, cmd, wait=0.5, callback=None):
        self.cmd_queue.put((cmd, wait, callback))

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

    def _connect(self):
        port = self.port_var.get()
        if not port:
            messagebox.showerror("Error", "Select a port first")
            return

        self.status_msg.set("Connecting...")
        self.status_label.config(foreground="orange")
        self.root.update()

        try:
            self.ser = serial.Serial(port, 115200, timeout=0.5)
            self._log(f"Opened {port}, waiting for boot...")
            # Wait for ESP32 boot (setup() has delay(1000) + init time)
            time.sleep(3)
            # Drain all boot messages
            while self.ser.in_waiting:
                line = self.ser.readline().decode("utf-8", errors="replace").strip()
                if line:
                    self._log(f"boot: {line}")
            time.sleep(0.5)
            while self.ser.in_waiting:
                line = self.ser.readline().decode("utf-8", errors="replace").strip()
                if line:
                    self._log(f"boot: {line}")

            # Start the worker thread
            self._start_worker()

            # First reset to IDLE in case we're in some other state
            self._queue_cmd("r", 0.5)
            # Enter test mode
            self._queue_cmd("admin", 1.0)
            # Init IMU
            self._queue_cmd("imu", 2.0)
            # Unlock motor position limits so we can find physical extents
            self._queue_cmd("munlock", 0.5)
            # Query positions to verify communication
            self._queue_cmd("mpos", 0.5, self._on_connect_mpos)

            self.connected = True
            self.connect_btn.config(text="Disconnect")

        except Exception as e:
            self._log(f"Connection failed: {e}")
            messagebox.showerror("Connection Error", str(e))
            self.status_msg.set("Disconnected")
            self.status_label.config(foreground="red")

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
        if self.connected:
            self._stop_worker()
        if self.ser:
            try:
                self._send_raw("exit", 0.5)
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

    def _on_move_done(self, lines):
        self._queue_cmd("mpos", 0.5, self._on_mpos_response)

    # ==================== Commands ====================

    def _move_motor(self, motor_num, direction):
        if not self.connected:
            messagebox.showwarning("Not Connected", "Connect first")
            return
        if self.busy:
            return
        self.busy = True
        steps = self.step_amount.get() * direction
        wait = max(0.5, abs(steps) * 0.003)
        self.status_msg.set(f"Moving M{motor_num} {steps:+d} steps...")
        self.status_label.config(foreground="orange")
        self._queue_cmd(f"m{motor_num} {steps}", wait, self._on_move_done)

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
            if 1 <= val <= 10000:
                self.step_amount.set(val)
        except ValueError:
            pass

    def _reset_positions(self):
        if not self.connected:
            return
        self._queue_cmd("mreset", 0.5, self._on_mpos_response)

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
