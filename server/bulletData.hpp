#pragma once

class BulletData {
  public:
    int bulletId;
    int ownerPlayerId;

    int positionX;
    int positionY;
    int direction;
    int speed;
    int lifetime;
    int damage;
};
