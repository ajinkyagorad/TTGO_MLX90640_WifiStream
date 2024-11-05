# 🌡️ Thermal Camera Display with MLX90640 and XIAO Round Display

This project demonstrates how to build a **thermal camera display** using the **MLX90640** thermal sensor and the **Seeed Studio XIAO ESP32S3** round 240x240 display. The code reads thermal data from the sensor, processes it, and displays a heatmap on the XIAO's round screen.

## 🛠️ What We Did
1. **Set up the hardware**:
   - Connected the **MLX90640** sensor to the **XIAO ESP32S3** over I2C.
   - Set up the 240x240 round TFT display.

2. **Configured Display for XIAO Round**:
   - Changed the display configuration to support the XIAO round display by updating `User_Setup_Select.h`:
     ```cpp
     #define USER_SETUP_ID 48 // Select the appropriate setup for XIAO round display
     ```

3. **Included MLX90640 Files**:
   - Added the **MLX90640** library files (`MLX90640_API.h`, `MLX90640_I2C_Driver.h`, and their corresponding `.cpp` files) in the project folder.
   - Ensure these files are in the same folder as your main `.ino` file for ease of use.

4. **Initialized WiFi**:
   - Configured the ESP32S3 to connect to a WiFi network.
   - Started an HTTP server to remotely access the thermal data if needed.

5. **Configured MLX90640 Sensor**:
   - Set the sensor's refresh rate to 16Hz.
   - Calculated the ambient temperature and thermal pixel data.

6. **Developed the Display Heatmap**:
   - Scaled the 32x24 thermal pixel grid to fit the 240x240 round display.
   - Mapped temperature values to colors for a visually intuitive heatmap.
   - Centered the heatmap on the round display, with each thermal sensor pixel expanded to cover the display area.

7. **Optimized Memory Usage**:
   - Used static allocation to avoid stack overflow.
   - Scaled and offset the heatmap correctly on the display without excessive memory use.

## 📜 How to Use

1. **Hardware Requirements**:
   - **Seeed Studio XIAO ESP32S3 with Round Display**.
   - **MLX90640 Thermal Camera** (configured with I2C).

2. **Libraries Required**:
   - [LVGL](https://github.com/lvgl/lvgl) for display rendering.
   - [ESP32 WiFi](https://github.com/espressif/arduino-esp32) for WiFi functionality.
   - [MLX90640](https://github.com/adafruit/Adafruit-MLX90640-Library) for handling the thermal sensor.

3. **Setup Instructions**:
   - Clone this repository and open it in your favorite Arduino-compatible IDE.
   - **Update Display Configuration**:
     - Open the `User_Setup_Select.h` file (from your display library) and select the XIAO round display configuration by ensuring:
       ```cpp
       #define USER_SETUP_ID 48
       ```
   - **Place MLX90640 Files**:
     - Ensure `MLX90640_API.h`, `MLX90640_I2C_Driver.h`, and their `.cpp` files are in the same folder as the main `.ino` file.
   - **Connect the MLX90640 sensor** to the XIAO ESP32S3 following I2C connections:
     - **SDA** to **GPIO21**
     - **SCL** to **GPIO22**
   - Modify the WiFi credentials in the code:
     ```cpp
     const char* ssid = "YOUR_SSID";
     const char* password = "YOUR_PASSWORD";
     ```
   - Upload the code to the XIAO ESP32S3.

4. **Running the Code**:
   - The thermal camera should automatically connect to WiFi (if available) and display the heatmap on the round screen.
   - Visit the IP address shown on the Serial Monitor to access the thermal data in JSON format.

## 💡 Code Overview

- **WiFi Connection**:
  - Sets up the XIAO ESP32S3 to connect to a WiFi network and starts a server.
  
- **Thermal Sensor Data Processing**:
  - Captures thermal data from the MLX90640 sensor.
  - Calculates the minimum and maximum temperatures and maps them to colors.

- **Display Heatmap Rendering**:
  - Scales the 32x24 grid of thermal data to fit the 240x240 display using a scaling factor.
  - Centers the heatmap on the display, ensuring all data is visible.

## 🐛 Troubleshooting
- **WiFi Connection Issues**: Check that the credentials are correct and ensure the device is within WiFi range.
- **Display Not Showing Data**: Verify connections to the MLX90640 sensor and ensure correct I2C addresses.
- **Stack Overflow**: If experiencing resets, ensure that the display buffer is statically allocated and that memory usage is optimized.

## 📷 Demo

This project creates a live heatmap on a 240x240 round display, offering a clear, visual representation of the temperature distribution captured by the MLX90640 sensor.

Enjoy building your thermal display! 🎉
