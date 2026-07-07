# 🌐 AI News Intelligent Platform

[![GitHub Pages Deployment](https://img.shields.io/badge/GitHub_Pages-Active-brightgreen?logo=github&style=flat-square)](https://shanmukha-belide-pixel.github.io/AI-News-Intelligent-AI-Powered-News-Platform/)
[![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg?style=flat-square&logo=c%2B%2B)](https://gcc.gnu.org/)
[![Flutter](https://img.shields.io/badge/Flutter-3.22-02569B?logo=flutter&style=flat-square)](https://flutter.dev/)
[![Docker](https://img.shields.io/badge/Docker-Supported-blue?logo=docker&style=flat-square)](https://www.docker.com/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg?style=flat-square)](LICENSE)

> A production-grade, premium AI-powered news platform offering real-time news aggregation, dynamic multi-lingual translations, AI reasoning drawer, offline text-to-speech audio reader, and keyless auto-geolocation weather. Built using a robust **C++20 Backend (Drogon), PostgreSQL, Redis, Elasticsearch, Kafka**, a **Flutter App**, and a gorgeous **Single-Page Web Application**.

### 🔗 Live Hosted Web App
👉 [**Access Live Platform on GitHub Pages**](https://shanmukha-belide-pixel.github.io/AI-News-Intelligent-AI-Powered-News-Platform/)

---

## ✨ Features

### 1. 🌍 Core Scope & Multi-Tier News
*   **International:** Real-time global feeds (BBC World, TechCrunch, Nature, Reuters, AP, Wired).
*   **National (India):** Major national top stories and business hubs in English and Hindi (Times of India, NDTV, The Hindu, Economic Times, Amar Ujala).
*   **State News (29 States + UTs):** Dynamic coverage of all Indian regions mapped to regional RSS publishers (e.g. Eenadu, Prajavani, Anandabazar Patrika, Dinamalar) with a **fail-safe local data generator fallback** to guarantee coverage.
*   **Sports Coverage:** Sports feeds from Cricbuzz, ESPN, Sky Sports, and Autosport.

### 2. 🌐 Real-Time Translation (22 Indian Languages)
Supports immediate layout translations into the 22 official regional Indian languages using the free, keyless MyMemory Translation API:
*   *Assamese, Bengali, Bodo, Dogri, Gujarati, Hindi, Kannada, Kashmiri, Konkani, Maithili, Malayalam, Manipuri, Marathi, Nepali, Odia, Punjabi, Sanskrit, Santhali, Sindhi, Tamil, Telugu, Urdu, and English.*
*   Translates card headlines, AI summaries, section headers, search fields, and greeting widgets dynamically upon selection.

### 3. 🤖 AI Summary & Comprehension Drawer
Clicking any article opens a slide-up AI drawer offering structured, translated details:
*   **Quick Summary:** High-level analytical overview of the story's context.
*   **Key Points:** Clear bullet-point analysis of why the event matters and its long-term societal/economic impact.
*   **Interactive Quiz Game:** Dynamic multiple-choice questions matching the article. Users can click answers to see instant correct/wrong animations and explanation details.

### 4. 🎧 Audio Player (TTS Voice Synthesis)
*   A premium, floating audio player panel popping up from the bottom of the screen.
*   Uses native browser Speech Synthesis to read translated headlines and summaries out loud.
*   Features a **live-animating audio wave visualization**, play/pause triggers, and a **speed-multiplier toggle** (1.0x, 1.25x, 1.5x, 2.0x).

### 5. ⛅ Keyless Auto-Geolocation Weather Card
*   Requires **zero API keys** to run.
*   Automatically detects the user's city via IP geolocation API and queries the **Open-Meteo REST API** to retrieve current temperatures, humidity levels, wind speed, wind conditions, and weather icons.

---

## 🏗️ Architecture

```
                 Web App / Flutter App
                          │ HTTPS / WSS
                  Nginx Reverse Proxy
                          │
            C++20 Drogon Backend (Port 8080)
      ┌───────────────────┬───────────────┬────────────────┐
      │  Auth Controller  │ News Ingester │  AI & Chat API │
      └─────────┬─────────┴───────┬───────┴────────┬───────┘
                │                 │                │
            PostgreSQL          Redis            Kafka
          (Partitioned DB)  (PubSub/Cache)    (Telemetry)
                │                 │                │
          Elasticsearch       ClickHouse         MinIO
         (Semantic Search)   (Big Data OLAP)   (S3 Storage)
```

---

## 📂 Project Structure

```
ai-news-platform/
├── index.html                  # Single-Page Web App (Aesthetics, Geolocation, TTS, Translations)
├── database/
│   ├── schema.sql              # Partitioned table schema & RBAC controls
│   ├── stored_procedures.sql   # Personalized matching algorithms
│   └── partitioning.sql        # Dynamic month-by-month table partition triggers
├── backend/
│   ├── CMakeLists.txt          # Drogon, Redis++, rdkafka configuration
│   ├── Dockerfile              # Multi-stage production compiler image
│   ├── docker-compose.yml      # Multi-container service stack setup
│   └── src/
│       ├── main.cpp            # Drogon setup, database pools, global CORS, middleware
│       ├── core/
│       │   ├── auth/           # OAuth, JWT token generation
│       │   ├── news/           # RSS ingesters and category controller mappings
│       │   ├── weather/        # Weather REST controller
│       │   └── ai/             # GPT-4o streaming chat controller
│       └── infrastructure/
│           ├── redis_client.hpp
│           ├── kafka_producer.hpp
│           └── elasticsearch_client.hpp
└── flutter_app/
    ├── pubspec.yaml
    └── lib/
        ├── main.dart
        ├── core/theme/         # Material 3 Light/Dark theme configuration
        └── features/           # Home, News, Weather, and Sports Providers (Riverpod)
```

---

## 🔧 Installation & Setup

### 1. Launching the Web App (Static / GitHub Pages)
To run the lightweight demonstration web app instantly in your browser:
*   Open the root `index.html` file, or
*   Deploy it directly to any static provider (e.g. GitHub Pages).

### 2. Spinning Up Infrastructure
Launch the backend databases and analytics systems using Docker:
```bash
cd backend
docker-compose up -d
```

### 3. Compiling the C++ Backend
Ensure you have CMake, GCC/Clang, and Drogon dependencies installed:
```bash
cd backend
mkdir build && cd build
cmake ..
make
./news_platform
```

### 4. Running the Flutter App
Ensure you have the Flutter SDK configured:
```bash
cd flutter_app
flutter pub get
flutter run
```

---

## 📄 License
This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.
