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

static Window *s_main_window, *s_splash_window;
static ScrollLayer *s_scroll_layer;
static TextLayer *s_title_layer, *s_snippet_layer, *s_status_layer;
static Layer *s_sidebar_layer;
static BitmapLayer *s_bitmap_layer_splash;
static GBitmap *s_bitmap_splash;

static char s_title_buf[64] = "WIKIRADIUS";
static char s_snippet_buf[1024] = "";
static char s_status_buf[32] = "System Start...";
static int s_dist = 0, s_bear = 0, s_head = 0;

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
  if(title_t && snippet_t) {
    strncpy(s_title_buf, title_t->value->cstring, sizeof(s_title_buf));
    strncpy(s_snippet_buf, snippet_t->value->cstring, sizeof(s_snippet_buf));
    s_dist = dict_find(iter, MESSAGE_KEY_DISTANCE)->value->int32;
    s_bear = dict_find(iter, MESSAGE_KEY_BEARING)->value->int32;

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

static void prv_main_window_load(Window *window) {
  Layer *w_layer = window_get_root_layer(window);
  GRect b = layer_get_bounds(w_layer);
  window_set_background_color(window, GColorBlack);

  s_scroll_layer = scroll_layer_create(GRect(0, 0, b.size.w - SIDEBAR_W, b.size.h));
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);
  layer_add_child(w_layer, scroll_layer_get_layer(s_scroll_layer));

  s_title_layer = text_layer_create(GRect(4, 5, b.size.w - SIDEBAR_W - 8, 40));
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_T));
  text_layer_set_text_color(s_title_layer, GColorMagenta);
  text_layer_set_background_color(s_title_layer, GColorClear);
  text_layer_set_text(s_title_layer, s_title_buf);
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_title_layer));

  s_snippet_layer = text_layer_create(GRect(4, 45, b.size.w - SIDEBAR_W - 8, 2000));
  text_layer_set_font(s_snippet_layer, fonts_get_system_font(FONT_S));
  text_layer_set_text_color(s_snippet_layer, GColorWhite);
  text_layer_set_background_color(s_snippet_layer, GColorClear);
  text_layer_set_text(s_snippet_layer, s_snippet_buf);
  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_snippet_layer));

  s_status_layer = text_layer_create(GRect(4, 50, b.size.w - SIDEBAR_W - 8, 40));
  text_layer_set_font(s_status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_color(s_status_layer, GColorCyan);
  text_layer_set_text(s_status_layer, s_status_buf);
  layer_add_child(w_layer, text_layer_get_layer(s_status_layer));

  s_sidebar_layer = layer_create(GRect(b.size.w - SIDEBAR_W, 0, SIDEBAR_W, b.size.h));
  layer_set_update_proc(s_sidebar_layer, sidebar_update_proc);
  layer_add_child(w_layer, s_sidebar_layer);

  layer_set_hidden(scroll_layer_get_layer(s_scroll_layer), true);
}

static void splash_timer_callback(void *data) {
  window_stack_push(s_main_window, true);
  window_stack_remove(s_splash_window, false);
}

static void prv_splash_window_load(Window *window) {
  Layer *w_layer = window_get_root_layer(window);
  GRect b = layer_get_bounds(w_layer);
  window_set_background_color(window, GColorCyan);

  s_bitmap_splash = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SPLASH);
  s_bitmap_layer_splash = bitmap_layer_create(GRect(b.size.w/2 - 27, b.size.h/2 - 27, 55, 55));
  bitmap_layer_set_bitmap(s_bitmap_layer_splash, s_bitmap_splash);
  bitmap_layer_set_compositing_mode(s_bitmap_layer_splash, GCompOpSet);
  layer_add_child(w_layer, bitmap_layer_get_layer(s_bitmap_layer_splash));

  app_timer_register(2000, splash_timer_callback, NULL);
}

static void prv_splash_window_unload(Window *window) {
  bitmap_layer_destroy(s_bitmap_layer_splash);
  gbitmap_destroy(s_bitmap_splash);
}

static void prv_update_app_glance(AppGlanceReloadSession *session, size_t limit, void *context) {
  if (limit < 1) return;
  AppGlanceSlice slice = (AppGlanceSlice) {
    .layout = {
      .icon = RESOURCE_ID_IMAGE_MENU_ICON
    },
    .expiration_time = APP_GLANCE_SLICE_NO_EXPIRATION
  };
  app_glance_add_slice(session, slice);
}

static void prv_init() {
  s_splash_window = window_create();
  window_set_window_handlers(s_splash_window, (WindowHandlers) {
    .load = prv_splash_window_load,
    .unload = prv_splash_window_unload
  });
  
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) { .load = prv_main_window_load });
  
  window_stack_push(s_splash_window, true);

  app_glance_reload(prv_update_app_glance, NULL);

  app_message_register_inbox_received(inbox_handler);
  app_message_open(1024, 128);
  compass_service_subscribe(compass_handler);
  
  app_timer_register(2500, request_data_from_js, NULL);
}

int main() {
  prv_init();
  app_event_loop();
}