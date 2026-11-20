#pragma once
#include <vector>

struct PlayerTypeConfig {
    int startingHealth;
    float movementSpeed;
    int shootingCooldown;
    int bulletDamage;
    float bulletSize;
    int bulletSpeed;
};

inline const std::vector<PlayerTypeConfig> PlayerTypeConfigs = {
    {
        .startingHealth = 50,
        .movementSpeed = 6.0f,
        .shootingCooldown = 800,
        .bulletDamage = 35,
        .bulletSize = 0.2f,
        .bulletSpeed = 12
    },
    {
        .startingHealth = 100,
        .movementSpeed = 5.0f,
        .shootingCooldown = 500,
        .bulletDamage = 10,
        .bulletSize = 0.2f,
        .bulletSpeed = 7
    },
    {
        .startingHealth = 200,
        .movementSpeed = 4.0f,
        .shootingCooldown = 250,
        .bulletDamage = 5,
        .bulletSize = 0.2f,
        .bulletSpeed = 5
    },
};
