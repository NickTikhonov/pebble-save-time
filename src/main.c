#include <pebble.h>

#define KEY_PRODUCTIVITY 0
#define KEY_TOTAL_HOURS 1

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_rescuetime_layer;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char score_buffer[20];
  static char total_time_buffer[20];
  static char rescuetime_buffer[64];

  // Read tuples for data
  Tuple *score_tuple = dict_find(iterator, KEY_PRODUCTIVITY);
  Tuple *total_time_tuple = dict_find(iterator, KEY_TOTAL_HOURS);

  // If all data is available, use it
  if(score_tuple && total_time_tuple) {
    snprintf(score_buffer, sizeof(score_buffer), "%s", score_tuple->value->cstring);
    snprintf(total_time_buffer, sizeof(total_time_buffer), "%s", total_time_tuple->value->cstring);

    // Assemble full string and display
    snprintf(rescuetime_buffer, sizeof(rescuetime_buffer), "%s\n%s", score_buffer, total_time_buffer);
    text_layer_set_text(s_rescuetime_layer, rescuetime_buffer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();

  // Get update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Creat Clock Text Layer
  s_time_layer = text_layer_create(GRect(0, 15, bounds.size.w, bounds.size.h-100));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Create Rescuetime Layer
  s_rescuetime_layer = text_layer_create((GRect) {
    .origin = {0, 60},
    .size = { bounds.size.w, 100}
  });
  text_layer_set_background_color(s_rescuetime_layer, GColorClear);
  text_layer_set_text_color(s_rescuetime_layer, GColorWhite);
  text_layer_set_text_alignment(s_rescuetime_layer, GTextAlignmentCenter);
  text_layer_set_text(s_rescuetime_layer, "--%\n-hrs");
  text_layer_set_font(s_rescuetime_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  layer_add_child(window_layer, text_layer_get_layer(s_rescuetime_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_rescuetime_layer);
}


static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set the background color
  window_set_background_color(s_main_window, GColorBlack);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
