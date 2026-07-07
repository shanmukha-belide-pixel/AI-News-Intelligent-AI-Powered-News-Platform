# AI News Platform

> Production-grade AI-powered news platform — **C++20 Backend · Flutter App · PostgreSQL · Redis · Elasticsearch · Kafka**

---

## 📰 Real News Sources

### 🌍 International
| Source | Type | Free |
|--------|------|------|
| BBC World, Tech, Science, Business, Health | RSS | ✅ |
| Reuters World, Business, Tech | RSS | ✅ |
| AP News (Top, Tech, Business, Sports) | RSS | ✅ |
| Al Jazeera | RSS | ✅ |
| France24, DW (Deutsche Welle) | RSS | ✅ |
| TechCrunch, Ars Technica, The Verge, Wired | RSS | ✅ |
| Nature, Science Daily, NPR | RSS | ✅ |
| The Guardian | API | Free key |
| New York Times | API | Free key |
| NewsAPI.org | API | Free key |
| GNews | API | Free key |

### 🇮🇳 National (India)
| Source | Language |
|--------|----------|
| Times of India (General, Tech, Business, Sports) | English |
| NDTV, NDTV Tech | English |
| The Hindu (National, Business, Science) | English |
| India Today, Hindustan Times, Indian Express | English |
| LiveMint, Economic Times, Business Standard | English |
| Dainik Jagran, Amar Ujala, Navbharat Times, Bhaskar | **Hindi** |

### 📍 State (India) — 30+ Sources
| State | Sources |
|-------|---------|
| Telangana / AP | Eenadu (Telugu), Sakshi, Andhra Jyothy |
| Tamil Nadu | Dinamalar, Dinamani (Tamil), The Hindu Chennai |
| Kerala | Mathrubhumi, Malayala Manorama (Malayalam) |
| Karnataka | Prajavani, Vijaya Karnataka (Kannada) |
| Maharashtra | Lokmat, Maharashtra Times (Marathi), HT Mumbai |
| Punjab / Haryana | Punjab Kesari (Hindi), Tribune Chandigarh |
| Gujarat | Gujarat Samachar, Sandesh (Gujarati) |
| West Bengal | Anandabazar Patrika (Bengali), The Telegraph |
| Delhi NCR | Delhi Times, HT Delhi |
| + Rajasthan, MP, Assam, J&K, Bihar, UP, Odisha |

### ⚽ Sports
| Sport | Sources |
|-------|---------|
| 🏏 Cricket | Cricbuzz, ESPNcricinfo, NDTV Sports |
| ⚽ Football | Goal.com, Sky Sports, ESPN FC, BBC Football, Guardian |
| 🏎️ Formula 1 | Formula1.com official, Autosport |
| 🎾 Tennis | ATP Tour official, BBC Sport Tennis |
| 🏀 Basketball | NBA official, ESPN NBA |
| 🏅 Olympics | olympics.com |
| General | ESPN, Sky Sports, Reuters Sports, AP Sports, Sportskeeda |

### ⛅ Weather
- **OpenWeatherMap** — current weather, 5-day forecast, air quality index
- 20 Indian cities preset + any city/coordinates
- Dynamic weather card with condition-based gradients

---

## 🏗️ Architecture

```
Flutter App (Android/iOS)
        │ HTTPS/WSS
   Nginx (reverse proxy + TLS)
        │
   C++20 Drogon Backend (port 8080)
   ┌────────────────────────────────┐
   │  Auth  │  News  │  AI  │  WS  │
   └──┬─────┴───┬────┴──┬───┴──────┘
      │         │       │
   PostgreSQL  Redis  Kafka
   Elasticsearch  ClickHouse  MinIO
```

---





### 2. Access
| Service | URL |
|---------|-----|
| Backend API | http://localhost:8080 |
| Elasticsearch | http://localhost:9200 |
| MinIO Console | http://localhost:9002 |
| Redis | localhost:6379 |


```

---



```


```

---

## 📁 Project Structure

```
ai-news-platform/
├── database/
│   ├── schema.sql              # 18 tables, types, RBAC seed
│   ├── indexes.sql             # 40+ composite/GIN/partial indexes
│   ├── stored_procedures.sql   # Personalized feed, trending, search
│   └── triggers.sql            # Auto timestamps, streaks, counts
├── backend/
│   ├── CMakeLists.txt
│   ├── Dockerfile
│   ├── docker-compose.yml
│   └── src/
│       ├── main.cpp
│       ├── config/app_config.hpp
│       ├── core/
│       │   ├── auth/           # JWT, OAuth, AuthController
│       │   ├── news/           # NewsFetcher (5 tiers), NewsController
│       │   ├── ai/             # AIService (GPT-4o), ChatController
│       │   └── weather/        # WeatherService, WeatherController
│       ├── infrastructure/
│       │   ├── redis_client.hpp
│       │   ├── kafka_producer.hpp
│       │   └── elasticsearch_client.hpp
│       └── api/middleware/
│           ├── auth_middleware.hpp
│           └── rate_limiter.hpp
├── flutter_app/
│   ├── pubspec.yaml
│   └── lib/
│       ├── main.dart
│       ├── core/
│       │   ├── theme/app_theme.dart   # Material 3 dark+light
│       │   └── network/api_client.dart
│       └── features/
│           ├── home/                  # Dashboard (4 scope tabs)
│           │   └── widgets/
│           │       ├── news_card.dart
│           │       └── weather_widget.dart
│           ├── news/                  # Detail, AI analysis
│           ├── sports/                # Sports tab, ticker
│           ├── weather/               # Full forecast screen
│           ├── search/                # Voice + semantic search
│           ├── ai_chat/               # Streaming AI chat
│           ├── audio/                 # Podcast player
│           └── auth/                  # Login / register
└── .github/workflows/ci_cd.yml        # Full CI/CD pipeline
```

---

## 🔧 Tech Stack

| Layer | Technology |
|-------|-----------|
| Mobile | Flutter 3.22 (Android + iOS), Material 3 |
| Backend | C++20, Drogon Framework |
| Database | PostgreSQL 16 (partitioned, full-text search) |
| Cache | Redis 7.2 (LRU, pub/sub) |
| Search | Elasticsearch 8.12 |
| Events | Apache Kafka 7.6 |
| Analytics | ClickHouse 24 |
| Storage | MinIO |
| AI | OpenAI GPT-4o (summarization, chat, TTS) |
| Auth | JWT (HS256), Google OAuth, Apple Sign In |
| DevOps | Docker, Kubernetes, GitHub Actions |

---

## 📄 License
MIT License — Free to use for academic and commercial projects.
