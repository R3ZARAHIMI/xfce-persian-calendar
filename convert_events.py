#!/usr/bin/env python3
"""
تبدیل events_data.json به events_data.c / events_data.h
خروجی: سه آرایه‌ی استاتیک C (شمسی، قمری، میلادی) که هرکدوم
month, day, title (UTF-8)، و holiday را نگه می‌دارند.
"""
import json
import sys

def c_escape(s):
    return s.replace('\\', '\\\\').replace('"', '\\"')

def build_array(entries, var_name):
    """entries: list of (month, day, title, holiday) sorted by month,day"""
    lines = []
    lines.append(f"const CalendarEvent {var_name}[] = {{")
    for month, day, title, holiday in entries:
        h = "1" if holiday else "0"
        lines.append(f'    {{ {month}, {day}, "{c_escape(title)}", {h} }},')
    lines.append("};")
    lines.append(f"const int {var_name}_count = {len(entries)};")
    return "\n".join(lines)

def parse_section(section):
    entries = []
    for key, val in section.items():
        month_str, day_str = key.split('-')
        month, day = int(month_str), int(day_str)
        entries.append((month, day, val['title'], val.get('holiday', False)))
    entries.sort(key=lambda e: (e[0], e[1]))
    return entries

def main():
    with open('events_data.json', encoding='utf-8') as f:
        data = json.load(f)

    solar = parse_section(data.get('solar', {}))
    lunar = parse_section(data.get('lunar', {}))
    gregorian = parse_section(data.get('gregorian', {}))

    header = """#ifndef EVENTS_DATA_H
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
"""

    with open('events_data.h', 'w', encoding='utf-8') as f:
        f.write(header)

    src = []
    src.append('#include "events_data.h"')
    src.append('#include <string.h>')
    src.append("")
    src.append(build_array(solar, "solar_events"))
    src.append("")
    src.append(build_array(lunar, "lunar_events"))
    src.append("")
    src.append(build_array(gregorian, "gregorian_events"))
    src.append("")
    src.append("""int find_events(const CalendarEvent *arr, int arr_count,
                 int month, int day,
                 const CalendarEvent **out, int max_out) {
    int found = 0;
    for (int i = 0; i < arr_count && found < max_out; i++) {
        if (arr[i].month == month && arr[i].day == day) {
            out[found++] = &arr[i];
        }
    }
    return found;
}
""")

    with open('events_data.c', 'w', encoding='utf-8') as f:
        f.write("\n".join(src))

    print(f"solar: {len(solar)} رکورد")
    print(f"lunar: {len(lunar)} رکورد")
    print(f"gregorian: {len(gregorian)} رکورد")

if __name__ == '__main__':
    main()
