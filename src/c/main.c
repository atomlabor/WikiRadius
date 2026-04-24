#include <pebble.h>

#define MESSAGE_KEY_READY    0
#define MESSAGE_KEY_STATUS   1
#define MESSAGE_KEY_TITLE    2
#define MESSAGE_KEY_SNIPPET  3
#define MESSAGE_KEY_BEARING  4
#define MESSAGE_KEY_DISTANCE 5
#define MESSAGE_KEY_LOCALE   6

#if PBL_PLATFORM_EMERY
  #define SIDEBAR_W  24
  #define FONT_T_LG  FONT_KEY_GOTHIC_28_BOLD
  #define FONT_S     FONT_KEY_GOTHIC_24
  #define FONT_TIME  FONT_KEY_GOTHIC_18_BOLD
  #define TIME_STEP  18
  #define ICON_R     6
#else
  #define SIDEBAR_W  18
  #define FONT_T_LG  FONT_KEY_GOTHIC_24_BOLD
  #define FONT_S     FONT_KEY_GOTHIC_18
  #define FONT_TIME  FONT_KEY_GOTHIC_14_BOLD
  #define TIME_STEP  14
  #define ICON_R     5
#endif

#define LOGO_WIDTH  120
#define LOGO_HEIGHT 120

static Window      *s_main_window;
static ScrollLayer *s_scroll_layer;
static TextLayer   *s_title_layer, *s_snippet_layer, *s_status_layer;
static Layer       *s_sidebar_layer, *s_left_sidebar_layer, *s_radar_layer, *s_splash_bg_layer;
static BitmapLayer *s_bitmap_layer_splash_icon;
static GBitmap     *s_bitmap_splash_icon;

static char s_full_content[4096] = "";
static int  s_dist = 0, s_bear = 0, s_head = 0, s_radar_radius = 0;
static bool s_high_contrast = false;
static BatteryChargeState s_battery_state;
static AppTimer *s_radar_timer = NULL;
static AppTimer *s_update_timer = NULL;

static void splash_bg_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  static const int8_t bayer_matrix[4][4] = {
    { -8,  4, -6,  6 }, {  8, -4, 10, -2 }, { -5,  7, -7,  3 }, { 11, -1,  9, -3 }
  };
  for (int y = 0; y < b.size.h; y++) {
    int r_base = (y * 255) / b.size.h;
    int g_base = 255 - r_base;
    for (int x = 0; x < b.size.w; x++) {
      int noise = bayer_matrix[x % 4][y % 4] * 3;
      int r = r_base + noise; int g = g_base + noise;
      if (r < 0) { r = 0; } if (r > 255) { r = 255; }
      if (g < 0) { g = 0; } if (g > 255) { g = 255; }
      graphics_context_set_stroke_color(ctx, GColorFromRGB(r, g, 255));
      graphics_draw_pixel(ctx, GPoint(x, y));
    }
  }
}

static void radar_timer_callback(void *data) {
  s_radar_radius = (s_radar_radius + 3) % 100;
  layer_mark_dirty(s_radar_layer);
  s_radar_timer = app_timer_register(50, radar_timer_callback, NULL);
}

static void radar_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  GPoint center = GPoint(b.size.w / 2, b.size.h / 2);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_stroke_color(ctx, GColorMagenta);
  for (int i = 0; i < 3; i++) {
    int r = (s_radar_radius + (i * 33)) % 100;
    if (r > 2) graphics_draw_circle(ctx, center, r);
  }
}

static void left_sidebar_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, s_high_contrast ? GColorLightGray : GColorCyan);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  static char time_buf[8];
  strftime(time_buf, sizeof(time_buf), clock_is_24h_style() ? "%H:%M" : "%I:%M", t);
  graphics_context_set_text_color(ctx, GColorBlack);
  for(int i=0; i<5; i++) {
    char c[2] = { time_buf[i], '\0' };
    graphics_draw_text(ctx, c, fonts_get_system_font(FONT_TIME), 
                       GRect(2, 5 + (i*TIME_STEP), b.size.w - 4, TIME_STEP + 4), 
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
  
  int cy = b.size.h / 2;
  int cx = b.size.w / 2;
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  
  graphics_draw_line(ctx, GPoint(cx - 5, cy), GPoint(cx + 4, cy));
  graphics_draw_line(ctx, GPoint(cx - 5, cy), GPoint(cx - 1, cy - 4));
  graphics_draw_line(ctx, GPoint(cx - 5, cy), GPoint(cx - 1, cy + 4));

  int blocks = s_battery_state.charge_percent / 20;
  if (blocks == 0 && s_battery_state.charge_percent > 0) blocks = 1;
  for(int i=0; i<blocks; i++) {
    GRect r = GRect(3, b.size.h - 12 - (i*9), b.size.w - 6, 7);
    graphics_context_set_fill_color(ctx, GColorGreen);
    graphics_fill_rect(ctx, r, 0, GCornerNone);
    graphics_draw_rect(ctx, r);
  }
}

static void sidebar_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, s_high_contrast ? GColorLightGray : GColorCyan);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  
  GPoint cp = GPoint(b.size.w / 2, 25);
  graphics_draw_circle(ctx, cp, ICON_R);
  
  int needle_r = ICON_R - 1;
  GPoint end = gpoint_from_polar(GRect(cp.x-needle_r, cp.y-needle_r, needle_r*2, needle_r*2), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(s_bear - s_head));
  graphics_draw_line(ctx, cp, end);

  GPoint ic = GPoint(b.size.w / 2, b.size.h / 2);
  graphics_draw_circle(ctx, ic, ICON_R);
  if (s_high_contrast) {
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_radial(ctx, GRect(ic.x-ICON_R, ic.y-ICON_R, ICON_R*2, ICON_R*2), GOvalScaleModeFitCircle, ICON_R, 0, DEG_TO_TRIGANGLE(180));
  }

  if (s_dist > 0) {
    static char d_buf[12];
    snprintf(d_buf, sizeof(d_buf), "%dm", s_dist);
    graphics_context_set_text_color(ctx, GColorBlack);
    int len = (int)strlen(d_buf);
    for(int i=0; i < len; i++) {
      char c[2] = { d_buf[i], '\0' };
      graphics_draw_text(ctx, c, fonts_get_system_font(FONT_TIME), 
                         GRect(2, b.size.h - 12 - ((len - i) * TIME_STEP), b.size.w - 4, TIME_STEP + 4), 
                         GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    }
  }
}

static void request_update(void *data) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_uint8(iter, MESSAGE_KEY_READY, 1);
    const char *sys_lang = i18n_get_system_locale();
    dict_write_cstring(iter, MESSAGE_KEY_LOCALE, (sys_lang && sys_lang[0] != '\0') ? sys_lang : "en");
    app_message_outbox_send();
  }
  s_update_timer = app_timer_register(120000, request_update, NULL);
}

static void battery_handler(BatteryChargeState state) {
  s_battery_state = state;
  layer_mark_dirty(s_left_sidebar_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_left_sidebar_layer);
}

static void compass_handler(CompassHeadingData data) {
  if (data.compass_status == CompassStatusDataInvalid) return;
  int32_t target = (data.true_heading * 360) / TRIG_MAX_ANGLE;
  s_head = (s_head * 3 + target) / 4; 
  layer_mark_dirty(s_sidebar_layer);
}

static void toggle_contrast_handler(ClickRecognizerRef recognizer, void *context) {
  s_high_contrast = !s_high_contrast;
  vibes_short_pulse();
  window_set_background_color(s_main_window, s_high_contrast ? GColorWhite : GColorBlack);
  text_layer_set_text_color(s_title_layer, s_high_contrast ? GColorBlack : GColorMagenta);
  text_layer_set_text_color(s_snippet_layer, s_high_contrast ? GColorBlack : GColorWhite);
  text_layer_set_text_color(s_status_layer, s_high_contrast ? GColorDukeBlue : GColorCyan);
  layer_mark_dirty(s_sidebar_layer);
  layer_mark_dirty(s_left_sidebar_layer);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, toggle_contrast_handler);
}

static void inbox_handler(DictionaryIterator *iter, void *ctx) {
  Tuple *status_t = dict_find(iter, MESSAGE_KEY_STATUS);
  Tuple *title_t = dict_find(iter, MESSAGE_KEY_TITLE);
  Tuple *snippet_t = dict_find(iter, MESSAGE_KEY_SNIPPET);
  
  GRect b = layer_get_bounds(window_get_root_layer(s_main_window));
  int text_w = b.size.w - (2 * SIDEBAR_W) - 4;

  if (status_t) text_layer_set_text(s_status_layer, status_t->value->cstring);
  if (dict_find(iter, MESSAGE_KEY_DISTANCE)) s_dist = dict_find(iter, MESSAGE_KEY_DISTANCE)->value->int32;
  if (dict_find(iter, MESSAGE_KEY_BEARING)) s_bear = dict_find(iter, MESSAGE_KEY_BEARING)->value->int32;
  
  if (title_t && snippet_t) {
    layer_set_hidden(text_layer_get_layer(s_status_layer), true);
    layer_set_hidden(scroll_layer_get_layer(s_scroll_layer), false);
    text_layer_set_text(s_title_layer, title_t->value->cstring);
    snprintf(s_full_content, sizeof(s_full_content), "%s", snippet_t->value->cstring);
    text_layer_set_text(s_snippet_layer, s_full_content);
    
    GSize size = graphics_text_layout_get_content_size(s_full_content, fonts_get_system_font(FONT_S), GRect(0, 0, text_w, 10000), GTextOverflowModeWordWrap, GTextAlignmentLeft);
    layer_set_frame(text_layer_get_layer(s_snippet_layer), GRect(2, 50, text_w, size.h + 20));
    scroll_layer_set_content_size(s_scroll_layer, GSize(b.size.w - 2 * SIDEBAR_W, size.h + 80));
  }
  layer_mark_dirty(s_sidebar_layer);
  layer_mark_dirty(s_left_sidebar_layer);
}

static void transition_to_main(void *data) {
  if (s_radar_timer) { app_timer_cancel(s_radar_timer); s_radar_timer = NULL; }
  layer_set_hidden(s_radar_layer, true);
  layer_set_hidden(s_splash_bg_layer, true);
  layer_set_hidden(bitmap_layer_get_layer(s_bitmap_layer_splash_icon), true);
  window_set_background_color(s_main_window, GColorBlack);
  layer_set_hidden(s_sidebar_layer, false);
  layer_set_hidden(s_left_sidebar_layer, false);
  layer_set_hidden(text_layer_get_layer(s_status_layer), false);
  request_update(NULL);
}

static void prv_main_window_load(Window *window) {
  Layer *w_layer = window_get_root_layer(window);
  GRect b = layer_get_bounds(w_layer);
  int text_w = b.size.w - (2 * SIDEBAR_W) - 4;

  s_splash_bg_layer = layer_create(b);
  layer_set_update_proc(s_splash_bg_layer, splash_bg_update_proc);
  layer_add_child(w_layer, s_splash_bg_layer);
  s_radar_layer = layer_create(b);
  layer_set_update_proc(s_radar_layer, radar_update_proc);
  layer_add_child(w_layer, s_radar_layer);
  s_bitmap_splash_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SPLASH);
  s_bitmap_layer_splash_icon = bitmap_layer_create(GRect((b.size.w-LOGO_WIDTH)/2, (b.size.h-LOGO_HEIGHT)/2, LOGO_WIDTH, LOGO_HEIGHT));
  bitmap_layer_set_bitmap(s_bitmap_layer_splash_icon, s_bitmap_splash_icon);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_splash_icon, GCompOpSet);
  layer_add_child(w_layer, bitmap_layer_get_layer(s_bitmap_layer_splash_icon));
  s_scroll_layer = scroll_layer_create(GRect(SIDEBAR_W, 0, b.size.w - 2 * SIDEBAR_W, b.size.h));
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);
  scroll_layer_set_callbacks(s_scroll_layer, (ScrollLayerCallbacks){ .click_config_provider = click_config_provider });
  layer_add_child(w_layer, scroll_layer_get_layer(s_scroll_layer));

  s_title_layer = text_layer_create(GRect(2, 5, text_w, 45));
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_T_LG));
  text_layer_set_text_color(s_title_layer, GColorMagenta);
  text_layer_set_background_color(s_title_layer, GColorClear);
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_title_layer));

  s_snippet_layer = text_layer_create(GRect(2, 50, text_w, 2000));
  text_layer_set_font(s_snippet_layer, fonts_get_system_font(FONT_S));
  text_layer_set_text_color(s_snippet_layer, GColorWhite);
  text_layer_set_background_color(s_snippet_layer, GColorClear);
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_snippet_layer));

  s_status_layer = text_layer_create(GRect(SIDEBAR_W + 2, 60, text_w, 60));
  text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_color(s_status_layer, GColorCyan);
  text_layer_set_background_color(s_status_layer, GColorClear);
  text_layer_set_text_alignment(s_status_layer, GTextAlignmentCenter);
  text_layer_set_text(s_status_layer, "Fixing GPS...");
  layer_add_child(w_layer, text_layer_get_layer(s_status_layer));

  s_left_sidebar_layer = layer_create(GRect(0, 0, SIDEBAR_W, b.size.h));
  layer_set_update_proc(s_left_sidebar_layer, left_sidebar_update_proc);
  layer_add_child(w_layer, s_left_sidebar_layer);

  s_sidebar_layer = layer_create(GRect(b.size.w - SIDEBAR_W, 0, SIDEBAR_W, b.size.h));
  layer_set_update_proc(s_sidebar_layer, sidebar_update_proc);
  layer_add_child(w_layer, s_sidebar_layer);

  layer_set_hidden(s_sidebar_layer, true);
  layer_set_hidden(s_left_sidebar_layer, true);
  layer_set_hidden(scroll_layer_get_layer(s_scroll_layer), true);
  layer_set_hidden(text_layer_get_layer(s_status_layer), true);

  s_radar_timer = app_timer_register(50, radar_timer_callback, NULL);
  app_timer_register(4000, transition_to_main, NULL);
}

static void prv_main_window_unload(Window *window) {
  if (s_radar_timer) app_timer_cancel(s_radar_timer);
  if (s_update_timer) app_timer_cancel(s_update_timer);
  bitmap_layer_destroy(s_bitmap_layer_splash_icon);
  gbitmap_destroy(s_bitmap_splash_icon);
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_snippet_layer);
  text_layer_destroy(s_status_layer);
  layer_destroy(s_sidebar_layer);
  layer_destroy(s_left_sidebar_layer);
  layer_destroy(s_radar_layer);
  layer_destroy(s_splash_bg_layer);
  scroll_layer_destroy(s_scroll_layer);
}

static void prv_init(void) {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers){ .load = prv_main_window_load, .unload = prv_main_window_unload });
  window_stack_push(s_main_window, true);
  s_battery_state = battery_state_service_peek();
  battery_state_service_subscribe(battery_handler);
  compass_service_subscribe(compass_handler);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  app_message_register_inbox_received(inbox_handler);
  app_message_open(4096, 512);
}

static void prv_deinit(void) {
  battery_state_service_unsubscribe();
  compass_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
  return 0;
}