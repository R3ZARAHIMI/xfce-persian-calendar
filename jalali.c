#include "jalali.h"
#include <stdio.h>
#include <string.h>

const char *jalali_month_names[12] = {
    "فروردین", "اردیبهشت", "خرداد", "تیر", "مرداد", "شهریور",
    "مهر", "آبان", "آذر", "دی", "بهمن", "اسفند"
};

const char *jalali_weekday_names[7] = {
    "شنبه", "یکشنبه", "دوشنبه", "سه‌شنبه", "چهارشنبه", "پنج‌شنبه", "جمعه"
};

/* آیا سال جلالی کبیسه است */
int jalali_is_leap(int jy) {
    int r = (jy - 474) % 2820;
    r = (r + 474) % 33;
    return (r * 8 + 21) % 33 < 8;
}

/* تعداد روزهای یک ماه جلالی خاص */
int jalali_days_in_month(int jy, int jm) {
    if (jm >= 1 && jm <= 6)
        return 31;
    if (jm >= 7 && jm <= 11)
        return 30;
    if (jm == 12)
        return jalali_is_leap(jy) ? 30 : 29;
    return 0;
}

/* تبدیل تاریخ میلادی به جلالی */
JalaliDate gregorian_to_jalali(int gy, int gm, int gd) {
    JalaliDate jd;
    long g_days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    long gy2 = gy - 1600;
    long gm2 = gm - 1;
    long gd2 = gd - 1;

    long g_day_no = 365 * gy2 + (gy2 + 3) / 4 - (gy2 + 99) / 100 + (gy2 + 399) / 400;
    for (int i = 0; i < gm2; ++i)
        g_day_no += g_days_in_month[i];
    if (gm2 > 1 && ((gy % 4 == 0 && gy % 100 != 0) || (gy % 400 == 0)))
        g_day_no++;
    g_day_no += gd2;

    long j_day_no = g_day_no - 79;

    long j_np = j_day_no / 12053; /* 12053 = 33 * 365 + 8 */
    j_day_no %= 12053;

    long jy2 = 979 + 33 * j_np + 4 * (j_day_no / 1461); /* 1461 = 4 * 365 + 1 */
    j_day_no %= 1461;

    if (j_day_no >= 366) {
        jy2 += (j_day_no - 1) / 365;
        j_day_no = (j_day_no - 1) % 365;
    }

    int jm2 = 0;
    for (int i = 0; i < 11; ++i) {
        int days = (i < 6) ? 31 : 30;
        if (j_day_no < days)
            break;
        j_day_no -= days;
        jm2++;
    }

    jd.year = jy2;
    jd.month = jm2 + 1;
    jd.day = j_day_no + 1;

    /* Calculate weekday: 0 = Saturday, ..., 6 = Friday.
     * Gregorian to JDN:
     */
    long a = (14 - gm) / 12;
    long y = gy + 4800 - a;
    long m = gm + 12 * a - 3;
    long jdn = gd + (153 * m + 2) / 5 + 365L * y + y / 4 - y / 100 + y / 400 - 32045;
    
    /* JDN % 7: 0 is Monday, 1 is Tuesday, ..., 5 is Saturday, 6 is Sunday.
     * We convert to Persian weekday system where 0 is Saturday, ..., 6 is Friday:
     */
    jd.weekday = (int)((jdn + 2) % 7);

    return jd;
}

/* تبدیل تاریخ جلالی به میلادی */
void jalali_to_gregorian(int jy, int jm, int jd, int *gy, int *gm, int *gd) {
    long jy2 = jy - 979;
    long jm2 = jm - 1;
    long jd2 = jd - 1;

    long j_day_no = 365 * jy2 + (jy2 / 33) * 8 + ((jy2 % 33) + 3) / 4;
    for (int i = 0; i < jm2; ++i)
        j_day_no += (i < 6) ? 31 : 30;
    j_day_no += jd2;

    long g_day_no = j_day_no + 79;

    long gy2 = 1600 + 400 * (g_day_no / 146097); /* 146097 = 400 * 365 + 97 */
    g_day_no %= 146097;

    long leap = 1;
    if (g_day_no >= 36525) { /* 36525 = 100 * 365 + 25 */
        g_day_no--;
        gy2 += 100 * (g_day_no / 36524); /* 36524 = 100 * 365 + 24 */
        g_day_no %= 36524;
        if (g_day_no >= 365)
            g_day_no++;
        else
            leap = 0;
    }

    gy2 += 4 * (g_day_no / 1461); /* 1461 = 4 * 365 + 1 */
    g_day_no %= 1461;

    if (g_day_no >= 366) {
        leap = 0;
        g_day_no--;
        gy2 += g_day_no / 365;
        g_day_no %= 365;
    }

    long g_days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (leap && ((gy2 % 4 == 0 && gy2 % 100 != 0) || (gy2 % 400 == 0)))
        g_days_in_month[1] = 29;

    int gm2 = 0;
    for (int i = 0; i < 12; ++i) {
        if (g_day_no < g_days_in_month[i])
            break;
        g_day_no -= g_days_in_month[i];
        gm2++;
    }

    *gy = gy2;
    *gm = gm2 + 1;
    *gd = g_day_no + 1;
}

/* تبدیل عدد انگلیسی به اعداد فارسی در قالب رشته UTF-8 */
void to_persian_digits(int number, char *buf, int buflen) {
    char temp[32];
    snprintf(temp, sizeof(temp), "%d", number);
    char *p = buf;
    char *end = buf + buflen - 1;

    for (int i = 0; temp[i] != '\0' && p < end; i++) {
        char c = temp[i];
        if (c >= '0' && c <= '9') {
            /* در UTF-8، حروف فارسی ۰ تا ۹ با بایتهای 0xDB 0xB0 تا 0xDB 0xB9 رمزگذاری می‌شوند */
            if (p + 2 <= end) {
                *p++ = (char)0xDB;
                *p++ = (char)(0xB0 + (c - '0'));
            } else {
                break;
            }
        } else {
            *p++ = c;
        }
    }
    *p = '\0';
}
