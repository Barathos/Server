-- Source-backed Dynamic Quests prototype content.
-- This uses the existing EQEmu tasks/task_activities runtime and is safe to re-run.

DELETE FROM `spawn2` WHERE `id` = 910001 OR `spawngroupID` = 910001;
DELETE FROM `spawnentry` WHERE `spawngroupID` = 910001 OR `npcID` = 910001;
DELETE FROM `spawngroup` WHERE `id` = 910001;
DELETE FROM `npc_types` WHERE `id` = 910001;

DELETE FROM `task_activities` WHERE `taskid` = 910001;
DELETE FROM `tasks` WHERE `id` = 910001;

REPLACE INTO `tasks` (
  `id`, `type`, `duration`, `duration_code`, `title`, `description`,
  `reward_text`, `reward_id_list`, `cash_reward`, `exp_reward`,
  `reward_method`, `reward_points`, `reward_point_type`, `min_level`,
  `max_level`, `level_spread`, `min_players`, `max_players`, `repeatable`,
  `faction_reward`, `completion_emote`, `replay_timer_group`,
  `replay_timer_seconds`, `request_timer_group`, `request_timer_seconds`,
  `dz_template_id`, `lock_activity_id`, `faction_amount`, `enabled`
) VALUES (
  910001, 2, 0, 0, 'First Steps in Gloomingdeep',
  'Dynamic Quests prototype: Scout Deryn wants proof that an accepted quest can show multiple live objectives in the built-in task journal.',
  'Prototype reward: coin and experience', '', 500, 100,
  2, 0, 0, 1, 255, 0, 0, 0, 1,
  0, 'First Steps in Gloomingdeep is complete.', 0,
  0, 0, 0, 0, -1, 0, 1
);

REPLACE INTO `task_activities` (
  `taskid`, `activityid`, `req_activity_id`, `step`, `activitytype`,
  `target_name`, `goalmethod`, `goalcount`, `description_override`,
  `npc_match_list`, `item_id_list`, `item_list`, `dz_switch_id`,
  `min_x`, `min_y`, `min_z`, `max_x`, `max_y`, `max_z`,
  `skill_list`, `spell_list`, `zones`, `zone_version`, `optional`, `list_group`
) VALUES
(
  910001, 0, -1, 0, 2,
  'kobold invaders', 0, 3, 'Defeat kobold invaders in the Mines of Gloomingdeep.',
  'kobold', '', '', 0,
  0, 0, 0, 0, 0, 0,
  '-1', '0', '189', -1, 0, 0
),
(
  910001, 1, -1, 0, 4,
  'Scout Deryn', 0, 1, 'Report back to Scout Deryn.',
  '910001|Scout_Deryn|Scout Deryn', '', '', 0,
  0, 0, 0, 0, 0, 0,
  '-1', '0', '189', -1, 0, 1
);

REPLACE INTO `npc_types` (
  `id`, `name`, `lastname`, `level`, `race`, `class`, `bodytype`, `hp`, `gender`,
  `texture`, `size`, `runspeed`, `findable`, `trackable`, `isquest`, `show_name`,
  `special_abilities`, `skip_global_loot`
) VALUES (
  910001, 'Scout_Deryn', 'Dynamic Quest Prototype', 70, 1, 1, 1, 100000, 0,
  0, 6.0, 1.25, 1, 1, 1, 1,
  '19,1', 1
);

REPLACE INTO `spawngroup` (`id`, `name`, `spawn_limit`) VALUES
(910001, 'dynamic_quests_scout_deryn', 1);

REPLACE INTO `spawnentry` (`spawngroupID`, `npcID`, `chance`) VALUES
(910001, 910001, 100);

REPLACE INTO `spawn2` (
  `id`, `spawngroupID`, `zone`, `version`, `x`, `y`, `z`, `heading`, `respawntime`, `variance`
) VALUES (
  910001, 910001, 'tutorialb', 0, -64.0, -80.0, 25.0, 128.0, 300, 0
);
