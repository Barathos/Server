-- Live Items public testbed logging defaults.
-- CombatRecord defaults to GM say output in EQEmu, which is noisy for testers.

UPDATE logsys_categories
SET log_to_console = 0,
    log_to_file = 0,
    log_to_gmsay = 0,
    log_to_discord = 0
WHERE log_category_description = 'CombatRecord';
