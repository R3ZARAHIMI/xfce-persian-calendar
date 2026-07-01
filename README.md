# Persian Calendar Xfce Panel Plugin (پلاگین تقویم جلالی)

یک پلاگین کاربردی برای xfce4-panel به زبان C با استفاده از GTK3 و کتابخانه `libxfce4panel-2.0` که تاریخ جلالی را در پنل نمایش می‌دهد و با کلیک روی آن، یک تقویم جلالی شیک به همراه تاریخ‌های معادل میلادی/قمری و لیست مناسبت‌های هر روز نمایش داده می‌شود. ساختار این پروژه با جدیدترین متدهای GObject پیاده‌سازی شده است.

## پیش‌نیازها (Prerequisites)

برای ساخت این پروژه به ابزارهای زیر نیاز دارید که در آرچ لینوکس به صورت زیر نصب می‌شوند (همگی قبلاً نصب شده‌اند):
```bash
sudo pacman -S meson ninja pkgconf xfce4-panel libxfce4ui libxfce4util gtk3 glib2 xfce4-dev-tools
```

## ساخت پروژه (Build)

برای پیکربندی و کامپایل پروژه دستورات زیر را اجرا کنید:

```bash
# پیکربندی دایرکتوری build
meson setup build

# کامپایل پروژه
ninja -C build
```

## نصب محلی و بدون نیاز به نصب کامل سیستمی (Local Installation)

برای تست و توسعه آسان، فایل‌های باینری و دسکتاپ را به مسیرهای پیش‌فرض پنل لینک سیمبولیک (symlink) می‌کنیم:

```bash
# لینک کردن کتابخانه کامپایل‌شده پلاگین به دایرکتوری پلاگین‌های پنل
sudo ln -sf "$PWD/build/libpersian-calendar.so" /usr/lib/xfce4/panel/plugins/libpersian-calendar.so

# لینک کردن فایل دسکتاپ به دایرکتوری معرفی پلاگین‌ها
sudo ln -sf "$PWD/build/persian-calendar.desktop" /usr/share/xfce4/panel/plugins/persian-calendar.desktop
```

## راه‌اندازی مجدد پنل (Restart Panel)

برای اعمال تغییرات و لود شدن پلاگین جدید، پنل Xfce را ری‌استارت کنید:

```bash
xfce4-panel -r
```

پس از ری‌استارت، روی پنل راست‌کلیک کرده و مسیر زیر را بروید:
**Panel -> Add New Items...** و سپس **Persian Calendar** (یا **تقویم جلالی**) را انتخاب کرده و دکمه **Add** را بزنید تا در پنل نمایش داده شود.
