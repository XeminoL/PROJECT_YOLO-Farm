# YOLO-Farm 🌱

A smart farming system that combines **YOLO-based plant disease/pest detection** running on a host PC with an **AIoT controller (Yolo UNO / ESP32-S3)** that monitors the field environment and actuates pumps, fans, and lights through cloud-connected sensors.

The PC vision module looks at crop leaves through a webcam and classifies pests / diseases in real time. The Yolo UNO board, sitting in the field, reads air temperature & humidity, soil moisture, and ambient light, pushes the data to a cloud dashboard, and toggles relays automatically (or remotely) to keep the crops in good condition.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [System Architecture](#system-architecture)
- [Repository Structure](#repository-structure)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Installation and Setup](#installation-and-setup)
- [How It Works](#how-it-works)
- [Authors](#authors)
- [License](#license)

---

## Overview

Modern farming faces two recurring problems: **detecting crop diseases early enough to act**, and **maintaining a stable growing environment** when the farmer cannot be on site 24/7. YOLO-Farm tackles both with two cooperating subsystems:

1. **YOLO Vision (PC side)** — A laptop or PC runs a YOLO object detection model on a webcam feed, identifying common pests and leaf diseases and showing the result with bounding boxes and class labels.
2. **AIoT Farm Controller (Yolo UNO side)** — An OhStem Yolo UNO board (ESP32-S3) reads multiple environment sensors, pushes telemetry to a cloud IoT platform, and drives relays for water pumps, fans, and grow lights.

The two subsystems run independently but together form a complete smart-farm prototype.

---

## Features

### 🔬 YOLO Vision Module
- Real-time pest / disease detection on plant leaves
- Webcam input (USB camera or built-in laptop camera)
- Bounding-box visualization with class labels and confidence scores

### 🌾 AIoT Farm Controller (Yolo UNO)
- 🌡️ Air temperature and humidity monitoring (DHT11 / DHT22)
- 💧 Soil moisture monitoring
- ☀️ Ambient light monitoring (LDR)
- 🔌 Relay control for water pump, fan, and grow light
- ☁️ Cloud connectivity (MQTT / Blynk / Firebase / E-RA IoT)
- 📱 Remote monitoring and manual override from a phone/web dashboard
- ⚡ Wi-Fi and Bluetooth on board (ESP32-S3)

---

## System Architecture

```
┌───────────────────────┐         ┌─────────────────────────┐
│   Webcam / USB Cam    │────────▶│  PC running YOLO model  │
└───────────────────────┘         │  (Python + OpenCV)      │
                                  └─────────────────────────┘
                                              │
                                              ▼
                                    Pest / disease alerts
                                    shown on screen

┌──────────────┐    ┌──────────────┐
│   DHT22      │    │ Soil moist.  │
│   (T, RH)    │    │   sensor     │
└──────┬───────┘    └──────┬───────┘
       │                   │
       ▼                   ▼
   ┌──────────────────────────────┐         ┌──────────────────┐
   │   Yolo UNO  (ESP32-S3)       │◀───────▶│   Cloud (MQTT /  │
   │   - reads sensors            │  Wi-Fi  │   Blynk / E-RA)  │
   │   - pushes data to cloud     │         └──────────────────┘
   │   - toggles relays           │                   │
   └──────────────┬───────────────┘                   │
                  │                                   ▼
                  ▼                         ┌──────────────────┐
        ┌──────────────────┐                │  Phone / Web     │
        │  Relay module    │                │  dashboard       │
        │  ─ water pump    │                └──────────────────┘
        │  ─ fan           │
        │  ─ grow light    │
        └──────────────────┘
```

The YOLO vision side and the AIoT side run **independently**. They can be combined later (e.g. send a "disease detected" alert from PC → cloud → Yolo UNO triggers a spray pump), but in the current version they operate as two parallel pipelines.

---

## Repository Structure

```
PROJECT_YOLO-Farm/
├── AIoT_Farm/             # Firmware for Yolo UNO (sensors, relays, cloud)
├── Arduino/
│   └── libraries/         # Required Arduino libraries (DHT, MQTT, etc.)
└── README.md
```

---

## Hardware Requirements

### AIoT Farm Side

| Component | Notes |
|---|---|
| **Yolo UNO** (OhStem) | ESP32-S3 based, Arduino-compatible, 12 Grove ports |
| **DHT11 / DHT22** | Air temperature and humidity sensor |
| **Soil moisture sensor** | Capacitive type recommended (more durable than resistive) |
| **LDR module** | Light-dependent resistor for ambient light |
| **Relay module (3-channel or 4-channel)** | Controls water pump, fan, and grow light |
| **5V water pump** | Or any pump matched to your relay rating |
| **5V / 12V fan** | Cooling/ventilation |
| **LED grow light** | Or any 5V/12V lamp |
| **USB Type-C cable** | For programming the Yolo UNO |
| **External power supply** | Required when driving pumps/fans through relays |

### YOLO Vision Side

| Component | Notes |
|---|---|
| **Laptop or PC** | A modern CPU is enough; a CUDA GPU makes inference smoother |
| **USB webcam** | Or built-in laptop camera |

---

## Software Requirements

### For Yolo UNO (AIoT_Farm/)

- **Arduino IDE** 1.8.x or 2.x
- **OhStem Boards** package — installed via Arduino IDE Boards Manager (search `ohstem`)
- Libraries (already included under `Arduino/libraries/`):
  - `DHT sensor library`
  - WiFi + MQTT (or Blynk / E-RA IoT, depending on which cloud you use)

### For YOLO Vision

- **Python** 3.8 or newer
- Common dependencies:
  ```bash
  pip install ultralytics opencv-python numpy
  ```
- A trained YOLO weights file (`.pt`) for plant pest/disease classes

---

## Installation and Setup

### 1. Clone the repository

```bash
git clone https://github.com/XeminoL/PROJECT_YOLO-Farm.git
cd PROJECT_YOLO-Farm
```

### 2. Set up Arduino IDE for Yolo UNO

1. Open Arduino IDE → **File → Preferences → Additional Boards Manager URLs**, add the OhStem board URL (see [OhStem docs](https://docs.ohstem.vn/en/latest/yolo_uno/arduino_yolo_uno/cai_dat.html)).
2. Open **Tools → Board → Boards Manager**, search `ohstem`, install **OhStem Boards by OhStem Education**.
3. Copy the contents of `Arduino/libraries/` into your Arduino libraries folder (typically `Documents/Arduino/libraries/` on Windows).
4. Open the sketch inside `AIoT_Farm/`, select **Board: Yolo UNO** and the correct **COM port**.
5. Edit Wi-Fi SSID/password and your cloud credentials at the top of the sketch.
6. Click **Upload**.

### 3. Wire up the sensors

Connect using the Yolo UNO Grove ports (or jumper wires to standard headers):

| Sensor / Actuator | Yolo UNO pin (example) |
|---|---|
| DHT22 data | digital pin (e.g. D2) |
| Soil moisture analog out | analog pin (e.g. A0) |
| LDR analog out | analog pin (e.g. A1) |
| Relay IN1 (pump) | digital pin (e.g. D5) |
| Relay IN2 (fan) | digital pin (e.g. D6) |
| Relay IN3 (light) | digital pin (e.g. D7) |

> ⚠️ Adjust pin numbers in the sketch to match your actual wiring.

### 4. Run the YOLO vision module

```bash
# from the YOLO vision folder (add your own scripts here)
python detect.py --weights best.pt --source 0
```

`--source 0` uses the default webcam. Replace with a video file path or RTSP URL if needed.

---

## How It Works

### AIoT Farm Loop

The Yolo UNO firmware runs an endless loop that:

1. Reads air temperature and humidity from DHT22.
2. Reads soil moisture (analog).
3. Reads ambient light from LDR (analog).
4. Sends all readings to the cloud dashboard every few seconds.
5. Compares readings against thresholds:
   - Soil moisture below threshold → turn pump ON
   - Air temperature above threshold → turn fan ON
   - Light below threshold (and inside daytime window) → turn grow light ON
6. Listens for manual override commands from the dashboard.

### YOLO Detection Loop

On the PC side, the YOLO model:

1. Captures a frame from the webcam.
2. Runs inference and returns bounding boxes for any detected pest / disease class.
3. Draws boxes + labels on the frame and shows the live result.
4. (Optional) Logs detections or pushes alerts somewhere of your choice.

---

## Authors

- **XeminoL** — [GitHub Profile](https://github.com/XeminoL)

---

## License

This project is released for educational and research purposes. Feel free to fork, study, and modify.
