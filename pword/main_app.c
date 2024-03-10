// main_app.c
#include "app_context.h"
#include <furi.h> // For furi_delay_ms and other core functionalities
#include <stdint.h> // For int32_t

int32_t main_app(void* p) {
    (void)p;
    AppContext* app = app_context_alloc();

    while (app->running) {
        furi_delay_ms(100);
    }

    app_context_free(app);
    return 0;
}

