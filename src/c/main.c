#include <pebble.h>

#define MESSAGE_KEY_READY 0
#define MESSAGE_KEY_STATUS 1
#define MESSAGE_KEY_TITLE 2
#define MESSAGE_KEY_SNIPPET 3
#define MESSAGE_KEY_BEARING 4
#define MESSAGE_KEY_DISTANCE 5

#if PBL_PLATFORM_EMERY
  #define SIDEBAR_W 24
  #define FONT_T FONT_KEY_GOTHIC_28_BOLD
  #define FONT_S FONT_KEY_GOTHIC_24
#else
  #define SIDEBAR_W 18
  #define FONT_T FONT_KEY_GOTHIC_24_BOLD
  #define FONT_S FONT_KEY_GOTHIC_18
#endif

#define LOGO_WIDTH 120
#define LOGO_HEIGHT 120

static Window *s_main_window;
static ScrollLayer *s_scroll_layer;
static TextLayer *s_title_layer, *s_snippet_layer, *s_status_layer;
static Layer *s_sidebar_layer, *s_left_sidebar_layer;
static BitmapLayer *s_bitmap_layer_splash;
static GBitmap *s_bitmap_splash;

static char s_title_buf[64] = "WIKIRADIUS";
static char s_snippet_buf[1024] = "";
static char s_status_buf[32] = "System Start...";
static int s_dist = 0, s_bear = 0, s_head = 0;
static BatteryChargeState s_battery_state;

static void compass_handler(CompassHeadingData data) {
  if (data.compass_status == CompassStatusDataInvalid) return;
  int32_t target = (data.true_heading * 360) / TRIG_MAX_ANGLE;
  int32_t diff = target - s_head;
  if (diff > 180) diff -= 360;
  if (diff < -180) diff += 360;
  s_head = (s_head + (diff / 10)) % 360;
  if (s_head < 0) s_head += 360;
  layer_mark_dirty(s_sidebar_layer);
}

static void battery_handler(BatteryChargeState state) {
  s_battery_state = state;
  layer_mark_dirty(s_left_sidebar_layer);
}

static void left_sidebar_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorCyan);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  static char s_raw_time[6];
  static char s_vert_time[12];
  strftime(s_raw_time, sizeof(s_raw_time), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  int v_idx = 0;
  for(int i = 0; i < (int)strlen(s_raw_time); i++) {
    s_vert_time[v_idx++] = s_raw_time[i];
    s_vert_time[v_idx++] = '\n';
  }
  s_vert_time[v_idx] = '\0';
  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, s_vert_time, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0, 5, b.size.w, 100), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);

  int num_blocks = s_battery_state.charge_percent / 20;
  if (num_blocks == 0 && s_battery_state.charge_percent > 0) num_blocks = 1;
  int block_h = 6;
  int block_w = b.size.w - 8;
  int start_y = b.size.h - 10;
  
  for(int i = 0; i < num_blocks; i++) {
    GColor b_col = GColorGreen;
    if(s_battery_state.charge_percent <= 20) b_col = GColorRed;
    else if(s_battery_state.charge_percent <= 40) b_col = GColorOrange;
    
    graphics_context_set_fill_color(ctx, b_col);
    graphics_fill_rect(ctx, GRect(4, start_y - (i * (block_h + 2)), block_w, block_h), 0, GCornerNone);
  }
}

static void sidebar_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorCyan);
  graphics_fill_rect(ctx, b, 0, GCornerNone);
  
  if (s_dist > 0) {
    int angle = s_bear - s_head;
    GPoint center = GPoint(b.size.w / 2, b.size.h / 3);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_circle(ctx, center, 11);
    graphics_context_set_stroke_width(ctx, 3);
    GPoint end = gpoint_from_polar(GRect(center.x-8, center.y-8, 16, 16), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(angle));
    graphics_draw_line(ctx, center, end);
    static char d_str[10];
    snprintf(d_str, sizeof(d_str), "%dm", s_dist);
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, d_str, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0, b.size.h/2, b.size.w, 20), 0, GTextAlignmentCenter, NULL);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_left_sidebar_layer);
}

static void request_data_from_js(void *data) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  if(iter) {
    dict_write_uint8(iter, MESSAGE_KEY_READY, 1);
    app_message_outbox_send();
  }
}

static void inbox_handler(DictionaryIterator *iter, void *ctx) {
  Tuple *status_t = dict_find(iter, MESSAGE_KEY_STATUS);
  if(status_t) text_layer_set_text(s_status_layer, status_t->value->cstring);
  Tuple *title_t = dict_find(iter, MESSAGE_KEY_TITLE);
  Tuple *snippet_t = dict_find(iter, MESSAGE_KEY_SNIPPET);
  Tuple *dist_t = dict_find(iter, MESSAGE_KEY_DISTANCE);
  Tuple *bear_t = dict_find(iter, MESSAGE_KEY_BEARING);
  if(title_t && snippet_t && dist_t && bear_t) {
    strncpy(s_title_buf, title_t->value->cstring, sizeof(s_title_buf));
    strncpy(s_snippet_buf, snippet_t->value->cstring, sizeof(s_snippet_buf));
    s_dist = dist_t->value->int32;
    s_bear = bear_t->value->int32;
    text_layer_set_text(s_title_layer, s_title_buf);
    text_layer_set_text(s_snippet_layer, s_snippet_buf);
    GSize content_size = text_layer_get_content_size(s_snippet_layer);
    text_layer_set_size(s_snippet_layer, GSize(layer_get_bounds(text_layer_get_layer(s_snippet_layer)).size.w, content_size.h + 20));
    scroll_layer_set_content_size(s_scroll_layer, GSize(content_size.w, content_size.h + 80));
    layer_set_hidden(text_layer_get_layer(s_status_layer), true);
    layer_set_hidden(scroll_layer_get_layer(s_scroll_layer), false);
    layer_mark_dirty(s_sidebar_layer);
  }
}

static void transition_to_main(void *data) {
  window_set_background_color(s_main_window, GColorBlack);
  layer_set_hidden(bitmap_layer_get_layer(s_bitmap_layer_splash), true);
  layer_set_hidden(text_layer_get_layer(s_status_layer), false);
  layer_set_hidden(s_sidebar_layer, false);
  layer_set_hidden(s_left_sidebar_layer, false);
  request_data_from_js(NULL);
}

static void prv_main_window_load(Window *window) {
  Layer *w_layer = window_get_root_layer(window);
  GRect b = layer_get_bounds(w_layer);
  window_set_background_color(window, GColorCyan);

  s_left_sidebar_layer = layer_create(GRect(0, 0, SIDEBAR_W, b.size.h));
  layer_set_update_proc(s_left_sidebar_layer, left_sidebar_update_proc);
  layer_add_child(w_layer, s_left_sidebar_layer);

  s_scroll_layer = scroll_layer_create(GRect(SIDEBAR_W, 0, b.size.w - (2 * SIDEBAR_W), b.size.h));
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);
  layer_add_child(w_layer, scroll_layer_get_layer(s_scroll_layer));

  s_title_layer = text_layer_create(GRect(4, 5, b.size.w - (2 * SIDEBAR_W) - 8, 40));
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_T));
  text_layer_set_text_color(s_title_layer, GColorMagenta);
  text_layer_set_background_color(s_title_layer, GColorClear);
  text_layer_set_text(s_title_layer, s_title_buf);
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_title_layer));

  s_snippet_layer = text_layer_create(GRect(4, 45, b.size.w - (2 * SIDEBAR_W) - 8, 1000));
  text_layer_set_font(s_snippet_layer, fonts_get_system_font(FONT_S));
  text_layer_set_text_color(s_snippet_layer, GColorWhite);
  text_layer_set_background_color(s_snippet_layer, GColorClear);
  text_layer_set_text(s_snippet_layer, s_snippet_buf);
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_snippet_layer));

  s_status_layer = text_layer_create(GRect(SIDEBAR_W + 4, 50, b.size.w - (2 * SIDEBAR_W) - 8, 40));
  text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_color(s_status_layer, GColorCyan);
  text_layer_set_background_color(s_status_layer, GColorClear);
  text_layer_set_text(s_status_layer, s_status_buf);
  layer_add_child(w_layer, text_layer_get_layer(s_status_layer));

  s_sidebar_layer = layer_create(GRect(b.size.w - SIDEBAR_W, 0, SIDEBAR_W, b.size.h));
  layer_set_update_proc(s_sidebar_layer, sidebar_update_proc);
  layer_add_child(w_layer, s_sidebar_layer);

  layer_set_hidden(scroll_layer_get_layer(s_scroll_layer), true);
  layer_set_hidden(text_layer_get_layer(s_status_layer), true);
  layer_set_hidden(s_sidebar_layer, true);
  layer_set_hidden(s_left_sidebar_layer, true);

  s_bitmap_splash = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SPLASH);
  int center_x = (b.size.w - LOGO_WIDTH) / 2;
  int center_y = (b.size.h - LOGO_HEIGHT) / 2;
  s_bitmap_layer_splash = bitmap_layer_create(GRect(center_x, center_y, LOGO_WIDTH, LOGO_HEIGHT));
  bitmap_layer_set_bitmap(s_bitmap_layer_splash, s_bitmap_splash);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_splash, GCompOpSet);
  layer_add_child(w_layer, bitmap_layer_get_layer(s_bitmap_layer_splash));

  app_timer_register(2000, transition_to_main, NULL);
}

static void prv_main_window_unload(Window *window) {
  bitmap_layer_destroy(s_bitmap_layer_splash);
  gbitmap_destroy(s_bitmap_splash);
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_snippet_layer);
  text_layer_destroy(s_status_layer);
  layer_destroy(s_sidebar_layer);
  layer_destroy(s_left_sidebar_layer);
  scroll_layer_destroy(s_scroll_layer);
}

static void prv_init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) { 
    .load = prv_main_window_load,
    .unload = prv_main_window_unload
  });
  window_stack_push(s_main_window, true);
  s_battery_state = battery_state_service_peek();
  battery_state_service_subscribe(battery_handler);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  app_message_register_inbox_received(inbox_handler);
  app_message_open(1024, 128);
  compass_service_subscribe(compass_handler);
}

static void prv_deinit() {
  compass_service_unsubscribe();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  app_message_deregister_callbacks();
  window_destroy(s_main_window);
}

int main() {
  prv_init();
  app_event_loop();
  prv_deinit();
}
