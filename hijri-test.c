#include <stdio.h>
#include "hijri.h"

static void check(int gy, int gm, int gd, const char *label) {
    HijriDate h = gregorian_to_hijri(gy, gm, gd);
    printf("%s: میلادی %04d-%02d-%02d -> قمری %d %s %d\n",
           label, gy, gm, gd, h.day, hijri_month_names[h.month - 1], h.year);
}

int main(void) {
    /* چند تاریخ شناخته‌شده برای تست (منابع عمومی تقویم هجری) */
    check(2024, 3, 11, "شروع تقریبی رمضان 1445");
    check(2025, 6, 6, "عید قربان 1446 (تقریبی)");
    check(2026, 7, 2, "امروز");
    check(1979, 2, 1, "بازگشت امام خمینی");

    printf("\nتعداد روزهای چند ماه قمری نمونه:\n");
    for (int m = 1; m <= 12; m++) {
        printf("  ماه %d (%s) سال 1446: %d روز\n",
               m, hijri_month_names[m - 1], hijri_days_in_month(1446, m));
    }

    return 0;
}
