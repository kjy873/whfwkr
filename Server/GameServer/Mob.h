#pragma once
#include "Protocol.pb.h"

class Mob
{
public:
    uint64 mobId = 0;
    uint32 templateId = 0;
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    float dirX = 0.f;
    float dirY = 0.f;
    float dirZ = 0.f;
    int32 hp = 100;

public:
    Protocol::MobInfo ToInfo()
    {
        Protocol::MobInfo info;

        info.set_mobid(mobId);
        info.set_templateid(templateId);

        Protocol::Vector3* pos = info.mutable_pos();
        pos->set_x(x);
        pos->set_y(y);
        pos->set_z(z);

        Protocol::Vector3* dir = info.mutable_dir();
        dir->set_x(dirX);
        dir->set_y(dirY);
        dir->set_z(dirZ);

        info.set_hp(hp);

        return info;
    }
};

using MobRef = shared_ptr<Mob>;
