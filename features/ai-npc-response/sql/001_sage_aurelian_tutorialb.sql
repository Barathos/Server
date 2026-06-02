-- AI NPC response prototype: Sage Aurelian in Tutorial B.
-- Safe to re-run. This seeds only the prototype NPC/spawn id 900903.

START TRANSACTION;

INSERT INTO npc_types (
  id,
  name,
  lastname,
  level,
  race,
  class,
  bodytype,
  hp,
  gender,
  texture,
  helmtexture,
  size,
  npc_faction_id,
  loottable_id,
  merchant_id,
  aggroradius,
  assistradius,
  runspeed,
  walkspeed,
  isquest
) VALUES (
  900903,
  'Sage_Aurelian',
  'AI Dialogue Prototype',
  10,
  1,
  1,
  1,
  5000,
  0,
  0,
  0,
  6,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  1
) ON DUPLICATE KEY UPDATE
  name = VALUES(name),
  lastname = VALUES(lastname),
  level = VALUES(level),
  race = VALUES(race),
  class = VALUES(class),
  bodytype = VALUES(bodytype),
  hp = VALUES(hp),
  gender = VALUES(gender),
  texture = VALUES(texture),
  helmtexture = VALUES(helmtexture),
  size = VALUES(size),
  npc_faction_id = VALUES(npc_faction_id),
  loottable_id = VALUES(loottable_id),
  merchant_id = VALUES(merchant_id),
  aggroradius = VALUES(aggroradius),
  assistradius = VALUES(assistradius),
  runspeed = VALUES(runspeed),
  walkspeed = VALUES(walkspeed),
  isquest = VALUES(isquest);

INSERT INTO spawngroup (
  id,
  name,
  spawn_limit,
  dist,
  max_x,
  min_x,
  max_y,
  min_y,
  delay
) VALUES (
  900903,
  'ai_dialogue_sage_aurelian',
  1,
  0,
  0,
  0,
  0,
  0,
  45000
) ON DUPLICATE KEY UPDATE
  name = VALUES(name),
  spawn_limit = VALUES(spawn_limit),
  dist = VALUES(dist),
  max_x = VALUES(max_x),
  min_x = VALUES(min_x),
  max_y = VALUES(max_y),
  min_y = VALUES(min_y),
  delay = VALUES(delay);

DELETE FROM spawnentry
WHERE spawngroupID = 900903;

INSERT INTO spawnentry (
  spawngroupID,
  npcID,
  chance,
  condition_value_filter,
  min_expansion,
  max_expansion,
  content_flags,
  content_flags_disabled
) VALUES (
  900903,
  900903,
  100,
  1,
  -1,
  -1,
  NULL,
  NULL
);

INSERT INTO spawn2 (
  id,
  spawngroupID,
  zone,
  version,
  x,
  y,
  z,
  heading,
  respawntime,
  variance,
  pathgrid,
  path_when_zone_idle,
  _condition,
  cond_value,
  animation,
  min_expansion,
  max_expansion,
  content_flags,
  content_flags_disabled
) VALUES (
  900903,
  900903,
  'tutorialb',
  0,
  -52.000000,
  -88.000000,
  25.000000,
  128.000000,
  300,
  0,
  0,
  0,
  0,
  1,
  0,
  -1,
  -1,
  NULL,
  NULL
) ON DUPLICATE KEY UPDATE
  spawngroupID = VALUES(spawngroupID),
  zone = VALUES(zone),
  version = VALUES(version),
  x = VALUES(x),
  y = VALUES(y),
  z = VALUES(z),
  heading = VALUES(heading),
  respawntime = VALUES(respawntime),
  variance = VALUES(variance),
  pathgrid = VALUES(pathgrid),
  path_when_zone_idle = VALUES(path_when_zone_idle),
  _condition = VALUES(_condition),
  cond_value = VALUES(cond_value),
  animation = VALUES(animation),
  min_expansion = VALUES(min_expansion),
  max_expansion = VALUES(max_expansion),
  content_flags = VALUES(content_flags),
  content_flags_disabled = VALUES(content_flags_disabled);

COMMIT;
