#include <Wire.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WebServer.h>

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

const byte MLX90640_address = 0x33; // Default 7-bit unshifted address of the MLX90640
#define TA_SHIFT 8 // Default shift for MLX90640 in open air

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

void setup()
{
  Wire.begin();
  Wire.setClock(400000); // Increase I2C clock speed to 400kHz

  Serial.begin(115200); // Fast serial as possible
  while (!Serial);

  tft.init();           // Initialize TFT screen
  tft.setRotation(1);   // Set rotation if needed
  tft.fillScreen(TFT_BLACK);

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

  // Initialize WiFi if enabled
  if (enableWiFi) {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.begin();
    Serial.println("HTTP server started");
  }
}

void loop()
{
  if (enableWiFi) {
    server.handleClient();
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

  // Display thermal data as heatmap on TFT
  drawHeatmap(mlx90640To);
}

// Function to draw the heatmap on the TFT display
void drawHeatmap(float *data)
{
  for (int y = 0; y < 24; y++)
  {
    for (int x = 0; x < 32; x++)
    {
      float temp = data[y * 32 + x];
      uint16_t color = getColorFromTemp(temp);
      tft.fillRect(x * 5, y * 5, 5, 5, color);  // Scaling up each cell for better visualization
    }
  }
}

// Function to map temperature to color using a new HSV-like color gradient
uint16_t getColorFromTemp(float temp)
{
  // Map temperature to HSV-like gradient for better sensitivity
  float hue = map(temp, 0, 50, 0, 360); // Assuming temperature range 0-50 degrees Celsius
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
  String html = "<html><head><title>Thermal Camera Stream</title>";
  html += "<style>body { font-family: Arial; text-align: center; background-color: #000; color: #00ff00; } ";
  html += "h1 { color: #00ff00; } canvas { border: 1px solid #00ff00; width: 1600px; height: 1200px; image-rendering: pixelated; }</style></head><body>";
  html += "<h1>MLX90640 Thermal Camera Stream</h1>";
  html += "<canvas id='thermalCanvas' width='320' height='240'></canvas>";
  html += "<p>Min Temp: <span id='minTemp'></span> &#8451; | Max Temp: <span id='maxTemp'></span> &#8451;</p>";
  html += "<script>";
  html += "async function fetchData() {";
  html += "let response = await fetch('/data');";
  html += "let data = await response.json();";
  html += "let canvas = document.getElementById('thermalCanvas');";
  html += "let ctx = canvas.getContext('2d');";
  html += "let imgData = ctx.createImageData(32, 24);";
  html += "let minTemp = Math.min(...data);";
  html += "let maxTemp = Math.max(...data);";
  html += "document.getElementById('minTemp').innerText = minTemp.toFixed(2);";
  html += "document.getElementById('maxTemp').innerText = maxTemp.toFixed(2);";
  html += "for (let i = 0; i < data.length; i++) {";
  html += "let color = getColorFromTemp(data[i], minTemp, maxTemp);";
  html += "imgData.data[i * 4] = color.r;";
  html += "imgData.data[i * 4 + 1] = color.g;";
  html += "imgData.data[i * 4 + 2] = color.b;";
  html += "imgData.data[i * 4 + 3] = 255;";
  html += "}";
  html += "ctx.putImageData(imgData, 0, 0);";
  html += "requestAnimationFrame(fetchData);";
  html += "}";
  html += "function getColorFromTemp(temp, minTemp, maxTemp) {";
  html += "let normalized = (temp - minTemp) / (maxTemp - minTemp);";
  html += "let hue = normalized * 360;";
  html += "let red = 0, green = 0, blue = 0;";
  html += "if (hue < 60) { red = 255; green = Math.floor((hue / 60) * 255); blue = 0; }";
  html += "else if (hue < 120) { red = Math.floor(((120 - hue) / 60) * 255); green = 255; blue = 0; }";
  html += "else if (hue < 180) { red = 0; green = 255; blue = Math.floor(((hue - 120) / 60) * 255); }";
  html += "else if (hue < 240) { red = 0; green = Math.floor(((240 - hue) / 60) * 255); blue = 255; }";
  html += "else if (hue < 300) { red = Math.floor(((hue - 240) / 60) * 255); green = 0; blue = 255; }";
  html += "else { red = 255; green = 0; blue = Math.floor(((360 - hue) / 60) * 255); }";
  html += "return {r: red, g: green, b: blue}; }";
  html += "fetchData();";
  html += "</script></body></html>";

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