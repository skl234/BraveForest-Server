-- 몬스터 배회 후 4~8초 대기. 이동 제한 시간은 6~10초
UPDATE MonsterTypes
SET idleMinTimeMs = 4000, idleMaxTimeMs = 8000,
    wanderMinTimeMs = 6000, wanderMaxTimeMs = 10000
WHERE monsterType IN (0, 1, 2, 3);
