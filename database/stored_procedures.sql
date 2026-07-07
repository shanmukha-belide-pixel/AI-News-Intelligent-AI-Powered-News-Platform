-- =============================================================================
-- AI News Platform - Stored Procedures & Functions
-- =============================================================================

-- ─────────────────────────────────────────────────────────────────────────────
-- HELPER: Update search_vector on news_articles
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION update_article_search_vector()
RETURNS TRIGGER LANGUAGE plpgsql AS $$
BEGIN
    NEW.search_vector :=
        setweight(to_tsvector('english', COALESCE(NEW.title, '')),       'A') ||
        setweight(to_tsvector('english', COALESCE(NEW.excerpt, '')),      'B') ||
        setweight(to_tsvector('english', COALESCE(NEW.author, '')),       'C') ||
        setweight(to_tsvector('english', COALESCE(NEW.publisher, '')),    'C') ||
        setweight(to_tsvector('english', COALESCE(array_to_string(NEW.tags, ' '), '')), 'B');
    RETURN NEW;
END;
$$;

-- ─────────────────────────────────────────────────────────────────────────────
-- FUNCTION: Get personalised news feed for a user
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION get_personalized_feed(
    p_user_id       UUID,
    p_limit         INTEGER DEFAULT 20,
    p_offset        INTEGER DEFAULT 0,
    p_language      VARCHAR DEFAULT 'en'
)
RETURNS TABLE (
    article_id      UUID,
    title           TEXT,
    excerpt         TEXT,
    image_url       TEXT,
    publisher       TEXT,
    category_name   VARCHAR,
    category_slug   VARCHAR,
    reading_time    SMALLINT,
    published_at    TIMESTAMPTZ,
    sentiment       sentiment,
    relevance_score FLOAT
) LANGUAGE plpgsql STABLE SECURITY DEFINER AS $$
DECLARE
    v_categories    TEXT[];
    v_publishers    TEXT[];
BEGIN
    -- Load user preferences
    SELECT preferred_categories, preferred_publishers
    INTO v_categories, v_publishers
    FROM preferences
    WHERE user_id = p_user_id;

    RETURN QUERY
    WITH scored_articles AS (
        SELECT
            a.id,
            a.title,
            a.excerpt,
            a.image_url,
            a.publisher,
            c.name  AS category_name,
            c.slug  AS category_slug,
            a.reading_time_min,
            a.published_at,
            a.sentiment,
            -- Relevance scoring
            (
                CASE WHEN c.slug = ANY(v_categories)         THEN 3.0 ELSE 0.0 END +
                CASE WHEN a.publisher = ANY(v_publishers)    THEN 2.0 ELSE 0.0 END +
                CASE WHEN a.is_breaking                      THEN 1.5 ELSE 0.0 END +
                CASE WHEN a.is_featured                      THEN 1.0 ELSE 0.0 END +
                -- Recency bonus (decays over 48 hours)
                GREATEST(0, 2.0 - EXTRACT(EPOCH FROM (NOW() - a.published_at)) / 86400.0) +
                -- Popularity bonus (log scale)
                LN(GREATEST(1, a.view_count)) * 0.1
            )::FLOAT AS relevance_score,
            -- Exclude already-read articles (last 7 days)
            EXISTS(
                SELECT 1 FROM reading_history rh
                WHERE rh.user_id = p_user_id
                  AND rh.article_id = a.id
                  AND rh.read_at > NOW() - INTERVAL '7 days'
            ) AS already_read
        FROM news_articles a
        LEFT JOIN categories c ON c.id = a.category_id
        WHERE a.status    = 'published'
          AND a.language  = p_language
          AND a.published_at > NOW() - INTERVAL '7 days'
    )
    SELECT
        sa.id,
        sa.title,
        sa.excerpt,
        sa.image_url,
        sa.publisher,
        sa.category_name,
        sa.category_slug,
        sa.reading_time_min,
        sa.published_at,
        sa.sentiment,
        sa.relevance_score
    FROM scored_articles sa
    WHERE NOT sa.already_read
    ORDER BY sa.relevance_score DESC, sa.published_at DESC
    LIMIT p_limit
    OFFSET p_offset;
END;
$$;

-- ─────────────────────────────────────────────────────────────────────────────
-- FUNCTION: Upsert reading history + update profile stats
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION upsert_reading_history(
    p_user_id       UUID,
    p_article_id    UUID,
    p_progress      NUMERIC,
    p_read_seconds  INTEGER,
    p_completed     BOOLEAN DEFAULT FALSE
)
RETURNS VOID LANGUAGE plpgsql AS $$
BEGIN
    INSERT INTO reading_history (user_id, article_id, progress, read_seconds, completed, read_at)
    VALUES (p_user_id, p_article_id, p_progress, p_read_seconds, p_completed, NOW())
    ON CONFLICT DO NOTHING;

    -- Bump article view count (best-effort)
    UPDATE news_articles
    SET view_count = view_count + 1
    WHERE id = p_article_id;

    -- Update profile reading stats
    IF p_completed THEN
        UPDATE profiles
        SET total_read = total_read + 1
        WHERE user_id = p_user_id;
    END IF;
END;
$$;

-- ─────────────────────────────────────────────────────────────────────────────
-- FUNCTION: Calculate trending topics (run via cron every 15 min)
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION calculate_trending_topics(
    p_window_hours  INTEGER DEFAULT 6
)
RETURNS INTEGER LANGUAGE plpgsql AS $$
DECLARE
    v_inserted INTEGER := 0;
    v_period_start TIMESTAMPTZ := NOW() - (p_window_hours || ' hours')::INTERVAL;
    v_period_end   TIMESTAMPTZ := NOW();
BEGIN
    -- Deactivate old trending records for same period
    UPDATE trending_topics
    SET is_active = FALSE
    WHERE period_end < NOW() - INTERVAL '1 hour';

    -- Insert new trending from search history + article tags
    WITH tag_counts AS (
        SELECT
            UNNEST(a.tags) AS keyword,
            a.category_id,
            COUNT(*)        AS article_count,
            SUM(a.view_count) AS total_views
        FROM news_articles a
        WHERE a.published_at BETWEEN v_period_start AND v_period_end
          AND a.status = 'published'
        GROUP BY UNNEST(a.tags), a.category_id
        HAVING COUNT(*) >= 2
    ),
    search_counts AS (
        SELECT
            LOWER(TRIM(query)) AS keyword,
            COUNT(*)           AS search_count
        FROM search_history
        WHERE searched_at BETWEEN v_period_start AND v_period_end
        GROUP BY LOWER(TRIM(query))
        HAVING COUNT(*) >= 3
    ),
    combined AS (
        SELECT
            tc.keyword,
            tc.category_id,
            COALESCE(tc.article_count, 0)          AS article_count,
            COALESCE(sc.search_count, 0)            AS search_count,
            (COALESCE(tc.total_views, 0)::FLOAT / GREATEST(1, EXTRACT(EPOCH FROM (v_period_end - v_period_start)) / 3600)) AS velocity
        FROM tag_counts tc
        LEFT JOIN search_counts sc ON sc.keyword = LOWER(tc.keyword)
    )
    INSERT INTO trending_topics
        (keyword, slug, category_id, article_count, search_count, trend_score, velocity, period_start, period_end)
    SELECT
        keyword,
        REGEXP_REPLACE(LOWER(keyword), '[^a-z0-9]+', '-', 'g'),
        category_id,
        article_count,
        search_count,
        (article_count * 1.5 + search_count * 2.0 + velocity * 0.5) AS trend_score,
        velocity,
        v_period_start,
        v_period_end
    FROM combined
    ON CONFLICT (slug) DO UPDATE
        SET trend_score  = EXCLUDED.trend_score,
            article_count = EXCLUDED.article_count,
            search_count  = EXCLUDED.search_count,
            velocity      = EXCLUDED.velocity,
            period_start  = EXCLUDED.period_start,
            period_end    = EXCLUDED.period_end,
            is_active     = TRUE,
            updated_at    = NOW();

    GET DIAGNOSTICS v_inserted = ROW_COUNT;
    RETURN v_inserted;
END;
$$;

-- ─────────────────────────────────────────────────────────────────────────────
-- FUNCTION: Get user reading streak
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION get_user_streak(p_user_id UUID)
RETURNS TABLE (current_streak INTEGER, longest_streak INTEGER)
LANGUAGE plpgsql STABLE AS $$
DECLARE
    v_current  INTEGER := 0;
    v_longest  INTEGER := 0;
    v_prev_day DATE    := NULL;
    v_day      DATE;
BEGIN
    FOR v_day IN
        SELECT DISTINCT DATE(read_at)
        FROM reading_history
        WHERE user_id = p_user_id
        ORDER BY 1 DESC
    LOOP
        IF v_prev_day IS NULL OR v_prev_day - v_day = 1 THEN
            v_current := v_current + 1;
            v_longest := GREATEST(v_longest, v_current);
        ELSE
            EXIT;
        END IF;
        v_prev_day := v_day;
    END LOOP;

    current_streak := v_current;
    longest_streak := v_longest;
    RETURN NEXT;
END;
$$;

-- ─────────────────────────────────────────────────────────────────────────────
-- FUNCTION: Semantic article search
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION search_articles(
    p_query         TEXT,
    p_category_slug VARCHAR   DEFAULT NULL,
    p_language      VARCHAR   DEFAULT 'en',
    p_from_date     TIMESTAMPTZ DEFAULT NULL,
    p_to_date       TIMESTAMPTZ DEFAULT NULL,
    p_limit         INTEGER   DEFAULT 20,
    p_offset        INTEGER   DEFAULT 0
)
RETURNS TABLE (
    article_id      UUID,
    title           TEXT,
    excerpt         TEXT,
    image_url       TEXT,
    publisher       TEXT,
    published_at    TIMESTAMPTZ,
    rank            FLOAT
) LANGUAGE plpgsql STABLE AS $$
BEGIN
    RETURN QUERY
    SELECT
        a.id,
        a.title,
        a.excerpt,
        a.image_url,
        a.publisher,
        a.published_at,
        ts_rank_cd(a.search_vector, query, 32)::FLOAT AS rank
    FROM news_articles a,
         to_tsquery('english', REGEXP_REPLACE(
             TRIM(p_query), '\s+', ':* & ', 'g') || ':*'
         ) query
    LEFT JOIN categories c ON c.id = a.category_id
    WHERE a.status     = 'published'
      AND a.language   = p_language
      AND a.search_vector @@ query
      AND (p_category_slug IS NULL OR c.slug = p_category_slug)
      AND (p_from_date IS NULL OR a.published_at >= p_from_date)
      AND (p_to_date   IS NULL OR a.published_at <= p_to_date)
    ORDER BY rank DESC, a.published_at DESC
    LIMIT  p_limit
    OFFSET p_offset;
END;
$$;

-- ─────────────────────────────────────────────────────────────────────────────
-- FUNCTION: Cleanup expired sessions
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION cleanup_expired_sessions()
RETURNS INTEGER LANGUAGE plpgsql AS $$
DECLARE v_count INTEGER;
BEGIN
    UPDATE user_sessions
    SET is_active = FALSE
    WHERE expires_at < NOW() AND is_active = TRUE;
    GET DIAGNOSTICS v_count = ROW_COUNT;
    RETURN v_count;
END;
$$;
