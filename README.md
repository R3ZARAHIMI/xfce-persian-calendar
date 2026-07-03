<div align="center">

# Persian Calendar — Xfce Panel Plugin

**English** · [فارسی](README.fa.md)

[![Language](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Toolkit](https://img.shields.io/badge/Toolkit-GTK3-orange.svg)](https://www.gtk.org/)
[![Desktop](https://img.shields.io/badge/Desktop-Xfce-green.svg)](https://xfce.org/)
[![License](https://img.shields.io/badge/License-GPL--3.0-red.svg)](#-license)

A lightweight, native Xfce panel plugin that shows the Jalali (Persian) date with rich customization.

![Screenshot](images/sce.png)

</div>

---

## ✨ Features

- Smart Jalali date display with ready-made or custom formats
- Custom font and text size
- Separate colors for normal and holiday days
- Automatic holiday detection (Solar, Hijri, Gregorian)
- Vertical and horizontal panel support
- Popup calendar with Hijri/Gregorian equivalents and events

---

## 🚀 Installation

### Automatic (Recommended)

```bash
chmod +x install.sh
./install.sh
```

The script detects your distro, installs prerequisites, builds, installs, and restarts the panel.

### Manual

**1. Prerequisites**

```bash
# Arch
sudo pacman -S meson ninja pkgconf xfce4-panel libxfce4ui libxfce4util gtk3 glib2 xfce4-dev-tools

# Debian / Ubuntu
sudo apt install meson ninja-build pkg-config libxfce4panel-2.0-dev libxfce4ui-2-dev libgtk-3-dev libglib2.0-dev xfce4-dev-tools

# Fedora
sudo dnf install meson ninja-build pkgconf xfce4-panel-devel libxfce4ui-devel gtk3-devel glib2-devel xfce4-dev-tools
```

**2. Build & Install**

```bash
meson setup build --prefix=/usr
ninja -C build
sudo ninja -C build install
```

> [!IMPORTANT]
> `--prefix=/usr` is required. The plugin path (e.g. `/usr/lib64/`) is detected automatically by Meson.

---

## 🔄 Activate

```bash
xfce4-panel -r
```

Right-click the panel → **Add New Items…** → search **Persian Calendar** → **Add**.

---

## ⚙️ Configuration

Right-click the plugin → **Properties**.

**Custom format codes**

| Code | Meaning | Code | Meaning |
|---|---|---|---|
| `A%` | Weekday | `Y%` | 4-digit year |
| `d%` | Day of month | `y%` | 2-digit year |
| `m%` | Month number | `hd%` `hB%` `hY%` | Hijri date |
| `B%` | Month name | `gd%` `gB%` `gY%` | Gregorian date |

---

## 📄 License

Released under **GPL-3.0**. Free to use, modify, and distribute.
