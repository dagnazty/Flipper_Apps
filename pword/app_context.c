#include "app_context.h"
#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <gui/elements.h>
#include <furi_hal.h>
#include <math.h>
#include <string.h>
#include <pword_icons.h>

static void app_draw_callback(Canvas* const canvas, void* ctx);
static void app_input_callback(InputEvent* input_event, void* ctx);

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
    app->filter_level = 0;
    generate_filtered_string(app, app->current_random_string, sizeof(app->current_random_string), STRING_LENGTH);

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

void generate_filtered_string(AppContext* app, char *str, size_t size, int length) {
    // Define character sets for each filter level
    const char* charset_full = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()-_=+[]{}|;:,.<>?/";
    const char* charset_no_special = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const char* charset_letters_numbers = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const char* charset_lowercase = "abcdefghijklmnopqrstuvwxyz";
    const char* charset_numbers = "0123456789";
    const char* charset_characters = "!@#$%^&*()-_=+[]{}|;:,.<>?/";

    // Select character set based on filter level
    const char* charset;
    switch (app->filter_level) {
        case 0: charset = charset_full; break;
        case 1: charset = charset_no_special; break;
        case 2: charset = charset_letters_numbers; break;
        case 3: charset = charset_lowercase; break;
        case 4: charset = charset_numbers; break;
        case 5: charset = charset_characters; break;
        default: charset = charset_full; // Fallback to full charset
    }

    memset(str, 0, size); // Clear the buffer first
    if (size > 0) {
        --size; // Leave space for null terminator
        for (size_t n = 0; n < size && n < (size_t)length; n++) {
            int key = rand() % (int)(strlen(charset));
            str[n] = charset[key];
        }
        FURI_LOG_I("pword", "Generated password: %s", str);
    }
}

static void app_draw_callback(Canvas* const canvas, void* ctx) {
    AppContext* app = (AppContext*)ctx;
    canvas_clear(canvas);

    if (app->state == AppState_MenuNavigation) {
        // Black header with "pword" in white text
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_box(canvas, 0, 0, canvas_width(canvas), 20);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str_aligned(canvas, canvas_width(canvas) / 2, 10, AlignCenter, AlignCenter, "pword");

        const char* items[] = {"Generate Password", "About"};
        size_t items_count = sizeof(items) / sizeof(items[0]);
        int item_height = 20; // Adjust if necessary for item spacing
        int total_menu_height = item_height * items_count;
        int start_y = ((canvas_height(canvas) - total_menu_height) / 2) + 10; // Start below the header
        
        for (size_t i = 0; i < items_count; i++) {
            int item_y = start_y + (item_height * i);
            canvas_set_color(canvas, i == app->selected_index ? ColorBlack : ColorWhite);
            canvas_draw_box(canvas, 0, item_y, canvas_width(canvas), item_height);
            canvas_set_color(canvas, i == app->selected_index ? ColorWhite : ColorBlack);
            canvas_draw_str_aligned(canvas, canvas_width(canvas) / 2, item_y + (item_height / 2) - 4, AlignCenter, AlignCenter, items[i]);
        }
    } else if (app->state == AppState_GeneratingString) {
        char menuBarText[30];
        snprintf(menuBarText, sizeof(menuBarText), "%d-char Password", app->current_string_length);
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_box(canvas, 0, 0, canvas_width(canvas), 20);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str_aligned(canvas, canvas_width(canvas) / 2, 10, AlignCenter, AlignCenter, menuBarText);

        // Draw the password string centered
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_str_aligned(canvas, canvas_width(canvas) / 2, canvas_height(canvas) / 2, AlignCenter, AlignCenter, app->current_random_string);

        // Draw the character set info and arrows
        char charsetInfo[50];
        switch (app->filter_level) {
            case 0: strcpy(charsetInfo, "All"); break;
            case 1: strcpy(charsetInfo, "A-9"); break;
            case 2: strcpy(charsetInfo, "A-z"); break;
            case 3: strcpy(charsetInfo, "a-z"); break;
            case 4: strcpy(charsetInfo, "0-9"); break; // Just numbers
            case 5: strcpy(charsetInfo, "#"); break; // Just characters
            default: strcpy(charsetInfo, "Charset: Unknown");
        }
        
        // Calculate positions for arrows and text
        int charsetTextX = canvas_width(canvas) / 2;
        int charsetTextY = canvas_height(canvas) - 20; // Adjusted for better positioning
        int arrowY = charsetTextY - 10; // Position arrows slightly above the text

        // Draw horizontal (length change) arrow
        canvas_draw_icon(canvas, charsetTextX - 60, arrowY + 15, &I_Horizontal_arrow_9x7);
        // Text "Length" next to horizontal arrow
        canvas_draw_str(canvas, charsetTextX - 50, arrowY + 25, "Length");
        
        // Draw vertical (charset change) arrow
        canvas_draw_icon(canvas, charsetTextX - 10, arrowY + 20, &I_Vertical_arrow_7x9);
        // Draw character set info text next to the vertical arrow
        canvas_draw_str(canvas, charsetTextX, charsetTextY + 20, charsetInfo);
        
        // Draw back arrow
        canvas_draw_icon(canvas, charsetTextX + 30, arrowY + 15, &I_Pin_back_arrow_10x8);
        // Text "Back" next to back arrow
        canvas_draw_str(canvas, charsetTextX + 40, arrowY + 25, "Back");
    } else if (app->state == AppState_About) {
        // Black header with "pword" in white text
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_box(canvas, 0, 0, canvas_width(canvas), 20);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str_aligned(canvas, canvas_width(canvas) / 2, 10, AlignCenter, AlignCenter, "pword");

        // About text details
        const char* about_lines[] = {"Developed by @dagnazty", "Generates random passwords."};
        size_t line_count = sizeof(about_lines) / sizeof(about_lines[0]);
        int line_height = 20;
        int total_about_height = line_height * line_count;
        int startY = ((canvas_height(canvas) - total_about_height) / 2) + 20; // Start below the header

        canvas_set_color(canvas, ColorBlack);
        for(size_t i = 0; i < line_count; i++) {
            int line_y = startY + (line_height * i);
            canvas_draw_str_aligned(canvas, canvas_width(canvas) / 2, line_y, AlignCenter, AlignCenter, about_lines[i]);
        }
    }

    // Reset color for safety
    canvas_set_color(canvas, ColorBlack);
}

const char* get_charset_name(int filter_level) {
    switch(filter_level) {
        case 0: return "Full Charset";
        case 1: return "Alphanumeric";
        case 2: return "Letters Only";
        case 3: return "Lowercase Letters";
        case 4: return "Numbers Only";
        case 5: return "Special Characters";
        default: return "Unknown Charset";
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
                        // Adjust for new items count
                        if (app->selected_index < 1) {
                            app->selected_index++;
                        }
                        break;
                    case InputKeyOk:
                        if (app->selected_index == 0) { // Adjusted to trigger for the single Generate item
                            app->state = AppState_GeneratingString;
                            generate_filtered_string(app, app->current_random_string, sizeof(app->current_random_string), app->current_string_length);                        } else if (app->selected_index == 1) { // About item
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
                        if (app->current_string_length < 16) {
                            app->current_string_length++;
                            generate_filtered_string(app, app->current_random_string, sizeof(app->current_random_string), app->current_string_length);
                            FURI_LOG_I("pword", "String length increased to %d", app->current_string_length);
                        }
                        break;
                    case InputKeyLeft:
                        if (app->current_string_length > 4) {
                            app->current_string_length--;
                            generate_filtered_string(app, app->current_random_string, sizeof(app->current_random_string), app->current_string_length);
                            FURI_LOG_I("pword", "String length decreased to %d", app->current_string_length);
                        }
                        break;
                    case InputKeyOk:
                        generate_filtered_string(app, app->current_random_string, sizeof(app->current_random_string), app->current_string_length);
                        break;
                    case InputKeyBack:
                        app->state = AppState_MenuNavigation;
                        break;
                    default:
                        break;
                    case InputKeyDown:
                        if(app->filter_level < 5) {
                            app->filter_level++;
                            generate_filtered_string(app, app->current_random_string, sizeof(app->current_random_string), app->current_string_length);                        }
                            FURI_LOG_I("pword", "Charset changed to: %s", get_charset_name(app->filter_level));
                        break;
                    case InputKeyUp:
                        if(app->filter_level > 0) {
                            app->filter_level--;
                            generate_filtered_string(app, app->current_random_string, sizeof(app->current_random_string), app->current_string_length);                        }
                            FURI_LOG_I("pword", "Charset changed to: %s", get_charset_name(app->filter_level));                
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