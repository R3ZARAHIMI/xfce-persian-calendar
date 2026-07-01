#ifndef JALALI_H
#define JALALI_H

/* یک ساختار ساده برای نگهداری تاریخ جلالی */
typedef struct {
    int year;
    int month;   /* 1..12 */
    int day;     /* 1..31 */
    int weekday; /* 0 = شنبه ... 6 = جمعه */
} JalaliDate;

/* نام ماه‌های جلالی، ایندکس 0 = فروردین */
extern const char *jalali_month_names[12];

/* نام روزهای هفته، ایندکس 0 = شنبه */
extern const char *jalali_weekday_names[7];

/* تبدیل تاریخ میلادی (گریگوری) به جلالی */
JalaliDate gregorian_to_jalali(int gy, int gm, int gd);

/* تبدیل تاریخ جلالی به میلادی (برای محاسبه‌ی weekday و navigation بین ماه‌ها لازم می‌شود) */
void jalali_to_gregorian(int jy, int jm, int jd, int *gy, int *gm, int *gd);

/* تعداد روزهای یک ماه جلالی خاص (با در نظر گرفتن سال کبیسه برای اسفند) */
int jalali_days_in_month(int jy, int jm);

/* آیا سال جلالی کبیسه است */
int jalali_is_leap(int jy);

/* تبدیل عدد صحیح به رشته‌ی اعداد فارسی، خروجی را در buf می‌نویسد */
void to_persian_digits(int number, char *buf, int buflen);

#endif
