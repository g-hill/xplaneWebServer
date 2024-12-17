# X-Plane Web Control Panel Plugin

A lightweight web-based control panel for X-Plane 11/12 that allows you to monitor and control your aircraft through a browser interface.

Use your sim's IP address in a browser on the local network (iPad, tablet, etc.) with port 8080.

i.e., 192.168.1.12:8080

![Version](https://img.shields.io/badge/version-0.0.1b-blue.svg)
![X-Plane](https://img.shields.io/badge/X--Plane-11/12-green.svg)

## Features

- Real-time flight data monitoring
  - Altitude
  - Airspeed
  - Heading
  - COM1 Frequency
  - Navigation Lights Status
  - Beacon Lights Status

- Aircraft Controls
  - Flaps control (up/down)
  - Landing gear toggle
  - Beacon lights toggle
  - Navigation lights toggle
  - COM1 frequency adjustment
  - Cabin lighting control

## Installation

1. Download the latest release from the releases page
2. Extract the contents to your X-Plane plugins directory:
   - Windows: `X-Plane 11/12/Resources/plugins/`
   - macOS: `X-Plane 11/12/Resources/plugins/`
   - Linux: `X-Plane 11/12/Resources/plugins/`

## Usage

1. Start X-Plane
2. The plugin will automatically start a web server on port 8080
3. Open your web browser and navigate to:
   ```
   http://localhost:8080
   ```
4. The control panel will appear in your browser, showing real-time flight data and controls

## Building from Source

### Prerequisites

- C++ compiler with C++11 support
- CMake 3.10 or higher
- X-Plane SDK
- Windows SDK (for Windows builds)

### Build Steps

1. Clone the repository
2. Set the `XPLANE_SDK_PATH` environment variable to your X-Plane SDK location
3. Build using CMake:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Network Security Note

The web server runs on port 8080 and accepts connections from any IP address. If you're using this plugin, ensure your firewall is properly configured to prevent unauthorized access.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- X-Plane SDK team for the comprehensive flight simulator SDK
- Contributors to the project

## Support

For issues, feature requests, or questions, please use the GitHub issues system.
