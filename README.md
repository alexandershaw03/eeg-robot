# EEG-Controlled Mobile Robot

A real-time EEG-to-robotics system developed as my undergraduate capstone project in Mechanical Engineering.

The project integrates neural-signal acquisition, embedded communications, motor control and a desktop monitoring interface into a complete working mobile robot.

The overall or long-term goal for it, was for the robotic platform to be used as an alternatively accessible wheelchair.

> **Status:** Repository under construction.  
> Source code, schematics, documentation and test material are currently being organised and uploaded.

---

## Project Overview

The system converts EEG-derived commands into physical robot motion through the following pipeline:

```text
Emotiv Insight EEG
        │
        ▼
Emotiv Cortex API
        │
        ▼
Command generation
        │
        ▼
Arduino transmitter
        │
        ▼
nRF24L01+ wireless link
        │
        ▼
Arduino receiver
        │
        ▼
Motor-control logic
        │
        ▼
Mobile robot
```

The project was designed as an end-to-end robotics system rather than a standalone signal-processing demonstration.

---

## Core Features

- real-time EEG-derived robot control
- Emotiv Insight integration through the Cortex API
- embedded C/C++ firmware
- multi-node Arduino architecture
- nRF24L01+ wireless communication
- custom packet protocol with sequence numbering
- duplicate-packet rejection
- signal-loss detection
- watchdog and fail-safe stop behaviour
- automatic multi-stage recovery
- differential motor control
- soft-start motor ramping
- custom PCB / electronics integration
- mechanically designed mobile chassis
- Python desktop ground station
- live EEG visualisation, command logging and session monitoring

---

## System Architecture

The system spans several engineering layers:

```text
Neural sensing
      │
      ▼
Signal / command interface
      │
      ▼
Embedded transmitter
      │
      ▼
Wireless communications
      │
      ▼
Embedded receiver
      │
      ▼
Safety / recovery state logic
      │
      ▼
Motor actuation
      │
      ▼
Physical robot
```

A separate Python/Qt ground station provides live system monitoring and EEG visualisation.

---

## Reliability and Safety

Because the system controls physical hardware wirelessly, the embedded receiver includes explicit fault-handling behaviour.

This includes:

- duplicate-packet rejection
- communication timeout detection
- watchdog behaviour
- fail-safe stopping
- staged retry / recovery logic
- soft-start motor control

The intention was to make the robot behave predictably during communication loss or invalid command states rather than simply transmitting motor commands.

---

## Technologies

### Embedded

- C/C++
- Arduino
- nRF24L01+
- motor-driver electronics

### Software

- Python
- PySide6 / Qt
- pyqtgraph
- Emotiv Cortex API

### Hardware

- Emotiv Insight EEG headset
- microcontrollers
- RF modules
- custom PCBs
- DC motor control
- custom mobile chassis

---

## Repository Roadmap

Planned additions include:

- transmitter firmware
- receiver firmware
- desktop ground-station source
- system architecture diagrams
- electronics / PCB documentation
- mechanical design material
- communications protocol documentation
- safety and recovery logic
- setup instructions
- test results
- demonstration media

---

## Project Context

This project formed the basis of my undergraduate final-year engineering project and led directly into later work on multimodal sensing, synchronised EEG/vision recording and NVIDIA Jetson edge perception.

That follow-on work is available here:

[`multimodal-sensing`](https://github.com/alexandershaw03/multimodal-sensing)
