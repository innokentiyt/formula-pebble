#include <pebble.h>
#include "penger.h"

#if defined(PBL_DISPLAY_WIDTH)
    const int WIDTH = PBL_DISPLAY_WIDTH;
#else
    const int WIDTH = 144;
#endif

#if defined(PBL_DISPLAY_HEIGHT)
    const int HEIGHT = PBL_DISPLAY_HEIGHT;
#else
    const int HEIGHT = 168;
#endif

#define ASPECT ((float)WIDTH / (float)HEIGHT)

const int delay_ms = 33; // roughly 30 FPS
int32_t angles_per_frame = 4 * TRIG_MAX_ANGLE / 360;
const float dz = 1.0f;
const float scale = 1.0f;

int32_t angle = 0;

static Window *s_window;
static Layer *s_layer;
static AppTimer *s_timer;

static GPoint screen(Point2D p)
{
    GPoint gp = GPoint(
        (int)((p.x + 1.0f) / 2 * WIDTH),
        (int)((1.0f - (p.y + 1.0f) / 2) * HEIGHT));
    return gp;
}

static Point2D project(Point3D p)
{
    Point2D p2d = {
        .x = (scale * p.x) / p.z,
        .y = ((scale * p.y) / p.z) * ASPECT};
    return p2d;
}

static Point3D translate_z(Point3D p, float _dz)
{
    Point3D p3d = {
        .x = p.x,
        .y = p.y,
        .z = p.z + _dz};
    return p3d;
}

static Point3D rotate_xz(Point3D p, float c, float s)
{
    Point3D p3d = {
        .x = p.x * c - p.z * s,
        .y = p.y,
        .z = p.x * s + p.z * c};
    return p3d;
}

static void draw_frame(Layer *layer, GContext *ctx)
{
    angle += angles_per_frame;
    if (angle >= TRIG_MAX_ANGLE)
    {
        angle = 0;
    }
    float c = (float)cos_lookup(angle) / TRIG_MAX_RATIO;
    float s = (float)sin_lookup(angle) / TRIG_MAX_RATIO;
    graphics_context_set_stroke_width(ctx, 1);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_antialiased(ctx, false);

    for (int i = 0; i < LINES; i++)
    {
        GPoint start = screen(project(translate_z(rotate_xz(vs[lines[i][0]], c, s), dz)));
        GPoint end = screen(project(translate_z(rotate_xz(vs[lines[i][1]], c, s), dz)));
        graphics_draw_line(ctx, start, end);
    }
}

static void timer_callback(void *context)
{
    layer_mark_dirty(s_layer);
    s_timer = app_timer_register(delay_ms, timer_callback, NULL);
}

static void prv_window_load(Window *window)
{
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    s_layer = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
    layer_add_child(window_layer, s_layer);
    layer_set_update_proc(s_layer, draw_frame);

    // start updating the screen
    s_timer = app_timer_register(delay_ms, timer_callback, NULL);
}

static void prv_window_unload(Window *window)
{
    app_timer_cancel(s_timer);
    layer_destroy(s_layer);
}

static void prv_init(void)
{
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
                                             .load = prv_window_load,
                                             .unload = prv_window_unload,
                                         });
    const bool animated = true;
    window_stack_push(s_window, animated);
}

static void prv_deinit(void)
{
    window_destroy(s_window);
}

int main(void)
{
    prv_init();
    app_event_loop();
    prv_deinit();
}
