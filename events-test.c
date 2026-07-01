#include <stdio.h>
#include "events_data.h"

int main() {
    const CalendarEvent *out[8];
    int n = find_events(solar_events, solar_events_count, 1, 1, out, 8);
    printf("مناسبت‌های 1 فروردین: %d مورد\n", n);
    for (int i = 0; i < n; i++) {
        printf("  - %s (%s)\n", out[i]->title, out[i]->holiday ? "تعطیل" : "عادی");
    }

    n = find_events(solar_events, solar_events_count, 3, 14, out, 8);
    printf("مناسبت‌های 14 خرداد: %d مورد\n", n);
    for (int i = 0; i < n; i++) {
        printf("  - %s (%s)\n", out[i]->title, out[i]->holiday ? "تعطیل" : "عادی");
    }

    printf("جمع کل: solar=%d lunar=%d gregorian=%d\n",
           solar_events_count, lunar_events_count, gregorian_events_count);
    return 0;
}
