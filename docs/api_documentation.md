# AI News Platform — REST & WebSocket API Reference

The AI News Platform backend exposes a modern, high-performance C++ REST and WebSocket API built with the Drogon Framework.

---

## 🔐 Authentication Endpoints

### 1. Register User
`POST /api/v1/auth/register`

- **Headers:** `Content-Type: application/json`
- **Body:**
```json
{
  "email": "user@example.com",
  "username": "johndoe",
  "password": "StrongPassword123"
}
```
- **Response (200 OK):**
```json
{
  "success": true,
  "data": {
    "access_token": "eyJhbGci...",
    "refresh_token": "eyJhbGci...",
    "user_id": "9b1deb4d-3b7d-4bad-9bdd-2b0d7b3dcb6d",
    "username": "johndoe"
  }
}
```

### 2. Login User
`POST /api/v1/auth/login`

- **Body:**
```json
{
  "email": "user@example.com",
  "password": "StrongPassword123"
}
```
- **Response (200 OK):** Same structure as Register.

### 3. Guest Login
`POST /api/v1/auth/guest`

- **Response (200 OK):**
```json
{
  "success": true,
  "data": {
    "access_token": "eyJhbGci...",
    "user_id": "guest_1719918239",
    "role": "guest",
    "expires_in": 3600
  }
}
```

### 4. Refresh Token
`POST /api/v1/auth/refresh`

- **Body:**
```json
{
  "refresh_token": "eyJhbGci..."
}
```

---

## 📰 News Endpoints

### 1. Personalized News Feed
`GET /api/v1/news/feed`

- **Parameters:**
  - `category` (optional): `technology`, `business`, `sports`, etc.
  - `country` (optional, default: `in`): `in`, `us`, etc.
  - `limit` (optional, default: 30)
  - `page` (optional, default: 1)
- **Response (200 OK):**
```json
{
  "success": true,
  "data": [
    {
      "id": "external_id_123",
      "title": "Real-time AI Ingestion Engine Built in C++",
      "excerpt": "Architects design event-driven news summarizer...",
      "url": "https://example.com/news/ai-engine",
      "image_url": "https://example.com/images/ai.jpg",
      "publisher": "Tech News",
      "category": "technology",
      "published_at": "2026-07-06T12:00:00Z"
    }
  ]
}
```

### 2. State-Level News
`GET /api/v1/news/state`

- **Parameters:**
  - `state` (required): `Maharashtra`, `Tamil Nadu`, `Karnataka`, etc.
  - `language` (optional): `mr`, `ta`, `kn`
- **Response (200 OK):** Regional state articles feed.

### 3. Sports News
`GET /api/v1/news/sports`

- **Parameters:**
  - `sport` (optional): `cricket`, `football`, `tennis`, `nba`

### 4. Search (Elasticsearch Semantic)
`GET /api/v1/news/search`

- **Parameters:**
  - `q` (required): Search keywords
  - `category` (optional)
  - `scope` (optional): `international`, `national`, `state`

---

## ⛅ Weather Endpoints

### 1. Current Weather
`GET /api/v1/weather/current`

- **Parameters:** (Provide either `city` or `lat` + `lon`)
  - `city` / `country`
  - `lat` / `lon`
- **Response (200 OK):**
```json
{
  "success": true,
  "data": {
    "city": "Mumbai",
    "temp": 29.5,
    "humidity": 82,
    "description": "light intensity drizzle"
  }
}
```

---

## 🤖 AI Endpoints (OpenAI GPT-4o)

### 1. Streaming Chat
`POST /api/v1/ai/chat`

- **Headers:** `Authorization: Bearer <TOKEN>`
- **Body:**
```json
{
  "message": "Explain the business implications of this rate cut",
  "article_context": "The central bank cut rates by 25 bps...",
  "history": []
}
```
- **Response:** Async stream reader chunk sequence.

### 2. TTS audio summary
`POST /api/v1/ai/tts`

- **Body:** `{"text": "Text to read...", "voice": "alloy"}`
- **Response:** `audio/mpeg` raw binary stream.

---

## 📡 WebSockets Live Stream
`WSS /api/v1/ws`

Connect to receive live breaking news alerts, sports scores, and system announcements instantly.
- **Parameters:** `user_id` (optional query parameter)
