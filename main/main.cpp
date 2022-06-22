// based on https://github.com/sukesh-ak/LVGL8-WT32-SC01-IDF
// adapted for MakerFabs parallel TFT dev boards (S2 and S3)

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "esp_log.h"

#define TAG "Demo"

// these do not have to be the same, not sure what is optimal
#define TASK_SLEEP_PERIOD_MS 4
#define LV_TICK_PERIOD_MS 4

#define LGFX_USE_V1

#define LV_DOUBLE_BUFFER
#define LANDSCAPE // if changing this, make sure to uncheck landscape in menuconfig -> components -> lvgl -> demos -> music

#if CONFIG_IDF_TARGET_ESP32S3
  #include "LGFX_MakerFabs_Parallel_S3.hpp"
  // if you get "will not fit, dram segment overflow" reduce this
  #ifdef LANDSCAPE
    #define LV_BUFFER_SIZE 80 /* if not double buffering, then buf will be 2x this */
  #else
    #define LV_BUFFER_SIZE 120 /* if not double buffering, then buf will be 2x this */
  #endif
#elif CONFIG_IDF_TARGET_ESP32S2
  #include "LGFX_MakerFabs_Parallel_S2.hpp"
  // if you get "will not fit, dram segment overflow" reduce this
  #ifdef LANDSCAPE
    #define LV_BUFFER_SIZE 40 /* if not double buffering, then buf will be 2x this */
  #else
    #define LV_BUFFER_SIZE 60 /* if not double buffering, then buf will be 2x this */
  #endif
#else
#error I don't know which board you're talking to! . ./set-target esp32s2 or esp32s3
#endif

// Uncomment to test benchmark speed without display refresh. You won't see any output on screen, look in the log window to see results
//#define DISABLE_FLUSH_DURING_BENCHMARK

#if defined(DISABLE_FLUSH_DURING_BENCHMARK) && !CONFIG_LV_USE_LOG
#error You'll need to enable LVGL logging (and probably set log to printf) in the menuconfig to get results.
#endif

static LGFX lcd;

#include <lvgl.h>
#include <demos/lv_demos.h>

/*** Setup screen resolution for LVGL ***/
#ifdef LANDSCAPE
static const uint16_t screenWidth = 480;
static const uint16_t screenHeight = 320;
#else
static const uint16_t screenWidth = 320;
static const uint16_t screenHeight = 480;
#endif
static lv_disp_draw_buf_t draw_buf;

#ifdef LV_DOUBLE_BUFFER
static lv_color_t buf[screenWidth * LV_BUFFER_SIZE];
static lv_color_t buf2[screenWidth * LV_BUFFER_SIZE];
#else
static lv_color_t buf[screenWidth * LV_BUFFER_SIZE * 2];
#endif

typedef void (*function_pointer_t)(void);

typedef struct demo_button
{
    const char * const name;
    function_pointer_t function;
} demo_button_t;

/* List of buttons to create, and the associated demo function that needs to be called when clicked */
static demo_button_t demos[] = {
    {"Music", lv_demo_music},
    {"Widgets", lv_demo_widgets},
    {"Encoder", lv_demo_keypad_encoder},
    {"Benchmark", lv_demo_benchmark},
    {"Stress", lv_demo_stress}
};


static lv_obj_t *demo_selection_panel;

static bool disable_flush = false;

/*** Function declaration ***/
static void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
static void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
static void lv_tick_task(void *arg);

/* Button event handler */
static void button_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED)
    {
        function_pointer_t demo_function = (function_pointer_t)lv_event_get_user_data(e);
        if (demo_function)
        {
            lv_obj_t *label = lv_obj_get_child(btn, 0);
            ESP_LOGI(TAG, "Starting %s", lv_label_get_text(label));

#ifdef DISABLE_FLUSH_DURING_BENCHMARK
            if (demo_function == lv_demo_benchmark)
            {
                ESP_LOGI(TAG, "Starting benchmark with flush disabled. Wait a couple minutes for benchmark results. They'll be here soon.");
                disable_flush = true;
            }
#endif
            demo_function();
            lv_obj_del(demo_selection_panel);
        }
    }
}

static void init_lvgl_lgfx()
{
    lcd.init();
    lv_init();

#ifdef LANDSCAPE
    // Rotate to landscape
    lcd.setRotation(1);
#endif

    //lcd.setBrightness(10);

#ifdef LV_DOUBLE_BUFFER
    lv_disp_draw_buf_init(&draw_buf, buf, buf2, screenWidth * LV_BUFFER_SIZE);
#else
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * LV_BUFFER_SIZE * 2);
#endif

    /*** LVGL : Setup & Initialize the display device driver ***/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = display_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*** LVGL : Setup & Initialize the input device driver ***/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));
}

extern "C" void app_main(void)
{
    init_lvgl_lgfx();

    ESP_LOGI(TAG, "Ready to start a demo. Tap a button on screen. Reset the board with the reset button or Ctrl+T Ctrl+R to pick a new one.");

    // Create buttons to pick which demo
    demo_selection_panel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(demo_selection_panel, lv_pct(100), 120);
    lv_obj_set_scroll_snap_x(demo_selection_panel, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_flex_flow(demo_selection_panel, LV_FLEX_FLOW_ROW);
    lv_obj_align(demo_selection_panel, LV_ALIGN_CENTER, 0, 20);

    for(auto& demo : demos)
    {
        lv_obj_t * btn = lv_btn_create(demo_selection_panel);
        lv_obj_set_size(btn, 120, lv_pct(100));

        lv_obj_add_event_cb(btn, button_event_handler, LV_EVENT_CLICKED, (void*)demo.function);

        lv_obj_t * label = lv_label_create(btn);
        lv_label_set_text_static(label, demo.name);

        lv_obj_center(label);
    }
    lv_obj_update_snap(demo_selection_panel, LV_ANIM_ON);

    /* UI thread */
    while (true)
    {
        lv_timer_handler(); /* let the GUI do its work */
        vTaskDelay(pdMS_TO_TICKS(TASK_SLEEP_PERIOD_MS));
    }
}

/*** Display callback to flush the buffer to screen ***/
static void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
#ifdef DISABLE_FLUSH_DURING_BENCHMARK
    if (disable_flush)
    {
        lv_disp_flush_ready(disp);
        return;
    }
#endif
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    lcd.startWrite();
    lcd.setAddrWindow(area->x1, area->y1, w, h);
    lcd.writePixels((uint16_t *)&color_p->full, w * h, true);
    lcd.endWrite();

    lv_disp_flush_ready(disp);
}

/*** Touchpad callback to read the touchpad ***/
static void touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX, touchY;
    bool touched = lcd.getTouch(&touchX, &touchY);

    if (!touched)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

/* Setting up tick task for lvgl */
static void lv_tick_task(void *arg)
{
    (void)arg;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}
