#include <Wire.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WebServer.h>

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

const byte MLX90640_address = 0x33; // Default 7-bit unshifted address of the MLX90640
#define TA_SHIFT 8 // Default shift for MLX90640 in open air
#define WIFI_BUTTON_PIN 0 // GPIO0 for push button

float mlx90640To[768];
paramsMLX90640 mlx90640;

// TFT Display instance
TFT_eSPI tft = TFT_eSPI();

// WiFi and WebServer instance
const char* ssid = "Triton9";
const char* password = "88888888";
WebServer server(80);

// Flag to enable WiFi streaming
bool enableWiFi = true;
bool wifiConnected = false;

void setup()
{
  Wire.begin();
  Wire.setClock(400000); // Increase I2C clock speed to 400kHz

  Serial.begin(115200); // Fast serial as possible
  while (!Serial);

  tft.init();           // Initialize TFT screen
  tft.setRotation(1);   // Set rotation if needed
  tft.fillScreen(TFT_BLACK);

  pinMode(WIFI_BUTTON_PIN, INPUT_PULLUP); // Set GPIO0 as input with pull-up resistor

  if (isConnected() == false)
  {
    Serial.println("MLX90640 not detected at default I2C address. Please check wiring. Freezing.");
    while (1);
  }

  // Get device parameters - We only have to do this once
  int status;
  uint16_t eeMLX90640[832];
  status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
  if (status != 0)
    Serial.println("Failed to load system parameters");

  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  if (status != 0)
    Serial.println("Parameter extraction failed");

  // Set refresh rate
  MLX90640_SetRefreshRate(MLX90640_address, 0x06); // Set rate to 16Hz effective

  // Increase I2C clock after EEPROM read
  Wire.setClock(1000000); // Set I2C clock to 1MHz

  // Attempt WiFi connection at startup
  connectToWiFi();
}

void loop()
{
  if (enableWiFi && wifiConnected) {
    server.handleClient();
  }

  if (digitalRead(WIFI_BUTTON_PIN) == LOW) {
    delay(50); // Debounce delay
    if (digitalRead(WIFI_BUTTON_PIN) == LOW) {
      connectToWiFi();
    }
  }

  // Read thermal data
  uint16_t mlx90640Frame[834];
  int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
  if (status < 0)
  {
    Serial.println("Failed to get frame data");
    return;
  }

  float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
  float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);

  float tr = Ta - TA_SHIFT; // Reflected temperature
  float emissivity = 0.95;

  MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);

  // Get min and max temperature for the current frame
  float minTemp = mlx90640To[0], maxTemp = mlx90640To[0];
  for (int i = 1; i < 768; i++) {
    if (mlx90640To[i] < minTemp) minTemp = mlx90640To[i];
    if (mlx90640To[i] > maxTemp) maxTemp = mlx90640To[i];
  }

  // Display thermal data as heatmap on TFT
  drawHeatmap(mlx90640To, minTemp, maxTemp);
}

// Function to draw the heatmap on the TFT display
void drawHeatmap(float *data, float minTemp, float maxTemp)
{
  for (int y = 0; y < 24; y++)
  {
    for (int x = 0; x < 32; x++)
    {
      float temp = data[y * 32 + x];
      uint16_t color = getColorFromTemp(temp, minTemp, maxTemp);
      tft.fillRect(x * 5, y * 5, 5, 5, color);  // Scaling up each cell for better visualization
    }
  }
}

// Function to map temperature to color using a new HSV-like color gradient
uint16_t getColorFromTemp(float temp, float minTemp, float maxTemp)
{
  // Map temperature to HSV-like gradient for better sensitivity based on min and max values
  float normalized = (temp - minTemp) / (maxTemp - minTemp);
  float hue = normalized * 360; // Scale normalized value to 360 degrees hue
  uint8_t red = 0, green = 0, blue = 0;

  if (hue < 60) {
    red = 255;
    green = map(hue, 0, 60, 0, 255);
    blue = 0;
  } else if (hue < 120) {
    red = map(hue, 60, 120, 255, 0);
    green = 255;
    blue = 0;
  } else if (hue < 180) {
    red = 0;
    green = 255;
    blue = map(hue, 120, 180, 0, 255);
  } else if (hue < 240) {
    red = 0;
    green = map(hue, 180, 240, 255, 0);
    blue = 255;
  } else if (hue < 300) {
    red = map(hue, 240, 300, 0, 255);
    green = 0;
    blue = 255;
  } else {
    red = 255;
    green = 0;
    blue = map(hue, 300, 360, 255, 0);
  }

  return tft.color565(red, green, blue);
}

// Handle HTTP request to serve the thermal data
void handleRoot() {
  String html = "<html><head><title>Thermal Camera Dashboard</title>";
  // HTML content omitted for brevity
  server.send(200, "text/html", html);
}

// Function to generate BMP image data in base64 format
void handleData() {
  String json = "[";
  for (int i = 0; i < 768; i++) {
    json += String(mlx90640To[i], 2);
    if (i < 767) json += ",";
  }
  json += "]";

  server.send(200, "application/json", json);
}

// Returns true if the MLX90640 is detected on the I2C bus
boolean isConnected()
{
  Wire.beginTransmission((uint8_t)MLX90640_address);
  if (Wire.endTransmission() != 0)
    return (false); // Sensor did not ACK
  return (true);
}

void connectToWiFi() {
  if (!wifiConnected) {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 2000) {
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
