#!/usr/bin/env python3
"""
Pico / Pico 2 (RP2040 / RP2350) Memory Footprint Analyzer & Visual Report Generator
Parses GCC/GNU ld .map files and ELF targets to provide:
  1. Detailed console memory summary in decimal (Bytes, KB, %, Free, Bars).
  2. Interactive self-contained HTML visual report with Treemap, Table & Search.
"""

from __future__ import annotations

import argparse
import datetime
import json
import os
import pathlib
import re
import sys
from collections import defaultdict
from html import escape


def clean_symbol_name(name: str) -> str:
    s = name.strip()
    for pfx in (".text.", ".rodata.", ".bss.", ".data.", "text.", "rodata.", "bss.", "data.", "_ZL"):
        if s.startswith(pfx):
            s = s[len(pfx):]
    m_lead = re.match(r'^\d+([A-Za-z_].*)$', s)
    if m_lead:
        s = m_lead.group(1)
    return s


def format_bytes(n: int | float) -> str:
    if not isinstance(n, (int, float)) or n < 0:
        return "0 B"
    n_int = int(round(n))
    return f"{n_int:,} B".replace(",", " ")


def format_kb(n: int | float) -> str:
    kb = n / 1024.0
    return f"{kb:,.2f} KB".replace(",", " ")


def make_bar(percent: float, width: int = 20) -> str:
    p = max(0.0, min(100.0, percent))
    filled = int(round((p / 100.0) * width))
    empty = width - filled
    return "[" + ("#" * filled) + ("-" * empty) + "]"


def categorize_path(path: str) -> str:
    p = path.replace("\\", "/").lower()
    base = pathlib.PurePosixPath(p).name
    
    # Specific drivers / libraries in this workspace
    if "utft2" in p or "st7789" in p or "resource_classic_fonts" in p:
        return "Display: uTFT2"
    if "loguart" in p or "timber" in p:
        return "Lib: logUART (Timber)"
    if "tinyusb" in p:
        return "Pico SDK: TinyUSB"
    if "pico-sdk" in p or "rp2_common" in p or "rp2350" in p or "bs2_" in p:
        return "Pico SDK"
    if "libgcc.a" in p or "libgcc" in p:
        return "Toolchain: libgcc"
    if "libg.a" in p or "libc.a" in p or "libm.a" in p:
        return "Toolchain: libc (Newlib)"
    if "libstdc++" in p:
        return "Toolchain: libstdc++"
    if "pico2_2.cpp" in p or "main.c" in p or "main.cpp" in p:
        return "Application (pico2_2)"
    if p.endswith(".cpp.obj") or p.endswith(".c.obj"):
        return f"App Module: {base.replace('.obj', '')}"
    return "Other"


class MemoryAnalyzer:
    def __init__(self, map_path: pathlib.Path, elf_path: pathlib.Path | None = None):
        self.map_path = map_path
        self.elf_path = elf_path
        
        # Memory configuration regions: name -> {origin: int, length: int, attributes: str}
        self.regions: dict[str, dict] = {}
        
        # Top-level sections: name -> {addr: int, size: int, load_addr: int}
        self.top_sections: dict[str, dict] = {}
        
        # Detailed items: list of input section entries
        self.items: list[dict] = []
        
        # Aggregated stats
        self.components: dict[str, dict] = defaultdict(lambda: {
            "name": "", "total": 0, "flash": 0, "ram": 0, "text": 0, "rodata": 0, "data": 0, "bss": 0, "objects": set()
        })
        self.objects: dict[str, dict] = defaultdict(lambda: {
            "name": "", "full": "", "component": "", "total": 0, "flash": 0, "ram": 0,
            "text": 0, "rodata": 0, "data": 0, "bss": 0, "sections": defaultdict(int), "symbols": []
        })
        self.symbols: list[dict] = []

        self.flash_total = 4 * 1024 * 1024  # default 4MB for Pico 2
        self.ram_total = 512 * 1024         # default 512KB for RP2350
        self.scratch_x_total = 4 * 1024
        self.scratch_y_total = 4 * 1024

        self.parse_map()

    def parse_map(self) -> None:
        if not self.map_path.exists():
            raise FileNotFoundError(f"Map file not found: {self.map_path}")

        with open(self.map_path, "r", encoding="utf-8", errors="ignore") as f:
            lines = f.readlines()

        in_mem_config = False
        in_map = False
        cur_top_sec = ""
        cur_load_addr = 0

        # Pass 1: Parse Memory Configuration & Linker script entries
        i = 0
        total_lines = len(lines)
        while i < total_lines:
            line = lines[i].rstrip()

            if "Memory Configuration" in line:
                in_mem_config = True
                i += 1
                continue

            if in_mem_config:
                if line.startswith("Linker script and memory map") or line.startswith("OUTPUT("):
                    in_mem_config = False
                    in_map = True
                else:
                    parts = line.split()
                    if len(parts) >= 3 and parts[0] != "Name":
                        name = parts[0]
                        try:
                            origin = int(parts[1], 16)
                            length = int(parts[2], 16)
                            attrs = parts[3] if len(parts) > 3 else ""
                            self.regions[name] = {"origin": origin, "length": length, "attributes": attrs}
                            if name == "FLASH" and length > 0:
                                self.flash_total = length
                            elif name == "RAM" and length > 0:
                                self.ram_total = length
                            elif name == "SCRATCH_X" and length > 0:
                                self.scratch_x_total = length
                            elif name == "SCRATCH_Y" and length > 0:
                                self.scratch_y_total = length
                        except ValueError:
                            pass
                    i += 1
                    continue

            if "Linker script and memory map" in line:
                in_map = True
                i += 1
                continue

            if not in_map:
                i += 1
                continue

            if line.startswith("OUTPUT("):
                break

            # Check for top-level section: e.g.
            # .text           0x10000000     0x78a4
            # .data           0x20000110     0x1078 load address 0x10009f74
            top_m = re.match(r"^(\.[a-zA-Z0-9_\.]+)\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)(?:\s+load address\s+(0x[0-9a-fA-F]+))?", line)
            if top_m:
                cur_top_sec = top_m.group(1)
                s_addr = int(top_m.group(2), 16)
                s_size = int(top_m.group(3), 16)
                cur_load_addr = int(top_m.group(4), 16) if top_m.group(4) else s_addr
                self.top_sections[cur_top_sec] = {
                    "addr": s_addr,
                    "size": s_size,
                    "load_addr": cur_load_addr
                }
                i += 1
                continue

            # Match input sections:
            # Single-line:
            #   .text.main     0x10000490      0x368 CMakeFiles/pico2_2.dir/pico2_2.cpp.obj
            #   .bss.LCD_Buffer16
            #                  0x20001310     0xfd20 CMakeFiles/pico2_2.dir/pico2_2.cpp.obj
            m_single = re.match(r"^\s+(\.[a-zA-Z0-9_\.\$]+)\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+(.+)$", line)
            sec_name, addr_s, size_s, file_str = None, None, None, None
            advance = 1

            if m_single:
                sec_name, addr_s, size_s, file_str = m_single.groups()
            else:
                m_wrap = re.match(r"^\s+(\.[a-zA-Z0-9_\.\$]+)$", line)
                if m_wrap and i + 1 < total_lines:
                    next_line = lines[i + 1].rstrip()
                    m_next = re.match(r"^\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+(.+)$", next_line)
                    if m_next:
                        sec_name = m_wrap.group(1)
                        addr_s, size_s, file_str = m_next.groups()
                        advance = 2

            if sec_name and addr_s and size_s and file_str:
                addr = int(addr_s, 16)
                size = int(size_s, 16)
                if size > 0 and not sec_name.startswith((".debug_", ".comment", ".ARM.attributes", ".group")):
                    clean_file = file_str.strip().replace("\\", "/")
                    obj_name = pathlib.PurePosixPath(clean_file).name
                    comp_name = categorize_path(clean_file)

                    # Determine memory region
                    is_flash = False
                    is_ram = False
                    is_scratch_x = False
                    is_scratch_y = False

                    if 0x10000000 <= addr < 0x11000000:
                        is_flash = True
                        mem_kind = "FLASH"
                    elif 0x20000000 <= addr < 0x20080000:
                        is_ram = True
                        mem_kind = "RAM"
                    elif 0x20080000 <= addr < 0x20081000:
                        is_scratch_x = True
                        mem_kind = "SCRATCH_X"
                    elif 0x20081000 <= addr < 0x20082000:
                        is_scratch_y = True
                        mem_kind = "SCRATCH_Y"
                    else:
                        mem_kind = "OTHER"

                    # Classify section type (.text, .rodata, .data, .bss)
                    sec_type = "other"
                    if ".text" in sec_name or sec_name in (".vectors", ".reset", ".init", ".fini"):
                        sec_type = "text"
                    elif ".rodata" in sec_name or sec_name.startswith((".rodata", ".binary_info")):
                        sec_type = "rodata"
                    elif ".data" in sec_name:
                        sec_type = "data"
                        # Initialized data lives in RAM at runtime, but its initial image is stored in Flash!
                    elif ".bss" in sec_name or sec_name in (".tbss", ".sbss"):
                        sec_type = "bss"

                    item = {
                        "section": sec_name,
                        "type": sec_type,
                        "addr": addr,
                        "size": size,
                        "file": clean_file,
                        "obj": obj_name,
                        "component": comp_name,
                        "mem_kind": mem_kind,
                        "is_flash": is_flash or (sec_type == "data"),
                        "is_ram": is_ram or is_scratch_x or is_scratch_y,
                        "symbols": []
                    }
                    self.items.append(item)

                    # Update component stats
                    c = self.components[comp_name]
                    c["name"] = comp_name
                    c["total"] += size
                    c["objects"].add(obj_name)
                    if item["is_flash"]:
                        c["flash"] += size
                    if item["is_ram"]:
                        c["ram"] += size
                    if sec_type in ("text", "rodata", "data", "bss"):
                        c[sec_type] += size

                    # Update object stats
                    o = self.objects[obj_name]
                    o["name"] = obj_name
                    o["full"] = clean_file
                    o["component"] = comp_name
                    o["total"] += size
                    if item["is_flash"]:
                        o["flash"] += size
                    if item["is_ram"]:
                        o["ram"] += size
                    if sec_type in ("text", "rodata", "data", "bss"):
                        o[sec_type] += size
                    o["sections"][sec_name] += size

                    # Check for symbols immediately following this input section
                    peek_idx = i + advance
                    while peek_idx < total_lines:
                        p_line = lines[peek_idx].rstrip()
                        if p_line.startswith(" .") or p_line.startswith(".") or p_line.startswith("OUTPUT("):
                            break
                        # Symbol line format: 16 spaces, hex address, symbol name
                        sym_m = re.match(r"^\s{16}(0x[0-9a-fA-F]+)\s+([a-zA-Z0-9_:\<\>\*&,\(\)\~ \.\+\[\]]+)$", p_line)
                        if sym_m:
                            sym_addr = int(sym_m.group(1), 16)
                            sym_name = sym_m.group(2).strip()
                            if not sym_name.startswith(("__", ".")) and " = " not in sym_name:
                                item["symbols"].append({"name": sym_name, "addr": sym_addr})
                        peek_idx += 1

                    # Create a symbol record if input section has a primary symbol
                    primary_name = item["symbols"][0]["name"] if item["symbols"] else sec_name
                    self.symbols.append({
                        "name": primary_name,
                        "section": sec_name,
                        "type": sec_type,
                        "size": size,
                        "addr": addr,
                        "obj": obj_name,
                        "file": clean_file,
                        "component": comp_name,
                        "mem_kind": mem_kind,
                        "is_flash": item["is_flash"],
                        "is_ram": item["is_ram"]
                    })

                i += advance
                continue

            i += 1

    def get_totals(self) -> dict:
        flash_used = 0
        ram_used = 0
        scratch_x_used = 0
        scratch_y_used = 0

        # Calculate exact footprint from top sections
        text_size = self.top_sections.get(".text", {}).get("size", 0)
        rodata_size = self.top_sections.get(".rodata", {}).get("size", 0)
        data_size = self.top_sections.get(".data", {}).get("size", 0)
        bss_size = self.top_sections.get(".bss", {}).get("size", 0)
        bin_info_size = self.top_sections.get(".binary_info", {}).get("size", 0)
        ram_vec_size = self.top_sections.get(".ram_vector_table", {}).get("size", 0)
        scratch_x_size = self.top_sections.get(".scratch_x", {}).get("size", 0)
        scratch_y_size = self.top_sections.get(".scratch_y", {}).get("size", 0)
        stack_dummy_size = self.top_sections.get(".stack_dummy", {}).get("size", 0)
        heap_size = self.top_sections.get(".heap", {}).get("size", 0)

        # If top_sections didn't catch all, sum from items
        if text_size == 0 and rodata_size == 0:
            for item in self.items:
                if item["is_flash"]:
                    flash_used += item["size"]
                if item["is_ram"]:
                    ram_used += item["size"]
        else:
            # Flash contains code (.text), constants (.rodata), binary_info, and initial data image (.data)
            flash_used = text_size + rodata_size + bin_info_size + data_size
            # RAM contains initialized data (.data), zero-init data (.bss), and ram vectors
            ram_used = data_size + bss_size + ram_vec_size
            scratch_x_used = scratch_x_size
            scratch_y_used = scratch_y_size + stack_dummy_size

        return {
            "flash_used": flash_used,
            "flash_total": self.flash_total,
            "flash_free": max(0, self.flash_total - flash_used),
            "flash_pct": (flash_used / self.flash_total * 100.0) if self.flash_total else 0.0,
            
            "ram_used": ram_used,
            "ram_total": self.ram_total,
            "ram_free": max(0, self.ram_total - ram_used),
            "ram_pct": (ram_used / self.ram_total * 100.0) if self.ram_total else 0.0,
            
            "scratch_x_used": scratch_x_used,
            "scratch_x_total": self.scratch_x_total,
            "scratch_x_free": max(0, self.scratch_x_total - scratch_x_used),
            "scratch_x_pct": (scratch_x_used / self.scratch_x_total * 100.0) if self.scratch_x_total else 0.0,

            "scratch_y_used": scratch_y_used,
            "scratch_y_total": self.scratch_y_total,
            "scratch_y_free": max(0, self.scratch_y_total - scratch_y_used),
            "scratch_y_pct": (scratch_y_used / self.scratch_y_total * 100.0) if self.scratch_y_total else 0.0,

            "sections": {
                ".text (Code)": text_size,
                ".rodata (Constants)": rodata_size,
                ".data (Init Data)": data_size,
                ".bss (Zero-Init Data)": bss_size,
                ".ram_vectors": ram_vec_size,
                ".heap (Min Reserve)": heap_size,
                ".stack (Min Reserve)": stack_dummy_size,
            }
        }

    def print_console_summary(self, top_n: int = 8) -> None:
        stats = self.get_totals()
        w = 80
        sep = "=" * w
        thin_sep = "-" * w

        print()
        print(sep)
        print("                   PICO 2 (RP2350) - MEMORY FOOTPRINT")
        print(sep)
        print(f"  {'Memory Region':<16} {'Used (Dec)':>12} {'Used (KB)':>12} {'Total (KB)':>12} {'Free (KB)':>12} {'Usage':>7}   {'Progress'}")
        print(thin_sep)

        # FLASH
        f_u_b = format_bytes(stats['flash_used'])
        f_u_k = format_kb(stats['flash_used'])
        f_t_k = format_kb(stats['flash_total'])
        f_f_k = format_kb(stats['flash_free'])
        f_bar = make_bar(stats['flash_pct'], 16)
        print(f"  {'FLASH':<16} {f_u_b:>12} {f_u_k:>12} {f_t_k:>12} {f_f_k:>12} {stats['flash_pct']:>6.2f}%   {f_bar}")

        # RAM
        r_u_b = format_bytes(stats['ram_used'])
        r_u_k = format_kb(stats['ram_used'])
        r_t_k = format_kb(stats['ram_total'])
        r_f_k = format_kb(stats['ram_free'])
        r_bar = make_bar(stats['ram_pct'], 16)
        print(f"  {'RAM':<16} {r_u_b:>12} {r_u_k:>12} {r_t_k:>12} {r_f_k:>12} {stats['ram_pct']:>6.2f}%   {r_bar}")

        # RAM details
        d_sz = stats['sections']['.data (Init Data)']
        b_sz = stats['sections']['.bss (Zero-Init Data)']
        v_sz = stats['sections']['.ram_vectors']
        if d_sz > 0:
            print(f"    |-- .data (init)   {format_bytes(d_sz):>10} {format_kb(d_sz):>12} (stored in Flash, loaded to RAM)")
        if b_sz > 0:
            print(f"    |-- .bss (zero)    {format_bytes(b_sz):>10} {format_kb(b_sz):>12}")
        if v_sz > 0:
            print(f"    \\-- .ram_vectors   {format_bytes(v_sz):>10} {format_kb(v_sz):>12}")

        # SCRATCH_X
        sx_u_b = format_bytes(stats['scratch_x_used'])
        sx_u_k = format_kb(stats['scratch_x_used'])
        sx_t_k = format_kb(stats['scratch_x_total'])
        sx_f_k = format_kb(stats['scratch_x_free'])
        sx_bar = make_bar(stats['scratch_x_pct'], 16)
        print(f"  {'SCRATCH_X':<16} {sx_u_b:>12} {sx_u_k:>12} {sx_t_k:>12} {sx_f_k:>12} {stats['scratch_x_pct']:>6.2f}%   {sx_bar}")

        # SCRATCH_Y
        sy_u_b = format_bytes(stats['scratch_y_used'])
        sy_u_k = format_kb(stats['scratch_y_used'])
        sy_t_k = format_kb(stats['scratch_y_total'])
        sy_f_k = format_kb(stats['scratch_y_free'])
        sy_bar = make_bar(stats['scratch_y_pct'], 16)
        print(f"  {'SCRATCH_Y':<16} {sy_u_b:>12} {sy_u_k:>12} {sy_t_k:>12} {sy_f_k:>12} {stats['scratch_y_pct']:>6.2f}%   {sy_bar}")

        print(thin_sep)
        print(f"  Top {top_n} Largest Object Files (RAM + Flash):")
        sorted_objs = sorted(self.objects.values(), key=lambda x: x["total"], reverse=True)[:top_n]
        for idx, obj in enumerate(sorted_objs, 1):
            nm = obj["name"]
            tot = format_bytes(obj["total"])
            fl = format_bytes(obj["flash"])
            rm = format_bytes(obj["ram"])
            print(f"   {idx}. {nm:<34} {tot:>10}  (RAM: {rm:>8}, Flash: {fl:>8})")

            # Find top symbol in this object if any
            obj_syms = [s for s in self.symbols if s["obj"] == nm and s["size"] > 512]
            if obj_syms:
                top_sym = max(obj_syms, key=lambda s: s["size"])
                s_name = clean_symbol_name(top_sym["name"])
                if len(s_name) > 30:
                    s_name = s_name[:27] + "..."
                s_tot = format_bytes(top_sym["size"])
                s_kb = format_kb(top_sym["size"])
                sec_short = top_sym["section"].split(".")[1] if "." in top_sym["section"] else top_sym["section"]
                m_type = "RAM" if top_sym["is_ram"] else "Flash"
                print(f"      \\-- {s_name} (.{sec_short}) {s_tot:>10} ({s_kb}) [{m_type}]")

        print(sep)
        print(f"  Map: {self.map_path.name}")
        print(sep)
        print()


HTML_TEMPLATE = r"""<!doctype html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Pico 2 (RP2350) Memory Footprint</title>
<style>
:root {
  color-scheme: dark;
  --bg: #0d1117;
  --panel: #161b22;
  --panel2: #21262d;
  --text: #f0f6fc;
  --muted: #8b949e;
  --line: #30363d;
  --accent: #58a6ff;
  --accent-green: #3fb950;
  --accent-purple: #bc8cff;
  --accent-orange: #d29922;
}
* { box-sizing: border-box; }
body {
  margin: 0;
  font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
  background: var(--bg);
  color: var(--text);
  overflow-x: hidden;
}
header {
  padding: 16px 24px;
  background: #12161f;
  border-bottom: 1px solid var(--line);
  display: flex;
  justify-content: space-between;
  align-items: center;
  flex-wrap: wrap;
  gap: 12px;
}
h1 {
  margin: 0 0 4px;
  font-size: 20px;
  font-weight: 650;
  letter-spacing: -0.2px;
  display: flex;
  align-items: center;
  gap: 10px;
}
.badge {
  font-size: 11px;
  font-weight: 600;
  padding: 2px 8px;
  border-radius: 12px;
  background: rgba(88, 166, 255, 0.15);
  color: var(--accent);
  border: 1px solid rgba(88, 166, 255, 0.3);
}
.meta {
  color: var(--muted);
  font-size: 13px;
}
.cards-row {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
  gap: 12px;
  padding: 16px 24px;
  background: #0f141c;
  border-bottom: 1px solid var(--line);
}
.stat-card {
  background: var(--panel);
  border: 1px solid var(--line);
  border-radius: 8px;
  padding: 12px 16px;
  display: flex;
  flex-direction: column;
  gap: 6px;
}
.stat-title {
  font-size: 12px;
  font-weight: 600;
  color: var(--muted);
  text-transform: uppercase;
  letter-spacing: 0.5px;
  display: flex;
  justify-content: space-between;
}
.stat-value {
  font-size: 18px;
  font-weight: 700;
  display: flex;
  align-items: baseline;
  gap: 8px;
}
.stat-sub {
  font-size: 12px;
  color: var(--muted);
  font-weight: 400;
}
.stat-progress {
  height: 6px;
  background: #28303e;
  border-radius: 4px;
  overflow: hidden;
  margin-top: 4px;
}
.stat-bar-fill {
  height: 100%;
  border-radius: 4px;
  transition: width 0.3s ease;
}
.bar-flash { background: linear-gradient(90deg, #388bfd, #58a6ff); }
.bar-ram { background: linear-gradient(90deg, #2ea043, #3fb950); }
.bar-scratch { background: linear-gradient(90deg, #bb80ff, #bc8cff); }

.toolbar {
  display: grid;
  grid-template-columns: repeat(4, minmax(130px, 1fr));
  gap: 12px;
  padding: 12px 24px;
  background: #11151d;
  border-bottom: 1px solid var(--line);
}
label {
  display: grid;
  gap: 4px;
  color: var(--muted);
  font-size: 11px;
  font-weight: 600;
  text-transform: uppercase;
}
select, input {
  width: 100%;
  height: 34px;
  border: 1px solid var(--line);
  border-radius: 6px;
  background: var(--panel);
  color: var(--text);
  padding: 0 10px;
  font: inherit;
  font-size: 13px;
  outline: none;
  transition: border-color 0.2s;
}
select:focus, input:focus {
  border-color: var(--accent);
}

main {
  display: grid;
  grid-template-columns: minmax(0, 1fr) 440px;
  gap: 14px;
  padding: 14px 24px 24px;
}
.panel {
  min-width: 0;
  background: var(--panel);
  border: 1px solid var(--line);
  border-radius: 8px;
  overflow: hidden;
  display: flex;
  flex-direction: column;
}
.panel h2 {
  margin: 0;
  padding: 12px 16px;
  font-size: 13px;
  font-weight: 650;
  color: #c9d1d9;
  background: var(--panel2);
  border-bottom: 1px solid var(--line);
  display: flex;
  justify-content: space-between;
  align-items: center;
}
#treemap {
  position: relative;
  height: calc(100vh - 290px);
  min-height: 540px;
  overflow: hidden;
  background: #090c10;
}
.cell {
  position: absolute;
  border: 1px solid rgba(0, 0, 0, 0.4);
  overflow: hidden;
  padding: 6px 8px;
  cursor: pointer;
  transition: transform 0.1s, box-shadow 0.1s;
}
.cell:hover {
  outline: 2px solid #fff;
  z-index: 10;
  box-shadow: 0 4px 14px rgba(0,0,0,0.5);
}
.cell-name {
  font-size: 11px;
  font-weight: 700;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  color: #fff;
  text-shadow: 0 1px 2px rgba(0,0,0,0.8);
}
.cell-size {
  margin-top: 2px;
  font-size: 11px;
  color: rgba(255, 255, 255, 0.9);
  font-variant-numeric: tabular-nums;
  text-shadow: 0 1px 2px rgba(0,0,0,0.8);
}
#details {
  padding: 14px 16px;
  background: #141820;
  border-bottom: 1px solid var(--line);
  font-size: 12px;
  line-height: 1.5;
  min-height: 100px;
}
.detail-title {
  font-size: 14px;
  font-weight: 700;
  color: var(--accent);
  margin-bottom: 4px;
  word-break: break-all;
}
.detail-grid {
  display: grid;
  grid-template-columns: 80px 1fr;
  gap: 4px 8px;
  margin-top: 8px;
  font-size: 12px;
}
.detail-label {
  color: var(--muted);
}
.detail-val {
  color: var(--text);
  font-family: monospace;
}
#tableWrap {
  flex: 1;
  max-height: calc(100vh - 430px);
  overflow-y: auto;
}
table {
  width: 100%;
  border-collapse: collapse;
  font-size: 12px;
}
th, td {
  padding: 8px 12px;
  border-bottom: 1px solid var(--line);
  text-align: left;
}
th {
  color: var(--muted);
  background: #151a23;
  position: sticky;
  top: 0;
  font-weight: 600;
  font-size: 11px;
  text-transform: uppercase;
  z-index: 2;
}
tr:hover {
  background: rgba(88, 166, 255, 0.08);
  cursor: pointer;
}
td.size {
  white-space: nowrap;
  text-align: right;
  font-variant-numeric: tabular-nums;
  font-family: monospace;
  font-size: 12px;
}
.bar-wrap {
  width: 100%;
  height: 4px;
  background: #252b36;
  border-radius: 2px;
  margin-top: 4px;
  overflow: hidden;
}
.bar-fill {
  height: 100%;
  background: var(--accent-green);
  border-radius: 2px;
}
@media (max-width: 1100px) {
  main { grid-template-columns: 1fr; }
  .toolbar { grid-template-columns: repeat(2, minmax(130px, 1fr)); }
  #treemap { height: 500px; }
  #tableWrap { max-height: 400px; }
}
</style>
</head>
<body>
<header>
  <div>
    <h1>Pico 2 (RP2350) Memory Footprint <span class="badge">ARM Cortex-M33</span></h1>
    <div class="meta">Map: <code>__MAP_FILE__</code> · Сгенерировано: __GENERATED__</div>
  </div>
</header>

<section class="cards-row">
  <div class="stat-card">
    <div class="stat-title">FLASH (__FLASH_LABEL__) <span>__FLASH_PCT__%</span></div>
    <div class="stat-value">__FLASH_USED__ <span class="stat-sub">из __FLASH_TOTAL__</span></div>
    <div class="stat-progress"><div class="stat-bar-fill bar-flash" style="width: __FLASH_PCT__%"></div></div>
    <div class="stat-sub" style="margin-top:4px;">Свободно: <b>__FLASH_FREE__</b></div>
  </div>
  <div class="stat-card">
    <div class="stat-title">RAM (__RAM_LABEL__) <span>__RAM_PCT__%</span></div>
    <div class="stat-value">__RAM_USED__ <span class="stat-sub">из __RAM_TOTAL__</span></div>
    <div class="stat-progress"><div class="stat-bar-fill bar-ram" style="width: __RAM_PCT__%"></div></div>
    <div class="stat-sub" style="margin-top:4px;">Свободно: <b>__RAM_FREE__</b></div>
  </div>
  <div class="stat-card">
    <div class="stat-title">SCRATCH_X (4 KB) <span>__SX_PCT__%</span></div>
    <div class="stat-value">__SX_USED__ <span class="stat-sub">из __SX_TOTAL__</span></div>
    <div class="stat-progress"><div class="stat-bar-fill bar-scratch" style="width: __SX_PCT__%"></div></div>
    <div class="stat-sub" style="margin-top:4px;">Свободно: <b>__SX_FREE__</b></div>
  </div>
  <div class="stat-card">
    <div class="stat-title">SCRATCH_Y (4 KB) <span>__SY_PCT__%</span></div>
    <div class="stat-value">__SY_USED__ <span class="stat-sub">из __SY_TOTAL__</span></div>
    <div class="stat-progress"><div class="stat-bar-fill bar-scratch" style="width: __SY_PCT__%"></div></div>
    <div class="stat-sub" style="margin-top:4px;">Свободно: <b>__SY_FREE__</b></div>
  </div>
</section>

<section class="toolbar">
  <label>Вид
    <select id="kind">
      <option value="components">Компоненты / Библиотеки</option>
      <option value="objects" selected>Файлы (.obj)</option>
      <option value="symbols">Символы / Переменные</option>
    </select>
  </label>
  <label>Память / Секция
    <select id="metric">
      <option value="total">Всего (Total Size)</option>
      <option value="flash">Flash (Code + RoData + Data Load)</option>
      <option value="ram">RAM (Data + BSS)</option>
      <option value="text">.text (Код функции)</option>
      <option value="rodata">.rodata (Константы / Шрифты)</option>
      <option value="bss">.bss (Глобальные буферы = 0)</option>
      <option value="data">.data (Инициализированные данные)</option>
    </select>
  </label>
  <label>Фильтр / Поиск
    <input id="filter" placeholder="LCD, Font, ST7789, timber...">
  </label>
  <label>Количество
    <select id="limit">
      <option value="40">Top 40</option>
      <option value="80" selected>Top 80</option>
      <option value="200">Top 200</option>
      <option value="999999">Все</option>
    </select>
  </label>
</section>

<main>
  <section class="panel">
    <h2 id="mapTitle">Treemap <span></span></h2>
    <div id="treemap"></div>
  </section>

  <aside class="panel">
    <h2>Детали и Рейтинг</h2>
    <div id="details">
      <div class="meta">Наведите курсор на прямоугольник или кликните строку таблицы для подробностей.</div>
    </div>
    <div id="tableWrap">
      <table>
        <thead>
          <tr>
            <th>Имя элемента</th>
            <th class="size">Размер (Dec)</th>
          </tr>
        </thead>
        <tbody id="rows"></tbody>
      </table>
    </div>
  </aside>
</main>

<script>
const DATA = __DATA__;

const els = {
  kind: document.getElementById('kind'),
  metric: document.getElementById('metric'),
  filter: document.getElementById('filter'),
  limit: document.getElementById('limit'),
  treemap: document.getElementById('treemap'),
  rows: document.getElementById('rows'),
  details: document.getElementById('details'),
  mapTitle: document.getElementById('mapTitle')
};

function bytes(n) {
  if (!Number.isFinite(n) || n <= 0) return '0 B';
  const decStr = Math.round(n).toLocaleString('ru-RU') + ' B';
  if (n >= 1024 * 1024) {
    return `${(n / 1024 / 1024).toFixed(2)} MB (${decStr})`;
  }
  if (n >= 1024) {
    return `${(n / 1024).toFixed(2)} KB (${decStr})`;
  }
  return decStr;
}

function shortBytes(n) {
  if (!Number.isFinite(n) || n <= 0) return '0 B';
  if (n >= 1024 * 1024) return (n / 1024 / 1024).toFixed(2) + ' MB';
  if (n >= 1024) return (n / 1024).toFixed(1) + ' KB';
  return Math.round(n) + ' B';
}

function valueOf(item, metric) {
  if (metric === 'total') return item.total || item.size || 0;
  if (metric === 'flash') return item.flash || (item.is_flash ? item.size : 0) || 0;
  if (metric === 'ram') return item.ram || (item.is_ram ? item.size : 0) || 0;
  if (metric === 'text') return item.text || (item.type === 'text' ? item.size : 0) || 0;
  if (metric === 'rodata') return item.rodata || (item.type === 'rodata' ? item.size : 0) || 0;
  if (metric === 'bss') return item.bss || (item.type === 'bss' ? item.size : 0) || 0;
  if (metric === 'data') return item.data || (item.type === 'data' ? item.size : 0) || 0;
  return 0;
}

function colorFor(name, value, maxValue) {
  let hash = 0;
  for (let i = 0; i < name.length; i++) hash = ((hash << 5) - hash + name.charCodeAt(i)) | 0;
  const hue = Math.abs(hash) % 360;
  const light = 32 + Math.min(24, Math.sqrt(value / Math.max(maxValue, 1)) * 24);
  return `hsl(${hue}, 60%, ${light}%)`;
}

function layout(items, x, y, w, h, out) {
  if (!items.length || w <= 1 || h <= 1) return;
  if (items.length === 1) {
    out.push({ item: items[0], x, y, w, h });
    return;
  }
  const total = items.reduce((sum, item) => sum + item.val, 0);
  if (total <= 0) return;

  let acc = 0;
  let split = 1;
  for (; split < items.length; split++) {
    if (acc + items[split - 1].val >= total / 2) break;
    acc += items[split - 1].val;
  }
  split = Math.max(1, Math.min(items.length - 1, split));
  const left = items.slice(0, split);
  const right = items.slice(split);
  const leftTotal = left.reduce((sum, item) => sum + item.val, 0);
  const ratio = leftTotal / total;

  if (w >= h) {
    const w1 = Math.round(w * ratio);
    layout(left, x, y, w1, h, out);
    layout(right, x + w1, y, w - w1, h, out);
  } else {
    const h1 = Math.round(h * ratio);
    layout(left, x, y, w, h1, out);
    layout(right, x, y + h1, w, h - h1, out);
  }
}

function visibleItems() {
  const kind = els.kind.value;
  const metric = els.metric.value;
  const filter = els.filter.value.trim().toLowerCase();
  const limit = Number(els.limit.value);

  const rawList = DATA[kind] || [];
  return rawList
    .map(item => ({ ...item, val: valueOf(item, metric) }))
    .filter(item => item.val > 0)
    .filter(item => {
      if (!filter) return true;
      const haystack = (item.name + ' ' + (item.full || '') + ' ' + (item.component || '') + ' ' + (item.section || '')).toLowerCase();
      return haystack.includes(filter);
    })
    .sort((a, b) => b.val - a.val)
    .slice(0, limit);
}

function render() {
  const items = visibleItems();
  const total = items.reduce((sum, item) => sum + item.val, 0);
  const maxValue = items[0]?.val || 1;

  els.mapTitle.innerHTML = `Treemap · <b>${items.length}</b> элементов <span style="font-weight:normal;color:var(--muted)">Итого: ${bytes(total)}</span>`;
  els.treemap.innerHTML = '';

  const rect = els.treemap.getBoundingClientRect();
  const placed = [];
  layout(items, 0, 0, Math.max(1, rect.width), Math.max(1, rect.height), placed);

  for (const node of placed) {
    const item = node.item;
    const cell = document.createElement('div');
    cell.className = 'cell';
    cell.style.left = node.x + 'px';
    cell.style.top = node.y + 'px';
    cell.style.width = Math.max(0, node.w) + 'px';
    cell.style.height = Math.max(0, node.h) + 'px';
    cell.style.background = colorFor(item.name, item.val, maxValue);
    cell.title = `${item.name}\n${bytes(item.val)}`;

    if (node.w > 65 && node.h > 32) {
      cell.innerHTML = `<div class="cell-name">${escapeHtml(item.name)}</div><div class="cell-size">${shortBytes(item.val)}</div>`;
    }

    cell.addEventListener('mouseenter', () => showDetails(item, total));
    cell.addEventListener('click', () => showDetails(item, total));
    els.treemap.appendChild(cell);
  }

  els.rows.innerHTML = '';
  for (const item of items) {
    const tr = document.createElement('tr');
    const pct = total ? Math.max(0.5, (item.val / total) * 100) : 0;
    tr.innerHTML = `
      <td>
        <div><b>${escapeHtml(item.name)}</b> <span style="color:var(--muted);font-size:11px">${item.component ? '[' + escapeHtml(item.component) + ']' : ''}</span></div>
        <div class="bar-wrap"><div class="bar-fill" style="width:${pct.toFixed(1)}%"></div></div>
      </td>
      <td class="size">${shortBytes(item.val)}<br><span style="color:var(--muted);font-size:11px">${pct.toFixed(1)}%</span></td>
    `;
    tr.addEventListener('mouseenter', () => showDetails(item, total));
    tr.addEventListener('click', () => showDetails(item, total));
    els.rows.appendChild(tr);
  }

  if (items.length > 0) {
    showDetails(items[0], total);
  }
}

function showDetails(item, total) {
  const pct = total ? ((item.val / total) * 100).toFixed(2) : '0';
  const hexAddr = item.addr !== undefined ? '0x' + Number(item.addr).toString(16).padStart(8, '0') : '';
  const decAddr = item.addr !== undefined ? item.addr.toLocaleString('ru-RU') : '';

  let html = `
    <div class="detail-title">${escapeHtml(item.name)}</div>
    <div class="detail-grid">
      <span class="detail-label">Размер:</span>
      <span class="detail-val"><b>${bytes(item.val)}</b> (${pct}% от видимого)</span>
  `;

  if (item.component) {
    html += `<span class="detail-label">Модуль:</span><span class="detail-val">${escapeHtml(item.component)}</span>`;
  }
  if (item.obj) {
    html += `<span class="detail-label">Файл .obj:</span><span class="detail-val">${escapeHtml(item.obj)}</span>`;
  }
  if (item.full) {
    html += `<span class="detail-label">Путь:</span><span class="detail-val" style="word-break:break-all">${escapeHtml(item.full)}</span>`;
  }
  if (item.section) {
    html += `<span class="detail-label">Секция:</span><span class="detail-val">${escapeHtml(item.section)}</span>`;
  }
  if (hexAddr) {
    html += `<span class="detail-label">Адрес:</span><span class="detail-val">${hexAddr} (dec: ${decAddr})</span>`;
  }

  // Memory breakdown if available
  if (item.flash || item.ram) {
    html += `
      <span class="detail-label">Расклад:</span>
      <span class="detail-val">Flash: ${shortBytes(item.flash || 0)} · RAM: ${shortBytes(item.ram || 0)}</span>
    `;
  }
  if (item.text || item.rodata || item.data || item.bss) {
    html += `
      <span class="detail-label">Секции:</span>
      <span class="detail-val">.text: ${shortBytes(item.text||0)} · .rodata: ${shortBytes(item.rodata||0)} · .data: ${shortBytes(item.data||0)} · .bss: ${shortBytes(item.bss||0)}</span>
    `;
  }

  html += `</div>`;
  els.details.innerHTML = html;
}

function escapeHtml(text) {
  return String(text || '').replace(/[&<>"']/g, ch => ({'&':'&amp;', '<':'&lt;', '>':'&gt;', '"':'&quot;', "'":'&#039;'}[ch]));
}

for (const el of [els.kind, els.metric, els.filter, els.limit]) {
  el.addEventListener('input', render);
}
window.addEventListener('resize', render);
render();
</script>
</body>
</html>
"""


def build_html_report(analyzer: MemoryAnalyzer, output_path: pathlib.Path) -> None:
    stats = analyzer.get_totals()

    # Format components
    comp_list = []
    for c in analyzer.components.values():
        comp_list.append({
            "name": c["name"],
            "total": c["total"],
            "flash": c["flash"],
            "ram": c["ram"],
            "text": c["text"],
            "rodata": c["rodata"],
            "data": c["data"],
            "bss": c["bss"],
            "objects_count": len(c["objects"])
        })
    comp_list.sort(key=lambda x: x["total"], reverse=True)

    # Format objects
    obj_list = []
    for o in analyzer.objects.values():
        obj_list.append({
            "name": o["name"],
            "full": o["full"],
            "component": o["component"],
            "total": o["total"],
            "flash": o["flash"],
            "ram": o["ram"],
            "text": o["text"],
            "rodata": o["rodata"],
            "data": o["data"],
            "bss": o["bss"]
        })
    obj_list.sort(key=lambda x: x["total"], reverse=True)

    # Format symbols
    sym_list = []
    for s in analyzer.symbols:
        if s["size"] > 0:
            sym_list.append({
                "name": clean_symbol_name(s["name"]),
                "raw_name": s["name"],
                "section": s["section"],
                "type": s["type"],
                "size": s["size"],
                "addr": s["addr"],
                "obj": s["obj"],
                "file": s["file"],
                "component": s["component"],
                "is_flash": s["is_flash"],
                "is_ram": s["is_ram"]
            })
    sym_list.sort(key=lambda x: x["size"], reverse=True)

    data = {
        "components": comp_list,
        "objects": obj_list,
        "symbols": sym_list
    }

    html = HTML_TEMPLATE
    html = html.replace("__DATA__", json.dumps(data, ensure_ascii=False, separators=(",", ":")))
    html = html.replace("__MAP_FILE__", escape(str(analyzer.map_path.name)))
    html = html.replace("__GENERATED__", datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))

    flash_label = f"{stats['flash_total'] // (1024 * 1024)} MB" if stats['flash_total'] >= 1024 * 1024 else format_kb(stats['flash_total'])
    ram_label = f"{stats['ram_total'] // 1024} KB" if stats['ram_total'] < 1024 * 1024 else f"{stats['ram_total'] // (1024 * 1024)} MB"
    html = html.replace("__FLASH_LABEL__", flash_label)
    html = html.replace("__RAM_LABEL__", ram_label)

    html = html.replace("__FLASH_USED__", format_bytes(stats["flash_used"]))
    html = html.replace("__FLASH_TOTAL__", format_bytes(stats["flash_total"]))
    html = html.replace("__FLASH_FREE__", format_bytes(stats["flash_free"]))
    html = html.replace("__FLASH_PCT__", f"{stats['flash_pct']:.2f}")

    html = html.replace("__RAM_USED__", format_bytes(stats["ram_used"]))
    html = html.replace("__RAM_TOTAL__", format_bytes(stats["ram_total"]))
    html = html.replace("__RAM_FREE__", format_bytes(stats["ram_free"]))
    html = html.replace("__RAM_PCT__", f"{stats['ram_pct']:.2f}")

    html = html.replace("__SX_USED__", format_bytes(stats["scratch_x_used"]))
    html = html.replace("__SX_TOTAL__", format_bytes(stats["scratch_x_total"]))
    html = html.replace("__SX_FREE__", format_bytes(stats["scratch_x_free"]))
    html = html.replace("__SX_PCT__", f"{stats['scratch_x_pct']:.2f}")

    html = html.replace("__SY_USED__", format_bytes(stats["scratch_y_used"]))
    html = html.replace("__SY_TOTAL__", format_bytes(stats["scratch_y_total"]))
    html = html.replace("__SY_FREE__", format_bytes(stats["scratch_y_free"]))
    html = html.replace("__SY_PCT__", f"{stats['scratch_y_pct']:.2f}")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(html, encoding="utf-8")


def main() -> int:
    if sys.platform == "win32":
        try:
            if hasattr(sys.stdout, "reconfigure"):
                sys.stdout.reconfigure(encoding="utf-8", errors="replace")
            if hasattr(sys.stderr, "reconfigure"):
                sys.stderr.reconfigure(encoding="utf-8", errors="replace")
        except Exception:
            pass

    parser = argparse.ArgumentParser(description="Pico 2 Memory Footprint Analyzer & Visual Report Generator")
    parser.add_argument("--map", help="Path to .elf.map file")
    parser.add_argument("--elf", help="Path to .elf file")
    parser.add_argument("--html", help="Path to output .html visual report")
    parser.add_argument("--no-console", action="store_true", help="Do not print terminal summary")
    parser.add_argument("--top", type=int, default=8, help="Number of top items to show in console")
    args = parser.parse_args()

    # Find map file if not provided
    map_file = None
    if args.map:
        map_file = pathlib.Path(args.map)
    else:
        # Search build directory
        candidates = [
            pathlib.Path("build/pico2_2.elf.map"),
            pathlib.Path("pico2_2.elf.map"),
            pathlib.Path("../build/pico2_2.elf.map"),
        ]
        for c in candidates:
            if c.exists():
                map_file = c
                break

    if not map_file or not map_file.exists():
        print(f"[ERROR] Map file not found. Specify with --map path/to/target.elf.map", file=sys.stderr)
        return 1

    elf_file = pathlib.Path(args.elf) if args.elf else None

    analyzer = MemoryAnalyzer(map_file, elf_file)

    if not args.no_console:
        analyzer.print_console_summary(top_n=args.top)

    if args.html:
        html_out = pathlib.Path(args.html)
        build_html_report(analyzer, html_out)
        print(f"  Visual HTML report: {html_out.resolve()}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
