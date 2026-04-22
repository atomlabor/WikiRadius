#include <pebble.h>

// Explizite Definition der MessageKeys
#define MESSAGE_KEY_READY 0
#define MESSAGE_KEY_TITLE 1
#define MESSAGE_KEY_SNIPPET 2
#define MESSAGE_KEY_BEARING 3
#define MESSAGE_KEY_DISTANCE 4

// WikiRadius v2.0.0
#define SIDEBAR_WIDTH 18
#define ARRIVAL_THRESHOLD 50

// Persistenz-Schlüssel für Offline-Modus
#define PERSIST_TITLE 10
#define PERSIST_SNIPPET 11
#define PERSIST_DIST 12
#define PERSIST_BEARING 13

static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_title_layer;
static TextLayer *s_snippet_layer;

// Daten-State
static char s_title_buffer[32] = "Searching...";
static char s_snippet_buffer[160] = "Waiting for GPS and Wikipedia connection...";
static int s_distance = 0;
static int s_bearing = 0;
static int s_filtered_heading = 0;
static bool s_arrival_vibe_fired = false;

// ---------------------------------------------------------------------------
// HYBRID NAVIGATION & DRAWING
// ---------------------------------------------------------------------------
static void update_navigation() {
  if (s_distance > 0 && s_distance <= ARRIVAL_THRESHOLD && !s_arrival_vibe_fired) {
    vibes_double_pulse(); 
    s_arrival_vibe_fired = true;
  } else if (s_distance > ARRIVAL_THRESHOLD) {
    s_arrival_vibe_fired = false;
  }
  layer_mark_dirty(s_canvas_layer);
}

static void compass_handler(CompassHeadingData heading_data) {
  if (heading_data.compass_status == CompassStatusDataInvalid) return;
  s_filtered_heading = (heading_data.true_heading * 0.1) + (s_filtered_heading * 0.9);
  update_navigation();
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // 30% Sidebar in CadetBlue
  graphics_context_set_fill_color(ctx, GColorCadetBlue);
  graphics_fill_rect(ctx, GRect(bounds.size.w - SIDEBAR_WIDTH, 0, SIDEBAR_WIDTH, bounds.size.h), 0, GCornerNone);
  
  if (s_distance > 0) {
    // 10% Accent Color: Richtungspfeil in SunsetOrange
    int arrow_angle = s_bearing - s_filtered_heading;
    
    // Responsive Y-Achse für Basalt (168px) und Emery (228px)
    int center_y_arrow = bounds.size.h / 4;
    int center_y_text = bounds.size.h / 2;
    
    GPoint center = GPoint(bounds.size.w - (SIDEBAR_WIDTH / 2), center_y_arrow);
    
    graphics_context_set_stroke_color(ctx, GColorSunsetOrange);
    graphics_context_set_stroke_width(ctx, 3);
    
    GPoint end_pt = gpoint_from_polar(GRect(center.x - 7, center.y - 7, 14, 14), GOvalScaleModeFitCircle, DEG_TO_TRIGANGLE(arrow_angle));
    graphics_draw_line(ctx, center, end_pt);
    
    // Distanz zentriert unter dem Pfeil
    static char dist_str[8];
    snprintf(dist_str, sizeof(dist_str), "%dm", s_distance);
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, dist_str, fonts_get_system_font(FONT_KEY_GOTHIC_14), 
                       GRect(bounds.size.w - SIDEBAR_WIDTH - 2, center_y_text, SIDEBAR_WIDTH + 4, 20), 
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  }
}

// ---------------------------------------------------------------------------
// APPMESSAGE (JS -> C)
// ---------------------------------------------------------------------------
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *title_t = dict_find(iterator, MESSAGE_KEY_TITLE);
  Tuple *snippet_t = dict_find(iterator, MESSAGE_KEY_SNIPPET);
  Tuple *dist_t = dict_find(iterator, MESSAGE_KEY_DISTANCE);
  Tuple *bearing_t = dict_find(iterator, MESSAGE_KEY_BEARING);
  
  if(title_t && snippet_t && dist_t && bearing_t) {
    snprintf(s_title_buffer, sizeof(s_title_buffer), "%s", title_t->value->cstring);
    snprintf(s_snippet_buffer, sizeof(s_snippet_buffer), "%s", snippet_t->value->cstring);
    s_distance = dist_t->value->int32;
    s_bearing = bearing_t->value->int32;
    
    text_layer_set_text(s_title_layer, s_title_buffer);
    text_layer_set_text(s_snippet_layer, s_snippet_buffer);
    
    persist_write_string(PERSIST_TITLE, s_title_buffer);
    persist_write_string(PERSIST_SNIPPET, s_snippet_buffer);
    persist_write_int(PERSIST_DIST, s_distance);
    persist_write_int(PERSIST_BEARING, s_bearing);
    
    update_navigation();
  }
}

// ---------------------------------------------------------------------------
// WINDOW MANAGEMENT
// ---------------------------------------------------------------------------
static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // 60% Wes Anderson Base (PastelYellow)
  window_set_background_color(window, GColorPastelYellow); 

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
  
  // Responsive Text Layers
  s_title_layer = text_layer_create(GRect(4, 2, bounds.size.w - SIDEBAR_WIDTH - 8, 28));
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentLeft);
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_background_color(s_title_layer, GColorClear);
  text_layer_set_text_color(s_title_layer, GColorBlack);
  text_layer_set_text(s_title_layer, s_title_buffer);
  layer_add_child(window_layer, text_layer_get_layer(s_title_layer));
  
  s_snippet_layer = text_layer_create(GRect(4, 30, bounds.size.w - SIDEBAR_WIDTH - 8, bounds.size.h - 32));
  text_layer_set_text_alignment(s_snippet_layer, GTextAlignmentLeft);
  text_layer_set_font(s_snippet_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(s_snippet_layer, GColorClear);
  text_layer_set_text_color(s_snippet_layer, GColorBlack);
  text_layer_set_text(s_snippet_layer, s_snippet_buffer);
  layer_add_child(window_layer, text_layer_get_layer(s_snippet_layer));
  
  if (persist_exists(PERSIST_TITLE)) {
    persist_read_string(PERSIST_TITLE, s_title_buffer, sizeof(s_title_buffer));
    persist_read_string(PERSIST_SNIPPET, s_snippet_buffer, sizeof(s_snippet_buffer));
    s_distance = persist_read_int(PERSIST_DIST);
    s_bearing = persist_read_int(PERSIST_BEARING);
    text_layer_set_text(s_title_layer, s_title_buffer);
    text_layer_set_text(s_snippet_layer, s_snippet_buffer);
  }
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_title_layer);
  text_layer_destroy(s_snippet_layer);
  layer_destroy(s_canvas_layer);
}

static void prv_init(void) {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  window_stack_push(s_main_window, true);
  
  app_message_register_inbox_received(inbox_received_callback);
  app_message_open(256, 256);
  
  compass_service_set_heading_filter(DEG_TO_TRIGANGLE(2));
  compass_service_subscribe(compass_handler);
}

static void prv_deinit(void) {
  compass_service_unsubscribe();
  app_message_deregister_callbacks();
  window_destroy(s_main_window);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}