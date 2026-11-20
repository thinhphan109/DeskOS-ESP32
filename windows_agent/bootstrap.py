#!/usr/bin/env python3
"""Zero-command source launcher. Only installs packages after clicking Install."""
from __future__ import annotations

import importlib.util
import queue
import subprocess
import sys
import threading
from pathlib import Path


HERE = Path(__file__).resolve().parent


def installed() -> tuple[bool, str]:
    required = {"psutil": "PC metrics", "PySide6": "Modern GUI",
                "keyring": "Windows Credential Manager", "PIL": "Album artwork (Pillow)"}

    def has_module(name: str) -> bool:
        try:
            return importlib.util.find_spec(name) is not None
        except (ImportError, ModuleNotFoundError, ValueError):
            return False

    missing = [friendly for module, friendly in required.items() if not has_module(module)]
    if sys.platform == "win32":
        if not (has_module("winrt.windows.media.control") or
                has_module("winsdk.windows.media.control")):
            missing.append("Windows Media Controls")
        if not (has_module("winrt.windows.storage.streams") or
                has_module("winsdk.windows.storage.streams")):
            missing.append("Windows album image reader")
    return not missing, ("Ready — click Launch" if not missing else "Missing: " + ", ".join(missing))


def main() -> None:
    import tkinter as tk
    from tkinter import messagebox, scrolledtext, ttk

    root = tk.Tk()
    root.title("MèoDesk Companion · Setup")
    root.geometry("640x435")
    root.minsize(530, 370)
    root.configure(bg="#f7f4ef")
    ttk.Style().configure("TButton", padding=10)
    frame = tk.Frame(root, bg="#f7f4ef", padx=28, pady=24)
    frame.pack(fill="both", expand=True)
    tk.Label(frame, text="mèo.desk", bg="#f7f4ef", fg="#29473a",
             font=("Segoe UI", 30, "bold")).pack(anchor="w")
    tk.Label(frame, text="COMPANION / ONE-CLICK SETUP", bg="#f7f4ef", fg="#a4766c",
             font=("Consolas", 10)).pack(anchor="w", pady=(0, 15))
    state = tk.StringVar(value=installed()[1])
    tk.Label(frame, textvariable=state, bg="#f7f4ef", fg="#405147",
             wraplength=555, font=("Segoe UI", 11)).pack(anchor="w", pady=(0, 13))
    row = tk.Frame(frame, bg="#f7f4ef")
    row.pack(fill="x")
    log = scrolledtext.ScrolledText(frame, height=9, bg="#fffefa", fg="#34443a",
                                   font=("Consolas", 9))
    log.pack(fill="both", expand=True, pady=(15, 0))
    events: queue.SimpleQueue[str] = queue.SimpleQueue()

    def install() -> None:
        install_btn.configure(state="disabled")
        state.set("Installing selected packages; this can take a few minutes...")

        def job() -> None:
            proc = subprocess.Popen(
                [sys.executable, "-m", "pip", "install", "-r", str(HERE / "requirements-gui.txt")],
                cwd=HERE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                text=True, errors="replace", bufsize=1,
                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
            assert proc.stdout is not None
            for line in proc.stdout:
                events.put(line)
            events.put("__DONE__" + str(proc.wait()))

        threading.Thread(target=job, daemon=True).start()

    def launch() -> None:
        if not installed()[0]:
            messagebox.showinfo("Cần thư viện", "Click 'Cài thư viện' trước rồi thử lại.")
            return
        subprocess.Popen([sys.executable, str(HERE / "companion.py")], cwd=HERE,
                         creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        root.destroy()

    install_btn = ttk.Button(row, text="①  Cài thư viện còn thiếu", command=install)
    install_btn.pack(side="left", padx=(0, 10))
    ttk.Button(row, text="②  Mở MèoDesk", command=launch).pack(side="left")

    def tick() -> None:
        while not events.empty():
            item = events.get_nowait()
            if item.startswith("__DONE__"):
                ok = item == "__DONE__0"
                state.set("Installation complete. " + installed()[1] if ok else "Install failed. See log below.")
                install_btn.configure(state="normal")
            else:
                log.insert("end", item)
                log.see("end")
        root.after(150, tick)

    tick()
    root.mainloop()


if __name__ == "__main__":
    main()
