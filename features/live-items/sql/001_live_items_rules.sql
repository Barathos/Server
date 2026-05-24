-- Optional explicit Live Items rule values.
-- The source feature also provides defaults in common/ruletypes.h.

INSERT INTO `rule_values` (`ruleset_id`, `rule_name`, `rule_value`, `notes`)
VALUES
(1, 'Items:LiveItemLoading', 'true', 'Enable live DB item fallback'),
(1, 'Items:LiveItemMinID', '900000', 'First live item ID'),
(1, 'Items:LiveItemMaxID', '999999', 'Last live item ID'),
(1, 'Items:LiveItemPollIntervalSeconds', '1', 'Live item DB poll interval')
ON DUPLICATE KEY UPDATE
`rule_value` = VALUES(`rule_value`),
`notes` = VALUES(`notes`);
