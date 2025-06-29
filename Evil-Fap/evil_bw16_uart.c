#include "evil_bw16.h"
#include <furi_hal.h>
#include <ctype.h>

#define BAUDRATE (115200)
#define RX_BUFFER_SIZE (512)

// Forward declarations for response handler functions
static void handle_info_response(EvilBw16UartWorker* worker, const char* line);
static void handle_error_response(EvilBw16UartWorker* worker, const char* line);
static void handle_debug_response(EvilBw16UartWorker* worker, const char* line);
static void handle_cmd_response(EvilBw16UartWorker* worker, const char* line);
static void handle_mgmt_response(EvilBw16UartWorker* worker, const char* line);
static void handle_data_response(EvilBw16UartWorker* worker, const char* line);
static void handle_hop_response(EvilBw16UartWorker* worker, const char* line);
static void handle_generic_response(EvilBw16UartWorker* worker, const char* line);
static void parse_scan_result_line(EvilBw16UartWorker* worker, const char* line);

struct EvilBw16UartWorker {
    FuriThread* thread;
    FuriStreamBuffer* rx_stream;
    FuriHalSerialHandle* serial_handle;
    bool running;
    EvilBw16App* app;
    FuriMutex* tx_mutex;
};

static EvilBw16UartWorker* uart_worker = NULL;

// UART receive callback function
static void uart_on_irq_cb(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, void* context) {
    EvilBw16UartWorker* worker = (EvilBw16UartWorker*)context;
    
    if(event == FuriHalSerialRxEventData) {
        // Read available data and put it in stream buffer
        while(furi_hal_serial_async_rx_available(handle)) {
            uint8_t data = furi_hal_serial_async_rx(handle);
            furi_stream_buffer_send(worker->rx_stream, &data, 1, 0);
        }
    }
}

// Worker thread for processing UART data
static int32_t uart_worker_thread(void* context) {
    EvilBw16UartWorker* worker = (EvilBw16UartWorker*)context;
    
    char line_buffer[256];
    size_t line_pos = 0;
    
    while(worker->running) {
        uint8_t byte;
        size_t received = furi_stream_buffer_receive(worker->rx_stream, &byte, 1, 100);
        
        if(received > 0) {
            // Handle incoming byte
            if(byte == '\n' || byte == '\r') {
                if(line_pos > 0) {
                    line_buffer[line_pos] = '\0';
                    
                    // Process complete line
                    FURI_LOG_I("EvilBw16", "RX: %s", line_buffer);
                    
                    // Parse different response types
                    if(strstr(line_buffer, "[INFO]") == line_buffer) {
                        handle_info_response(worker, line_buffer);
                    }
                    else if(strstr(line_buffer, "[ERROR]") == line_buffer) {
                        handle_error_response(worker, line_buffer);
                    }
                    else if(strstr(line_buffer, "[DEBUG]") == line_buffer) {
                        handle_debug_response(worker, line_buffer);
                    }
                    else if(strstr(line_buffer, "[CMD]") == line_buffer) {
                        handle_cmd_response(worker, line_buffer);
                    }
                    else if(strstr(line_buffer, "[MGMT]") == line_buffer) {
                        handle_mgmt_response(worker, line_buffer);
                    }
                    else if(strstr(line_buffer, "[DATA]") == line_buffer) {
                        handle_data_response(worker, line_buffer);
                    }
                    else if(strstr(line_buffer, "[HOP]") == line_buffer) {
                        handle_hop_response(worker, line_buffer);
                    }
                    else {
                        // Generic response
                        handle_generic_response(worker, line_buffer);
                    }
                    
                    // Add to app log
                    if(worker->app) {
                        evil_bw16_append_log(worker->app, line_buffer);
                    }
                }
                line_pos = 0;
            }
            else if(line_pos < sizeof(line_buffer) - 1) {
                line_buffer[line_pos++] = byte;
            }
        }
    }
    
    return 0;
}

// Response handler functions
static void handle_info_response(EvilBw16UartWorker* worker, const char* line) {
    if(!worker->app) return;
    
    // Write to debug log
    debug_write_to_sd(line);
    
    // Parse scan results - look for the actual header format
    if(strstr(line, "Index") != NULL && strstr(line, "SSID") != NULL && strstr(line, "BSSID") != NULL) {
        // This is a scan result header - clear previous results
        worker->app->network_count = 0;
        memset(worker->app->networks, 0, sizeof(worker->app->networks));
        worker->app->scan_in_progress = true;
        FURI_LOG_I("EvilBw16", "Scan results header detected - cleared previous results");
    }
    // Look for actual scan result lines - they start with a digit after [INFO] 
    else if(strstr(line, "\t") != NULL && strlen(line) > 7 && 
            line[7] >= '0' && line[7] <= '9') {
        // This looks like a scan result line: "[INFO] 0\tSSID\t..."
        parse_scan_result_line(worker, line);
    }
    // Scan completion - wait for "Scan results printed" message
    else if(strstr(line, "Scan results printed") != NULL || 
            strstr(line, "Scan completed") != NULL) {
        // Scan finished - update UI
        worker->app->scan_in_progress = false;
        FURI_LOG_I("EvilBw16", "Scan completed with %d networks", worker->app->network_count);
        // Send event to update scanner scene
        if(worker->app->view_dispatcher) {
            view_dispatcher_send_custom_event(worker->app->view_dispatcher, EvilBw16EventScanComplete);
        }
    }
    else if(strstr(line, "Deauth") != NULL) {
        // Attack progress notification
        FURI_LOG_I("EvilBw16", "Attack progress: %s", line);
    }
}

static void handle_error_response(EvilBw16UartWorker* worker, const char* line) {
    UNUSED(worker);
    FURI_LOG_E("EvilBw16", "Arduino Error: %s", line);
}

static void handle_debug_response(EvilBw16UartWorker* worker, const char* line) {
    UNUSED(worker);
    FURI_LOG_D("EvilBw16", "Arduino Debug: %s", line);
}

static void handle_cmd_response(EvilBw16UartWorker* worker, const char* line) {
    if(!worker->app) return;
    
    FURI_LOG_I("EvilBw16", "Command response: %s", line);
    
    // Update sniffer state based on command responses
    if(strstr(line, "sniffing mode") != NULL) {
        worker->app->sniffer_state.is_running = true;
        // Send event to update sniffer UI
        if(worker->app->view_dispatcher) {
            view_dispatcher_send_custom_event(worker->app->view_dispatcher, EvilBw16EventSnifferStarted);
        }
    }
    else if(strstr(line, "Sniffer stopped") != NULL) {
        worker->app->sniffer_state.is_running = false;
        if(worker->app->view_dispatcher) {
            view_dispatcher_send_custom_event(worker->app->view_dispatcher, EvilBw16EventSnifferStopped);
        }
    }
}

static void handle_mgmt_response(EvilBw16UartWorker* worker, const char* line) {
    if(!worker->app) return;
    
    // Increment packet count for management frames
    worker->app->sniffer_state.packet_count++;
    
    // Log the management frame
    FURI_LOG_I("EvilBw16", "MGMT Frame: %s", line);
    
    // Send event to update sniffer UI if in sniffer scene
    if(worker->app->view_dispatcher) {
        view_dispatcher_send_custom_event(worker->app->view_dispatcher, EvilBw16EventPacketReceived);
    }
}

static void handle_data_response(EvilBw16UartWorker* worker, const char* line) {
    if(!worker->app) return;
    
    // Increment packet count for data frames
    worker->app->sniffer_state.packet_count++;
    
    // Log the data frame (especially EAPOL)
    FURI_LOG_I("EvilBw16", "DATA Frame: %s", line);
    
    // Special handling for EAPOL
    if(strstr(line, "EAPOL") != NULL) {
        FURI_LOG_I("EvilBw16", "EAPOL handshake detected!");
        // Could add special notification here
    }
    
    if(worker->app->view_dispatcher) {
        view_dispatcher_send_custom_event(worker->app->view_dispatcher, EvilBw16EventPacketReceived);
    }
}

static void handle_hop_response(EvilBw16UartWorker* worker, const char* line) {
    UNUSED(worker);
    FURI_LOG_I("EvilBw16", "Channel hop: %s", line);
}

static void handle_generic_response(EvilBw16UartWorker* worker, const char* line) {
    UNUSED(worker);
    FURI_LOG_I("EvilBw16", "Generic: %s", line);
}

// Parse scan result line like: "[INFO] 0\tATTuPvzvVc_2.4\t38:17:B1:17:B7:C6\t3\t-54\t2.4GHz"
static void parse_scan_result_line(EvilBw16UartWorker* worker, const char* line) {
    if(!worker->app || worker->app->network_count >= EVIL_BW16_MAX_NETWORKS) return;
    
    // Find the actual data after "[INFO] "
    const char* ptr = line + 7; // Skip "[INFO] "
    
    // Parse index first
    int index = atoi(ptr);
    
    // Find first tab for SSID
    const char* ssid_start = strchr(ptr, '\t');
    if(!ssid_start) return;
    ssid_start++; // Skip tab
    
    // Find second tab for BSSID
    const char* bssid_start = strchr(ssid_start, '\t');
    if(!bssid_start) return;
    
    // Extract SSID (between first and second tab)
    int ssid_len = bssid_start - ssid_start;
    if(ssid_len >= 64) ssid_len = 63; // Clamp to buffer size
    
    bssid_start++; // Skip tab
    
    // Find third tab for channel
    const char* channel_start = strchr(bssid_start, '\t');
    if(!channel_start) return;
    
    // Extract BSSID
    int bssid_len = channel_start - bssid_start;
    if(bssid_len >= 18) bssid_len = 17; // Clamp to buffer size
    
    channel_start++; // Skip tab
    
    // Parse channel
    int channel = atoi(channel_start);
    
    // Find fourth tab for RSSI
    const char* rssi_start = strchr(channel_start, '\t');
    if(!rssi_start) return;
    rssi_start++; // Skip tab
    
    // Parse RSSI
    int rssi = atoi(rssi_start);
    
    // Find fifth tab for frequency
    const char* freq_start = strchr(rssi_start, '\t');
    if(!freq_start) return;
    freq_start++; // Skip tab
    
    // Store network data
    int network_idx = worker->app->network_count;
    EvilBw16Network* network = &worker->app->networks[network_idx];
    
    // Copy SSID
    strncpy(network->ssid, ssid_start, ssid_len);
    network->ssid[ssid_len] = '\0';
    
    // Copy BSSID
    strncpy(network->bssid, bssid_start, bssid_len);
    network->bssid[bssid_len] = '\0';
    
    network->channel = channel;
    network->rssi = rssi;
    network->index = index;
    
    // Determine band from frequency
    if(strstr(freq_start, "5GHz")) {
        network->band = EvilBw16Band5GHz;
    } else {
        network->band = EvilBw16Band24GHz;
    }
    
    worker->app->network_count++;
    
    FURI_LOG_I("EvilBw16", "Parsed network %d: %s (%s) Ch:%d RSSI:%d", 
               network_idx, network->ssid, network->bssid, network->channel, network->rssi);
}

EvilBw16UartWorker* evil_bw16_uart_init(EvilBw16App* app) {
    EvilBw16UartWorker* worker = malloc(sizeof(EvilBw16UartWorker));
    
    worker->app = app;
    worker->running = true;
    worker->tx_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    worker->rx_stream = furi_stream_buffer_alloc(RX_BUFFER_SIZE, 1);
    
    // Get USART serial handle (expansion connector pins 13/14)
    worker->serial_handle = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    if(!worker->serial_handle) {
        FURI_LOG_E("EvilBw16", "Failed to acquire serial handle");
        furi_stream_buffer_free(worker->rx_stream);
        furi_mutex_free(worker->tx_mutex);
        free(worker);
        return NULL;
    }
    
    // Initialize UART
    furi_hal_serial_init(worker->serial_handle, BAUDRATE);
    
    // Start async RX
    furi_hal_serial_async_rx_start(worker->serial_handle, uart_on_irq_cb, worker, false);
    
    // Start worker thread
    worker->thread = furi_thread_alloc_ex("EvilBw16UartWorker", 1024, uart_worker_thread, worker);
    furi_thread_start(worker->thread);
    
    uart_worker = worker;
    
    FURI_LOG_I("EvilBw16", "Hardware UART initialized on USART at %u baud", BAUDRATE);
    FURI_LOG_I("EvilBw16", "UART worker thread started");
    
    return worker;
}

void evil_bw16_uart_free(EvilBw16UartWorker* worker) {
    if(!worker) return;
    
    worker->running = false;
    
    // Stop async RX
    furi_hal_serial_async_rx_stop(worker->serial_handle);
    
    // Stop worker thread
    furi_thread_join(worker->thread);
    furi_thread_free(worker->thread);
    
    // Deinitialize UART
    furi_hal_serial_deinit(worker->serial_handle);
    furi_hal_serial_control_release(worker->serial_handle);
    
    // Free resources
    furi_stream_buffer_free(worker->rx_stream);
    furi_mutex_free(worker->tx_mutex);
    free(worker);
    
    uart_worker = NULL;
    
    FURI_LOG_I("EvilBw16", "UART worker freed");
}

void evil_bw16_uart_tx(EvilBw16UartWorker* worker, const uint8_t* data, size_t len) {
    if(!worker || !data || len == 0) return;
    
    furi_mutex_acquire(worker->tx_mutex, FuriWaitForever);
    
    // Send data via hardware UART
    furi_hal_serial_tx(worker->serial_handle, data, len);
    furi_hal_serial_tx_wait_complete(worker->serial_handle);
    
    furi_mutex_release(worker->tx_mutex);
    
    FURI_LOG_D("EvilBw16", "TX: %.*s", (int)len, data);
}

void evil_bw16_uart_tx_string(EvilBw16UartWorker* worker, const char* str) {
    if(!worker || !str) return;
    evil_bw16_uart_tx(worker, (const uint8_t*)str, strlen(str));
}

void evil_bw16_uart_send_command(EvilBw16UartWorker* worker, const char* command) {
    if(!worker || !command) return;
    
    // Add newline to command
    char cmd_with_newline[256];
    snprintf(cmd_with_newline, sizeof(cmd_with_newline), "%s\n", command);
    
    evil_bw16_uart_tx_string(worker, cmd_with_newline);
}

bool evil_bw16_uart_read_line(EvilBw16UartWorker* worker, char* buffer, size_t buffer_size, uint32_t timeout_ms) {
    if(!worker || !buffer || buffer_size == 0) return false;
    
    UNUSED(timeout_ms);
    
    // Try to read a line from stream buffer
    size_t bytes_read = 0;
    uint32_t start_time = furi_get_tick();
    
    while(bytes_read < buffer_size - 1 && (furi_get_tick() - start_time) < timeout_ms) {
        uint8_t byte;
        if(furi_stream_buffer_receive(worker->rx_stream, &byte, 1, 10) > 0) {
            buffer[bytes_read++] = byte;
            if(byte == '\n') {
                break;
            }
        }
    }
    
    buffer[bytes_read] = '\0';
    return bytes_read > 0;
}

size_t evil_bw16_uart_rx_available(EvilBw16UartWorker* worker) {
    if(!worker) return 0;
    return furi_stream_buffer_bytes_available(worker->rx_stream);
}

void evil_bw16_uart_flush_rx(EvilBw16UartWorker* worker) {
    if(!worker) return;
    furi_stream_buffer_reset(worker->rx_stream);
}

bool evil_bw16_uart_wait_for_response(EvilBw16UartWorker* worker, const char* expected_prefix, char* response_buffer, size_t buffer_size, uint32_t timeout_ms) {
    if(!worker || !expected_prefix || !response_buffer) return false;
    
    UNUSED(buffer_size);
    UNUSED(timeout_ms);
    
        // For now, return false - focusing on TX first
    response_buffer[0] = '\0';
    return false;
} 