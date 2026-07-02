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

/* مثل find_events اما مخصوص مناسبت‌های قمری. مناسبت‌هایی که روی روز ۳۰ ثبت
 * شده‌اند (مانند شهادت امام رضا (ع) در آخر صفر و شهادت امام محمد تقی (ع) در
 * آخر ذیقعده) در واقع «آخر ماه» هستند؛ پس در ماه‌های ۲۹ روزه روی آخرین روز
 * ماه نمایش داده می‌شوند تا حذف نشوند. days_in_month تعداد روزهای ماه قمری
 * جاری (۲۹ یا ۳۰) است. */
int find_lunar_events(int month, int day, int days_in_month,
                       const CalendarEvent **out, int max_out);

#endif
