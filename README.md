# EEG-Controlled Mobile Robot

A real-time EEG-to-robotics system developed as my undergraduate capstone project in Mechanical Engineering.

The project integrates neural-signal acquisition, embedded communications, motor control and a desktop monitoring interface into a complete working mobile robot.

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
