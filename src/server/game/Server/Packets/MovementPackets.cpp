/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MovementPackets.h"
#include "MovementTypedefs.h"

ByteBuffer& operator<<(ByteBuffer& data, G3D::Vector3 const& v)
{
    data << v.x << v.y << v.z;
    return data;
}

ByteBuffer& operator>>(ByteBuffer& data, G3D::Vector3& v)
{
    data >> v.x >> v.y >> v.z;
    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, MovementInfo& movementInfo)
{
    bool hasTransportData = !movementInfo.transport.guid.IsEmpty();
    bool hasTransportPrevTime = hasTransportData && movementInfo.transport.prevTime != 0;
    bool hasTransportVehicleId = hasTransportData && movementInfo.transport.vehicleId != 0;
    bool hasFallDirection = movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR);
    bool hasFallData = hasFallDirection || movementInfo.jump.fallTime != 0;

    data << movementInfo.guid;
    data << movementInfo.time;
    data << movementInfo.pos.PositionXYZOStream();
    data << movementInfo.pitch;
    data << movementInfo.splineElevation;

    uint32 removeMovementForcesCount = 0;
    data << removeMovementForcesCount;

    uint32 int168 = 0;
    data << int168;

    /*for (uint32 i = 0; i < removeMovementForcesCount; ++i)
    {
        data << ObjectGuid;
    }*/

    data.WriteBits(movementInfo.flags, 30);
    data.WriteBits(movementInfo.flags2, 15);

    data.WriteBit(hasTransportData);
    data.WriteBit(hasFallData);

    data.WriteBit(0); // HeightChangeFailed
    data.WriteBit(0); // RemoteTimeValid

    if (hasTransportData)
    {
        data << movementInfo.transport.guid;
        data << movementInfo.transport.pos.PositionXYZOStream();
        data << movementInfo.transport.seat;
        data << movementInfo.transport.time;

        data.WriteBit(hasTransportPrevTime);
        data.WriteBit(hasTransportVehicleId);

        if (hasTransportPrevTime)
            data << movementInfo.transport.prevTime;

        if (hasTransportVehicleId)
            data << movementInfo.transport.vehicleId;
    }

    if (hasFallData)
    {
        data << movementInfo.jump.fallTime;
        data << movementInfo.jump.zspeed;

        data.WriteBit(hasFallDirection);
        if (hasFallDirection)
        {
            data << movementInfo.jump.sinAngle;
            data << movementInfo.jump.cosAngle;
            data << movementInfo.jump.xyspeed;
        }
    }

    data.FlushBits();

    return data;
}

ByteBuffer& operator>>(ByteBuffer& data, MovementInfo& movementInfo)
{
    data >> movementInfo.guid;
    data >> movementInfo.time;
    data >> movementInfo.pos.PositionXYZOStream();
    data >> movementInfo.pitch;
    data >> movementInfo.splineElevation;

    uint32 removeMovementForcesCount;
    data >> removeMovementForcesCount;

    uint32 int168;
    data >> int168;

    for (uint32 i = 0; i < removeMovementForcesCount; ++i)
    {
        ObjectGuid guid;
        data >> guid;
    }

    movementInfo.flags = data.ReadBits(30);
    movementInfo.flags2 = data.ReadBits(15);

    bool hasTransport = data.ReadBit();
    bool hasFall = data.ReadBit();

    data.ReadBit(); // HeightChangeFailed
    data.ReadBit(); // RemoteTimeValid

    if (hasTransport)
    {
        data >> movementInfo.transport.guid;
        data >> movementInfo.transport.pos.PositionXYZOStream();
        data >> movementInfo.transport.seat;
        data >> movementInfo.transport.time;

        bool hasPrevTime = data.ReadBit();
        bool hasVehicleId = data.ReadBit();

        if (hasPrevTime)
            data >> movementInfo.transport.prevTime;

        if (hasVehicleId)
            data >> movementInfo.transport.vehicleId;
    }

    if (hasFall)
    {
        data >> movementInfo.jump.fallTime;
        data >> movementInfo.jump.zspeed;

        // ResetBitReader

        bool hasFallDirection = data.ReadBit();
        if (hasFallDirection)
        {
            data >> movementInfo.jump.sinAngle;
            data >> movementInfo.jump.cosAngle;
            data >> movementInfo.jump.xyspeed;
        }
    }

    return data;
}

void WorldPackets::Movement::ClientPlayerMovement::Read()
{
    _worldPacket >> movementInfo;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Movement::MonsterSplineFilterKey& monsterSplineFilterKey)
{
    data << monsterSplineFilterKey.Idx;
    data << monsterSplineFilterKey.Speed;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Movement::MonsterSplineFilter& monsterSplineFilter)
{
    data << uint32(monsterSplineFilter.FilterKeys.size());
    data << monsterSplineFilter.BaseSpeed;
    data << monsterSplineFilter.StartOffset;
    data << monsterSplineFilter.DistToPrevFilterKey;
    for (WorldPackets::Movement::MonsterSplineFilterKey& filterKey : monsterSplineFilter.FilterKeys)
        data << filterKey;
    data << monsterSplineFilter.AddedToStart;
    data.WriteBits(monsterSplineFilter.FilterFlags, 2);
    data.FlushBits();

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Movement::MovementSpline& movementSpline)
{
    data << movementSpline.Flags;
    data << movementSpline.AnimTier;
    data << movementSpline.TierTransStartTime;
    data << movementSpline.Elapsed;
    data << movementSpline.MoveTime;
    data << movementSpline.JumpGravity;
    data << movementSpline.SpecialTime;
    data << int32(movementSpline.Points.size());
    data << movementSpline.Mode;
    data << movementSpline.VehicleExitVoluntary;
    data << movementSpline.TransportGUID;
    data << movementSpline.VehicleSeat;
    data << int32(movementSpline.PackedDeltas.size());
    for (G3D::Vector3 const& pos : movementSpline.Points)
        data << pos;
    for (G3D::Vector3 const& pos : movementSpline.PackedDeltas)
        data.appendPackXYZ(pos.x, pos.y, pos.z);
    data.WriteBits(movementSpline.Face, 2);
    data.WriteBit(movementSpline.SplineFilter.HasValue);
    data.FlushBits();

    switch (movementSpline.Face)
    {
        case MONSTER_MOVE_FACING_SPOT:
            data << movementSpline.FaceSpot;
            break;
        case MONSTER_MOVE_FACING_TARGET:
            data << movementSpline.FaceDirection;
            data << movementSpline.FaceGUID;
            break;
        case MONSTER_MOVE_FACING_ANGLE:
            data << movementSpline.FaceDirection;
            break;
    }

    if (movementSpline.SplineFilter.HasValue)
        data << movementSpline.SplineFilter.Value;

    return data;
}

ByteBuffer& operator<<(ByteBuffer& data, WorldPackets::Movement::MovementMonsterSpline& movementMonsterSpline)
{
    data << movementMonsterSpline.ID;
    data << movementMonsterSpline.Destination;
    data << movementMonsterSpline.Move;
    data.WriteBit(movementMonsterSpline.CrzTeleport);
    data.FlushBits();

    return data;
}

WorldPacket const* WorldPackets::Movement::MonsterMove::Write()
{
    _worldPacket << MoverGUID;
    _worldPacket << Pos;
    _worldPacket << SplineData;

    // Unk bits. 0 if monster is moving, 1 or 2 if stopped
    if (SplineData.Move.Flags)
        _worldPacket.WriteBits(0, 2);
    else
        _worldPacket.WriteBits(2, 2);

    _worldPacket.FlushBits();

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Movement::MoveSplineSet::Write()
{
    _worldPacket << MoverGUID;
    _worldPacket << Speed;
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Movement::MoveUpdate::Write()
{
    _worldPacket << movementInfo;
    _worldPacket << Speed;
    return &_worldPacket;
}

WorldPacket const* WorldPackets::Movement::ServerPlayerMovement::Write()
{
    MovementInfo movementInfo = mover->m_movementInfo;

    _worldPacket << movementInfo;

    return &_worldPacket;
}

WorldPacket const* WorldPackets::Movement::NewWorld::Write()
{
    _worldPacket << MapID;
    _worldPacket << Pos.PositionXYZOStream();
    _worldPacket << Reason;
    return &_worldPacket;
}
