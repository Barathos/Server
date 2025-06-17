ALTER TABLE zone_points ADD COLUMN target_version INT NOT NULL DEFAULT 0 AFTER target_zone_id;
