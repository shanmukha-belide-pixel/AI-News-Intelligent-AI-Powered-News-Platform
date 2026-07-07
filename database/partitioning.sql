-- =============================================================================
-- database/partitioning.sql  —  Partition Maintenance Procedures
-- =============================================================================
-- Automation helper to dynamically create partitions for future quarters
-- =============================================================================

CREATE OR REPLACE FUNCTION create_future_partitions()
RETURNS VOID AS $$
DECLARE
    next_date DATE;
    partition_name TEXT;
    q_start DATE;
    q_end DATE;
    yr INT;
    qtr INT;
BEGIN
    -- Determine dates for the next 4 quarters
    FOR i IN 1..4 LOOP
        next_date := CURRENT_DATE + (i * INTERVAL '3 months');
        yr  := EXTRACT(YEAR FROM next_date);
        qtr := EXTRACT(QUARTER FROM next_date);
        
        -- Partition start & end dates
        q_start := DATE_TRUNC('quarter', next_date);
        q_end   := q_start + INTERVAL '3 months';
        
        -- 1. Create partition for analytics_events
        partition_name := 'analytics_events_' || yr || '_q' || qtr;
        IF NOT EXISTS (
            SELECT 1 FROM pg_class c 
            JOIN pg_namespace n ON n.oid = c.relnamespace
            WHERE c.relname = partition_name
        ) THEN
            EXECUTE format(
                'CREATE TABLE %I PARTITION OF analytics_events FOR VALUES FROM (%L) TO (%L)',
                partition_name, q_start, q_end
            );
            RAISE NOTICE 'Partition % created for values % to %', partition_name, q_start, q_end;
        END IF;

        -- 2. Create partition for audit_logs
        partition_name := 'audit_logs_' || yr;
        q_start := DATE_TRUNC('year', next_date);
        q_end   := q_start + INTERVAL '1 year';
        IF NOT EXISTS (
            SELECT 1 FROM pg_class c 
            JOIN pg_namespace n ON n.oid = c.relnamespace
            WHERE c.relname = partition_name
        ) THEN
            EXECUTE format(
                'CREATE TABLE %I PARTITION OF audit_logs FOR VALUES FROM (%L) TO (%L)',
                partition_name, q_start, q_end
            );
            RAISE NOTICE 'Partition % created for values % to %', partition_name, q_start, q_end;
        END IF;
    END LOOP;
END;
$$ LANGUAGE plpgsql;

-- Execute on migration to auto-provision next partitions
SELECT create_future_partitions();
