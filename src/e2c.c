#include "pebble.h"
#include "pin_window.h"

static Window *s_main_window;

static TextLayer *s_course_layer;
static TextLayer *s_temperature_layer;
static TextLayer *s_city_layer;
static TextLayer *s_title_layer;

static AppSync s_sync;
static uint8_t s_sync_buffer[64];

PinWindow *easting_window = NULL;
PinWindow *northing_window = NULL;
int32_t easting = 0;
int32_t northing = 0;

enum WeatherKey 
{
  RANGE_KEY = 0x0,    // TUPLE_CSTRING
  BEARING_KEY = 0x1,  // TUPLE_CSTRING
  EASTING_KEY = 0x2,
  NORTHING_KEY = 0x3,
  LOADSAVE_KEY = 0x4,
  COURSE_KEY = 0x5
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) 
{
  switch (key) 
  {
    case RANGE_KEY:
      // App Sync keeps new_tuple in s_sync_buffer, so we may use it directly
      text_layer_set_text(s_temperature_layer, new_tuple->value->cstring);
      break;

    case BEARING_KEY:
      text_layer_set_text(s_city_layer, new_tuple->value->cstring);
      break;

    case COURSE_KEY:
      text_layer_set_text(s_course_layer, new_tuple->value->cstring);
      break;
  }
}

static void send_UTM( int32_t easting, int32_t northing ) 
{
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (!iter) 
  {
    // Error creating outbound message
    return;
  }

  int32_t value = easting;
  dict_write_int(iter, EASTING_KEY, &value, sizeof(int32_t), true);
  
  value = northing;
  dict_write_int(iter, NORTHING_KEY, &value, sizeof(int32_t), true);
  
  dict_write_end(iter);

  app_message_outbox_send();
}

static void send_LoadSave( int32_t isLoad ) 
{
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (!iter) 
  {
    // Error creating outbound message
    return;
  }

  dict_write_int(iter, LOADSAVE_KEY, &isLoad, sizeof(int32_t), true);
  
  dict_write_end(iter);

  app_message_outbox_send();
}

static void window_load(Window *window) 
{
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_course_layer = text_layer_create(GRect(0, 45, bounds.size.w, 32));
  text_layer_set_text_color(s_course_layer, GColorWhite);
  text_layer_set_background_color(s_course_layer, GColorClear);
  text_layer_set_font(s_course_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_course_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_course_layer));

  s_title_layer = text_layer_create(GRect(0, 2, bounds.size.w, 32));
  text_layer_set_text_color(s_title_layer, GColorGreen);
  text_layer_set_background_color(s_title_layer, GColorClear);
  text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_title_layer));
  
  text_layer_set_text(s_title_layer, "E2C");

  s_temperature_layer = text_layer_create(GRect(0, 90, bounds.size.w, 32));
  text_layer_set_text_color(s_temperature_layer, GColorWhite);
  text_layer_set_background_color(s_temperature_layer, GColorClear);
  text_layer_set_font(s_temperature_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_temperature_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_temperature_layer));

  s_city_layer = text_layer_create(GRect(0, 122, bounds.size.w, 32));
  text_layer_set_text_color(s_city_layer, GColorWhite);
  text_layer_set_background_color(s_city_layer, GColorClear);
  text_layer_set_font(s_city_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(s_city_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_city_layer));

  Tuplet initial_values[] = 
  {
    TupletCString(RANGE_KEY, "0"),
    TupletCString(BEARING_KEY, "0"),
    TupletCString(COURSE_KEY, "0")
  };

  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);
}

static void window_unload(Window *window) 
{
  text_layer_destroy(s_city_layer);
  text_layer_destroy(s_temperature_layer);
  text_layer_destroy(s_course_layer);
  text_layer_destroy(s_title_layer);
}

static void pin_complete_callback(PIN pin, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_INFO, "%c was %d %d %d %d %d", pin.type, pin.digits[0], pin.digits[1], pin.digits[2], pin.digits[3], pin.digits[4]);
  
  if( pin.type == 'E' )
  {
    if( pin.digits[0]+pin.digits[1]+pin.digits[2]+pin.digits[3]+pin.digits[4] >0 )
      easting = 4 * 100000 + pin.digits[0] * 10000 + pin.digits[1] * 1000 + pin.digits[2] * 100 + pin.digits[3] * 10 + pin.digits[4];
    else
      easting = 0;
    //easting = 2 * 100000 + pin.digits[0] * 10000 + pin.digits[1] * 1000 + pin.digits[2] * 100 + pin.digits[3] * 10 + pin.digits[4];
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Easting: %d", (int)easting);
  }
  else if( pin.type == 'N' )
  {
    if( pin.digits[0]+pin.digits[1]+pin.digits[2]+pin.digits[3]+pin.digits[4] >0 )
      northing = 49 * 100000 + pin.digits[0] * 10000 + pin.digits[1] * 1000 + pin.digits[2] * 100 + pin.digits[3] * 10 + pin.digits[4];
    else
      northing = 0;
    //northing = 43 * 100000 + pin.digits[0] * 10000 + pin.digits[1] * 1000 + pin.digits[2] * 100 + pin.digits[3] * 10 + pin.digits[4];
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Northing: %d", (int)northing);
    
    send_UTM( easting, northing );
  }
  
  pin_window_pop((PinWindow*)context, true);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) 
{
  // A single select click has just occured
  APP_LOG(APP_LOG_LEVEL_DEBUG, "select click");
    
  if( !easting_window )
  {
    char *s = "Easting";
    
    easting_window = pin_window_create((PinWindowCallbacks)
    {
      .pin_complete = pin_complete_callback
    }, s);
  }
  
  if( !northing_window )
  {
    char *s = "Northing";
    
    northing_window = pin_window_create((PinWindowCallbacks)
    {
      .pin_complete = pin_complete_callback
    }, s);
  }
  
  pin_window_push(northing_window, true);
  pin_window_push(easting_window, true);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "up click");
  
  send_LoadSave( 0 );
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) 
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "down click");
  
  send_LoadSave( 1 );
}

static void click_config_provider(void *context) 
{
  ButtonId id = BUTTON_ID_SELECT;  // The Select button

  window_single_click_subscribe(id, select_click_handler);
  
  id = BUTTON_ID_UP;  // The Up button

  window_single_click_subscribe(id, up_click_handler);
  
  id = BUTTON_ID_DOWN;  // The Down button

  window_single_click_subscribe(id, down_click_handler);
}

static void init(void) 
{
  s_main_window = window_create();
  
  window_set_click_config_provider(s_main_window, click_config_provider);

  window_set_background_color(s_main_window, PBL_IF_COLOR_ELSE(GColorIndigo, GColorBlack));
  
  window_set_window_handlers(s_main_window, (WindowHandlers) 
  {
    .load = window_load,
    .unload = window_unload
  });
  
  window_stack_push(s_main_window, true);

  app_message_open(64, 64);
}

static void deinit(void) 
{
  if( easting_window )
    pin_window_destroy( easting_window );

  if( northing_window )
    pin_window_destroy( northing_window );

  window_destroy(s_main_window);

  app_sync_deinit(&s_sync);
}

int main(void) 
{
  init();
  app_event_loop();
  deinit();
}
