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
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; background-color: #ffffff; color: #000000; display: flex; flex-wrap: wrap; justify-content: space-between; align-items: center; height: 80vh; margin: 20px; overflow: hidden; }";
  html += ".section { display: flex; flex-direction: column; align-items: center; }";
  html += "#thermalCanvas { border: 1px solid #000000; width: 25vw; height: auto; image-rendering: pixelated; }";
  html += "#minMaxAvg { width: 40vw; text-align: center; }";
  html += "#histChart { width: 20vw; height: auto; }";
  html += "#tempChart { width: 80vw; height: 30vh; max-height: 30vh; }"; // Set fixed height with max-height
  html += ".stat { font-size: .8em; margin-top: 3px; }";
  html += "</style></head><body>";
  html += "<div class='section'>";
  html += "<h2>Thermal Image</h2>";
  html += "<canvas id='thermalCanvas' width='320' height='240'></canvas>";
  html += "</div>";
  html += "<div id='minMaxAvg' class='section'>";
  html += "<h2>Temperature Stats</h2>";
  html += "<p class='stat'>Min Temp: <span id='minTemp'></span> &#8451; | Max Temp: <span id='maxTemp'></span> &#8451; | Avg Temp: <span id='avgTemp'></span> &#8451;</p>";
  html += "<p class='stat'>Y-Axis Limits: <input type='number' id='yMin' value='20'> to <input type='number' id='yMax' value='40'></p>";
  html += "</div>";
  html += "<div class='section'>";
  html += "<h2>Temperature Distribution</h2>";
  html += "<canvas id='histChart'></canvas>";
  html += "</div>";
  html += "<div class='section' style='width: 100%;'>";
  html += "<h2>Temperature Trends</h2>";
  html += "<canvas id='tempChart'></canvas>";
  html += "</div>";
  html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script><script>";

  // Fetch Data Function
  html += "let tempChart, histChart;";
  html += "let startTime = Date.now();";
  html += "async function fetchData() {";
  html += "let response = await fetch('/data');";
  html += "let data = await response.json();";
  html += "let canvas = document.getElementById('thermalCanvas');";
  html += "let ctx = canvas.getContext('2d');";
  html += "let imgData = ctx.createImageData(320, 240);";
  html += "let minTemp = Math.min(...data);";
  html += "let maxTemp = Math.max(...data);";
  html += "let avgTemp = data.reduce((sum, value) => sum + value, 0) / data.length;";
  html += "document.getElementById('minTemp').innerText = minTemp.toFixed(2);";
  html += "document.getElementById('maxTemp').innerText = maxTemp.toFixed(2);";
  html += "document.getElementById('avgTemp').innerText = avgTemp.toFixed(2);";

  // Scaling up the 32x24 data to 320x240
  html += "for (let y = 0; y < 24; y++) {";
  html += "  for (let x = 0; x < 32; x++) {";
  html += "    let temp = data[y * 32 + x];";
  html += "    let color = getColorFromTemp(temp, minTemp, maxTemp);";
  html += "    for (let scaleY = 0; scaleY < 10; scaleY++) {";
  html += "      for (let scaleX = 0; scaleX < 10; scaleX++) {";
  html += "        let index = 4 * ((y * 10 + scaleY) * 320 + (x * 10 + scaleX));";
  html += "        imgData.data[index] = color.r;";
  html += "        imgData.data[index + 1] = color.g;";
  html += "        imgData.data[index + 2] = color.b;";
  html += "        imgData.data[index + 3] = 255;";
  html += "      }";
  html += "    }";
  html += "  }";
  html += "}";

  html += "ctx.putImageData(imgData, 0, 0);";
  html += "updateCharts(minTemp, maxTemp, avgTemp, data);";
  html += "requestAnimationFrame(fetchData);";
  html += "}";

  // Data Arrays for Charts
  html += "let minSeries = [], maxSeries = [], avgSeries = [], histogram = new Array(51).fill(0);";

  // Update Charts Function
  html += "function updateCharts(minTemp, maxTemp, avgTemp, data) {";
  html += "  let currentTime = (Date.now() - startTime) / 1000;";
  html += "  if (minSeries.length >= 20 * 10) { minSeries.shift(); maxSeries.shift(); avgSeries.shift(); }";  // Limit data points to 20 seconds with higher resolution
  html += "  minSeries.push({x: currentTime, y: minTemp});";
  html += "  maxSeries.push({x: currentTime, y: maxTemp});";
  html += "  avgSeries.push({x: currentTime, y: avgTemp});";
  html += "  histogram.fill(0);";  // Reset histogram
  html += "  data.forEach(temp => { histogram[Math.min(50, Math.floor(temp))]++; });";  // Update histogram
  html += "  renderCharts();";
  html += "}";

  // Render Charts Function
  html += "function renderCharts() {";
  html += "  let yMin = parseFloat(document.getElementById('yMin').value);";
  html += "  let yMax = parseFloat(document.getElementById('yMax').value);";
  
  html += "  if (!tempChart) {";
  html += "    const ctx1 = document.getElementById('tempChart').getContext('2d');";
  html += "    tempChart = new Chart(ctx1, { type: 'line', data: { datasets: [{ label: 'Min Temp', data: minSeries, borderColor: 'blue', fill: false, pointRadius: 0 }, { label: 'Max Temp', data: maxSeries, borderColor: 'red', fill: false, pointRadius: 0 }, { label: 'Avg Temp', data: avgSeries, borderColor: 'green', fill: false, pointRadius: 0 }] }, options: { responsive: true, maintainAspectRatio: false, scales: { x: { type: 'linear', position: 'bottom', title: { display: true, text: 'Time (s)' }, min: Math.max(0, minSeries[0]?.x || 0), max: Math.max(20, minSeries[minSeries.length - 1]?.x || 20) }, y: { min: yMin, max: yMax, ticks: { stepSize: 5 } } } } });";
  html += "  } else {";
  html += "    tempChart.options.scales.y.min = yMin;";
  html += "    tempChart.options.scales.y.max = yMax;";
  html += "    tempChart.options.scales.x.min = Math.max(0, minSeries[0]?.x || 0);";
  html += "    tempChart.options.scales.x.max = Math.max(20, minSeries[minSeries.length - 1]?.x || 20);";
  html += "    tempChart.data.datasets[0].data = minSeries;";
  html += "    tempChart.data.datasets[1].data = maxSeries;";
  html += "    tempChart.data.datasets[2].data = avgSeries;";
  html += "    tempChart.update();";
  html += "  }";
  
  html += "  if (!histChart) {";
  html += "    const ctx2 = document.getElementById('histChart').getContext('2d');";
  html += "    histChart = new Chart(ctx2, { type: 'bar', data: { labels: Array.from({length: 51}, (_, i) => i), datasets: [{ label: 'Temperature Frequency', data: histogram, backgroundColor: 'orange' }] }, options: { responsive: true, maintainAspectRatio: true, aspectRatio: 1, scales: { y: { beginAtZero: true } } } });";
  html += "  } else {";
  html += "    histChart.data.datasets[0].data = histogram;";
  html += "    histChart.update();";
  html += "  }";
  html += "}";


  // Get Color From Temperature
  html += "function getColorFromTemp(temp, minTemp, maxTemp) {";
  html += "let normalized = (temp - minTemp) / (maxTemp - minTemp);";
  html += "let hue = normalized * 360;";  // Use full hue scale from 0 to 360 degrees
  html += "let red = 0, green = 0, blue = 0;";
  
  html += "if (hue < 60) {";
  html += "  red = 255;";
  html += "  green = Math.floor((hue / 60) * 255);";
  html += "  blue = 0;";
  html += "} else if (hue < 120) {";
  html += "  red = Math.floor(((120 - hue) / 60) * 255);";
  html += "  green = 255;";
  html += "  blue = 0;";
  html += "} else if (hue < 180) {";
  html += "  red = 0;";
  html += "  green = 255;";
  html += "  blue = Math.floor(((hue - 120) / 60) * 255);";
  html += "} else if (hue < 240) {";
  html += "  red = 0;";
  html += "  green = Math.floor(((240 - hue) / 60) * 255);";
  html += "  blue = 255;";
  html += "} else if (hue < 300) {";
  html += "  red = Math.floor(((hue - 240) / 60) * 255);";
  html += "  green = 0;";
  html += "  blue = 255;";
  html += "} else {";
  html += "  red = 255;";
  html += "  green = 0;";
  html += "  blue = Math.floor(((360 - hue) / 60) * 255);";
  html += "}";

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
