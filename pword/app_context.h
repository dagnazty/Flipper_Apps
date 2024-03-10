// app_context.h
#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

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
    int filter_level;
} AppContext;

AppContext* app_context_alloc();
void app_context_free(AppContext* app);
void generate_filtered_string(AppContext* app, char *str, size_t size, int length);

#endif // APP_CONTEXT_H
