#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <lvgl.h>
#define USE_TFT_ESPI_LIBRARY

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include "lv_xiao_round_screen.h"

const byte MLX90640_address = 0x33; // Default address of the MLX90640
#define TA_SHIFT 8 // Default shift for MLX90640 in open air
#define WIFI_BUTTON_PIN 0 // GPIO0 for push button

static float mlx90640To[768];
static uint16_t mlx90640Frame[834];
static lv_color_t cbuf[240 * 240];  // Static allocation for 240x240 round display
paramsMLX90640 mlx90640;

// WiFi and WebServer instance
const char* ssid = "Triton9";
const char* password = "88888888";
WebServer server(80);
bool wifiConnected = false;

// LVGL objects
lv_obj_t *heatmap_canvas;

void setup() {
    Serial.begin(115200);
    Serial.println("XIAO round screen - LVGL Thermal Display");

    lv_init();
    lv_xiao_disp_init();
    lv_xiao_touch_init();

    // Setup WiFi
    connectToWiFi();

    // Initialize LVGL display
    heatmap_canvas = lv_canvas_create(lv_scr_act());
    lv_obj_set_size(heatmap_canvas, 240, 240);
    lv_obj_align(heatmap_canvas, LV_ALIGN_CENTER, 0, 0);
    lv_canvas_set_buffer(heatmap_canvas, cbuf, 240, 240, LV_IMG_CF_TRUE_COLOR);

    // MLX90640 sensor setup
    Wire.begin();
    Wire.setClock(400000); // Increase I2C clock speed to 400kHz

    if (!isConnected()) {
        Serial.println("MLX90640 not detected at default I2C address. Check wiring.");
        while (1);
    }

    uint16_t eeMLX90640[832];
    int status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
    if (status != 0) Serial.println("Failed to load system parameters");

    status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
    if (status != 0) Serial.println("Parameter extraction failed");

    MLX90640_SetRefreshRate(MLX90640_address, 0x06); // Set refresh rate to 16Hz
    Wire.setClock(1000000); // Set I2C clock to 1MHz after EEPROM read
}

void loop() {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 500) {  // Update every 500 ms
        lastUpdate = millis();
        updateThermalData();
        drawHeatmap();
    }

    lv_timer_handler();  // LVGL handling
    delay(5);

    if (wifiConnected) {
        server.handleClient();
    }
}

// Function to connect to WiFi
void connectToWiFi() {
    if (!wifiConnected) {
        WiFi.begin(ssid, password);
        Serial.print("Connecting to WiFi...");
        unsigned long startAttemptTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000) {
            delay(500);
            Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Connected!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            server.on("/", handleRoot);
            server.on("/data", handleData);
            server.begin();
            Serial.println("HTTP server started");
            wifiConnected = true;
        } else {
            Serial.println("Failed to connect to WiFi within timeout");
        }
    }
}

// Returns true if the MLX90640 is detected on the I2C bus
bool isConnected() {
    Wire.beginTransmission((uint8_t)MLX90640_address);
    return (Wire.endTransmission() == 0); // Sensor ACKs
}

// Function to read and update thermal data
void updateThermalData() {
    int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
    if (status < 0) {
        Serial.println("Failed to get frame data");
        return;
    }

    float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
    float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);
    float tr = Ta - TA_SHIFT; // Reflected temperature
    float emissivity = 0.95;

    MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
}

// Function to draw the heatmap on LVGL canvas
void drawHeatmap() {
    // Calculate scaling and centering factors
    const int scaleX = 7;  // Scale width by 7 (approximation to fit 32x24 grid into 240x240)
    const int scaleY = 10; // Scale height by 10
    const int offsetX = (240 - 32 * scaleX) / 2; // Center horizontally
    const int offsetY = (240 - 24 * scaleY) / 2; // Center vertically

    // Determine min and max temperature for color mapping
    float minTemp = mlx90640To[0], maxTemp = mlx90640To[0];
    for (int i = 1; i < 768; i++) {
        if (mlx90640To[i] < minTemp) minTemp = mlx90640To[i];
        if (mlx90640To[i] > maxTemp) maxTemp = mlx90640To[i];
    }

    // Draw heatmap using static buffer
    for (int y = 0; y < 24; y++) {
        for (int x = 0; x < 32; x++) {
            float temp = mlx90640To[y * 32 + x];
            lv_color_t color = getColorFromTemp(temp, minTemp, maxTemp);

            // Draw a scaled rectangle for each data point to fill the display
            for (int i = 0; i < scaleY; i++) {
                for (int j = 0; j < scaleX; j++) {
                    int displayX = offsetX + x * scaleX + j;
                    int displayY = offsetY + y * scaleY + i;
                    if (displayX < 240 && displayY < 240) {
                        cbuf[displayY * 240 + displayX] = color;
                    }
                }
            }
        }
    }
    lv_obj_invalidate(heatmap_canvas);
}

// Function to map temperature to color for heatmap
lv_color_t getColorFromTemp(float temp, float minTemp, float maxTemp) {
    float normalized = (temp - minTemp) / (maxTemp - minTemp);
    int hue = int(normalized * 360); // Map normalized to 0-360 hue range

    // Convert hue to RGB (simplified for LVGL)
    return lv_color_hsv_to_rgb(hue, 100, 100);
}

// HTTP request handler for root
void handleRoot() {
    server.send(200, "text/plain", "Hello from ESP32 with MLX90640!");
}

// Handle request for data endpoint
void handleData() {
    String json = "[";
    for (int i = 0; i < 768; i++) {
        json += String(mlx90640To[i], 2);
        if (i < 767) json += ",";
    }
    json += "]";

    server.send(200, "application/json", json);
}
