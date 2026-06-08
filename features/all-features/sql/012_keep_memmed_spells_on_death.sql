-- Keep memorized spell gems populated after player death.

INSERT INTO `rule_values` (`ruleset_id`, `rule_name`, `rule_value`, `notes`)
VALUES
(1, 'Character:UnmemSpellsOnDeath', 'false', 'Setting whether at death all memorized Spells are forgotten')
ON DUPLICATE KEY UPDATE
`rule_value` = VALUES(`rule_value`),
`notes` = VALUES(`notes`);
