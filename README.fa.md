<div align="center">

# تقویم جلالی برای پنل Xfce

[English](README.md) · **فارسی**

[![Language](https://img.shields.io/badge/Language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Toolkit](https://img.shields.io/badge/Toolkit-GTK3-orange.svg)](https://www.gtk.org/)
[![Desktop](https://img.shields.io/badge/Desktop-Xfce-green.svg)](https://xfce.org/)
[![License](https://img.shields.io/badge/License-GPL--3.0-red.svg)](#-لایسنس)

افزونه‌ای سبک و بومی برای پنل Xfce که تاریخ جلالی (شمسی) را با قابلیت شخصی‌سازی فراوان نمایش می‌دهد.

![اسکرین‌شات](images/sce.png)

</div>

---

## ✨ ویژگی‌ها

- نمایش هوشمند تاریخ شمسی با قالب‌های آماده یا سفارشی
- تنظیم فونت و اندازه متن
- رنگ مجزا برای روزهای عادی و تعطیل
- تشخیص خودکار تعطیلات شمسی، قمری و میلادی
- پشتیبانی از پنل عمودی و افقی
- تقویم بازشو با معادل قمری/میلادی و مناسبت‌ها

---

## 🚀 نصب

### خودکار (پیشنهادی)

```bash
chmod +x install.sh
./install.sh
```

اسکریپت به‌صورت خودکار توزیع را شناسایی می‌کند و تمام مراحل نصب پیش‌نیازها، ساخت، نصب و راه‌اندازی مجدد پنل را انجام می‌دهد.

### دستی

**۱. پیش‌نیازها**

```bash
# Arch
sudo pacman -S meson ninja pkgconf xfce4-panel libxfce4ui libxfce4util gtk3 glib2 xfce4-dev-tools

# Debian / Ubuntu
sudo apt install meson ninja-build pkg-config libxfce4panel-2.0-dev libxfce4ui-2-dev libgtk-3-dev libglib2.0-dev xfce4-dev-tools

# Fedora
sudo dnf install meson ninja-build pkgconf xfce4-panel-devel libxfce4ui-devel gtk3-devel glib2-devel xfce4-dev-tools
```

**۲. ساخت و نصب**

```bash
meson setup build --prefix=/usr
ninja -C build
sudo ninja -C build install
```

> [!IMPORTANT]
> تعیین `--prefix=/usr` الزامی است. مسیر افزونه (مثلاً `/usr/lib64/`) به‌طور خودکار توسط Meson شناسایی می‌شود.

---

## 🔄 فعال‌سازی

```bash
xfce4-panel -r
```

راست‌کلیک روی پنل ← **Add New Items…** ← جستجوی **تقویم جلالی** ← **Add**.

---

## ⚙️ شخصی‌سازی

راست‌کلیک روی افزونه ← **Properties**.

**کدهای قالب سفارشی**

| کد | معنی | کد | معنی |
|---|---|---|---|
| `A%` | روز هفته | `Y%` | سال ۴ رقمی |
| `d%` | روز ماه | `y%` | سال ۲ رقمی |
| `m%` | شماره ماه | `hd%` `hB%` `hY%` | تاریخ قمری |
| `B%` | نام ماه | `gd%` `gB%` `gY%` | تاریخ میلادی |

---

## 📄 لایسنس

منتشرشده تحت لایسنس **GPL-3.0** — استفاده، ویرایش و توزیع آزاد است.
