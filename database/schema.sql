-- =============================================================================
-- AI News Platform - PostgreSQL Database Schema
-- Version: 1.0.0  |  Requires PostgreSQL 15+
-- =============================================================================

-- Extensions
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";
CREATE EXTENSION IF NOT EXISTS "pgcrypto";
CREATE EXTENSION IF NOT EXISTS "pg_trgm";
CREATE EXTENSION IF NOT EXISTS "unaccent";
CREATE EXTENSION IF NOT EXISTS "btree_gin";
CREATE EXTENSION IF NOT EXISTS "pg_stat_statements";

-- =============================================================================
-- CUSTOM TYPES & DOMAINS
-- =============================================================================

CREATE TYPE user_role AS ENUM ('guest', 'user', 'premium', 'admin', 'super_admin');
CREATE TYPE auth_provider AS ENUM ('email', 'google', 'apple', 'guest');
CREATE TYPE article_status AS ENUM ('draft', 'published', 'archived', 'flagged');
CREATE TYPE summary_type AS ENUM ('one_line', 'thirty_sec', 'one_min', 'detailed');
CREATE TYPE sentiment AS ENUM ('positive', 'negative', 'neutral', 'mixed');
CREATE TYPE notification_type AS ENUM ('breaking_news', 'personalized', 'system', 'comment', 'subscription');
CREATE TYPE subscription_plan AS ENUM ('free', 'basic', 'premium', 'enterprise');
CREATE TYPE subscription_status AS ENUM ('active', 'cancelled', 'expired', 'trial');
CREATE TYPE audio_status AS ENUM ('pending', 'processing', 'ready', 'failed');
CREATE TYPE event_action AS ENUM ('create', 'read', 'update', 'delete', 'login', 'logout', 'share', 'bookmark', 'like');

-- =============================================================================
-- TABLE 1: USERS
-- =============================================================================

CREATE TABLE users (
    id                  UUID            PRIMARY KEY DEFAULT uuid_generate_v4(),
    email               VARCHAR(255)    UNIQUE,
    phone               VARCHAR(20)     UNIQUE,
    username            VARCHAR(50)     UNIQUE NOT NULL,
    password_hash       TEXT,
    auth_provider       auth_provider   NOT NULL DEFAULT 'email',
    provider_id         VARCHAR(255),
    role                user_role       NOT NULL DEFAULT 'user',
    is_verified         BOOLEAN         NOT NULL DEFAULT FALSE,
    is_active           BOOLEAN         NOT NULL DEFAULT TRUE,
    is_deleted          BOOLEAN         NOT NULL DEFAULT FALSE,
    email_verified_at   TIMESTAMPTZ,
    last_login_at       TIMESTAMPTZ,
    failed_login_count  SMALLINT        NOT NULL DEFAULT 0,
    locked_until        TIMESTAMPTZ,
    created_at          TIMESTAMPTZ     NOT NULL DEFAULT NOW(),
    updated_at          TIMESTAMPTZ     NOT NULL DEFAULT NOW(),
    deleted_at          TIMESTAMPTZ,
    CONSTRAINT chk_auth_provider CHECK (
        (auth_provider = 'email' AND email IS NOT NULL) OR
        (auth_provider != 'email' AND provider_id IS NOT NULL)
    )
);

-- =============================================================================
-- TABLE 2: USER SESSIONS
-- =============================================================================

CREATE TABLE user_sessions (
    id              UUID        PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id         UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    refresh_token   TEXT        NOT NULL UNIQUE,
    access_jti      UUID        NOT NULL UNIQUE DEFAULT uuid_generate_v4(),
    device_id       VARCHAR(255),
    device_name     VARCHAR(100),
    device_type     VARCHAR(50),
    ip_address      INET,
    user_agent      TEXT,
    is_active       BOOLEAN     NOT NULL DEFAULT TRUE,
    expires_at      TIMESTAMPTZ NOT NULL,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    last_used_at    TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- =============================================================================
-- TABLE 3: PROFILES
-- =============================================================================

CREATE TABLE profiles (
    id              UUID        PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id         UUID        NOT NULL UNIQUE REFERENCES users(id) ON DELETE CASCADE,
    display_name    VARCHAR(100),
    bio             TEXT,
    avatar_url      TEXT,
    cover_url       TEXT,
    website_url     VARCHAR(255),
    location        VARCHAR(100),
    birth_date      DATE,
    gender          VARCHAR(20),
    language        VARCHAR(10) NOT NULL DEFAULT 'en',
    timezone        VARCHAR(50) NOT NULL DEFAULT 'UTC',
    reading_streak  INTEGER     NOT NULL DEFAULT 0,
    longest_streak  INTEGER     NOT NULL DEFAULT 0,
    total_read      INTEGER     NOT NULL DEFAULT 0,
    total_listened  INTEGER     NOT NULL DEFAULT 0,
    listen_minutes  INTEGER     NOT NULL DEFAULT 0,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- =============================================================================
-- TABLE 4: PREFERENCES
-- =============================================================================

CREATE TABLE preferences (
    id                      UUID        PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id                 UUID        NOT NULL UNIQUE REFERENCES users(id) ON DELETE CASCADE,
    preferred_categories    TEXT[]      NOT NULL DEFAULT '{}',
    preferred_publishers    TEXT[]      NOT NULL DEFAULT '{}',
    preferred_language      VARCHAR(10) NOT NULL DEFAULT 'en',
    preferred_country       CHAR(2)     NOT NULL DEFAULT 'US',
    summary_length          summary_type NOT NULL DEFAULT 'thirty_sec',
    dark_mode               BOOLEAN     NOT NULL DEFAULT TRUE,
    push_notifications      BOOLEAN     NOT NULL DEFAULT TRUE,
    email_digest            BOOLEAN     NOT NULL DEFAULT TRUE,
    digest_frequency        VARCHAR(20) NOT NULL DEFAULT 'daily',
    autoplay_audio          BOOLEAN     NOT NULL DEFAULT FALSE,
    playback_speed          NUMERIC(3,1) NOT NULL DEFAULT 1.0,
    font_size               VARCHAR(10) NOT NULL DEFAULT 'medium',
    reduce_motion           BOOLEAN     NOT NULL DEFAULT FALSE,
    data_saver_mode         BOOLEAN     NOT NULL DEFAULT FALSE,
    created_at              TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at              TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- =============================================================================
-- TABLE 5: SUBSCRIPTIONS
-- =============================================================================

CREATE TABLE subscriptions (
    id                  UUID                PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id             UUID                NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    plan                subscription_plan   NOT NULL DEFAULT 'free',
    status              subscription_status NOT NULL DEFAULT 'trial',
    started_at          TIMESTAMPTZ         NOT NULL DEFAULT NOW(),
    expires_at          TIMESTAMPTZ,
    trial_ends_at       TIMESTAMPTZ,
    cancelled_at        TIMESTAMPTZ,
    payment_provider    VARCHAR(50),
    payment_reference   VARCHAR(255),
    amount_cents        INTEGER,
    currency            CHAR(3)             NOT NULL DEFAULT 'USD',
    auto_renew          BOOLEAN             NOT NULL DEFAULT TRUE,
    created_at          TIMESTAMPTZ         NOT NULL DEFAULT NOW(),
    updated_at          TIMESTAMPTZ         NOT NULL DEFAULT NOW()
);

-- =============================================================================
-- TABLE 6: CATEGORIES
-- =============================================================================

CREATE TABLE categories (
    id              SERIAL          PRIMARY KEY,
    name            VARCHAR(100)    NOT NULL UNIQUE,
    slug            VARCHAR(100)    NOT NULL UNIQUE,
    description     TEXT,
    icon_url        TEXT,
    color_hex       CHAR(7),
    parent_id       INTEGER         REFERENCES categories(id) ON DELETE SET NULL,
    sort_order      INTEGER         NOT NULL DEFAULT 0,
    is_active       BOOLEAN         NOT NULL DEFAULT TRUE,
    article_count   INTEGER         NOT NULL DEFAULT 0,
    created_at      TIMESTAMPTZ     NOT NULL DEFAULT NOW()
);

-- =============================================================================
-- TABLE 7: NEWS ARTICLES
-- =============================================================================

CREATE TABLE news_articles (
    id                  UUID            PRIMARY KEY DEFAULT uuid_generate_v4(),
    external_id         VARCHAR(255)    UNIQUE,
    title               TEXT            NOT NULL,
    slug                VARCHAR(300)    UNIQUE,
    content             TEXT,
    excerpt             TEXT,
    url                 TEXT            NOT NULL UNIQUE,
    image_url           TEXT,
    image_thumbnail_url TEXT,
    audio_url           TEXT,
    audio_status        audio_status    NOT NULL DEFAULT 'pending',
    author              VARCHAR(200),
    publisher           VARCHAR(200)    NOT NULL,
    publisher_icon      TEXT,
    category_id         INTEGER         REFERENCES categories(id) ON DELETE SET NULL,
    tags                TEXT[]          NOT NULL DEFAULT '{}',
    language            VARCHAR(10)     NOT NULL DEFAULT 'en',
    country             CHAR(2),
    status              article_status  NOT NULL DEFAULT 'published',
    sentiment           sentiment,
    bias_score          NUMERIC(4,3),
    credibility_score   NUMERIC(4,3),
    reading_time_min    SMALLINT,
    view_count          INTEGER         NOT NULL DEFAULT 0,
    like_count          INTEGER         NOT NULL DEFAULT 0,
    share_count         INTEGER         NOT NULL DEFAULT 0,
    bookmark_count      INTEGER         NOT NULL DEFAULT 0,
    comment_count       INTEGER         NOT NULL DEFAULT 0,
    is_breaking         BOOLEAN         NOT NULL DEFAULT FALSE,
    is_featured         BOOLEAN         NOT NULL DEFAULT FALSE,
    published_at        TIMESTAMPTZ,
    fetched_at          TIMESTAMPTZ     NOT NULL DEFAULT NOW(),
    created_at          TIMESTAMPTZ     NOT NULL DEFAULT NOW(),
    updated_at          TIMESTAMPTZ     NOT NULL DEFAULT NOW(),
    search_vector       TSVECTOR,
    CONSTRAINT chk_bias_score CHECK (bias_score BETWEEN -1 AND 1),
    CONSTRAINT chk_credibility CHECK (credibility_score BETWEEN 0 AND 1)
);

-- =============================================================================
-- TABLE 8: AI SUMMARIES
-- =============================================================================

CREATE TABLE ai_summaries (
    id              UUID            PRIMARY KEY DEFAULT uuid_generate_v4(),
    article_id      UUID            NOT NULL REFERENCES news_articles(id) ON DELETE CASCADE,
    summary_type    summary_type    NOT NULL,
    content         TEXT            NOT NULL,
    key_points      JSONB           NOT NULL DEFAULT '[]',
    timeline        JSONB           NOT NULL DEFAULT '[]',
    pros            JSONB           NOT NULL DEFAULT '[]',
    cons            JSONB           NOT NULL DEFAULT '[]',
    important_facts JSONB           NOT NULL DEFAULT '[]',
    why_it_matters  TEXT,
    sentiment       sentiment,
    sentiment_score NUMERIC(4,3),
    model_used      VARCHAR(50)     NOT NULL DEFAULT 'gpt-4o',
    token_count     INTEGER,
    generation_ms   INTEGER,
    created_at      TIMESTAMPTZ     NOT NULL DEFAULT NOW(),
    UNIQUE (article_id, summary_type)
);

-- =============================================================================
-- TABLE 9: BOOKMARKS
-- =============================================================================

CREATE TABLE bookmarks (
    id              UUID        PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id         UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    article_id      UUID        NOT NULL REFERENCES news_articles(id) ON DELETE CASCADE,
    collection_name VARCHAR(100) NOT NULL DEFAULT 'Default',
    notes           TEXT,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (user_id, article_id)
);

-- =============================================================================
-- TABLE 10: READING HISTORY
-- =============================================================================

CREATE TABLE reading_history (
    id              UUID        PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id         UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    article_id      UUID        NOT NULL REFERENCES news_articles(id) ON DELETE CASCADE,
    progress        NUMERIC(5,2) NOT NULL DEFAULT 0 CHECK (progress BETWEEN 0 AND 100),
    read_seconds    INTEGER     NOT NULL DEFAULT 0,
    completed       BOOLEAN     NOT NULL DEFAULT FALSE,
    liked           BOOLEAN     NOT NULL DEFAULT FALSE,
    shared          BOOLEAN     NOT NULL DEFAULT FALSE,
    read_at         TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
) PARTITION BY RANGE (read_at);

-- Create monthly partitions for current year + next year
CREATE TABLE reading_history_2026_q1 PARTITION OF reading_history
    FOR VALUES FROM ('2026-01-01') TO ('2026-04-01');
CREATE TABLE reading_history_2026_q2 PARTITION OF reading_history
    FOR VALUES FROM ('2026-04-01') TO ('2026-07-01');
CREATE TABLE reading_history_2026_q3 PARTITION OF reading_history
    FOR VALUES FROM ('2026-07-01') TO ('2026-10-01');
CREATE TABLE reading_history_2026_q4 PARTITION OF reading_history
    FOR VALUES FROM ('2026-10-01') TO ('2027-01-01');
CREATE TABLE reading_history_2027 PARTITION OF reading_history
    FOR VALUES FROM ('2027-01-01') TO ('2028-01-01');
CREATE TABLE reading_history_default PARTITION OF reading_history DEFAULT;

-- =============================================================================
-- TABLE 11: LISTENING HISTORY
-- =============================================================================

CREATE TABLE listening_history (
    id              UUID        PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id         UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    article_id      UUID        NOT NULL REFERENCES news_articles(id) ON DELETE CASCADE,
    progress_sec    INTEGER     NOT NULL DEFAULT 0,
    duration_sec    INTEGER     NOT NULL DEFAULT 0,
    completed       BOOLEAN     NOT NULL DEFAULT FALSE,
    playback_speed  NUMERIC(3,1) NOT NULL DEFAULT 1.0,
    listened_at     TIMESTAMPTZ NOT NULL DEFAULT NOW()
) PARTITION BY RANGE (listened_at);

CREATE TABLE listening_history_2026 PARTITION OF listening_history
    FOR VALUES FROM ('2026-01-01') TO ('2027-01-01');
CREATE TABLE listening_history_2027 PARTITION OF listening_history
    FOR VALUES FROM ('2027-01-01') TO ('2028-01-01');
CREATE TABLE listening_history_default PARTITION OF listening_history DEFAULT;

-- =============================================================================
-- TABLE 12: COMMENTS
-- =============================================================================

CREATE TABLE comments (
    id              UUID        PRIMARY KEY DEFAULT uuid_generate_v4(),
    article_id      UUID        NOT NULL REFERENCES news_articles(id) ON DELETE CASCADE,
    user_id         UUID        NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    parent_id       UUID        REFERENCES comments(id) ON DELETE CASCADE,
    content         TEXT        NOT NULL CHECK (length(content) BETWEEN 1 AND 2000),
    like_count      INTEGER     NOT NULL DEFAULT 0,
    is_pinned       BOOLEAN     NOT NULL DEFAULT FALSE,
    is_hidden       BOOLEAN     NOT NULL DEFAULT FALSE,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- =============================================================================
-- TABLE 13: NOTIFICATIONS
-- =============================================================================

CREATE TABLE notifications (
    id              UUID                PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id         UUID                NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    type            notification_type   NOT NULL,
    title           VARCHAR(200)        NOT NULL,
    body            TEXT                NOT NULL,
    image_url       TEXT,
    action_url      TEXT,
    metadata        JSONB               NOT NULL DEFAULT '{}',
    is_read         BOOLEAN             NOT NULL DEFAULT FALSE,
    is_sent         BOOLEAN             NOT NULL DEFAULT FALSE,
    sent_at         TIMESTAMPTZ,
    read_at         TIMESTAMPTZ,
    expires_at      TIMESTAMPTZ,
    created_at      TIMESTAMPTZ         NOT NULL DEFAULT NOW()
);

-- =============================================================================
-- TABLE 14: TRENDING TOPICS
-- =============================================================================

CREATE TABLE trending_topics (
    id              SERIAL      PRIMARY KEY,
    keyword         VARCHAR(200) NOT NULL,
    slug            VARCHAR(200) NOT NULL UNIQUE,
    category_id     INTEGER     REFERENCES categories(id) ON DELETE SET NULL,
    article_count   INTEGER     NOT NULL DEFAULT 0,
    search_count    INTEGER     NOT NULL DEFAULT 0,
    trend_score     NUMERIC(10,4) NOT NULL DEFAULT 0,
    velocity        NUMERIC(10,4) NOT NULL DEFAULT 0,
    country         CHAR(2),
    language        VARCHAR(10) NOT NULL DEFAULT 'en',
    period_start    TIMESTAMPTZ NOT NULL,
    period_end      TIMESTAMPTZ NOT NULL,
    is_active       BOOLEAN     NOT NULL DEFAULT TRUE,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- =============================================================================
-- TABLE 15: SEARCH HISTORY
-- =============================================================================

CREATE TABLE search_history (
    id              UUID        PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id         UUID        REFERENCES users(id) ON DELETE CASCADE,
    session_id      VARCHAR(100),
    query           VARCHAR(500) NOT NULL,
    filters         JSONB       NOT NULL DEFAULT '{}',
    result_count    INTEGER,
    clicked_article UUID        REFERENCES news_articles(id) ON DELETE SET NULL,
    search_type     VARCHAR(20) NOT NULL DEFAULT 'keyword',
    searched_at     TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- =============================================================================
-- TABLE 16: ANALYTICS EVENTS
-- =============================================================================

CREATE TABLE analytics_events (
    id              UUID            PRIMARY KEY DEFAULT uuid_generate_v4(),
    user_id         UUID            REFERENCES users(id) ON DELETE SET NULL,
    session_id      VARCHAR(100),
    event_type      VARCHAR(50)     NOT NULL,
    entity_type     VARCHAR(50),
    entity_id       UUID,
    properties      JSONB           NOT NULL DEFAULT '{}',
    ip_address      INET,
    user_agent      TEXT,
    platform        VARCHAR(20),
    app_version     VARCHAR(20),
    occurred_at     TIMESTAMPTZ     NOT NULL DEFAULT NOW()
) PARTITION BY RANGE (occurred_at);

CREATE TABLE analytics_events_2026_q1 PARTITION OF analytics_events
    FOR VALUES FROM ('2026-01-01') TO ('2026-04-01');
CREATE TABLE analytics_events_2026_q2 PARTITION OF analytics_events
    FOR VALUES FROM ('2026-04-01') TO ('2026-07-01');
CREATE TABLE analytics_events_2026_q3 PARTITION OF analytics_events
    FOR VALUES FROM ('2026-07-01') TO ('2026-10-01');
CREATE TABLE analytics_events_2026_q4 PARTITION OF analytics_events
    FOR VALUES FROM ('2026-10-01') TO ('2027-01-01');
CREATE TABLE analytics_events_2027 PARTITION OF analytics_events
    FOR VALUES FROM ('2027-01-01') TO ('2028-01-01');
CREATE TABLE analytics_events_default PARTITION OF analytics_events DEFAULT;

-- =============================================================================
-- TABLE 17: AUDIT LOGS
-- =============================================================================

CREATE TABLE audit_logs (
    id              BIGSERIAL   PRIMARY KEY,
    user_id         UUID        REFERENCES users(id) ON DELETE SET NULL,
    action          event_action NOT NULL,
    table_name      VARCHAR(50),
    record_id       TEXT,
    old_values      JSONB,
    new_values      JSONB,
    ip_address      INET,
    user_agent      TEXT,
    status          VARCHAR(20) NOT NULL DEFAULT 'success',
    error_message   TEXT,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT NOW()
) PARTITION BY RANGE (created_at);

CREATE TABLE audit_logs_2026 PARTITION OF audit_logs
    FOR VALUES FROM ('2026-01-01') TO ('2027-01-01');
CREATE TABLE audit_logs_2027 PARTITION OF audit_logs
    FOR VALUES FROM ('2027-01-01') TO ('2028-01-01');
CREATE TABLE audit_logs_default PARTITION OF audit_logs DEFAULT;

-- =============================================================================
-- TABLE 18: USER ROLES (RBAC)
-- =============================================================================

CREATE TABLE user_role_permissions (
    id          SERIAL      PRIMARY KEY,
    role        user_role   NOT NULL,
    resource    VARCHAR(100) NOT NULL,
    action      VARCHAR(50) NOT NULL,
    is_allowed  BOOLEAN     NOT NULL DEFAULT TRUE,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (role, resource, action)
);

-- Seed default RBAC permissions
INSERT INTO user_role_permissions (role, resource, action) VALUES
    ('guest',       'articles',     'read'),
    ('guest',       'categories',   'read'),
    ('user',        'articles',     'read'),
    ('user',        'articles',     'bookmark'),
    ('user',        'articles',     'like'),
    ('user',        'articles',     'share'),
    ('user',        'comments',     'create'),
    ('user',        'comments',     'update'),
    ('user',        'comments',     'delete'),
    ('user',        'profile',      'update'),
    ('premium',     'articles',     'read'),
    ('premium',     'articles',     'listen'),
    ('premium',     'ai_chat',      'use'),
    ('premium',     'summaries',    'detailed'),
    ('admin',       'articles',     'manage'),
    ('admin',       'users',        'manage'),
    ('admin',       'analytics',    'view'),
    ('super_admin', 'all',          'all');

-- =============================================================================
-- SEED: CATEGORIES
-- =============================================================================

INSERT INTO categories (name, slug, description, color_hex, sort_order) VALUES
    ('World',           'world',        'Global news and international affairs',    '#4A90D9', 1),
    ('Technology',      'technology',   'Tech, science and innovation',             '#7B68EE', 2),
    ('Business',        'business',     'Markets, economy and finance',             '#2ECC71', 3),
    ('Politics',        'politics',     'Government and political developments',    '#E74C3C', 4),
    ('Sports',          'sports',       'Sports news and results',                  '#F39C12', 5),
    ('Entertainment',   'entertainment','Movies, music and celebrity',              '#E91E63', 6),
    ('Health',          'health',       'Medical and wellness news',                '#00BCD4', 7),
    ('Science',         'science',      'Scientific discoveries and research',      '#9C27B0', 8),
    ('Climate',         'climate',      'Environment and climate change',           '#4CAF50', 9),
    ('AI',              'ai',           'Artificial intelligence and machine learning','#FF5722', 10);
