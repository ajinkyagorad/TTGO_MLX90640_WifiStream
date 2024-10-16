# MLX90640 WiFi Thermal Camera 🌡️📡

Welcome to the **MLX90640 WiFi Thermal Camera** project! This project combines the MLX90640 thermal camera sensor with an ESP32 board to create a real-time thermal imaging system with a built-in web server. You'll be able to view the thermal stream on an onboard display or via a web browser over WiFi. It's perfect for anyone who wants to experiment with thermal imaging! 😎

## Demo 🎥
![Demo Image](https://github.com/ajinkyagorad/TTGO_MLX90640_WifiStream/blob/22417ce4ed0a078ebf3647fa3265e3bafd263cbe/img/demo.jpg)

## Features ✨
- **Real-Time Thermal Imaging**: Displays a thermal heatmap using the MLX90640 sensor.
- **Web Server Integration**: View the thermal camera output through your browser by connecting to a WiFi network.
- **Continuous Heatmap Visualization**: Smooth, interpolated visualization of temperature data.
- **Color Mapping**: A simple color gradient for temperature visualization, making it easy to see hot and cold spots. 🔴🟢🔵

## Hardware Required 🛠️
- **MLX90640 Thermal Camera Sensor**
- **ESP32 Dev Board** (with built-in display or TTGO T-Display)
- **Connecting Wires**
- **WiFi Network** (for streaming)

## Setup Instructions 🚀
1. **Wiring**: Connect the MLX90640 sensor to the ESP32 dev board:
   - VCC ➡️ 3.3V on ESP32
   - GND ➡️ GND on ESP32
   - SCL ➡️ GPIO 22 (I2C Clock)
   - SDA ➡️ GPIO 21 (I2C Data)

2. **Library Installation**:
   - Clone the **MLX90640 library** from [SparkFun's GitHub Repository](https://github.com/sparkfun/SparkFun_MLX90640_Arduino_Example/tree/master).
   - Place the library in your Arduino `libraries` folder.
   - If you encounter issues with `Serial` not being declared, add `#include <Arduino.h>` to the required files. 📄

3. **Uploading the Code**:
   - Use Arduino IDE to open and upload the provided code (`mlx90640_wifi_stream.ino`) to your ESP32 board.
   - Ensure you've set the correct board and COM port in **Tools**.

## Usage 💻
1. **Power On the ESP32**: After wiring and uploading the code, power on your ESP32.
2. **Connect to WiFi**: The ESP32 will attempt to connect to the configured WiFi network (`ssid: Triton9, password: 88888888`). 🔌
3. **Access the Web Interface**:
   - Once connected, check the serial monitor for the ESP32's IP address.
   - Open a web browser and go to `http://<ESP32_IP_ADDRESS>` to see the live thermal feed. 🌐
4. **Onboard Display**: The onboard display will also show the real-time heatmap directly, scaled up for better visualization.

## Color Mapping 🌈
The heatmap uses a simple color gradient to indicate temperature ranges:
- **Blue** 🔵: Cooler areas (below 20°C)
- **Green** 🟢: Intermediate areas (20°C - 30°C)
- **Red** 🔴: Hotter areas (above 30°C)

The onboard display and web stream will continuously update to reflect temperature changes, providing an intuitive understanding of the thermal data. 🚥

## Known Issues and Troubleshooting 🛠️
- **Serial Issues**: If the code fails to compile due to `Serial` errors, make sure you've added `#include <Arduino.h>` to the relevant files in the library.
- **WiFi Not Connecting**: Double-check the SSID and password. If the ESP32 fails to connect, move it closer to your WiFi router.
- **Tiny Image on Browser**: The canvas size on the web page is increased to `640x480` for better visualization.

## Acknowledgements 🙌
- The MLX90640 library files are adapted from [SparkFun's GitHub Repository](https://github.com/sparkfun/SparkFun_MLX90640_Arduino_Example/tree/master). Special thanks to SparkFun for their contributions to the maker community!

## License 📜
This project is licensed under the MIT License. Feel free to modify, share, and have fun with it!



Happy thermal imaging! 🔥✨

