-- =============================================================================
-- AI News Platform - Triggers
-- =============================================================================

-- ─────────────────────────────────────────────────────────────────────────────
-- Generic updated_at trigger function
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION set_updated_at()
RETURNS TRIGGER LANGUAGE plpgsql AS $$
BEGIN
    NEW.updated_at := NOW();
    RETURN NEW;
END;
$$;

-- Apply to all tables with updated_at
CREATE TRIGGER trg_users_updated_at
    BEFORE UPDATE ON users
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

CREATE TRIGGER trg_profiles_updated_at
    BEFORE UPDATE ON profiles
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

CREATE TRIGGER trg_preferences_updated_at
    BEFORE UPDATE ON preferences
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

CREATE TRIGGER trg_articles_updated_at
    BEFORE UPDATE ON news_articles
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

CREATE TRIGGER trg_comments_updated_at
    BEFORE UPDATE ON comments
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

CREATE TRIGGER trg_trending_updated_at
    BEFORE UPDATE ON trending_topics
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

CREATE TRIGGER trg_subscriptions_updated_at
    BEFORE UPDATE ON subscriptions
    FOR EACH ROW EXECUTE FUNCTION set_updated_at();

-- ─────────────────────────────────────────────────────────────────────────────
-- Full-text search vector: auto-update on INSERT/UPDATE
-- ─────────────────────────────────────────────────────────────────────────────
CREATE TRIGGER trg_articles_search_vector
    BEFORE INSERT OR UPDATE OF title, excerpt, author, publisher, tags
    ON news_articles
    FOR EACH ROW EXECUTE FUNCTION update_article_search_vector();

-- ─────────────────────────────────────────────────────────────────────────────
-- Auto-create profile + preferences on new user
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION create_user_defaults()
RETURNS TRIGGER LANGUAGE plpgsql AS $$
BEGIN
    INSERT INTO profiles (user_id, display_name)
    VALUES (NEW.id, NEW.username);

    INSERT INTO preferences (user_id)
    VALUES (NEW.id);

    INSERT INTO subscriptions (user_id, plan, status, trial_ends_at)
    VALUES (NEW.id, 'free', 'trial', NOW() + INTERVAL '30 days');

    RETURN NEW;
END;
$$;

CREATE TRIGGER trg_user_create_defaults
    AFTER INSERT ON users
    FOR EACH ROW EXECUTE FUNCTION create_user_defaults();

-- ─────────────────────────────────────────────────────────────────────────────
-- Audit log: capture changes on users table
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION audit_users_changes()
RETURNS TRIGGER LANGUAGE plpgsql SECURITY DEFINER AS $$
BEGIN
    IF TG_OP = 'UPDATE' THEN
        INSERT INTO audit_logs (action, table_name, record_id, old_values, new_values)
        VALUES (
            'update',
            'users',
            OLD.id::TEXT,
            jsonb_build_object(
                'email', OLD.email, 'role', OLD.role, 'is_active', OLD.is_active
            ),
            jsonb_build_object(
                'email', NEW.email, 'role', NEW.role, 'is_active', NEW.is_active
            )
        );
    ELSIF TG_OP = 'DELETE' THEN
        INSERT INTO audit_logs (action, table_name, record_id, old_values)
        VALUES ('delete', 'users', OLD.id::TEXT,
            jsonb_build_object('email', OLD.email, 'username', OLD.username)
        );
    END IF;
    RETURN NEW;
END;
$$;

CREATE TRIGGER trg_audit_users
    AFTER UPDATE OR DELETE ON users
    FOR EACH ROW EXECUTE FUNCTION audit_users_changes();

-- ─────────────────────────────────────────────────────────────────────────────
-- Bookmark count sync on news_articles
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION sync_bookmark_count()
RETURNS TRIGGER LANGUAGE plpgsql AS $$
BEGIN
    IF TG_OP = 'INSERT' THEN
        UPDATE news_articles SET bookmark_count = bookmark_count + 1 WHERE id = NEW.article_id;
    ELSIF TG_OP = 'DELETE' THEN
        UPDATE news_articles SET bookmark_count = GREATEST(0, bookmark_count - 1) WHERE id = OLD.article_id;
    END IF;
    RETURN NULL;
END;
$$;

CREATE TRIGGER trg_bookmark_count
    AFTER INSERT OR DELETE ON bookmarks
    FOR EACH ROW EXECUTE FUNCTION sync_bookmark_count();

-- ─────────────────────────────────────────────────────────────────────────────
-- Comment count sync on news_articles
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION sync_comment_count()
RETURNS TRIGGER LANGUAGE plpgsql AS $$
BEGIN
    IF TG_OP = 'INSERT' THEN
        UPDATE news_articles SET comment_count = comment_count + 1 WHERE id = NEW.article_id;
    ELSIF TG_OP = 'DELETE' THEN
        UPDATE news_articles SET comment_count = GREATEST(0, comment_count - 1) WHERE id = OLD.article_id;
    END IF;
    RETURN NULL;
END;
$$;

CREATE TRIGGER trg_comment_count
    AFTER INSERT OR DELETE ON comments
    FOR EACH ROW EXECUTE FUNCTION sync_comment_count();

-- ─────────────────────────────────────────────────────────────────────────────
-- Reading streak auto-update
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION update_reading_streak()
RETURNS TRIGGER LANGUAGE plpgsql AS $$
DECLARE
    v_streak INTEGER;
    v_longest INTEGER;
BEGIN
    -- Only fire once per day per user
    IF EXISTS (
        SELECT 1 FROM reading_history
        WHERE user_id = NEW.user_id
          AND DATE(read_at) = CURRENT_DATE
          AND id != NEW.id
    ) THEN
        RETURN NEW;
    END IF;

    SELECT current_streak, longest_streak
    INTO v_streak, v_longest
    FROM get_user_streak(NEW.user_id);

    UPDATE profiles
    SET reading_streak = v_streak,
        longest_streak = GREATEST(longest_streak, v_longest)
    WHERE user_id = NEW.user_id;

    RETURN NEW;
END;
$$;

CREATE TRIGGER trg_reading_streak
    AFTER INSERT ON reading_history
    FOR EACH ROW EXECUTE FUNCTION update_reading_streak();

-- ─────────────────────────────────────────────────────────────────────────────
-- Article slug auto-generate on INSERT
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION generate_article_slug()
RETURNS TRIGGER LANGUAGE plpgsql AS $$
DECLARE
    v_base_slug TEXT;
    v_slug      TEXT;
    v_counter   INTEGER := 0;
BEGIN
    IF NEW.slug IS NOT NULL THEN
        RETURN NEW;
    END IF;

    v_base_slug := LOWER(
        REGEXP_REPLACE(
            REGEXP_REPLACE(
                UNACCENT(NEW.title),
                '[^a-zA-Z0-9\s-]', '', 'g'
            ),
            '\s+', '-', 'g'
        )
    );
    v_base_slug := LEFT(v_base_slug, 200);
    v_slug := v_base_slug;

    WHILE EXISTS (SELECT 1 FROM news_articles WHERE slug = v_slug) LOOP
        v_counter := v_counter + 1;
        v_slug := v_base_slug || '-' || v_counter;
    END LOOP;

    NEW.slug := v_slug;
    RETURN NEW;
END;
$$;

CREATE TRIGGER trg_article_slug
    BEFORE INSERT ON news_articles
    FOR EACH ROW EXECUTE FUNCTION generate_article_slug();

-- ─────────────────────────────────────────────────────────────────────────────
-- Category article_count sync
-- ─────────────────────────────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION sync_category_count()
RETURNS TRIGGER LANGUAGE plpgsql AS $$
BEGIN
    IF TG_OP = 'INSERT' AND NEW.status = 'published' AND NEW.category_id IS NOT NULL THEN
        UPDATE categories SET article_count = article_count + 1 WHERE id = NEW.category_id;
    ELSIF TG_OP = 'UPDATE' THEN
        IF OLD.status != 'published' AND NEW.status = 'published' AND NEW.category_id IS NOT NULL THEN
            UPDATE categories SET article_count = article_count + 1 WHERE id = NEW.category_id;
        ELSIF OLD.status = 'published' AND NEW.status != 'published' AND OLD.category_id IS NOT NULL THEN
            UPDATE categories SET article_count = GREATEST(0, article_count - 1) WHERE id = OLD.category_id;
        END IF;
    END IF;
    RETURN NEW;
END;
$$;

CREATE TRIGGER trg_category_count
    AFTER INSERT OR UPDATE OF status ON news_articles
    FOR EACH ROW EXECUTE FUNCTION sync_category_count();
