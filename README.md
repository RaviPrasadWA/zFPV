# zFPV — HDMI → CSI2 → Low-Latency Digital FPV Architecture
*Architecture & Design Document*

## 1 — Goals & Constraints

### 🎯 Primary Goal
Achieve **low-latency, reliable HD video** from an HDMI source → zFPV transmitter (TX) → long-range RF link → zFPV receiver (RX) → display/recording.

### ⏱ Target Performance
- **End-to-end latency:** ≤ **120 ms**
- **Video profiles:**
  - 1280×720 @ 60 fps
  - 1920×1080 @ 30 fps

### ⚙️ Hardware Baseline
- **Raspberry Pi 4** (TX & RX) running **Bullseye (legacy)**
- **TC358743 HDMI → CSI-2** converter
- Pi hardware H.264 encoder (**MMAL / v4l2h264enc**)
- zFPV transport stack

### 📡 Network
- 5.8 GHz datalink (Wi-Fi Atheros/ath9k/ath10k or zFPV custom Wi-Fi drivers)
- Long-range, high-power radios recommended

### 🛡 Robustness
- Forward error correction (FEC)
- Adaptive bitrate
- Link monitoring
- Telemetry channel (embedded or separate)


## 2 — High-Level Architecture (ASCII Diagram)

```
    HDMI Source (FPV Camera / HDMI OSD / HDMI Adapter)
                       │
                       ▼
        TC358743 HDMI → CSI-2 Bridge (on TX Pi)
                       │   (CSI2 → V4L2 driver)
                       ▼
                 /dev/video0 (V4L2)
                       │
                       ▼
      Hardware H.264 Encoder (V4L2/MMAL/GStreamer v4l2h264enc)
           │     low-latency mode, no B-frames, small GOP
                       ▼
                zFPV TX Transport Stack
     ┌─────────────────────────────────────────────────────┐
     │ Packetizer (NAL fragmenter, RTP-like)               │
     │ FEC encoder (Reed-Solomon, RaptorQ optional)        │
     │ ARQ/RTT telemetry channel (optional)                │
     │ Link scheduler (prioritize control/telemetry)       │
     └─────────────────────────────────────────────────────┘
                       │
                       ▼
                RF Link (5.8 GHz / Wi-Fi)
                       ▼
                zFPV RX Transport Stack
     ┌─────────────────────────────────────────────────────┐
     │ Depacketizer & reorder                              │
     │ FEC decoder / loss recovery                         │
     │ Small jitter buffer (40–120 ms)                     │
     │ HW decoder (v4l2/MMAL) or SW decode (FFmpeg)        │
     └─────────────────────────────────────────────────────┘
                       │
                       ▼
        HDMI Display + Recorder + Telemetry UI (RX)
```


## 3 — Component Responsibilities

### 🟦 TX Capture Subsystem
- TC358743 exposes **/dev/video0** via V4L2
- Must configure input format (UYVY/YUYV) and resolution/framerate
- Optional small pre-processing (crop, scale)

### 🟧 TX Encoder
Use Raspberry Pi hardware H.264 encoder:
- `v4l2h264enc` (GStreamer)
- MMAL (legacy)

Settings:
- Profile: baseline/main
- Preset: **ultrafast / zerolatency**
- GOP: **30 frames** or **1–2 seconds**
- Bitrate:
  - 720p60 → **6–8 Mbps**
  - 1080p30 → **8–12 Mbps**
- **Disable B-frames**
- CBR or constrained VBR


### 🟥 Packetization & Transport (zFPV TX)
- Fragment NAL units into UDP datagrams
- Add headers (timestamp, seqno, nal size)
- Apply FEC (Reed-Solomon or RaptorQ)
- MTU: **~1200 bytes**
- Channels:
  - Video
  - Telemetry/control
  - Heartbeat
- Adaptive bitrate based on packet loss & SNR


### 🟩 RF Link
- Use strong long-range radios
- Ensure good antennas & legal power


### 🟦 RX Subsystem
- Depacketize and recover via FEC
- Very small jitter buffer (40–120 ms)
- Decode using HW decoder (preferred)
- Display via HDMI; optional recorder/streamer


### 🟨 Telemetry & Control
- Embedded or separate channel
- Reliable small-packet ARQ or repeat
- Highest priority in scheduler
- Includes heartbeat & link metrics


## 4 — Data Formats, Codecs & Parameters

### 🎥 Codec
- H.264 (hardware accelerated)

### 🌐 Over-the-wire Format
- Raw H.264 NALs packed in UDP
- Application header:
  - Timestamp
  - Seq number
  - NAL size

### 🔧 FEC
- Reed-Solomon (lightweight)
- Or RaptorQ (advanced)

### 🧱 MTU
- 1200 bytes typical

### 📊 Bitrate Targets
| Resolution | FPS | Bitrate |
|------------|-----|----------|
| 720p       | 60  | 6–8 Mbps |
| 1080p      | 30  | 8–12 Mbps |


## 5 — Latency Budget

| Stage | Latency |
|--------|----------|
| Capture | 5–10 ms |
| Encoding | 10–30 ms |
| Packetization + TX | 5–20 ms |
| RF Propagation | <5 ms |
| RX + FEC | 5–40 ms |
| Decode + Display | 10–20 ms |
| **Total** | **40–120 ms** |

If latency exceeds **200 ms**:
- Reduce resolution
- Reduce bitrate
- Reduce jitter buffer
- Improve RF quality


## 📘 Summary
This document defines a full architecture for a **low-latency HDMI → zFPV video system** using Raspberry Pi 4 and TC358743. Covers encoding, packetization, RF link behavior, receiver design, and performance budgeting.
