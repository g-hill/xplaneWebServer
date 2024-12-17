#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WIN32_WINNT 0x0601

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

#include "XPLMUtilities.h"
#include "XPLMDefs.h"
#include "XPLMDataAccess.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMMenus.h"


#include <string>
#include <thread>
#include <cstring>
#include <queue>
#include <mutex>

bool serverRunning = false;
std::thread* serverThread = nullptr;

std::queue<std::string> commandQueue;
std::mutex commandMutex;
float commandCallbackInterval = -1.0f; // negative means per-frame

#ifdef __cplusplus
extern "C" {
#endif

    typedef void* XPLMCommandRef;
    XPLM_API XPLMCommandRef XPLMFindCommand(const char* inName);
    XPLM_API void XPLMCommandBegin(XPLMCommandRef inCommand);
    XPLM_API void XPLMCommandEnd(XPLMCommandRef inCommand);\


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef void* XPLMDataRef;
    XPLM_API XPLMDataRef XPLMFindDataRef(const char* inDataRefName);

#ifdef __cplusplus
}
#endif

typedef void* XPLMDataRef;


struct {
    XPLMDataRef altitude;
    XPLMDataRef airspeed;
    XPLMDataRef heading;
    XPLMDataRef com1_freq;
    XPLMDataRef nav_lights;
    XPLMDataRef beacon_lights;
} datarefs;

const char* HTML_CONTENT = R"html(<!DOCTYPE html>
<html>
<head>
    <title>X-Plane Web Panel</title>
    <style>
        body { font-family: Arial; margin: 20px; }
        .control-section { margin: 20px 0; padding: 10px; background: #f5f5f5; border-radius: 5px; }
        button { padding: 10px 20px; margin: 5px; background: #007bff; color: white; border: none; cursor: pointer; }
        .slider-container { margin: 10px 0; }
        .toggle-container { margin: 10px 0; }
        .value-container { margin: 10px 0; }
        .data-display { font-family: monospace; font-size: 1.2em; }
    </style>
</head>
<body>
    <h1>X-Plane Web Panel v.00b1 by xGeoff</h1>

    <div class="control-section">
        <h2>Flight Data</h2>
        <div class="data-display">
            <p>Altitude: <span id="altitude">--</span> ft</p>
            <p>Airspeed: <span id="airspeed">--</span> kts</p>
            <p>Heading: <span id="heading">--</span>°</p>
            <p>COM1 Active: <span id="com1_freq">--</span> MHz</p>
            <p>Nav Lights: <span id="nav_lights">--</span></p>
            <p>Beacon: <span id="beacon_lights">--</span></p>
        </div>
    </div>

    <div class="control-section">
        <h2>Command Buttons</h2>
        <button onclick="sendCommand('sim/flight_controls/flaps_down')">Flaps Down</button>
        <button onclick="sendCommand('sim/flight_controls/flaps_up')">Flaps Up</button>
        <button onclick="sendCommand('sim/flight_controls/landing_gear_toggle')">Toggle Gear</button>
        <button onclick="sendCommand('sim/lights/beacon_lights_toggle')">Toggle Beacon Lights</button>
    </div>

    <div class="control-section">
        <h2>Slider Controls</h2>
        <div class="slider-container">
            <label>COM1 standby Volume:</label><br>
            <input type="range" min="0" max="1" step="0.1" value="0.5" oninput="setDataref('sim/cockpit2/radios/actuators/com1_standby_frequency_khz', this.value)">
        </div>
        <div class="slider-container">
            <label>Cabin Light Brightness:</label><br>
            <input type="range" min="0" max="1" step="0.1" value="0.5" oninput="setDataref('sim/cockpit2/electrical/cabin_lights_brightness_ratio', this.value)">
        </div>
    </div>

    <div class="control-section">
    <h2>Toggle Controls</h2>
    <div class="toggle-container">
        <label>
            <input type="checkbox" onchange="setDataref('sim/cockpit/electrical/nav_lights_on', this.checked ? 1 : 0)">
            Nav Lights
        </label>
    </div>
    <div class="toggle-container">
        <label>
            <input type="checkbox" onchange="setDataref('sim/cockpit/electrical/beacon_lights_on', this.checked ? 1 : 0)">
            Beacon Light
        </label>
    </div>
</div>

    <div class="control-section">
        <h2>Value Adjustments</h2>
        <div class="value-container">
            <label>COM1 Frequency:</label><br>
            <button onclick="adjustValue('sim/cockpit2/radios/actuators/com1_standby_frequency_khz', -0.025)">-</button>
            <span id="com1_freq">118.00</span>
            <button onclick="adjustValue('sim/cockpit2/radios/actuators/com1_standby_frequency_khz', 0.025)">+</button>
        </div>
    </div>

    <script>
        function sendCommand(cmd) {
            fetch('/command', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ command: cmd })
            });
        }

        function setDataref(dataref, value) {
            fetch('/dataref', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ dataref: dataref, value: value })
            });
        }

        function adjustValue(dataref, adjustment) {
            fetch('/adjust', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ dataref: dataref, adjustment: adjustment })
            });
        }

        function updateData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('altitude').textContent = Math.round(data.altitude || 0);
                    document.getElementById('airspeed').textContent = Math.round(data.airspeed || 0);
                    document.getElementById('heading').textContent = Math.round(data.heading || 0);
                    document.getElementById('com1_freq').textContent = (data.com1_freq || 118.00).toFixed(3);
                    document.getElementById('nav_lights').textContent = data.nav_lights ? 'ON' : 'OFF';
                    document.getElementById('beacon_lights').textContent = data.beacon_lights ? 'ON' : 'OFF';
                });
        }

        // Update data every 100ms (10 times per second)
        setInterval(updateData, 100);
    </script>
</body>
</html>)html";

void initializeDatarefs() {
    datarefs.altitude = XPLMFindDataRef("sim/flightmodel/position/elevation");
    datarefs.airspeed = XPLMFindDataRef("sim/flightmodel/position/indicated_airspeed");
    datarefs.heading = XPLMFindDataRef("sim/flightmodel/position/psi");
    datarefs.com1_freq = XPLMFindDataRef("sim/cockpit2/radios/actuators/com1_standby_frequency_khz");
    datarefs.nav_lights = XPLMFindDataRef("sim/cockpit/electrical/nav_lights_on");
    datarefs.beacon_lights = XPLMFindDataRef("sim/cockpit/electrical/beacon_lights_on");
}

float CommandCallback(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon) {
    std::string cmd;
    {
        std::lock_guard<std::mutex> lock(commandMutex);
        if (!commandQueue.empty()) {
            cmd = commandQueue.front();
            commandQueue.pop();
        }
    }

    if (!cmd.empty()) {
        XPLMCommandRef cmdRef = XPLMFindCommand(cmd.c_str());
        if (cmdRef != NULL) {
            XPLMCommandBegin(cmdRef);
            XPLMCommandEnd(cmdRef);
            XPLMDebugString("Command executed: ");
            XPLMDebugString(cmd.c_str());
            XPLMDebugString("\n");
        }
    }

    return commandCallbackInterval;
}

void handleClient(SOCKET clientSocket) {
    char buffer[4096] = { 0 };
    recv(clientSocket, buffer, sizeof(buffer), 0);
    std::string request(buffer);

    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Access-Control-Allow-Origin: *\r\n\r\n";

    if (request.find("POST /command") != std::string::npos) {
        size_t pos = request.find("\"command\":\"");
        if (pos != std::string::npos) {
            std::string cmd = request.substr(pos + 11);
            cmd = cmd.substr(0, cmd.find("\""));

            {
                std::lock_guard<std::mutex> lock(commandMutex);
                commandQueue.push(cmd);
            }
        }
        response += "{\"status\":\"ok\"}";
    }
    else if (request.find("POST /dataref") != std::string::npos) {
        size_t bodyStart = request.find("\r\n\r\n");
        if (bodyStart == std::string::npos) {
            XPLMDebugString("Could not find request body\n");
            return;
        }

        std::string jsonBody = request.substr(bodyStart + 4);
        XPLMDebugString("JSON Body: ");
        XPLMDebugString(jsonBody.c_str());
        XPLMDebugString("\n");

        // Debug each character in the JSON body
        XPLMDebugString("JSON Body characters: ");
        for (char c : jsonBody) {
            XPLMDebugString(std::to_string(static_cast<int>(c)).c_str());
            XPLMDebugString(" ");
        }
        XPLMDebugString("\n");

        // More robust parsing
        std::string datarefMarker = "\"dataref\":\"";
        size_t drefStart = jsonBody.find(datarefMarker);
        if (drefStart != std::string::npos) {
            drefStart += datarefMarker.length();
            size_t drefEnd = jsonBody.find("\"", drefStart);

            XPLMDebugString(("drefStart: " + std::to_string(drefStart) + "\n").c_str());
            XPLMDebugString(("drefEnd: " + std::to_string(drefEnd) + "\n").c_str());

            if (drefEnd != std::string::npos && drefEnd > drefStart) {
                std::string dref = jsonBody.substr(drefStart, drefEnd - drefStart);

                XPLMDebugString("Parsed dataref: '");
                XPLMDebugString(dref.c_str());
                XPLMDebugString("'\n");

                // Find the value
                size_t valPos = jsonBody.find("\"value\":");
                if (valPos != std::string::npos) {
                    std::string valStr = jsonBody.substr(valPos + 8);
                    valStr = valStr.substr(0, valStr.find_first_of(",}\""));

                    XPLMDebugString("Received dataref request: ");
                    XPLMDebugString(dref.c_str());
                    XPLMDebugString(" with value: ");
                    XPLMDebugString(valStr.c_str());
                    XPLMDebugString("\n");

                    try {
                        XPLMDataRef dataRef = XPLMFindDataRef(dref.c_str());
                        if (!dataRef) {
                            XPLMDebugString("ERROR: Could not find dataref: ");
                            XPLMDebugString(dref.c_str());
                            XPLMDebugString("\n");
                            response += "{\"status\":\"error\",\"message\":\"dataref not found\"}";
                            send(clientSocket, response.c_str(), response.length(), 0);
                            closesocket(clientSocket);
                            return;
                        }

                        if (!XPLMCanWriteDataRef(dataRef)) {
                            XPLMDebugString("ERROR: Cannot write to dataref: ");
                            XPLMDebugString(dref.c_str());
                            XPLMDebugString("\n");
                            response += "{\"status\":\"error\",\"message\":\"dataref not writable\"}";
                            send(clientSocket, response.c_str(), response.length(), 0);
                            closesocket(clientSocket);
                            return;
                        }

                        int dataRefTypes = XPLMGetDataRefTypes(dataRef);
                        XPLMDebugString(("Dataref type flags: " + std::to_string(dataRefTypes) + "\n").c_str());

                        if (dataRefTypes & xplmType_Int) {
                            int intValue = std::stoi(valStr);
                            XPLMDebugString(("Setting integer value: " + std::to_string(intValue) + "\n").c_str());
                            XPLMSetDatai(dataRef, intValue);
                            // Verify the value was set
                            int readBack = XPLMGetDatai(dataRef);
                            XPLMDebugString(("Read back value: " + std::to_string(readBack) + "\n").c_str());
                        }
                        else if (dataRefTypes & xplmType_Float) {
                            float floatValue = std::stof(valStr);
                            XPLMDebugString(("Setting float value: " + std::to_string(floatValue) + "\n").c_str());
                            XPLMSetDataf(dataRef, floatValue);
                            // Verify the value was set
                            float readBack = XPLMGetDataf(dataRef);
                            XPLMDebugString(("Read back value: " + std::to_string(readBack) + "\n").c_str());
                        }
                        else {
                            XPLMDebugString("ERROR: Unsupported dataref type\n");
                            response += "{\"status\":\"error\",\"message\":\"unsupported dataref type\"}";
                            send(clientSocket, response.c_str(), response.length(), 0);
                            closesocket(clientSocket);
                            return;
                        }

                        XPLMDebugString("Dataref operation completed successfully\n");
                    }
                    catch (const std::exception& e) {
                        XPLMDebugString("Error parsing value: ");
                        XPLMDebugString(e.what());
                        XPLMDebugString("\n");
                        response += "{\"status\":\"error\",\"message\":\"parse error\"}";
                        send(clientSocket, response.c_str(), response.length(), 0);
                        closesocket(clientSocket);
                        return;
                    }
                }
            }
        }
        response += "{\"status\":\"ok\"}";
    }
    else if (request.find("POST /adjust") != std::string::npos) {
        size_t drefPos = request.find("\"dataref\":\"");
        size_t adjPos = request.find("\"adjustment\":");
        if (drefPos != std::string::npos && adjPos != std::string::npos) {
            std::string dref = request.substr(drefPos + 10);
            dref = dref.substr(0, dref.find("\""));

            std::string adjStr = request.substr(adjPos + 13);
            adjStr = adjStr.substr(0, adjStr.find("}"));
            float adjustment = std::stof(adjStr);

            XPLMDataRef dataRef = XPLMFindDataRef(dref.c_str());
            if (dataRef) {
                float currentValue = XPLMGetDataf(dataRef);
                XPLMSetDataf(dataRef, currentValue + adjustment);
                XPLMDebugString("Dataref adjusted: ");
                XPLMDebugString(dref.c_str());
                XPLMDebugString("\n");
            }
        }
        response += "{\"status\":\"ok\"}";
    }
    else if (request.find("GET /data") != std::string::npos) {
        std::string json = "{";
        json += "\"altitude\":" + std::to_string(XPLMGetDataf(datarefs.altitude)) + ",";
        json += "\"airspeed\":" + std::to_string(XPLMGetDataf(datarefs.airspeed)) + ",";
        json += "\"heading\":" + std::to_string(XPLMGetDataf(datarefs.heading)) + ",";
        json += "\"com1_freq\":" + std::to_string(XPLMGetDataf(datarefs.com1_freq)) + ",";
        json += "\"nav_lights\":" + std::to_string(XPLMGetDatai(datarefs.nav_lights)) + ",";
        json += "\"beacon_lights\":" + std::to_string(XPLMGetDatai(datarefs.beacon_lights));
        json += "}";

        response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Access-Control-Allow-Origin: *\r\n\r\n";
        response += json;
    }
    else {
        response += HTML_CONTENT;
    }

    send(clientSocket, response.c_str(), response.length(), 0);
    closesocket(clientSocket);
}

void runServer() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 3);

    XPLMDebugString("Web Panel: Server started on port 8080\n");

    while (serverRunning) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket != -1) {
            handleClient(clientSocket);
        }
    }

    closesocket(serverSocket);
    WSACleanup();
}

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc) {
    strcpy(outName, "Web Control Panel");
    strcpy(outSig, "com.costanza.webcontrolpanel");
    strcpy(outDesc, "Web-based control panel for X-Plane");

    initializeDatarefs();

    XPLMRegisterFlightLoopCallback(CommandCallback, commandCallbackInterval, NULL);

    serverRunning = true;
    serverThread = new std::thread(runServer);

    XPLMDebugString("Web Control Panel v.01: Plugin started Successfully\n");
    return 1;
}

PLUGIN_API void XPluginStop(void) {
    serverRunning = false;
    if (serverThread) {
        serverThread->join();
        delete serverThread;
    }
    XPLMUnregisterFlightLoopCallback(CommandCallback, NULL);
    XPLMDebugString("Web Control Panel: Plugin stopped successfully\n");
}

PLUGIN_API void XPluginDisable(void) {}
PLUGIN_API void XPluginEnable(void) {}
PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam) {}