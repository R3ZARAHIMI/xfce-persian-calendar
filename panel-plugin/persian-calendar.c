#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4ui/libxfce4ui.h>
#include <xfconf/xfconf.h>
#include "../jalali.h"
#include "../hijri.h"
#include "../events_data.h"

#define PERSIAN_TYPE_CALENDAR (persian_calendar_get_type())
G_DECLARE_FINAL_TYPE (PersianCalendar, persian_calendar, PERSIAN, CALENDAR, XfcePanelPlugin)

/* Display format options for the panel label */
enum {
  FORMAT_DAY_MONTH = 0,       /* "۱۲ تیر" */
  FORMAT_DAY_MONTH_YEAR,     /* "۱۲ تیر ۱۴۰۴" */
  FORMAT_WEEKDAY_DAY_MONTH,  /* "شنبه ۱۲ تیر" */
  FORMAT_WEEKDAY_DAY_MONTH_YEAR, /* "شنبه ۹ تیر ۱۴۰۵" */
  FORMAT_JALALI_HIJRI,       /* "۱۲ تیر | ۵ محرم" */
  FORMAT_DAY_ONLY,           /* "۱۲" */
  FORMAT_MONTH_ONLY,         /* "تیر" */
  FORMAT_FULL_DATES,         /* "۱۲ تیر ۱۴۰۴ | 1 Jul 2024" */
  FORMAT_NUMERIC_DAY_MONTH,  /* "۱۱-۰۴" */
  FORMAT_CUSTOM,             /* سفارشی (قالب دلخواه) */
  FORMAT_COUNT
};

#define XFCONF_CHANNEL_NAME "persian-calendar"
#define PROP_DISPLAY_FORMAT "/displayFormat"
#define PROP_CUSTOM_FONT    "/customFont"
#define PROP_CUSTOM_FORMAT  "/customFormat"
#define PROP_HOLIDAY_COLOR  "/holidayColor"
#define DEFAULT_HOLIDAY_COLOR "#f38ba8"
#define PROP_NORMAL_COLOR   "/normalColor"
#define DEFAULT_NORMAL_COLOR "#cdd6f4"

#ifndef DATADIR
#define DATADIR "/usr/share"
#endif

static const char *preset_formats[FORMAT_CUSTOM] = {
  "%d %B",          /* FORMAT_DAY_MONTH */
  "%d %B %Y",       /* FORMAT_DAY_MONTH_YEAR */
  "%A %d %B",       /* FORMAT_WEEKDAY_DAY_MONTH */
  "%A %d %B %Y",    /* FORMAT_WEEKDAY_DAY_MONTH_YEAR */
  "%d %B | %hd %hB",/* FORMAT_JALALI_HIJRI */
  "%d",             /* FORMAT_DAY_ONLY */
  "%B",             /* FORMAT_MONTH_ONLY */
  "%d %B %Y | %gd %gB %gY", /* FORMAT_FULL_DATES */
  "%m-%d"           /* FORMAT_NUMERIC_DAY_MONTH */
};

struct _PersianCalendar
{
  XfcePanelPlugin parent_instance;

  /* plugin widgets */
  GtkWidget *label;
  GtkWidget *window; /* Pointer to the open calendar window, or NULL */

  /* settings */
  XfconfChannel *channel;
  int display_format;
  gchar *custom_font;    /* Pango font description string */
  gchar *custom_format;  /* Custom format string, e.g. "%A %d %B %Y" */
  gchar *holiday_color;  /* Hex color string for holidays, e.g. "#f38ba8" */
  gchar *normal_color;   /* Hex color string for normal days, e.g. "#cdd6f4" */

  /* state */
  gint64 last_destroy_time; /* Monotonic time of last destroy in ms */
};

/* Struct for each day cell in the calendar grid */
typedef struct {
  GtkWidget *button;
  GtkWidget *main_lbl;
  GtkWidget *greg_lbl;
  GtkWidget *hijri_lbl;
  
  int year;  /* Jalali */
  int month; /* Jalali */
  int day;   /* Jalali */
  gboolean other_month;
  gboolean is_holiday;
} DayCell;

/* Struct to hold the state of the calendar window */
typedef struct {
  GtkWidget *window;
  
  /* Current view month/year */
  int view_year;
  int view_month;
  
  /* Selected date */
  int sel_year;
  int sel_month;
  int sel_day;
  
  /* Today's date */
  int today_year;
  int today_month;
  int today_day;

  /* UI widgets */
  GtkWidget *title_label;
  GtkWidget *hijri_greg_label;
  GtkWidget *grid;
  GtkWidget *details_title;
  GtkWidget *details_date;
  GtkWidget *events_list_box;
  
  DayCell cells[42];
} CalendarWindow;

/* Register the plugin type with the panel */
XFCE_PANEL_DEFINE_PLUGIN (PersianCalendar, persian_calendar)

/* Gregorian month names (short) */
static const char *greg_month_names[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/* Gregorian month names (full) */
static const char *greg_months_full[12] = {
  "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December"
};

/* Load premium CSS theme for the calendar window */
static void
load_css (void)
{
  static gboolean css_loaded = FALSE;
  if (css_loaded) return;

  GtkCssProvider *provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider,
    ".calendar-window { background-color: #1e1e2e; color: #cdd6f4; font-family: 'Vazirmatn', 'Outfit', sans-serif; }\n"
    ".calendar-header { padding: 12px; background-color: #181825; border-bottom: 1px solid #313244; }\n"
    ".nav-button { background: none; border: none; color: #cdd6f4; font-size: 16px; padding: 4px 12px; border-radius: 6px; }\n"
    ".nav-button:hover { background-color: #313244; color: #f5c2e7; }\n"
    ".weekday-header { font-weight: bold; color: #89b4fa; padding: 6px; text-align: center; font-size: 12px; }\n"
    ".day-btn { background: #1e1e2e; border: 1px solid #313244; border-radius: 6px; padding: 4px; margin: 1px; min-width: 38px; min-height: 38px; }\n"
    ".day-btn:hover { background-color: #313244; border-color: #f5c2e7; }\n"
    ".day-btn.current-day { background-color: #b4befe; border-color: #b4befe; color: #11111b; }\n"
    ".day-btn.current-day .sub-text { color: #313244; }\n"
    ".day-btn.holiday { border-color: #f38ba8; }\n"
    ".day-btn.holiday .main-text { color: #f38ba8; }\n"
    ".day-btn.other-month { opacity: 0.35; }\n"
    ".main-text { font-size: 14px; font-weight: bold; }\n"
    ".sub-text { font-size: 9px; color: #a6adc8; }\n"
    ".sidebar { background-color: #11111b; border-right: 1px solid #313244; padding: 16px; width: 280px; }\n"
    ".sidebar-title { font-size: 15px; font-weight: bold; color: #b4befe; margin-bottom: 4px; }\n"
    ".sidebar-subtitle { font-size: 11px; color: #a6adc8; margin-bottom: 12px; line-height: 1.4; }\n"
    ".event-item { background-color: #1e1e2e; border-radius: 6px; padding: 10px; margin-bottom: 8px; border-right: 4px solid #89b4fa; }\n"
    ".event-item.holiday { border-right-color: #f38ba8; background-color: #2d1f27; }\n"
    ".event-title { font-size: 12px; color: #cdd6f4; font-weight: bold; }\n"
    ".event-badge { font-size: 9px; color: #f38ba8; font-weight: bold; }\n",
    -1, NULL);
  
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);
  css_loaded = TRUE;
}

/* Helper to build a day cell widget */
static GtkWidget *
create_day_cell (GtkWidget **main_lbl, GtkWidget **greg_lbl, GtkWidget **hijri_lbl)
{
  GtkWidget *btn = gtk_button_new ();
  GtkStyleContext *context = gtk_widget_get_style_context (btn);
  gtk_style_context_add_class (context, "day-btn");

  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  
  *main_lbl = gtk_label_new ("");
  GtkStyleContext *main_ctx = gtk_widget_get_style_context (*main_lbl);
  gtk_style_context_add_class (main_ctx, "main-text");
  gtk_box_pack_start (GTK_BOX (box), *main_lbl, TRUE, TRUE, 0);

  GtkWidget *sub_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  
  *hijri_lbl = gtk_label_new ("");
  GtkStyleContext *hijri_ctx = gtk_widget_get_style_context (*hijri_lbl);
  gtk_style_context_add_class (hijri_ctx, "sub-text");
  gtk_box_pack_start (GTK_BOX (sub_box), *hijri_lbl, FALSE, FALSE, 0);

  GtkWidget *spacer = gtk_label_new ("");
  gtk_box_pack_start (GTK_BOX (sub_box), spacer, TRUE, TRUE, 0);

  *greg_lbl = gtk_label_new ("");
  GtkStyleContext *greg_ctx = gtk_widget_get_style_context (*greg_lbl);
  gtk_style_context_add_class (greg_ctx, "sub-text");
  gtk_box_pack_end (GTK_BOX (sub_box), *greg_lbl, FALSE, FALSE, 0);

  gtk_box_pack_end (GTK_BOX (box), sub_box, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (btn), box);
  
  return btn;
}

/* Add an event entry to the events list view */
static void
add_event_widget (GtkWidget *container, const char *title, int holiday, const char *type)
{
  GtkWidget *item = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  GtkStyleContext *item_ctx = gtk_widget_get_style_context (item);
  gtk_style_context_add_class (item_ctx, "event-item");
  if (holiday) {
      gtk_style_context_add_class (item_ctx, "holiday");
  }

  GtkWidget *top_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  
  GtkWidget *title_lbl = gtk_label_new (title);
  gtk_label_set_line_wrap (GTK_LABEL (title_lbl), TRUE);
  gtk_label_set_xalign (GTK_LABEL (title_lbl), 1.0); /* Align right for RTL */
  GtkStyleContext *title_ctx = gtk_widget_get_style_context (title_lbl);
  gtk_style_context_add_class (title_ctx, "event-title");
  gtk_box_pack_start (GTK_BOX (top_row), title_lbl, TRUE, TRUE, 0);

  if (holiday) {
      GtkWidget *badge = gtk_label_new ("تعطیل");
      GtkStyleContext *badge_ctx = gtk_widget_get_style_context (badge);
      gtk_style_context_add_class (badge_ctx, "event-badge");
      gtk_box_pack_end (GTK_BOX (top_row), badge, FALSE, FALSE, 0);
  }

  gtk_box_pack_start (GTK_BOX (item), top_row, FALSE, FALSE, 0);

  GtkWidget *type_lbl = gtk_label_new (type);
  gtk_label_set_xalign (GTK_LABEL (type_lbl), 1.0);
  GtkStyleContext *type_ctx = gtk_widget_get_style_context (type_lbl);
  gtk_style_context_add_class (type_ctx, "sub-text");
  gtk_box_pack_end (GTK_BOX (item), type_lbl, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (container), item, FALSE, FALSE, 0);
  gtk_widget_show_all (item);
}

/* Query events for selected day and update the sidebar */
static void
calendar_window_update_details (CalendarWindow *win)
{
  int gy, gm, gd;
  jalali_to_gregorian (win->sel_year, win->sel_month, win->sel_day, &gy, &gm, &gd);
  HijriDate hd = gregorian_to_hijri (gy, gm, gd);

  /* Selected weekday */
  JalaliDate jd = gregorian_to_jalali (gy, gm, gd);

  /* Title: weekday name + day + month + year */
  char sel_day_p[16];
  to_persian_digits (win->sel_day, sel_day_p, sizeof (sel_day_p));
  char sel_year_p[16];
  to_persian_digits (win->sel_year, sel_year_p, sizeof (sel_year_p));

  char title_text[256];
  g_snprintf (title_text, sizeof (title_text), "%s %s %s %s",
              jalali_weekday_names[jd.weekday],
              sel_day_p,
              jalali_month_names[win->sel_month - 1],
              sel_year_p);
  gtk_label_set_text (GTK_LABEL (win->details_title), title_text);

  /* Subtitle: Gregorian & Hijri full dates */
  char hd_day_p[16];
  to_persian_digits (hd.day, hd_day_p, sizeof (hd_day_p));
  char hd_year_p[16];
  to_persian_digits (hd.year, hd_year_p, sizeof (hd_year_p));

  char date_text[256];
  g_snprintf (date_text, sizeof (date_text), "میلادی: %d %s %d\nقمری: %s %s %s",
              gd, greg_months_full[gm - 1], gy,
              hd_day_p, hijri_month_names[hd.month - 1], hd_year_p);
  gtk_label_set_text (GTK_LABEL (win->details_date), date_text);

  /* Clear existing events list */
  GList *children = gtk_container_get_children (GTK_CONTAINER (win->events_list_box));
  for (GList *iter = children; iter != NULL; iter = g_list_next (iter)) {
      gtk_widget_destroy (GTK_WIDGET (iter->data));
  }
  g_list_free (children);

  int total_events = 0;

  /* Solar events */
  const CalendarEvent *solar_out[8];
  int solar_cnt = find_events (solar_events, solar_events_count, win->sel_month, win->sel_day, solar_out, 8);
  for (int i = 0; i < solar_cnt; i++) {
      add_event_widget (win->events_list_box, solar_out[i]->title, solar_out[i]->holiday, "خورشیدی");
      total_events++;
  }

  /* Lunar events */
  const CalendarEvent *lunar_out[8];
  int lunar_cnt = find_lunar_events (hd.month, hd.day, hijri_days_in_month (hd.year, hd.month), lunar_out, 8);
  for (int i = 0; i < lunar_cnt; i++) {
      add_event_widget (win->events_list_box, lunar_out[i]->title, lunar_out[i]->holiday, "قمری");
      total_events++;
  }

  /* Gregorian events */
  const CalendarEvent *greg_out[8];
  int greg_cnt = find_events (gregorian_events, gregorian_events_count, gm, gd, greg_out, 8);
  for (int i = 0; i < greg_cnt; i++) {
      add_event_widget (win->events_list_box, greg_out[i]->title, greg_out[i]->holiday, "میلادی");
      total_events++;
  }

  if (total_events == 0) {
      GtkWidget *no_event_lbl = gtk_label_new ("مناسبتی برای این روز ثبت نشده است.");
      gtk_widget_set_halign (no_event_lbl, GTK_ALIGN_CENTER);
      GtkStyleContext *no_event_ctx = gtk_widget_get_style_context (no_event_lbl);
      gtk_style_context_add_class (no_event_ctx, "sub-text");
      gtk_box_pack_start (GTK_BOX (win->events_list_box), no_event_lbl, FALSE, FALSE, 16);
      gtk_widget_show (no_event_lbl);
  }
}

/* Render the 42 cells and titles for the current view month */
static void
calendar_window_render (CalendarWindow *win)
{
  /* Render header title */
  char year_buf[16];
  to_persian_digits (win->view_year, year_buf, sizeof (year_buf));
  char title_text[128];
  g_snprintf (title_text, sizeof (title_text), "%s %s", jalali_month_names[win->view_month - 1], year_buf);
  gtk_label_set_text (GTK_LABEL (win->title_label), title_text);

  /* Render Gregorian and Hijri secondary month ranges */
  int gy1, gm1, gd1;
  jalali_to_gregorian (win->view_year, win->view_month, 1, &gy1, &gm1, &gd1);
  int cur_days = jalali_days_in_month (win->view_year, win->view_month);
  int gy2, gm2, gd2;
  jalali_to_gregorian (win->view_year, win->view_month, cur_days, &gy2, &gm2, &gd2);

  HijriDate h1 = gregorian_to_hijri (gy1, gm1, gd1);
  HijriDate h2 = gregorian_to_hijri (gy2, gm2, gd2);

  char hg_text[256];
  if (gy1 == gy2) {
      if (gm1 == gm2) {
          g_snprintf (hg_text, sizeof (hg_text), "%s %d  |  %s %d",
                      greg_month_names[gm1-1], gy1,
                      hijri_month_names[h1.month-1], h1.year);
      } else {
          g_snprintf (hg_text, sizeof (hg_text), "%s - %s %d  |  %s - %s %d",
                      greg_month_names[gm1-1], greg_month_names[gm2-1], gy1,
                      hijri_month_names[h1.month-1], hijri_month_names[h2.month-1], h1.year);
      }
  } else {
      g_snprintf (hg_text, sizeof (hg_text), "%s %d - %s %d  |  %s %d - %s %d",
                  greg_month_names[gm1-1], gy1, greg_month_names[gm2-1], gy2,
                  hijri_month_names[h1.month-1], h1.year, hijri_month_names[h2.month-1], h2.year);
  }
  gtk_label_set_text (GTK_LABEL (win->hijri_greg_label), hg_text);

  /* Day of week of the 1st of the Jalali month */
  JalaliDate first_jd = gregorian_to_jalali (gy1, gm1, gd1);
  int start_wday = first_jd.weekday; /* 0 = Sat ... 6 = Fri */

  /* Trailing days of previous month */
  int prev_month = win->view_month - 1;
  int prev_year = win->view_year;
  if (prev_month == 0) {
      prev_month = 12;
      prev_year--;
  }
  int prev_days = jalali_days_in_month (prev_year, prev_month);

  /* Next month details */
  int next_month = win->view_month + 1;
  int next_year = win->view_year;
  if (next_month == 13) {
      next_month = 1;
      next_year++;
  }

  /* Populate cells array */
  int cell_idx = 0;
  for (int i = start_wday - 1; i >= 0; i--) {
      DayCell *cell = &win->cells[cell_idx++];
      cell->year = prev_year;
      cell->month = prev_month;
      cell->day = prev_days - i;
      cell->other_month = TRUE;
  }

  for (int i = 1; i <= cur_days; i++) {
      DayCell *cell = &win->cells[cell_idx++];
      cell->year = win->view_year;
      cell->month = win->view_month;
      cell->day = i;
      cell->other_month = FALSE;
  }

  int next_day = 1;
  while (cell_idx < 42) {
      DayCell *cell = &win->cells[cell_idx++];
      cell->year = next_year;
      cell->month = next_month;
      cell->day = next_day++;
      cell->other_month = TRUE;
  }

  /* Set widget labels and styling */
  for (int i = 0; i < 42; i++) {
      DayCell *cell = &win->cells[i];
      int c_gy, c_gm, c_gd;
      jalali_to_gregorian (cell->year, cell->month, cell->day, &c_gy, &c_gm, &c_gd);
      HijriDate c_hd = gregorian_to_hijri (c_gy, c_gm, c_gd);

      char main_buf[16];
      to_persian_digits (cell->day, main_buf, sizeof (main_buf));
      gtk_label_set_text (GTK_LABEL (cell->main_lbl), main_buf);

      char greg_buf[16];
      g_snprintf (greg_buf, sizeof (greg_buf), "%d", c_gd);
      gtk_label_set_text (GTK_LABEL (cell->greg_lbl), greg_buf);

      char hijri_buf[16];
      to_persian_digits (c_hd.day, hijri_buf, sizeof (hijri_buf));
      gtk_label_set_text (GTK_LABEL (cell->hijri_lbl), hijri_buf);

      /* Calculate if cell has holiday */
      cell->is_holiday = FALSE;

      const CalendarEvent *solar_out[8];
      int solar_cnt = find_events (solar_events, solar_events_count, cell->month, cell->day, solar_out, 8);
      for (int k = 0; k < solar_cnt; k++) {
          if (solar_out[k]->holiday) cell->is_holiday = TRUE;
      }

      const CalendarEvent *lunar_out[8];
      int lunar_cnt = find_lunar_events (c_hd.month, c_hd.day, hijri_days_in_month (c_hd.year, c_hd.month), lunar_out, 8);
      for (int k = 0; k < lunar_cnt; k++) {
          if (lunar_out[k]->holiday) cell->is_holiday = TRUE;
      }

      const CalendarEvent *greg_out[8];
      int greg_cnt = find_events (gregorian_events, gregorian_events_count, c_gm, c_gd, greg_out, 8);
      for (int k = 0; k < greg_cnt; k++) {
          if (greg_out[k]->holiday) cell->is_holiday = TRUE;
      }

      GtkStyleContext *ctx = gtk_widget_get_style_context (cell->button);
      gtk_style_context_remove_class (ctx, "other-month");
      gtk_style_context_remove_class (ctx, "holiday");
      gtk_style_context_remove_class (ctx, "current-day");

      if (cell->other_month) {
          gtk_style_context_add_class (ctx, "other-month");
      }
      if (cell->is_holiday) {
          gtk_style_context_add_class (ctx, "holiday");
      }
      if (cell->year == win->today_year && cell->month == win->today_month && cell->day == win->today_day) {
          gtk_style_context_add_class (ctx, "current-day");
      }
  }
}

/* Handler for cell clicks */
static void
on_day_clicked (GtkWidget *button, gpointer user_data)
{
  CalendarWindow *win = (CalendarWindow *)user_data;
  
  DayCell *cell = NULL;
  for (int i = 0; i < 42; i++) {
      if (win->cells[i].button == button) {
          cell = &win->cells[i];
          break;
      }
  }
  
  if (cell != NULL) {
      win->sel_year = cell->year;
      win->sel_month = cell->month;
      win->sel_day = cell->day;
      
      calendar_window_update_details (win);
  }
}

/* Handler for navigation buttons */
static void
on_prev_month_clicked (GtkWidget *button, gpointer user_data)
{
  (void)button;
  CalendarWindow *win = (CalendarWindow *)user_data;
  win->view_month--;
  if (win->view_month == 0) {
      win->view_month = 12;
      win->view_year--;
  }
  calendar_window_render (win);
}

static void
on_next_month_clicked (GtkWidget *button, gpointer user_data)
{
  (void)button;
  CalendarWindow *win = (CalendarWindow *)user_data;
  win->view_month++;
  if (win->view_month == 13) {
      win->view_month = 1;
      win->view_year++;
  }
  calendar_window_render (win);
}

/* Window destroy callback */
static void
on_window_destroy (GtkWidget *widget, gpointer user_data)
{
  (void)widget;
  PersianCalendar *calendar = PERSIAN_CALENDAR (user_data);
  calendar->last_destroy_time = g_get_monotonic_time () / 1000;
  calendar->window = NULL;
}

/* Callback when window loses focus to auto-close it */
static gboolean
on_window_focus_out (GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
  (void)event;
  PersianCalendar *calendar = PERSIAN_CALENDAR (user_data);
  if (calendar->window == widget) {
      gtk_widget_destroy (widget);
  }
  return FALSE;
}

/* Create the premium calendar popup dialog */
static GtkWidget *
create_calendar_window (PersianCalendar *calendar)
{
  load_css ();

  CalendarWindow *win = g_new0 (CalendarWindow, 1);
  
  win->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (win->window), "تقویم جلالی");
  gtk_window_set_default_size (GTK_WINDOW (win->window), 600, 360);
  gtk_window_set_resizable (GTK_WINDOW (win->window), FALSE);
  gtk_window_set_keep_above (GTK_WINDOW (win->window), TRUE);
  gtk_window_set_type_hint (GTK_WINDOW (win->window), GDK_WINDOW_TYPE_HINT_UTILITY);
  gtk_window_set_position (GTK_WINDOW (win->window), GTK_WIN_POS_MOUSE);

  /* Set transient link with panel plugin window */
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (calendar));
  if (GTK_IS_WINDOW (toplevel)) {
      gtk_window_set_transient_for (GTK_WINDOW (win->window), GTK_WINDOW (toplevel));
  }

  GtkStyleContext *win_ctx = gtk_widget_get_style_context (win->window);
  gtk_style_context_add_class (win_ctx, "calendar-window");

  /* Force Right-to-Left (RTL) text direction for Persian layout */
  gtk_widget_set_direction (win->window, GTK_TEXT_DIR_RTL);

  GtkWidget *main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_container_add (GTK_CONTAINER (win->window), main_hbox);

  /* Left Calendar grid vbox */
  GtkWidget *cal_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (main_hbox), cal_vbox, TRUE, TRUE, 0);

  /* Header Controls box */
  GtkWidget *header_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  GtkStyleContext *header_ctx = gtk_widget_get_style_context (header_hbox);
  gtk_style_context_add_class (header_ctx, "calendar-header");
  gtk_box_pack_start (GTK_BOX (cal_vbox), header_hbox, FALSE, FALSE, 0);

  /* Force LTR direction on the header box to keep previous month on the left
   * and next month on the right, matching standard calendar navigation. */
  gtk_widget_set_direction (header_hbox, GTK_TEXT_DIR_LTR);

  GtkWidget *prev_btn = gtk_button_new_with_label ("◀");
  GtkStyleContext *prev_ctx = gtk_widget_get_style_context (prev_btn);
  gtk_style_context_add_class (prev_ctx, "nav-button");
  gtk_box_pack_start (GTK_BOX (header_hbox), prev_btn, FALSE, FALSE, 0);

  GtkWidget *title_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (header_hbox), title_vbox, TRUE, TRUE, 0);

  win->title_label = gtk_label_new ("");
  gtk_widget_set_halign (win->title_label, GTK_ALIGN_CENTER);
  GtkStyleContext *title_lbl_ctx = gtk_widget_get_style_context (win->title_label);
  gtk_style_context_add_class (title_lbl_ctx, "main-text");
  gtk_box_pack_start (GTK_BOX (title_vbox), win->title_label, FALSE, FALSE, 0);

  win->hijri_greg_label = gtk_label_new ("");
  gtk_widget_set_halign (win->hijri_greg_label, GTK_ALIGN_CENTER);
  GtkStyleContext *hg_ctx = gtk_widget_get_style_context (win->hijri_greg_label);
  gtk_style_context_add_class (hg_ctx, "sub-text");
  gtk_box_pack_start (GTK_BOX (title_vbox), win->hijri_greg_label, FALSE, FALSE, 0);

  GtkWidget *next_btn = gtk_button_new_with_label ("▶");
  GtkStyleContext *next_ctx = gtk_widget_get_style_context (next_btn);
  gtk_style_context_add_class (next_ctx, "nav-button");
  gtk_box_pack_start (GTK_BOX (header_hbox), next_btn, FALSE, FALSE, 0);

  /* Calendar grid packing */
  GtkWidget *grid_container = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_set_border_width (GTK_CONTAINER (grid_container), 12);
  gtk_box_pack_start (GTK_BOX (cal_vbox), grid_container, TRUE, TRUE, 0);

  /* Grid component */
  win->grid = gtk_grid_new ();
  gtk_grid_set_row_homogeneous (GTK_GRID (win->grid), TRUE);
  gtk_grid_set_column_homogeneous (GTK_GRID (win->grid), TRUE);
  gtk_box_pack_start (GTK_BOX (grid_container), win->grid, TRUE, TRUE, 0);

  /* Force LTR direction on the grid container so column 0 is always left and column 6 is always right,
   * regardless of the user's GTK settings or locale. */
  gtk_widget_set_direction (win->grid, GTK_TEXT_DIR_LTR);

  /* Weekday headers row inside the grid (row 0) */
  for (int i = 0; i < 7; i++) {
      GtkWidget *lbl = gtk_label_new (jalali_weekday_names[i]);
      GtkStyleContext *lbl_ctx = gtk_widget_get_style_context (lbl);
      gtk_style_context_add_class (lbl_ctx, "weekday-header");
      
      /* Sat (i=0) is attached to col 6 (far right), Fri (i=6) to col 0 (far left) */
      gtk_grid_attach (GTK_GRID (win->grid), lbl, 6 - i, 0, 1, 1);
  }

  /* Create cells and attach to rows 1 to 6 */
  for (int row = 0; row < 6; row++) {
      for (int col = 0; col < 7; col++) {
          int idx = row * 7 + col;
          DayCell *cell = &win->cells[idx];
          cell->button = create_day_cell (&cell->main_lbl, &cell->greg_lbl, &cell->hijri_lbl);
          
          /* Force LTR layout inside the day button as well */
          gtk_widget_set_direction (cell->button, GTK_TEXT_DIR_LTR);

          /* Attach to grid: Sat (col=0) at column 6, Fri (col=6) at column 0 */
          gtk_grid_attach (GTK_GRID (win->grid), cell->button, 6 - col, row + 1, 1, 1);
          g_signal_connect (cell->button, "clicked", G_CALLBACK (on_day_clicked), win);
      }
  }

  /* Right Sidebar details panel */
  GtkWidget *sidebar = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request (sidebar, 280, -1); /* Make the events sidebar wider */
  GtkStyleContext *side_ctx = gtk_widget_get_style_context (sidebar);
  gtk_style_context_add_class (side_ctx, "sidebar");
  gtk_box_pack_end (GTK_BOX (main_hbox), sidebar, FALSE, FALSE, 0);

  win->details_title = gtk_label_new ("");
  gtk_widget_set_halign (win->details_title, GTK_ALIGN_CENTER);
  gtk_label_set_justify (GTK_LABEL (win->details_title), GTK_JUSTIFY_CENTER);
  GtkStyleContext *dt_ctx = gtk_widget_get_style_context (win->details_title);
  gtk_style_context_add_class (dt_ctx, "sidebar-title");
  gtk_box_pack_start (GTK_BOX (sidebar), win->details_title, FALSE, FALSE, 0);

  win->details_date = gtk_label_new ("");
  gtk_widget_set_halign (win->details_date, GTK_ALIGN_CENTER);
  gtk_label_set_justify (GTK_LABEL (win->details_date), GTK_JUSTIFY_CENTER);
  GtkStyleContext *dd_ctx = gtk_widget_get_style_context (win->details_date);
  gtk_style_context_add_class (dd_ctx, "sidebar-subtitle");
  gtk_box_pack_start (GTK_BOX (sidebar), win->details_date, FALSE, FALSE, 0);

  GtkWidget *sep = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (sidebar), sep, FALSE, FALSE, 8);

  GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (sidebar), scroll, TRUE, TRUE, 0);

  win->events_list_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_add (GTK_CONTAINER (scroll), win->events_list_box);

  /* Set current dates */
  GDateTime *now = g_date_time_new_now_local ();
  int gy = g_date_time_get_year (now);
  int gm = g_date_time_get_month (now);
  int gd = g_date_time_get_day_of_month (now);
  g_date_time_unref (now);

  JalaliDate today = gregorian_to_jalali (gy, gm, gd);
  win->today_year = today.year;
  win->today_month = today.month;
  win->today_day = today.day;

  win->view_year = today.year;
  win->view_month = today.month;

  win->sel_year = today.year;
  win->sel_month = today.month;
  win->sel_day = today.day;

  /* Connect signals */
  g_signal_connect (prev_btn, "clicked", G_CALLBACK (on_next_month_clicked), win);
  g_signal_connect (next_btn, "clicked", G_CALLBACK (on_prev_month_clicked), win);

  g_signal_connect (win->window, "destroy", G_CALLBACK (on_window_destroy), calendar);
  g_signal_connect_data (win->window, "destroy", G_CALLBACK (g_free), win, NULL, G_CONNECT_SWAPPED);
  g_signal_connect (win->window, "focus-out-event", G_CALLBACK (on_window_focus_out), calendar);

  /* Initial rendering */
  calendar_window_render (win);
  calendar_window_update_details (win);

  gtk_widget_show_all (win->window);
  gtk_window_present (GTK_WINDOW (win->window));

  return win->window;
}

/* Apply the custom font to the panel label */
static void
apply_label_font (PersianCalendar *calendar)
{
  PangoFontDescription *font_desc = NULL;

  if (calendar->custom_font != NULL && calendar->custom_font[0] != '\0') {
    font_desc = pango_font_description_from_string (calendar->custom_font);
  }

  GtkStyleContext *ctx = gtk_widget_get_style_context (calendar->label);
  GtkCssProvider *provider = g_object_get_data (G_OBJECT (calendar->label), "font-provider");

  if (provider != NULL) {
    gtk_style_context_remove_provider (ctx, GTK_STYLE_PROVIDER (provider));
    g_object_set_data (G_OBJECT (calendar->label), "font-provider", NULL);
  }

  if (font_desc != NULL) {
    const gchar *family = pango_font_description_get_family (font_desc);
    gint size = pango_font_description_get_size (font_desc);
    gboolean size_is_absolute = pango_font_description_get_size_is_absolute (font_desc);
    PangoWeight weight = pango_font_description_get_weight (font_desc);
    PangoStyle style = pango_font_description_get_style (font_desc);

    if (family == NULL) family = "sans-serif";
    if (size <= 0) {
      size = 14 * PANGO_SCALE;
      size_is_absolute = FALSE;
    }

    const char *style_str = "normal";
    if (style == PANGO_STYLE_ITALIC) {
      style_str = "italic";
    } else if (style == PANGO_STYLE_OBLIQUE) {
      style_str = "oblique";
    }

    gchar *css;
    int size_val = size / PANGO_SCALE;
    const char *size_unit = size_is_absolute ? "px" : "pt";

    css = g_strdup_printf ("label { font-family: '%s'; font-size: %d%s; font-weight: %d; font-style: %s; }",
                           family, size_val, size_unit, (int)weight, style_str);

    provider = gtk_css_provider_new ();
    gtk_css_provider_load_from_data (provider, css, -1, NULL);
    gtk_style_context_add_provider (ctx, GTK_STYLE_PROVIDER (provider),
                                     GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_set_data_full (G_OBJECT (calendar->label), "font-provider", provider, g_object_unref);
    g_free (css);
    pango_font_description_free (font_desc);
  }
}



/* Helper to format dynamic date values using custom string specifiers */
static void
format_date_string (const gchar *format, char *out_buf, int out_len,
                    JalaliDate *jd, HijriDate *hd, int gy, int gm, int gd)
{
  char *p = out_buf;
  char *end = out_buf + out_len - 1;
  const gchar *f = format;

  while (*f != '\0' && p < end)
    {
      if (*f == '%' && *(f + 1) != '\0')
        {
          f++; /* skip '%' */
          char val_buf[128] = "";

          if (*f == 'd')
            {
              to_persian_digits (jd->day, val_buf, sizeof (val_buf));
            }
          else if (*f == 'm')
            {
              to_persian_digits (jd->month, val_buf, sizeof (val_buf));
            }
          else if (*f == 'B')
            {
              g_strlcpy (val_buf, jalali_month_names[jd->month - 1], sizeof (val_buf));
            }
          else if (*f == 'Y')
            {
              to_persian_digits (jd->year, val_buf, sizeof (val_buf));
            }
          else if (*f == 'y')
            {
              to_persian_digits (jd->year % 100, val_buf, sizeof (val_buf));
            }
          else if (*f == 'A')
            {
              g_strlcpy (val_buf, jalali_weekday_names[jd->weekday], sizeof (val_buf));
            }
          else if (*f == 'h' && *(f + 1) != '\0')
            {
              f++; /* skip 'h' */
              if (*f == 'd')
                {
                  to_persian_digits (hd->day, val_buf, sizeof (val_buf));
                }
              else if (*f == 'B')
                {
                  g_strlcpy (val_buf, hijri_month_names[hd->month - 1], sizeof (val_buf));
                }
              else if (*f == 'Y')
                {
                  to_persian_digits (hd->year, val_buf, sizeof (val_buf));
                }
              else
                {
                  g_snprintf (val_buf, sizeof (val_buf), "%%h%c", *f);
                }
            }
          else if (*f == 'g' && *(f + 1) != '\0')
            {
              f++; /* skip 'g' */
              if (*f == 'd')
                {
                  g_snprintf (val_buf, sizeof (val_buf), "%d", gd);
                }
              else if (*f == 'B')
                {
                  g_strlcpy (val_buf, greg_month_names[gm - 1], sizeof (val_buf));
                }
              else if (*f == 'Y')
                {
                  g_snprintf (val_buf, sizeof (val_buf), "%d", gy);
                }
              else
                {
                  g_snprintf (val_buf, sizeof (val_buf), "%%g%c", *f);
                }
            }
          else if (*f == '%')
            {
              val_buf[0] = '%';
              val_buf[1] = '\0';
            }
          else
            {
              g_snprintf (val_buf, sizeof (val_buf), "%%%c", *f);
            }

          int val_len = strlen (val_buf);
          if (p + val_len < end)
            {
              strcpy (p, val_buf);
              p += val_len;
            }
          f++;
        }
      else
        {
          *p++ = *f++;
        }
    }
  *p = '\0';
}

/* Build the panel label text based on the selected display format */
static void
build_panel_label_text (PersianCalendar *calendar, JalaliDate jd, HijriDate hd, int gy, int gm, int gd)
{
  char day_buf[16];
  to_persian_digits (jd.day, day_buf, sizeof (day_buf));

  char year_buf[16];
  to_persian_digits (jd.year, year_buf, sizeof (year_buf));

  char hd_day_p[16];
  to_persian_digits (hd.day, hd_day_p, sizeof (hd_day_p));

  char label_text[256];
  label_text[0] = '\0';

  switch (calendar->display_format) {
    case FORMAT_DAY_MONTH_YEAR:
      g_snprintf (label_text, sizeof (label_text), "%s %s %s",
                  day_buf, jalali_month_names[jd.month - 1], year_buf);
      break;
    case FORMAT_WEEKDAY_DAY_MONTH:
      g_snprintf (label_text, sizeof (label_text), "%s %s %s",
                  jalali_weekday_names[jd.weekday], day_buf, jalali_month_names[jd.month - 1]);
      break;
    case FORMAT_WEEKDAY_DAY_MONTH_YEAR:
      g_snprintf (label_text, sizeof (label_text), "%s %s %s %s",
                  jalali_weekday_names[jd.weekday], day_buf, jalali_month_names[jd.month - 1], year_buf);
      break;
    case FORMAT_JALALI_HIJRI:
      g_snprintf (label_text, sizeof (label_text), "%s %s | %s %s",
                  day_buf, jalali_month_names[jd.month - 1],
                  hd_day_p, hijri_month_names[hd.month - 1]);
      break;
    case FORMAT_DAY_ONLY:
      g_snprintf (label_text, sizeof (label_text), "%s", day_buf);
      break;
    case FORMAT_MONTH_ONLY:
      g_snprintf (label_text, sizeof (label_text), "%s", jalali_month_names[jd.month - 1]);
      break;
    case FORMAT_FULL_DATES:
      g_snprintf (label_text, sizeof (label_text), "%s %s %s | %d %s %d",
                  day_buf, jalali_month_names[jd.month - 1], year_buf,
                  gd, greg_month_names[gm - 1], gy);
      break;
    case FORMAT_NUMERIC_DAY_MONTH:
      {
        char m_buf[8];
        to_persian_digits (jd.month, m_buf, sizeof (m_buf));
        char day_padded[16], mon_padded[16];
        if (jd.day < 10)
          g_snprintf (day_padded, sizeof (day_padded), "۰%s", day_buf);
        else
          g_strlcpy (day_padded, day_buf, sizeof (day_padded));
        if (jd.month < 10)
          g_snprintf (mon_padded, sizeof (mon_padded), "۰%s", m_buf);
        else
          g_strlcpy (mon_padded, m_buf, sizeof (mon_padded));
        /* Swap month and day in C memory so RTL rendering displays day-month on screen */
        g_snprintf (label_text, sizeof (label_text), "%s-%s", mon_padded, day_padded);
      }
      break;
    case FORMAT_CUSTOM:
      if (calendar->custom_format != NULL && calendar->custom_format[0] != '\0') {
        format_date_string (calendar->custom_format, label_text, sizeof (label_text), &jd, &hd, gy, gm, gd);
      } else {
        format_date_string ("%d %B", label_text, sizeof (label_text), &jd, &hd, gy, gm, gd);
      }
      break;
    case FORMAT_DAY_MONTH:
    default:
      g_snprintf (label_text, sizeof (label_text), "%s %s", day_buf, jalali_month_names[jd.month - 1]);
      break;
  }

  /* Render text natively in RTL */
  gtk_widget_set_direction (calendar->label, GTK_TEXT_DIR_RTL);
  gtk_label_set_text (GTK_LABEL (calendar->label), label_text);
}

/* Update the panel label and plugin tooltip periodically */
static gboolean
persian_calendar_update_label (gpointer user_data)
{
  PersianCalendar *calendar = PERSIAN_CALENDAR (user_data);
  
  GDateTime *now = g_date_time_new_now_local ();
  int gy = g_date_time_get_year (now);
  int gm = g_date_time_get_month (now);
  int gd = g_date_time_get_day_of_month (now);
  g_date_time_unref (now);

  JalaliDate jd = gregorian_to_jalali (gy, gm, gd);
  HijriDate hd = gregorian_to_hijri (gy, gm, gd);

  build_panel_label_text (calendar, jd, hd, gy, gm, gd);

  /* Calculate if today is holiday */
  gboolean today_is_holiday = FALSE;
  if (jd.weekday == 6) { /* Friday is weekend holiday in Iran */
    today_is_holiday = TRUE;
  } else {
    const CalendarEvent *solar_out[8];
    int solar_cnt = find_events (solar_events, solar_events_count, jd.month, jd.day, solar_out, 8);
    for (int k = 0; k < solar_cnt; k++) {
        if (solar_out[k]->holiday) today_is_holiday = TRUE;
    }

    const CalendarEvent *lunar_out[8];
    int lunar_cnt = find_lunar_events (hd.month, hd.day, hijri_days_in_month (hd.year, hd.month), lunar_out, 8);
    for (int k = 0; k < lunar_cnt; k++) {
        if (lunar_out[k]->holiday) today_is_holiday = TRUE;
    }

    const CalendarEvent *greg_out[8];
    int greg_cnt = find_events (gregorian_events, gregorian_events_count, gm, gd, greg_out, 8);
    for (int k = 0; k < greg_cnt; k++) {
        if (greg_out[k]->holiday) today_is_holiday = TRUE;
    }
  }

  GtkStyleContext *ctx = gtk_widget_get_style_context (calendar->label);
  GtkCssProvider *provider = g_object_get_data (G_OBJECT (calendar->label), "color-provider");

  if (provider != NULL) {
    gtk_style_context_remove_provider (ctx, GTK_STYLE_PROVIDER (provider));
    g_object_set_data (G_OBJECT (calendar->label), "color-provider", NULL);
  }

  const char *color_str = today_is_holiday ?
    ((calendar->holiday_color != NULL && calendar->holiday_color[0] != '\0') ? calendar->holiday_color : DEFAULT_HOLIDAY_COLOR) :
    ((calendar->normal_color != NULL && calendar->normal_color[0] != '\0') ? calendar->normal_color : DEFAULT_NORMAL_COLOR);

  gchar *css = g_strdup_printf ("label { color: %s; }", color_str);
  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, css, -1, NULL);
  gtk_style_context_add_provider (ctx, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_set_data_full (G_OBJECT (calendar->label), "color-provider", provider, g_object_unref);
  g_free (css);

  /* Hover Tooltip format: full details of Solar, Gregorian, and Hijri dates */
  char day_buf[16];
  to_persian_digits (jd.day, day_buf, sizeof (day_buf));

  char year_buf[16];
  to_persian_digits (jd.year, year_buf, sizeof (year_buf));
  
  char hd_day_p[16];
  to_persian_digits (hd.day, hd_day_p, sizeof (hd_day_p));
  char hd_year_p[16];
  to_persian_digits (hd.year, hd_year_p, sizeof (hd_year_p));

  char tooltip_text[512];
  g_snprintf (tooltip_text, sizeof (tooltip_text),
              "تقویم خورشیدی: %s %s %s %s\n"
              "تقویم میلادی: %d/%d/%d\n"
              "تقویم قمری: %s %s %s",
              jalali_weekday_names[jd.weekday],
              day_buf,
              jalali_month_names[jd.month - 1],
              year_buf,
              gy, gm, gd,
              hd_day_p,
              hijri_month_names[hd.month - 1],
              hd_year_p);
  
  gtk_widget_set_tooltip_text (GTK_WIDGET (calendar), tooltip_text);

  return TRUE;
}

static void
persian_calendar_orientation_changed (XfcePanelPlugin *plugin,
                                      GtkOrientation orientation)
{
  PersianCalendar *calendar = PERSIAN_CALENDAR (plugin);

  if (calendar->label != NULL)
    {
      if (orientation == GTK_ORIENTATION_VERTICAL)
        {
          /* Rotate panel text for vertical panels */
          gtk_label_set_angle (GTK_LABEL (calendar->label), 270.0);
        }
      else
        {
          /* Horizontal text for horizontal panels */
          gtk_label_set_angle (GTK_LABEL (calendar->label), 0.0);
        }
    }
}

static gboolean
persian_calendar_button_press (GtkWidget      *widget,
                               GdkEventButton *event,
                               gpointer        user_data)
{
  (void)user_data;

  if (event->button == 1) /* Left-click triggers the calendar dialog popup */
    {
      PersianCalendar *calendar = PERSIAN_CALENDAR (widget);

      if (calendar->window != NULL)
        {
          gtk_widget_destroy (calendar->window);
          calendar->window = NULL;
        }
      else
        {
          gint64 now_ms = g_get_monotonic_time () / 1000;
          /* Avoid recreating window if it was just closed via focus-out on the button */
          if (now_ms - calendar->last_destroy_time > 250)
            {
              calendar->window = create_calendar_window (calendar);
            }
        }
      return TRUE;
    }

  return FALSE; /* Let panel handle other events (e.g. right-click menu) */
}

/* Callback when xfconf property changes */
static void
on_xfconf_property_changed (XfconfChannel *channel, gchar *property, GValue *value, gpointer user_data)
{
  PersianCalendar *calendar = PERSIAN_CALENDAR (user_data);
  (void)channel;

  if (g_str_has_suffix (property, PROP_DISPLAY_FORMAT)) {
    if (G_VALUE_HOLDS_INT (value)) {
      int fmt = g_value_get_int (value);
      if (fmt >= 0 && fmt < FORMAT_COUNT) {
        calendar->display_format = fmt;
        persian_calendar_update_label (calendar);
      }
    }
  } else if (g_str_has_suffix (property, PROP_CUSTOM_FORMAT)) {
    if (G_VALUE_HOLDS_STRING (value)) {
      const gchar *fmt = g_value_get_string (value);
      g_free (calendar->custom_format);
      calendar->custom_format = (fmt != NULL && fmt[0] != '\0') ? g_strdup (fmt) : g_strdup ("%d %B");
      persian_calendar_update_label (calendar);
    }
  } else if (g_str_has_suffix (property, PROP_CUSTOM_FONT)) {
    if (G_VALUE_HOLDS_STRING (value)) {
      const gchar *font = g_value_get_string (value);
      g_free (calendar->custom_font);
      calendar->custom_font = (font != NULL && font[0] != '\0') ? g_strdup (font) : NULL;
      apply_label_font (calendar);
    }
  } else if (g_str_has_suffix (property, PROP_HOLIDAY_COLOR)) {
    if (G_VALUE_HOLDS_STRING (value)) {
      const gchar *color = g_value_get_string (value);
      g_free (calendar->holiday_color);
      calendar->holiday_color = (color != NULL && color[0] != '\0') ? g_strdup (color) : g_strdup (DEFAULT_HOLIDAY_COLOR);
      persian_calendar_update_label (calendar);
    }
  } else if (g_str_has_suffix (property, PROP_NORMAL_COLOR)) {
    if (G_VALUE_HOLDS_STRING (value)) {
      const gchar *color = g_value_get_string (value);
      g_free (calendar->normal_color);
      calendar->normal_color = (color != NULL && color[0] != '\0') ? g_strdup (color) : g_strdup (DEFAULT_NORMAL_COLOR);
      persian_calendar_update_label (calendar);
    }
  }
}

/* Configure dialog response handler */
static void
on_configure_response (GtkDialog *dialog, gint response, gpointer user_data)
{
  PersianCalendar *calendar = PERSIAN_CALENDAR (user_data);

  if (response == GTK_RESPONSE_OK) {
    GtkWidget *combo = g_object_get_data (G_OBJECT (dialog), "format-combo");
    int fmt = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
    calendar->display_format = fmt;
    xfconf_channel_set_int (calendar->channel, PROP_DISPLAY_FORMAT, fmt);

    GtkWidget *entry = g_object_get_data (G_OBJECT (dialog), "format-entry");
    if (entry != NULL) {
      const gchar *fmt_str = gtk_entry_get_text (GTK_ENTRY (entry));
      xfconf_channel_set_string (calendar->channel, PROP_CUSTOM_FORMAT, fmt_str != NULL ? fmt_str : "");
      g_free (calendar->custom_format);
      calendar->custom_format = (fmt_str != NULL && fmt_str[0] != '\0') ? g_strdup (fmt_str) : g_strdup ("%d %B");
    }

    GtkWidget *font_btn = g_object_get_data (G_OBJECT (dialog), "font-btn");
    if (font_btn != NULL) {
      gchar *font_name = gtk_font_chooser_get_font (GTK_FONT_CHOOSER (font_btn));
      xfconf_channel_set_string (calendar->channel, PROP_CUSTOM_FONT, font_name != NULL ? font_name : "");
      g_free (calendar->custom_font);
      calendar->custom_font = (font_name != NULL && font_name[0] != '\0') ? g_strdup (font_name) : NULL;
      apply_label_font (calendar);
      g_free (font_name);
    }

    GtkWidget *normal_btn = g_object_get_data (G_OBJECT (dialog), "normal-btn");
    if (normal_btn != NULL) {
      GdkRGBA rgba;
      gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (normal_btn), &rgba);
      gchar *color_str = g_strdup_printf ("#%02x%02x%02x",
                                          (int) (rgba.red * 255.0 + 0.5),
                                          (int) (rgba.green * 255.0 + 0.5),
                                          (int) (rgba.blue * 255.0 + 0.5));
      xfconf_channel_set_string (calendar->channel, PROP_NORMAL_COLOR, color_str);
      g_free (calendar->normal_color);
      calendar->normal_color = color_str;
    }

    GtkWidget *color_btn = g_object_get_data (G_OBJECT (dialog), "color-btn");
    if (color_btn != NULL) {
      GdkRGBA rgba;
      gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (color_btn), &rgba);
      gchar *color_str = g_strdup_printf ("#%02x%02x%02x",
                                          (int) (rgba.red * 255.0 + 0.5),
                                          (int) (rgba.green * 255.0 + 0.5),
                                          (int) (rgba.blue * 255.0 + 0.5));
      xfconf_channel_set_string (calendar->channel, PROP_HOLIDAY_COLOR, color_str);
      g_free (calendar->holiday_color);
      calendar->holiday_color = color_str;
    }

    persian_calendar_update_label (calendar);
  }
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

/* Callback when display format combo changed to toggle sensitivity and fill custom entry */
static void
on_format_combo_changed (GtkComboBox *combo, gpointer user_data)
{
  GtkWidget *entry = GTK_WIDGET (user_data);
  int active = gtk_combo_box_get_active (combo);

  if (active == FORMAT_CUSTOM) {
    gtk_widget_set_sensitive (entry, TRUE);
  } else {
    gtk_widget_set_sensitive (entry, FALSE);
    if (active >= 0 && active < FORMAT_CUSTOM) {
      gtk_entry_set_text (GTK_ENTRY (entry), preset_formats[active]);
    }
  }
}

/* Show the settings dialog */
static void
persian_calendar_configure_dialog (XfcePanelPlugin *plugin)
{
  PersianCalendar *calendar = PERSIAN_CALENDAR (plugin);

  GtkWidget *dialog = gtk_dialog_new_with_buttons (
    "تنظیمات تقویم جلالی",
    GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    "انصراف", GTK_RESPONSE_CANCEL,
    "تایید", GTK_RESPONSE_OK,
    NULL);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 380, 390);
  gtk_widget_set_direction (dialog, GTK_TEXT_DIR_RTL);

  GtkWidget *content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_set_border_width (GTK_CONTAINER (content), 12);

  GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_box_pack_start (GTK_BOX (content), vbox, TRUE, TRUE, 0);

  GtkWidget *label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), "<span size=\"large\"><b>نحوه نمایش تاریخ در پنل:</b></span>");
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_widget_set_direction (label, GTK_TEXT_DIR_RTL);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

  GtkWidget *combo = gtk_combo_box_text_new ();
  gtk_widget_set_direction (combo, GTK_TEXT_DIR_RTL);
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "روز و ماه (۱۲ تیر)");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "روز، ماه و سال (۱۲ تیر ۱۴۰۴)");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "روز هفته و تاریخ (شنبه ۱۲ تیر)");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "روز هفته، تاریخ و سال (شنبه ۹ تیر ۱۴۰۵)");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "شمسی و قمری (۱۲ تیر | ۵ محرم)");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "فقط روز (۱۲)");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "فقط ماه (تیر)");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "کامل شمسی و میلادی (۱۲ تیر ۱۴۰۴ | 1 Jul 2024)");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "روز و ماه عددی (۱۱-۰۴)");
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (combo), "قالب سفارشی (Custom)...");
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), calendar->display_format);
  gtk_widget_set_halign (combo, GTK_ALIGN_FILL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (dialog), "format-combo", combo);

  /* Custom Format Entry */
  GtkWidget *entry_label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (entry_label), "<span size=\"large\"><b>فرمت سفارشی:</b></span> <small>(فقط در حالت قالب سفارشی فعال است)</small>");
  gtk_widget_set_halign (entry_label, GTK_ALIGN_START);
  gtk_widget_set_direction (entry_label, GTK_TEXT_DIR_RTL);
  gtk_box_pack_start (GTK_BOX (vbox), entry_label, FALSE, FALSE, 0);

  GtkWidget *entry = gtk_entry_new ();
  gtk_widget_set_direction (entry, GTK_TEXT_DIR_LTR);
  if (calendar->custom_format != NULL) {
    gtk_entry_set_text (GTK_ENTRY (entry), calendar->custom_format);
  } else {
    gtk_entry_set_text (GTK_ENTRY (entry), "%d %B");
  }
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
  g_object_set_data (G_OBJECT (dialog), "format-entry", entry);

  /* Help list for custom format specifiers */
  GtkWidget *help_title = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (help_title), "<small><b>راهنمای قالب سفارشی:</b></small>");
  gtk_widget_set_halign (help_title, GTK_ALIGN_START);
  gtk_widget_set_direction (help_title, GTK_TEXT_DIR_RTL);
  gtk_box_pack_start (GTK_BOX (vbox), help_title, FALSE, FALSE, 2);

  GtkWidget *help_body = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (help_body),
    "<small>  <b>%A</b>: روز هفته (شنبه)  |  <b>%d</b>: روز شمسی (۱۲)  |  <b>%m</b>: شماره ماه (۰۴)\n"
    "  <b>%B</b>: نام ماه (تیر)  |  <b>%Y</b>: سال شمسی (۱۴۰۴)  |  <b>%y</b>: سال ۲ رقمی (۰۴)\n"
    "  <b>%hd</b>: روز قمری (۵)  |  <b>%hB</b>: ماه قمری  |  <b>%hY</b>: سال قمری (۱۴۴۷)\n"
    "  <b>%gd</b>: روز میلادی (1)  |  <b>%gB</b>: ماه میلادی  |  <b>%gY</b>: سال میلادی (2024)</small>");
  gtk_widget_set_halign (help_body, GTK_ALIGN_START);
  gtk_label_set_xalign (GTK_LABEL (help_body), 1.0);
  gtk_label_set_justify (GTK_LABEL (help_body), GTK_JUSTIFY_RIGHT);
  gtk_widget_set_direction (help_body, GTK_TEXT_DIR_RTL);
  gtk_box_pack_start (GTK_BOX (vbox), help_body, FALSE, FALSE, 2);

  /* Connect changed signal of combo to toggle entry sensitivity and pre-fill its value */
  g_signal_connect (combo, "changed", G_CALLBACK (on_format_combo_changed), entry);

  /* Trigger initial state */
  on_format_combo_changed (GTK_COMBO_BOX (combo), entry);

  /* Font selection */
  GtkWidget *font_label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (font_label), "<span size=\"large\"><b>فونت و اندازه متن پنل:</b></span>");
  gtk_widget_set_halign (font_label, GTK_ALIGN_START);
  gtk_widget_set_direction (font_label, GTK_TEXT_DIR_RTL);
  gtk_box_pack_start (GTK_BOX (vbox), font_label, FALSE, FALSE, 0);

  GtkWidget *font_btn = gtk_font_button_new ();
  gtk_widget_set_direction (font_btn, GTK_TEXT_DIR_RTL);
  gtk_font_button_set_show_style (GTK_FONT_BUTTON (font_btn), FALSE);
  gtk_font_button_set_show_size (GTK_FONT_BUTTON (font_btn), TRUE);
  if (calendar->custom_font != NULL && calendar->custom_font[0] != '\0') {
    gtk_font_chooser_set_font (GTK_FONT_CHOOSER (font_btn), calendar->custom_font);
  } else {
    gtk_font_chooser_set_font (GTK_FONT_CHOOSER (font_btn), "Sans 14");
  }
  gtk_box_pack_start (GTK_BOX (vbox), font_btn, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (dialog), "font-btn", font_btn);

  /* Normal day color selection */
  GtkWidget *normal_label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (normal_label), "<span size=\"large\"><b>رنگ روزهای عادی:</b></span>");
  gtk_widget_set_halign (normal_label, GTK_ALIGN_START);
  gtk_widget_set_direction (normal_label, GTK_TEXT_DIR_RTL);
  gtk_box_pack_start (GTK_BOX (vbox), normal_label, FALSE, FALSE, 0);

  GtkWidget *normal_btn = gtk_color_button_new ();
  gtk_widget_set_direction (normal_btn, GTK_TEXT_DIR_RTL);
  GdkRGBA rgba_normal;
  const char *curr_normal = (calendar->normal_color != NULL && calendar->normal_color[0] != '\0') ? calendar->normal_color : DEFAULT_NORMAL_COLOR;
  if (gdk_rgba_parse (&rgba_normal, curr_normal)) {
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (normal_btn), &rgba_normal);
  }
  gtk_box_pack_start (GTK_BOX (vbox), normal_btn, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (dialog), "normal-btn", normal_btn);

  /* Holiday color selection */
  GtkWidget *color_label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (color_label), "<span size=\"large\"><b>رنگ روزهای تعطیل:</b></span>");
  gtk_widget_set_halign (color_label, GTK_ALIGN_START);
  gtk_widget_set_direction (color_label, GTK_TEXT_DIR_RTL);
  gtk_box_pack_start (GTK_BOX (vbox), color_label, FALSE, FALSE, 0);

  GtkWidget *color_btn = gtk_color_button_new ();
  gtk_widget_set_direction (color_btn, GTK_TEXT_DIR_RTL);
  GdkRGBA rgba;
  const char *curr_color = (calendar->holiday_color != NULL && calendar->holiday_color[0] != '\0') ? calendar->holiday_color : DEFAULT_HOLIDAY_COLOR;
  if (gdk_rgba_parse (&rgba, curr_color)) {
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (color_btn), &rgba);
  }
  gtk_box_pack_start (GTK_BOX (vbox), color_btn, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (dialog), "color-btn", color_btn);

  g_signal_connect (dialog, "response", G_CALLBACK (on_configure_response), calendar);

  gtk_widget_show_all (dialog);
}

static void
persian_calendar_construct (XfcePanelPlugin *plugin)
{
  PersianCalendar *calendar = PERSIAN_CALENDAR (plugin);

  /* Allow panel plugin to span the full panel height */
  xfce_panel_plugin_set_small (plugin, FALSE);

  calendar->window = NULL;
  calendar->display_format = FORMAT_DAY_MONTH;
  calendar->custom_font = NULL;
  calendar->custom_format = NULL;
  calendar->holiday_color = NULL;
  calendar->normal_color = NULL;
  calendar->last_destroy_time = 0;

  /* Initialize Xfconf */
  if (xfconf_init (NULL)) {
    calendar->channel = xfconf_channel_get (XFCONF_CHANNEL_NAME);
    calendar->display_format = xfconf_channel_get_int (calendar->channel, PROP_DISPLAY_FORMAT, FORMAT_DAY_MONTH);
    if (calendar->display_format < 0 || calendar->display_format >= FORMAT_COUNT)
      calendar->display_format = FORMAT_DAY_MONTH;
    
    gchar *font = xfconf_channel_get_string (calendar->channel, PROP_CUSTOM_FONT, NULL);
    if (font != NULL && font[0] != '\0') {
      calendar->custom_font = font;
    } else {
      g_free (font);
    }

    gchar *fmt = xfconf_channel_get_string (calendar->channel, PROP_CUSTOM_FORMAT, NULL);
    if (fmt != NULL && fmt[0] != '\0') {
      calendar->custom_format = fmt;
    } else {
      g_free (fmt);
      calendar->custom_format = g_strdup ("%d %B");
    }

    gchar *color = xfconf_channel_get_string (calendar->channel, PROP_HOLIDAY_COLOR, NULL);
    if (color != NULL && color[0] != '\0') {
      calendar->holiday_color = color;
    } else {
      g_free (color);
      calendar->holiday_color = g_strdup (DEFAULT_HOLIDAY_COLOR);
    }

    gchar *normal_c = xfconf_channel_get_string (calendar->channel, PROP_NORMAL_COLOR, NULL);
    if (normal_c != NULL && normal_c[0] != '\0') {
      calendar->normal_color = normal_c;
    } else {
      g_free (normal_c);
      calendar->normal_color = g_strdup (DEFAULT_NORMAL_COLOR);
    }

    g_signal_connect (calendar->channel, "property-changed",
                      G_CALLBACK (on_xfconf_property_changed), calendar);
  } else {
    calendar->channel = NULL;
    calendar->custom_format = g_strdup ("%d %B");
    calendar->holiday_color = g_strdup (DEFAULT_HOLIDAY_COLOR);
    calendar->normal_color = g_strdup (DEFAULT_NORMAL_COLOR);
  }

  /* Create and style label */
  calendar->label = gtk_label_new ("");
  gtk_widget_set_margin_start (calendar->label, 6);
  gtk_widget_set_margin_end (calendar->label, 6);
  gtk_widget_set_halign (calendar->label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (calendar->label, GTK_ALIGN_CENTER);

  /* Apply custom font if set */
  apply_label_font (calendar);

  /* Load initial date label text */
  persian_calendar_update_label (calendar);

  /* Setup dynamic periodic update every 60 seconds */
  g_timeout_add_seconds (60, persian_calendar_update_label, calendar);

  /* Set initial text angle based on orientation */
  GtkOrientation orientation = xfce_panel_plugin_get_orientation (plugin);
  persian_calendar_orientation_changed (plugin, orientation);

  gtk_container_add (GTK_CONTAINER (plugin), calendar->label);
  gtk_widget_show (calendar->label);

  /* Enable left-clicks on panel button to open the popup calendar */
  gtk_widget_add_events (GTK_WIDGET (plugin), GDK_BUTTON_PRESS_MASK);
  g_signal_connect (plugin, "button-press-event",
                    G_CALLBACK (persian_calendar_button_press), NULL);

  /* Show configure menu item */
  xfce_panel_plugin_menu_show_configure (plugin);
}

static void
persian_calendar_free_data (XfcePanelPlugin *plugin)
{
  PersianCalendar *calendar = PERSIAN_CALENDAR (plugin);

  if (calendar->window != NULL)
    {
      gtk_widget_destroy (calendar->window);
      calendar->window = NULL;
    }

  if (calendar->channel != NULL)
    {
      g_signal_handlers_disconnect_by_data (calendar->channel, calendar);
      calendar->channel = NULL;
    }

  g_free (calendar->custom_font);
  calendar->custom_font = NULL;
  g_free (calendar->custom_format);
  calendar->custom_format = NULL;
  g_free (calendar->holiday_color);
  calendar->holiday_color = NULL;
  g_free (calendar->normal_color);
  calendar->normal_color = NULL;
}

static void
persian_calendar_init (PersianCalendar *calendar)
{
  calendar->label = NULL;
  calendar->window = NULL;
  calendar->channel = NULL;
  calendar->display_format = FORMAT_DAY_MONTH;
  calendar->custom_font = NULL;
  calendar->custom_format = NULL;
  calendar->holiday_color = NULL;
  calendar->normal_color = NULL;
  calendar->last_destroy_time = 0;
}

static void
persian_calendar_class_init (PersianCalendarClass *klass)
{
  XfcePanelPluginClass *plugin_class;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = persian_calendar_construct;
  plugin_class->orientation_changed = persian_calendar_orientation_changed;
  plugin_class->free_data = persian_calendar_free_data;
  plugin_class->configure_plugin = persian_calendar_configure_dialog;
}
