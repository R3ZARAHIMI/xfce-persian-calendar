#ifndef HIJRI_H
#define HIJRI_H

typedef struct {
    int year;
    int month;  /* 1..12 */
    int day;    /* 1..30 */
} HijriDate;

/* نام ماه‌های قمری، ایندکس 0 = محرم */
extern const char *hijri_month_names[12];

/* گرفتن Julian Day Number از تاریخ میلادی (مشترک با jalali.c، این‌جا هم
 * برای استقلال فایل دوباره تعریف شده) */
long hijri_gregorian_to_jdn(int gy, int gm, int gd);
void hijri_jdn_to_gregorian(long jdn, int *gy, int *gm, int *gd);

/* تبدیل تاریخ هجری قمری به JDN و برعکس، بر اساس محاسبه‌ی رؤیت هلال
 * (پورت‌شده از AlternativeHijriConverter.kt) */
long hijri_to_jdn(int year, int month, int day);
HijriDate jdn_to_hijri(long jdn);

/* میانبر: گرفتن تاریخ هجری از روی تاریخ میلادی */
HijriDate gregorian_to_hijri(int gy, int gm, int gd);

/* تعداد روزهای یک ماه هجری قمری خاص (29 یا 30، بر اساس محاسبه‌ی رؤیت) */
int hijri_days_in_month(int year, int month);

#endif
