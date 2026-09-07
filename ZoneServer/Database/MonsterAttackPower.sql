USE game_db;

ALTER TABLE MonsterTypes
	ADD COLUMN attackPower BIGINT UNSIGNED NOT NULL DEFAULT 1 AFTER experience;

UPDATE MonsterTypes SET attackPower = 7 WHERE monsterType = 3;
UPDATE MonsterTypes SET attackPower = 17 WHERE monsterType = 1;
