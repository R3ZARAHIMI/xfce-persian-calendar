#include "hijri.h"
#include <math.h>
#include <stddef.h>

const char *hijri_month_names[12] = {
    "محرم", "صفر", "ربیع‌الاول", "ربیع‌الثانی", "جمادی‌الاول", "جمادی‌الثانی",
    "رجب", "شعبان", "رمضان", "شوال", "ذیقعده", "ذیحجه"
};

/* --- توابع کمکی مشترک برای JDN --- */

long hijri_gregorian_to_jdn(int gy, int gm, int gd) {
    long a = (14 - gm) / 12;
    long y = gy + 4800 - a;
    long m = gm + 12 * a - 3;
    return gd + (153 * m + 2) / 5 + 365L * y + y / 4 - y / 100 + y / 400 - 32045;
}

void hijri_jdn_to_gregorian(long jdn, int *gy, int *gm, int *gd) {
    long a = jdn + 32044;
    long b = (4 * a + 3) / 146097;
    long c = a - (146097 * b) / 4;
    long d = (4 * c + 3) / 1461;
    long e = c - (1461 * d) / 4;
    long m = (5 * e + 2) / 153;
    *gd = (int)(e - (153 * m + 2) / 5 + 1);
    *gm = (int)(m + 3 - 12 * (m / 10));
    *gy = (int)(100 * b + d - 4800 + m / 10);
}

/* --- جدول محاسباتی ۱۸۶ ساله پورت شده از IranianIslamicDateConverter.kt --- */
#define SUPPORTED_START_JDN 2396005L
#define SUPPORTED_START_YEAR 1264
#define SUPPORTED_YEARS_COUNT 186

static const unsigned short hijri_months_table[SUPPORTED_YEARS_COUNT] = {
    0b101010010111,
    0b101010101010,
    0b101011010101,
    0b010110010100,
    0b101110101010,
    0b010110110101,
    0b010010110110,
    0b101001010111,
    0b010100101011,
    0b011010100011,
    0b011011010001,
    0b101011101001,
    0b010101101010,
    0b101001101101,
    0b010100101101,
    0b110010010101,
    0b111001001010,
    0b111010100101,
    0b011010110100,
    0b100110111010,
    0b010100111011,
    0b001001011011,
    0b010100101011,
    0b101001010101,
    0b101010101010,
    0b101101011001,
    0b010101110100,
    0b100101111010,
    0b010010111010,
    0b101001011010,
    0b110100110100,
    0b111010110001,
    0b011011011000,
    0b101011101100,
    0b010101011100,
    0b101001101110,
    0b010100110110,
    0b101010100110,
    0b101101010010,
    0b101110101001,
    0b001110110100,
    0b100111011010,
    0b010101011010,
    0b101010101010,
    0b110101001010,
    0b111010100101,
    0b011101010010,
    0b101101101001,
    0b010110110100,
    0b101010101101,
    0b011001010110,
    0b110100100110,
    0b111010010010,
    0b111101001001,
    0b011101010100,
    0b101101011010,
    0b100110011011,
    0b010010011011,
    0b100101001011,
    0b101100100101,
    0b110101010010,
    0b110101101010,
    0b010101101101,
    0b001010110110,
    0b101000110111,
    0b010010011011,
    0b011001001101,
    0b011010101010,
    0b101101010101,
    0b001101011100,
    0b100101101110,
    0b010010101111,
    0b001001010111,
    0b001100101011,
    0b010110010101,
    0b001110101010,
    0b010111011001,
    0b001011011010,
    0b100101011101,
    0b001010101011,
    0b010101010101,
    0b011011001001,
    0b011011100100,
    0b101101101010,
    0b010110110101,
    0b001010110110,
    0b100110010110,
    0b110101001010,
    0b110111000101,
    0b011101010010,
    0b011110100101,
    0b001101101010,
    0b100110101101,
    0b010101001101,
    0b101010010101,
    0b110101001001,
    0b110110100101,
    0b010110110010,
    0b101011010101,
    0b010101010110,
    0b101001010111,
    0b010100101011,
    0b011010010101,
    0b101101001010,
    0b101101100101,
    0b010101101011,
    0b001010101101,
    0b010101001110,
    0b110010010111,
    0b010101001011,
    0b011010100101,
    0b011011010010,
    0b101011011001,
    0b010011011101,
    0b001001010111,
    0b100100101101,
    0b101010010101,
    0b101101010010,
    0b101101101001,
    0b001101110100,
    0b100101110110,
    0b010010110111,
    0b001001010111,
    0b010101001011,
    0b011010100101,
    0b011011010010,
    0b101011101010,
    0b010011101101,
    0b001001101101,
    0b100100110101,
    0b110100100101,
    0b110101010001,
    0b101110101001,
    0b010111010100,
    0b101010110101,
    0b010100110110,
    0b101010010111,
    0b011001001010,
    0b111010100101,
    0b011101010010,
    0b101110101001,
    0b010110110101,
    0b001010110101,
    0b101001010110,
    0b110100100110,
    0b111001010011,
    0b011010101001,
    0b110101010100,
    0b110101010110,
    0b101001010111,
    0b010010100111,
    0b110001000111,
    0b110100100110,
    0b111001010100,
    0b110110100110,
    0b010101100111,
    0b001010110110,
    0b100100110111,
    0b010010010111,
    0b011001010101,
    0b101010101010,
    0b101101100101,
    0b001011101100,
    0b100101110101,
    0b010001101110,
    0b101000110110,
    0b110010100110,
    0b110101010010,
    0b110111010010,
    0b010111010101,
    0b001011011010,
    0b010101011101,
    0b010010101011,
    0b011010010011,
    0b011101001001,
    0b011110100100,
    0b101110110010,
    0b010110110101,
    0b001010110110,
    0b011001011010,
    0b110100101010,
    0b111010010100,
    0b111011010001,
    0b011011101000,
    0b101011101010,
    0b100101011100
};

static int hijri_months_days_offsets[SUPPORTED_YEARS_COUNT * 12 + 1];
static long hijri_support_end_jdn = 0;
static int hijri_table_initialized = 0;

static void
initialize_hijri_table (void)
{
    if (hijri_table_initialized) return;

    int jd = 0;
    for (int m = 0; m < SUPPORTED_YEARS_COUNT * 12; m++)
    {
        hijri_months_days_offsets[m] = jd;
        int year_idx = m / 12;
        int month_idx = m % 12;
        int is_30 = (hijri_months_table[year_idx] >> (11 - month_idx)) & 1;
        jd += is_30 ? 30 : 29;
    }
    hijri_months_days_offsets[SUPPORTED_YEARS_COUNT * 12] = jd;
    hijri_support_end_jdn = SUPPORTED_START_JDN + jd;
    hijri_table_initialized = 1;
}

/* --- توابع نجومی قدیمی (به عنوان Fallback در صورت خارج بودن تاریخ از محدوده جدول) --- */

static double sin_of_degree(double degree) {
    return sin(degree * M_PI / 180.0);
}

static double tmoonphase(long n, int nph) {
    double k = n + nph / 4.0;
    double T = k / 1236.85;
    double t2 = T * T;
    double t3 = t2 * T;
    double jd = 2415020.75933 + 29.53058868 * k - .0001178 * t2 - .000000155 * t3
                + .00033 * sin_of_degree(166.56 + 132.87 * T - .009173 * t2);

    double sa = (359.2242 + 29.10535608 * k - .0000333 * t2 - .00000347 * t3) * M_PI / 180.0;
    double ma = (306.0253 + 385.81691806 * k + .0107306 * t2 + .00001236 * t3) * M_PI / 180.0;
    double tf = (2 * (21.2964 + 390.67050646 * k - .0016528 * t2 - .00000239 * t3)) * M_PI / 180.0;

    double xtra;
    if (nph == 0 || nph == 2) {
        xtra = (((.1734 - .000393 * T) * sin(sa)) + (.0021 * sin(sa * 2)) - (.4068 * sin(ma)) +
                (.0161 * sin(2 * ma)) - (.0004 * sin(3 * ma)) + (.0104 * sin(tf)))
               - (.0051 * sin(sa + ma)) - (.0074 * sin(sa - ma)) + (.0004 * sin(tf + sa))
               - (.0004 * sin(tf - sa)) - (.0006 * sin(tf + ma)) + (.001 * sin(tf - ma))
               + (.0005 * sin(sa + 2 * ma));
    } else if (nph == 1 || nph == 3) {
        xtra = (((.1721 - .0004 * T) * sin(sa)) + (.0021 * sin(sa * 2)) - (.628 * sin(ma)) +
                (.0089 * sin(2 * ma)) - (.0004 * sin(3 * ma)) + (.0079 * sin(tf)))
               - (.0119 * sin(sa + ma)) - (.0047 * sin(sa - ma)) + (.0003 * sin(tf + sa))
               - (.0004 * sin(tf - sa)) - (.0006 * sin(tf + ma)) + (.0021 * sin(tf - ma))
               + (.0003 * sin(sa + 2 * ma)) + (.0004 * sin(sa - 2 * ma)) - (.0003 * sin(2 * sa + ma))
               + (nph == 1 ? (.0028 - (.0004 * cos(sa)) + (.0003 * cos(ma)))
                           : (-.0028 + (.0004 * cos(sa)) - (.0003 * cos(ma))));
    } else {
        return 0.0;
    }
    return jd + xtra - (.41 + (1.2053 * T) + (.4992 * t2)) / 1440.0;
}

static double visibility(long n) {
    const double TIMZ = 3.0;
    const double MINAGE = 13.5;
    const double SUNSET = 19.5;
    const double TIMDIF = SUNSET - MINAGE;
    double jd = tmoonphase(n, 0);
    double d = floor(jd);
    double tf = jd - d;
    if (tf <= .5) {
        return jd + 1.0;
    } else {
        tf = (tf - .5) * 24 + TIMZ;
        return (tf > TIMDIF) ? jd + 1.0 : jd;
    }
}

static long hijri_to_jdn_astronomical(int year, int month, int day) {
    if (month < 1 || month > 12 || day < 1 || day > 30) return -1;
    long hm = (long)(year - 1405) * 12L + (month - 1);
    long k = hm + 1048;
    double mjd = visibility(k);
    return (long)floor(mjd + day - .5);
}

static HijriDate jdn_to_hijri_astronomical(long jd) {
    HijriDate result;

    int gy, gm, gd;
    hijri_jdn_to_gregorian(jd, &gy, &gm, &gd);

    double year_d = gy;
    double month_d = gm;
    double day_d = gd;

    long k = (long)floor(.6 + (year_d + (((int)month_d % 2 == 0) ? month_d : month_d - 1) / 12.0
                                + day_d / 365.0 - 1900) * 12.3685);

    double mjd;
    do {
        mjd = visibility(k);
        k -= 1;
    } while (mjd > jd - .5);
    k += 1;

    long hm = k - 1048;
    int year = 1405 + (int)(hm / 12);
    int month = (int)(hm % 12) + 1;
    if (hm != 0 && month <= 0) {
        month += 12;
        year -= 1;
    }
    if (year <= 0) year -= 1;
    int day = (int)floor(jd - mjd + .5);

    result.year = year;
    result.month = month;
    result.day = day;
    return result;
}

static int hijri_days_in_month_astronomical(int year, int month) {
    long start = hijri_to_jdn_astronomical(year, month, 1);
    int next_month = month + 1;
    int next_year = year;
    if (next_month > 12) {
        next_month = 1;
        next_year += 1;
    }
    long next_start = hijri_to_jdn_astronomical(next_year, next_month, 1);
    return (int)(next_start - start);
}

/* --- توابع پیاده‌سازی نهایی متقارن و بدون آفست اشتباه سال --- */

long hijri_to_jdn(int year, int month, int day) {
    initialize_hijri_table();

    int year_index = year - SUPPORTED_START_YEAR;
    int month_index = month - 1;

    if (year_index >= 0 && year_index < SUPPORTED_YEARS_COUNT && month_index >= 0 && month_index < 12) {
        int index = year_index * 12 + month_index;
        int month_len = hijri_months_days_offsets[index + 1] - hijri_months_days_offsets[index];
        if (day >= 1 && day <= month_len) {
            return SUPPORTED_START_JDN + hijri_months_days_offsets[index] + day - 1;
        }
        return -1;
    }

    return hijri_to_jdn_astronomical(year, month, day);
}

HijriDate jdn_to_hijri(long jd) {
    initialize_hijri_table();

    if (jd >= SUPPORTED_START_JDN && jd < hijri_support_end_jdn) {
        int days = (int)(jd - SUPPORTED_START_JDN);
        int index = days / 30;
        while (index + 1 < SUPPORTED_YEARS_COUNT * 12 && hijri_months_days_offsets[index + 1] <= days) {
            ++index;
        }
        int year_index = index / 12;
        int month = index % 12;
        int day = days - hijri_months_days_offsets[index];

        HijriDate result;
        result.year = year_index + SUPPORTED_START_YEAR;
        result.month = month + 1;
        result.day = day + 1;
        return result;
    }

    return jdn_to_hijri_astronomical(jd);
}

HijriDate gregorian_to_hijri(int gy, int gm, int gd) {
    long jdn = hijri_gregorian_to_jdn(gy, gm, gd);
    return jdn_to_hijri(jdn);
}

int hijri_days_in_month(int year, int month) {
    initialize_hijri_table();

    int year_index = year - SUPPORTED_START_YEAR;
    int month_index = month - 1;

    if (year_index >= 0 && year_index < SUPPORTED_YEARS_COUNT && month_index >= 0 && month_index < 12) {
        int index = year_index * 12 + month_index;
        return hijri_months_days_offsets[index + 1] - hijri_months_days_offsets[index];
    }

    return hijri_days_in_month_astronomical(year, month);
}
