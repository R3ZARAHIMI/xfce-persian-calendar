#ifndef EVENTS_DATA_H
#define EVENTS_DATA_H

typedef struct {
    int month;
    int day;
    const char *title;
    int holiday; /* 1 = تعطیل رسمی */
} CalendarEvent;

extern const CalendarEvent solar_events[];
extern const int solar_events_count;

extern const CalendarEvent lunar_events[];
extern const int lunar_events_count;

extern const CalendarEvent gregorian_events[];
extern const int gregorian_events_count;

/* جستجوی مناسبت‌های یک روز خاص در یک آرایه؛ نتایج را در out می‌نویسد
 * و تعداد موارد پیدا شده را برمی‌گرداند (حداکثر max_out) */
int find_events(const CalendarEvent *arr, int arr_count,
                 int month, int day,
                 const CalendarEvent **out, int max_out);

#endif
