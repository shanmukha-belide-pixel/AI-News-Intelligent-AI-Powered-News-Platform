-- =============================================================================
-- AI News Platform - Indexes
-- =============================================================================

-- ──────────────────────────────────────────────
-- USERS
-- ──────────────────────────────────────────────
CREATE INDEX idx_users_email           ON users (email) WHERE is_deleted = FALSE;
CREATE INDEX idx_users_role            ON users (role) WHERE is_active = TRUE;
CREATE INDEX idx_users_provider        ON users (auth_provider, provider_id);
CREATE INDEX idx_users_created_at      ON users (created_at DESC);

-- ──────────────────────────────────────────────
-- USER SESSIONS
-- ──────────────────────────────────────────────
CREATE INDEX idx_sessions_user_id      ON user_sessions (user_id) WHERE is_active = TRUE;
CREATE INDEX idx_sessions_expires_at   ON user_sessions (expires_at) WHERE is_active = TRUE;
CREATE INDEX idx_sessions_device       ON user_sessions (user_id, device_id);

-- ──────────────────────────────────────────────
-- PROFILES
-- ──────────────────────────────────────────────
CREATE INDEX idx_profiles_user_id      ON profiles (user_id);
CREATE INDEX idx_profiles_language     ON profiles (language);

-- ──────────────────────────────────────────────
-- NEWS ARTICLES  (most query-critical table)
-- ──────────────────────────────────────────────
-- Full-text search vector  (GIN for fast @@ queries)
CREATE INDEX idx_articles_search       ON news_articles USING GIN (search_vector);

-- Category + status  (feed queries)
CREATE INDEX idx_articles_cat_status   ON news_articles (category_id, status, published_at DESC)
    WHERE status = 'published';

-- Published timeline
CREATE INDEX idx_articles_published    ON news_articles (published_at DESC)
    WHERE status = 'published';

-- Breaking news banner
CREATE INDEX idx_articles_breaking     ON news_articles (published_at DESC)
    WHERE is_breaking = TRUE AND status = 'published';

-- Tags GIN index (ANY tag search)
CREATE INDEX idx_articles_tags         ON news_articles USING GIN (tags);

-- Publisher lookups
CREATE INDEX idx_articles_publisher    ON news_articles (publisher, published_at DESC);

-- Language + country (personalised feed)
CREATE INDEX idx_articles_lang_country ON news_articles (language, country, published_at DESC);

-- Popularity sort
CREATE INDEX idx_articles_view_count   ON news_articles (view_count DESC)
    WHERE status = 'published';

-- Slug lookup
CREATE UNIQUE INDEX idx_articles_slug  ON news_articles (slug) WHERE slug IS NOT NULL;

-- ──────────────────────────────────────────────
-- AI SUMMARIES
-- ──────────────────────────────────────────────
CREATE INDEX idx_summaries_article     ON ai_summaries (article_id, summary_type);

-- ──────────────────────────────────────────────
-- BOOKMARKS
-- ──────────────────────────────────────────────
CREATE INDEX idx_bookmarks_user        ON bookmarks (user_id, created_at DESC);
CREATE INDEX idx_bookmarks_article     ON bookmarks (article_id);
CREATE INDEX idx_bookmarks_collection  ON bookmarks (user_id, collection_name);

-- ──────────────────────────────────────────────
-- READING HISTORY  (partitioned)
-- ──────────────────────────────────────────────
CREATE INDEX idx_reading_user_time     ON reading_history (user_id, read_at DESC);
CREATE INDEX idx_reading_article       ON reading_history (article_id);
CREATE INDEX idx_reading_completed     ON reading_history (user_id, completed) WHERE completed = TRUE;

-- ──────────────────────────────────────────────
-- LISTENING HISTORY  (partitioned)
-- ──────────────────────────────────────────────
CREATE INDEX idx_listening_user        ON listening_history (user_id, listened_at DESC);
CREATE INDEX idx_listening_article     ON listening_history (article_id);

-- ──────────────────────────────────────────────
-- COMMENTS
-- ──────────────────────────────────────────────
CREATE INDEX idx_comments_article      ON comments (article_id, created_at DESC) WHERE is_hidden = FALSE;
CREATE INDEX idx_comments_user         ON comments (user_id, created_at DESC);
CREATE INDEX idx_comments_parent       ON comments (parent_id) WHERE parent_id IS NOT NULL;

-- ──────────────────────────────────────────────
-- NOTIFICATIONS
-- ──────────────────────────────────────────────
CREATE INDEX idx_notif_user_unread     ON notifications (user_id, created_at DESC) WHERE is_read = FALSE;
CREATE INDEX idx_notif_user_all        ON notifications (user_id, created_at DESC);
CREATE INDEX idx_notif_expires         ON notifications (expires_at) WHERE expires_at IS NOT NULL;

-- ──────────────────────────────────────────────
-- TRENDING TOPICS
-- ──────────────────────────────────────────────
CREATE INDEX idx_trending_score        ON trending_topics (trend_score DESC, period_end DESC)
    WHERE is_active = TRUE;
CREATE INDEX idx_trending_country      ON trending_topics (country, trend_score DESC)
    WHERE is_active = TRUE;
CREATE INDEX idx_trending_keyword_trgm ON trending_topics USING GIN (keyword gin_trgm_ops);

-- ──────────────────────────────────────────────
-- SEARCH HISTORY
-- ──────────────────────────────────────────────
CREATE INDEX idx_search_user           ON search_history (user_id, searched_at DESC)
    WHERE user_id IS NOT NULL;
CREATE INDEX idx_search_query_trgm     ON search_history USING GIN (query gin_trgm_ops);

-- ──────────────────────────────────────────────
-- ANALYTICS EVENTS  (partitioned)
-- ──────────────────────────────────────────────
CREATE INDEX idx_analytics_user_time   ON analytics_events (user_id, occurred_at DESC)
    WHERE user_id IS NOT NULL;
CREATE INDEX idx_analytics_event_type  ON analytics_events (event_type, occurred_at DESC);
CREATE INDEX idx_analytics_entity      ON analytics_events (entity_type, entity_id)
    WHERE entity_id IS NOT NULL;

-- ──────────────────────────────────────────────
-- SUBSCRIPTIONS
-- ──────────────────────────────────────────────
CREATE INDEX idx_subs_user_status      ON subscriptions (user_id, status);
CREATE INDEX idx_subs_expires          ON subscriptions (expires_at) WHERE status = 'active';

-- ──────────────────────────────────────────────
-- AUDIT LOGS  (partitioned)
-- ──────────────────────────────────────────────
CREATE INDEX idx_audit_user            ON audit_logs (user_id, created_at DESC)
    WHERE user_id IS NOT NULL;
CREATE INDEX idx_audit_action          ON audit_logs (action, created_at DESC);
CREATE INDEX idx_audit_table           ON audit_logs (table_name, record_id);
