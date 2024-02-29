#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <gui/elements.h>
#include <furi_hal.h>
#include <math.h>

#define STRING_LENGTH 8 

typedef enum {
    AppState_MenuNavigation,
    AppState_AdjustingItem1,
    AppState_AdjustingItem2,
    AppState_GeneratingString,
    AppState_About,
} AppState;

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    size_t selected_index;
    int item1_value;
    int item2_value;
    AppState state;
    bool running;
    int current_string_length;
    char current_random_string[17];
} AppContext;

static void generate_random_string(char *str, size_t size, int length) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-_=+[]{}|;:,.<>?/";
    if (size) {
        --size;
        for (size_t n = 0; n < size && n < (size_t)length; n++) {
            int key = rand() % (int)(sizeof(charset) - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
}

static void app_draw_callback(Canvas* const canvas, void* ctx) {
    AppContext* app = (AppContext*)ctx;
    canvas_clear(canvas);

    if (app->state == AppState_MenuNavigation) {
        const char* items[] = {"Generate 8-char string", "Generate 16-char string", "About"};
        for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
            if (i == app->selected_index) {
                canvas_set_color(canvas, ColorBlack);
                canvas_draw_box(canvas, 0, 10 + i * 20, 128, 16);
                canvas_set_color(canvas, ColorWhite);
            } else {
                canvas_set_color(canvas, ColorBlack);
            }
            canvas_draw_str_aligned(canvas, 10, 10 + i * 20 + 4, AlignLeft, AlignTop, items[i]);
        }
    } else if (app->state == AppState_GeneratingString) {
        canvas_draw_str_aligned(canvas, canvas_width(canvas) / 2, canvas_height(canvas) / 2, AlignCenter, AlignCenter, app->current_random_string);
    } else if (app->state == AppState_About) {
        int lineHeight = 10;
        int gap = 2;
        int totalHeight = 3 * lineHeight + 2 * gap;

        int startY = (canvas_height(canvas) - totalHeight) / 2;

        canvas_draw_str_aligned(canvas, canvas_width(canvas) / 2, startY, AlignCenter, AlignCenter, "pword");
        startY += lineHeight + gap; 
        canvas_draw_str_aligned(canvas, canvas_width(canvas) / 2, startY, AlignCenter, AlignCenter, "Developed by dag");
        startY += lineHeight + gap; 
        canvas_draw_str_aligned(canvas, canvas_width(canvas) / 2, startY, AlignCenter, AlignCenter, "Generates random strings.");
    }
}

static void app_input_callback(InputEvent* input_event, void* ctx) {
    AppContext* app = (AppContext*)ctx;

    switch (app->state) {
        case AppState_MenuNavigation:
            if (input_event->type == InputTypeShort) {
                switch (input_event->key) {
                    case InputKeyUp:
                        if (app->selected_index > 0) {
                            app->selected_index--;
                        }
                        break;
                    case InputKeyDown:
                        if (app->selected_index < 2) {
                            app->selected_index++;
                        }
                        break;
                    case InputKeyOk:
                        if (app->selected_index == 0) {
                            app->current_string_length = 8;
                            app->state = AppState_GeneratingString;
                            generate_random_string(app->current_random_string, app->current_string_length + 1, app->current_string_length);
                        } else if (app->selected_index == 1) {
                            app->current_string_length = 16; 
                            app->state = AppState_GeneratingString;
                            generate_random_string(app->current_random_string, app->current_string_length + 1, app->current_string_length);
                        } else if (app->selected_index == 2) {
                            app->state = AppState_About;
                        }
                        break;
                    case InputKeyBack:
                        app->running = false;
                        break;
                    default:
                        break;
                }
            }
            break;
        case AppState_GeneratingString:
            if (input_event->type == InputTypeShort) {
                switch (input_event->key) {
                    case InputKeyRight:
                    case InputKeyLeft:
                        generate_random_string(app->current_random_string, app->current_string_length + 1, app->current_string_length);
                        break;
                    case InputKeyOk:
                    case InputKeyBack:
                        app->state = AppState_MenuNavigation;
                        break;
                    default:
                        break;
                }
            }
            break;
        case AppState_About:
            if (input_event->type == InputTypeShort && (input_event->key == InputKeyBack || input_event->key == InputKeyOk)) {
                app->state = AppState_MenuNavigation;
            }
            break;
        default:
            break;
    }

    view_port_update(app->view_port);
}

AppContext* app_context_alloc() {
    AppContext* app = malloc(sizeof(AppContext));
    app->gui = furi_record_open(RECORD_GUI);
    app->view_port = view_port_alloc();
    app->selected_index = 0;
    app->item1_value = 0;
    app->item2_value = 0;
    app->state = AppState_MenuNavigation;
    app->running = true;
    app->current_string_length = STRING_LENGTH;
    generate_random_string(app->current_random_string, sizeof(app->current_random_string), STRING_LENGTH);

    view_port_draw_callback_set(app->view_port, app_draw_callback, app);
    view_port_input_callback_set(app->view_port, app_input_callback, app);

    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    return app;
}

void app_context_free(AppContext* app) {
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);
    free(app);
}

int32_t main_app(void* p) {
    UNUSED(p);
    AppContext* app = app_context_alloc();

    while (app->running) {
        furi_delay_ms(100);
    }

    app_context_free(app);
    return 0;
}