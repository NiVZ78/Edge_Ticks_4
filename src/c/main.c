#include <pebble.h>
#include "dithered_rects.h"	

#define MAJOR_TICK_LENGTH   20
#define MAJOR_TICK_WIDTH    3
#define MINOR_TICK_LENGTH   10
#define MINOR_TICK_WIDTH    1

// pointer to main window
static Window *s_main_window;

// pointer to main window layer
static Layer *s_main_window_layer;

// pointer to tick mark layer
static Layer *s_tick_mark_layer;

// Array to hold info for tick marks
static int dial_points[60][2];
static int radius = 0;
static GRect circle_bounds;



// courtesy of @robisodd
int32_t abs32(int32_t a) {return (a^(a>>31)) - (a>>31);}     // returns absolute value of A (only works on 32bit signed)

// courtesy of @robisodd
GPoint getPointOnRect(GRect r, int angle) {
  int32_t sin = sin_lookup(angle), cos = cos_lookup(angle);  // Calculate once and store, to make quicker and cleaner
  int32_t dy = sin>0 ? (r.size.h/2) : (0-r.size.h)/2;        // Distance to top or bottom edge (from center)
  int32_t dx = cos>0 ? (r.size.w/2) : (0-r.size.w)/2;        // Distance to left or right edge (from center)
  if(abs32(dx*sin) < abs32(dy*cos)) {                        // if (distance to vertical line) < (distance to horizontal line)
    dy = (dx * sin) / cos;                                   // calculate distance to vertical line
  } else {                                                   // else: (distance to top or bottom edge) < (distance to left or right edge)
    dx = (dy * cos) / sin;                                   // move to top or bottom line
  }
  return GPoint(dx+r.origin.x+(r.size.w/2), dy+r.origin.y+(r.size.h/2));  // Return point on rectangle
}


static void update_points(){

  // DO ALL THE INTENSIVE CALCULATIONS IN HERE
  
  // Get the bounds of the layer we are using
  GRect tick_layer_bounds = layer_get_unobstructed_bounds(s_tick_mark_layer);
  
  // Calculate the radius of the circle we need to draw
  // For RECT this is the centre of the layer to the corner
  // For ROUND it is width divided by 2
  radius = PBL_IF_RECT_ELSE((TRIG_MAX_ANGLE * tick_layer_bounds.size.h/2) / sin_lookup(atan2_lookup(tick_layer_bounds.size.h/2, tick_layer_bounds.size.w/2)), tick_layer_bounds.size.w/2);

  // Calculate the rectangular bounds for our circle
  // For both this is the center minus the radius
  circle_bounds = GRect(((tick_layer_bounds.size.w/2)-radius)+tick_layer_bounds.origin.x,
                                ((tick_layer_bounds.size.h/2)-radius)+tick_layer_bounds.origin.y,
                                radius*2,
                                radius*2);
  
  // Minutes
  for (int i=0; i<60; i++){        
    if (i%5){
      dial_points[i][0] = DEG_TO_TRIGANGLE((i*6)-MINOR_TICK_WIDTH);  // Start Angle
      dial_points[i][1] = DEG_TO_TRIGANGLE((i*6)+MINOR_TICK_WIDTH);  // End Angle
    }
  }
  
  
  // 5 MINUTES
  for (int i=0; i<12; i++){        
    if (i%3){
      dial_points[i*5][0] = DEG_TO_TRIGANGLE((i*30)-MINOR_TICK_WIDTH);  // Start Angle
      dial_points[i*5][1] = DEG_TO_TRIGANGLE((i*30)+MINOR_TICK_WIDTH);  // End Angle
    }
  }
  
  // Hours 12, 3, 6, 9
  // We will store the X, Y co-ord instead of angles
  for (int i=0; i<4; i++){
    
    switch(i){
      case 0:
        // Hour 12
        dial_points[i*15][0] = (tick_layer_bounds.size.w/2) + tick_layer_bounds.origin.x;  // X co-ord
        dial_points[i*15][1] = tick_layer_bounds.origin.y;                                 // Y co-ord  
        break;
      
      case 1:
        // Hour 3
        dial_points[i*15][0] = tick_layer_bounds.size.w - MAJOR_TICK_LENGTH + tick_layer_bounds.origin.x;  // X co-ord
        dial_points[i*15][1] = (tick_layer_bounds.size.h/2) + tick_layer_bounds.origin.x;                  // Y co-ord
        break;
      
      case 2:
        // Hour 6
        dial_points[i*15][0] = tick_layer_bounds.size.w/2;                                                 // X co-ord
        dial_points[i*15][1] = tick_layer_bounds.size.h - MAJOR_TICK_LENGTH + tick_layer_bounds.origin.y;  // Y co-ord
        break;
      
      case 3:
        // Hour 9
        dial_points[i*15][0] = tick_layer_bounds.origin.x;                                // X co-ord
        dial_points[i*15][1] = (tick_layer_bounds.size.h/2) + tick_layer_bounds.origin.x; // Y co-ord
        break;
    }
  }
}


static void tick_mark_update_proc(Layer *this_layer, GContext *ctx) {

  // Set the colours
  GColor background_colour = GColorBlack;
  GColor foreground_colour = GColorWhite;

  // Get the bounds of the layer we are using
  GRect tick_layer_bounds = layer_get_unobstructed_bounds(this_layer);
  
  // Fill the screen with the border color using fill_rect as it works on Round and Rect
  draw_dithered_rect(ctx, tick_layer_bounds, background_colour, foreground_colour, DITHER_50_PERCENT);
  
  
  // MINUTES
  for (int i=0; i<60; i++){        
    if (i%5){
      graphics_context_set_fill_color(ctx, foreground_colour);
      graphics_fill_radial(ctx, circle_bounds, GOvalScaleModeFitCircle, PBL_IF_RECT_ELSE(radius, MINOR_TICK_LENGTH), dial_points[i][0], dial_points[i][1]);
    }
  }

  
  #ifdef PBL_RECT
    // Draw another rect in the middle using border color to hide the inner parts of the minute ticks
    draw_dithered_rect(ctx, grect_inset(tick_layer_bounds, GEdgeInsets(MINOR_TICK_LENGTH)), background_colour, foreground_colour, DITHER_50_PERCENT);
  #endif
  
  
  // 5 MINUTES
  for (int i=0; i<12; i++){        
    if (i%3){
      graphics_context_set_fill_color(ctx, foreground_colour);
      graphics_fill_radial(ctx, circle_bounds, GOvalScaleModeFitCircle, PBL_IF_RECT_ELSE(radius, MAJOR_TICK_LENGTH), dial_points[i*5][0], dial_points[i*5][1]);      
    }
  }

  
  GPoint rect_point = GPoint(0,0);  // GPoint to store top left co-ord of rectangle
  int orientation = 0;  // 0 for Vertical, 1 hor Horizontal
  
  // Hours 12, 3, 6, 9
  for (int i=0; i<4; i++){
        
    // Draw the double ticks using filled rectangles
    // Check if number is odd or even to determine horizontal or vertical orientation
    if (i%2){
      // HORIZONTAL
      graphics_fill_rect(ctx, GRect(dial_points[i*15][0], dial_points[i*15][1]-MAJOR_TICK_WIDTH-1, MAJOR_TICK_LENGTH, MAJOR_TICK_WIDTH), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(dial_points[i*15][0], dial_points[i*15][1]+1, MAJOR_TICK_LENGTH, MAJOR_TICK_WIDTH), 0, GCornerNone);
    }
    else{
      // VERTICAL
      graphics_fill_rect(ctx, GRect(dial_points[i*15][0]-MAJOR_TICK_WIDTH-1, dial_points[i*15][1], MAJOR_TICK_WIDTH, MAJOR_TICK_LENGTH), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(dial_points[i*15][0]+1, dial_points[i*15][1], MAJOR_TICK_WIDTH, MAJOR_TICK_LENGTH), 0, GCornerNone);
    }
    
    
  }
  
  
  // Fill the center with the background color
  graphics_context_set_fill_color(ctx, background_colour);
  #ifdef PBL_RECT
    graphics_fill_rect(ctx, grect_inset(tick_layer_bounds, GEdgeInsets(MAJOR_TICK_LENGTH)), 0, GCornerNone);
  #else
    graphics_fill_radial(ctx, grect_inset(tick_layer_bounds, GEdgeInsets(MAJOR_TICK_LENGTH)), GOvalScaleModeFitCircle, radius-MAJOR_TICK_LENGTH, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
  #endif
    
}


void prv_unobstructed_change(AnimationProgress progress, void *context) {

  // re-calculate the tick mark points
  update_points();
  
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {

  if (units_changed & SECOND_UNIT) 
  {
    layer_mark_dirty(s_main_window_layer);
  }
  
}




static void main_window_load(Window *window) {
  
  // get the main window layer
  s_main_window_layer = window_get_root_layer(s_main_window);
  
  // Get the boundaries of the main layer
  GRect s_main_window_bounds = layer_get_bounds(s_main_window_layer);
  
  // Create the layer we will draw on
  s_tick_mark_layer = layer_create(s_main_window_bounds);
  // Set the update procedure for our layer
  layer_set_update_proc(s_tick_mark_layer, tick_mark_update_proc);

  // Add the layer to our main window layer
  layer_add_child(s_main_window_layer, s_tick_mark_layer);
 
  // Calculate the tick mark points
  update_points();
  
  #if PBL_API_EXISTS(unobstructed_area_service_subscribe)
      UnobstructedAreaHandlers handlers = {
    .change = prv_unobstructed_change
  };
  unobstructed_area_service_subscribe(handlers, NULL);
  #endif
  
  // Subscribe to the seconds tick handler
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
}


static void main_window_unload(Window *window) {
      
}


static void init(void) {
    
  // Create the main window
  s_main_window = window_create();
  
  // set the background colour
  window_set_background_color(s_main_window, GColorBlack);
  
  // set the window load and unload handlers
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  // show the window on screen
  window_stack_push(s_main_window, true);
  
}


static void deinit(void) {
  
  // Destroy the main window
  window_destroy(s_main_window);
  
}


int main(void) {
  
  init();
  app_event_loop();
  deinit();
  
}