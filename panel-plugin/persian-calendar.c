#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4ui/libxfce4ui.h>
#include "../jalali.h"
#include "../hijri.h"
#include "../events_data.h"

#define PERSIAN_TYPE_CALENDAR (persian_calendar_get_type())
G_DECLARE_FINAL_TYPE (PersianCalendar, persian_calendar, PERSIAN, CALENDAR, XfcePanelPlugin)

struct _PersianCalendar
{
  XfcePanelPlugin parent_instance;

  /* plugin widgets */
  GtkWidget *label;
  GtkWidget *window; /* Pointer to the open calendar window, or NULL */
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
  int lunar_cnt = find_events (lunar_events, lunar_events_count, hd.month, hd.day, lunar_out, 8);
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
      int lunar_cnt = find_events (lunar_events, lunar_events_count, c_hd.month, c_hd.day, lunar_out, 8);
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
  calendar->window = NULL;
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
  g_signal_connect_data (win->window, "destroy", G_CALLBACK (g_free), win, NULL, 0);

  /* Initial rendering */
  calendar_window_render (win);
  calendar_window_update_details (win);

  gtk_widget_show_all (win->window);

  return win->window;
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

  /* Panel Text format: "۱۲ تیر" */
  char day_buf[16];
  to_persian_digits (jd.day, day_buf, sizeof (day_buf));

  char label_text[128];
  g_snprintf (label_text, sizeof (label_text), "%s %s", day_buf, jalali_month_names[jd.month - 1]);
  gtk_label_set_text (GTK_LABEL (calendar->label), label_text);

  /* Hover Tooltip format: full details of Solar, Gregorian, and Hijri dates */
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
          calendar->window = create_calendar_window (calendar);
        }
      return TRUE;
    }

  return FALSE; /* Let panel handle other events (e.g. right-click menu) */
}

static void
persian_calendar_construct (XfcePanelPlugin *plugin)
{
  PersianCalendar *calendar = PERSIAN_CALENDAR (plugin);

  /* Allow panel plugin to span the full panel height */
  xfce_panel_plugin_set_small (plugin, FALSE);

  calendar->window = NULL;

  /* Create and style label */
  calendar->label = gtk_label_new ("");
  gtk_widget_set_margin_start (calendar->label, 6);
  gtk_widget_set_margin_end (calendar->label, 6);
  gtk_widget_set_halign (calendar->label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (calendar->label, GTK_ALIGN_CENTER);

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
}

static void
persian_calendar_init (PersianCalendar *calendar)
{
  calendar->label = NULL;
  calendar->window = NULL;
}

static void
persian_calendar_class_init (PersianCalendarClass *klass)
{
  XfcePanelPluginClass *plugin_class;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = persian_calendar_construct;
  plugin_class->orientation_changed = persian_calendar_orientation_changed;
  plugin_class->free_data = persian_calendar_free_data;
}
