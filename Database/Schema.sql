-- Schema only. No accounts, character rows, passwords, or grants are included.
CREATE DATABASE IF NOT EXISTS account_db CHARACTER SET utf8mb4;
CREATE DATABASE IF NOT EXISTS game_db CHARACTER SET utf8mb4;
USE account_db;
CREATE TABLE IF NOT EXISTS `accounts` (
  `accountIndex` bigint NOT NULL AUTO_INCREMENT,
  `id` varchar(30) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL,
  `password` varchar(30) CHARACTER SET utf8mb3 COLLATE utf8mb3_general_ci NOT NULL,
  `loginTime` datetime DEFAULT NULL,
  `logoutTime` datetime DEFAULT NULL,
  PRIMARY KEY (`accountIndex`),
  UNIQUE KEY `user_id_UNIQUE` (`id`),
  UNIQUE KEY `accountIndex_UNIQUE` (`accountIndex`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;
USE game_db;
CREATE TABLE IF NOT EXISTS `characters` (
  `character_index` bigint unsigned NOT NULL AUTO_INCREMENT,
  `account_index` bigint unsigned NOT NULL,
  `name` varchar(30) NOT NULL,
  `class` bigint unsigned NOT NULL,
  `Level` bigint unsigned NOT NULL,
  `experience` bigint unsigned NOT NULL DEFAULT '0',
  `lastZoneId` bigint unsigned NOT NULL DEFAULT '1',
  `lastPositionX` float NOT NULL DEFAULT '0',
  `lastPositionY` float NOT NULL DEFAULT '0',
  `lastPositionZ` float NOT NULL DEFAULT '0',
  PRIMARY KEY (`character_index`),
  UNIQUE KEY `name_UNIQUE` (`name`),
  KEY `idx_characters_account_index` (`account_index`),
  CONSTRAINT `chk_characters_class` CHECK ((`class` in (0,1))),
  CONSTRAINT `chk_characters_level` CHECK ((`Level` >= 1))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb3;
USE game_db;
CREATE TABLE IF NOT EXISTS `monstertypes` (
  `monsterType` bigint unsigned NOT NULL,
  `name` varchar(30) NOT NULL,
  `level` bigint unsigned NOT NULL DEFAULT '1',
  `maxHP` bigint unsigned NOT NULL,
  `experience` bigint unsigned NOT NULL DEFAULT '0',
  `attackPower` bigint unsigned NOT NULL DEFAULT '1',
  `aggressive` tinyint unsigned NOT NULL DEFAULT '0',
  `detectionRange` float NOT NULL DEFAULT '5',
  `attackRange` float NOT NULL DEFAULT '1.5',
  `moveSpeed` float NOT NULL DEFAULT '3',
  `attackIntervalMs` bigint unsigned NOT NULL DEFAULT '2000',
  `canWander` tinyint unsigned NOT NULL DEFAULT '1',
  `canChase` tinyint unsigned NOT NULL DEFAULT '1',
  `canAttack` tinyint unsigned NOT NULL DEFAULT '1',
  `idleMinTimeMs` bigint unsigned NOT NULL DEFAULT '1000',
  `idleMaxTimeMs` bigint unsigned NOT NULL DEFAULT '3000',
  `wanderMinTimeMs` bigint unsigned NOT NULL DEFAULT '2000',
  `wanderMaxTimeMs` bigint unsigned NOT NULL DEFAULT '5000',
  `wanderRadius` float NOT NULL DEFAULT '3',
  `maxAggroRange` float NOT NULL DEFAULT '10',
  `respawnRadius` float NOT NULL DEFAULT '2',
  `respawnDelayMs` bigint unsigned NOT NULL DEFAULT '5000',
  PRIMARY KEY (`monsterType`),
  CONSTRAINT `chk_monster_types_aggressive` CHECK ((`aggressive` in (0,1))),
  CONSTRAINT `chk_monster_types_level` CHECK ((`level` >= 1)),
  CONSTRAINT `chk_monster_types_range` CHECK (((`detectionRange` >= 0) and (`attackRange` >= 0) and (`wanderRadius` >= 0) and (`maxAggroRange` >= 0) and (`respawnRadius` >= 0)))
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
USE game_db;
CREATE TABLE IF NOT EXISTS `playerlevelstats` (
  `class` bigint unsigned NOT NULL,
  `level` bigint unsigned NOT NULL,
  `requiredExperience` bigint unsigned NOT NULL,
  `maxHP` bigint unsigned NOT NULL,
  PRIMARY KEY (`class`,`level`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;
