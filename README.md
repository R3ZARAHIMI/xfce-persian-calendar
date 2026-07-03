<div align="center">

# Persian Calendar — Xfce Panel Plugin
### تقویم جلالی برای پنل Xfce

[![Language](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Toolkit](https://img.shields.io/badge/Toolkit-GTK3-orange.svg)](https://www.gtk.org/)
[![Desktop](https://img.shields.io/badge/Desktop-Xfce-green.svg)](https://xfce.org/)
[![License](https://img.shields.io/badge/License-GPL--3.0-red.svg)](#-license--لایسنس)

A lightweight, native Xfce panel plugin that shows the Jalali (Persian) date with rich customization.
افزونه‌ای سبک و بومی برای پنل Xfce که تاریخ جلالی را با قابلیت شخصی‌سازی فراوان نمایش می‌دهد.

![Screenshot](images/sce.png)

</div>

---

## ✨ Features · ویژگی‌ها

| English | فارسی |
|---|---|
| Smart Jalali date display with ready-made or custom formats | نمایش تاریخ شمسی با قالب‌های آماده یا سفارشی |
| Custom font and text size | تنظیم فونت و اندازه متن |
| Separate colors for normal and holiday days | رنگ مجزا برای روزهای عادی و تعطیل |
| Automatic holiday detection (Solar, Hijri, Gregorian) | تشخیص خودکار تعطیلات شمسی، قمری و میلادی |
| Vertical and horizontal panel support | پشتیبانی از پنل عمودی و افقی |
| Popup calendar with Hijri/Gregorian equivalents and events | تقویم بازشو با معادل قمری/میلادی و مناسبت‌ها |

---

## 🚀 Installation · نصب

### Automatic · خودکار (Recommended · پیشنهادی)

```bash
chmod +x install.sh
./install.sh
```

The script detects your distro, installs prerequisites, builds, installs, and restarts the panel.
اسکریپت به‌صورت خودکار توزیع را شناسایی و تمام مراحل نصب و راه‌اندازی را انجام می‌دهد.

### Manual · دستی

**1. Prerequisites · پیش‌نیازها**

```bash
# Arch
sudo pacman -S meson ninja pkgconf xfce4-panel libxfce4ui libxfce4util gtk3 glib2 xfce4-dev-tools

# Debian / Ubuntu
sudo apt install meson ninja-build pkg-config libxfce4panel-2.0-dev libxfce4ui-2-dev libgtk-3-dev libglib2.0-dev xfce4-dev-tools

# Fedora
sudo dnf install meson ninja-build pkgconf xfce4-panel-devel libxfce4ui-devel gtk3-devel glib2-devel xfce4-dev-tools
```

**2. Build & Install · ساخت و نصب**

```bash
meson setup build --prefix=/usr
ninja -C build
sudo ninja -C build install
```

> [!IMPORTANT]
> `--prefix=/usr` is required. The plugin path (e.g. `/usr/lib64/`) is detected automatically by Meson.
> تعیین `--prefix=/usr` الزامی است و مسیر افزونه به‌طور خودکار شناسایی می‌شود.

---

## 🔄 Activate · فعال‌سازی

```bash
xfce4-panel -r
```

Right-click the panel → **Add New Items…** → search **Persian Calendar** → **Add**.
راست‌کلیک روی پنل ← **Add New Items…** ← جستجوی **تقویم جلالی** ← **Add**.

---

## ⚙️ Configuration · شخصی‌سازی

Right-click the plugin → **Properties**.
راست‌کلیک روی افزونه ← **Properties**.

**Custom format codes · کدهای قالب سفارشی**

| Code | Meaning · معنی | Code | Meaning · معنی |
|---|---|---|---|
| `A%` | Weekday · روز هفته | `Y%` | 4-digit year · سال ۴ رقمی |
| `d%` | Day · روز ماه | `y%` | 2-digit year · سال ۲ رقمی |
| `m%` | Month number · شماره ماه | `hd%` `hB%` `hY%` | Hijri · قمری |
| `B%` | Month name · نام ماه | `gd%` `gB%` `gY%` | Gregorian · میلادی |

---

## 📄 License · لایسنس

Released under **GPL-3.0**. Free to use, modify, and distribute.
منتشرشده تحت لایسنس **GPL-3.0** — استفاده، ویرایش و توزیع آزاد.
