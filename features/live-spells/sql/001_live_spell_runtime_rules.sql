-- Optional explicit runtime rule values for Live Spells generated scroll items.
-- Live Spells persist spell definitions in data_buckets and generated scrolls in items.
-- The source feature also provides defaults in common/ruletypes.h.

INSERT INTO `rule_values` (`ruleset_id`, `rule_name`, `rule_value`, `notes`)
VALUES
(1, 'Items:LiveItemLoading', 'true', 'Enable live DB item fallback for generated spell scrolls'),
(1, 'Items:LiveItemMinID', '900000', 'First generated scroll item ID'),
(1, 'Items:LiveItemMaxID', '999999', 'Last generated scroll item ID'),
(1, 'Items:LiveItemPollIntervalSeconds', '1', 'Generated scroll item DB poll interval')
ON DUPLICATE KEY UPDATE
`rule_value` = VALUES(`rule_value`),
`notes` = VALUES(`notes`);
