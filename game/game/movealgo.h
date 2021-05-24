#pragma once

#include <cstdint>
#include <array>
#include <limits>

#include <zenload/zTypes.h>
#include <Tempest/Point>

#include "physics/dynamicworld.h"
#include "graphics/mesh/animationsolver.h"

class Npc;
class World;
class WayPoint;
class Serialize;

class MoveAlgo final {
  public:
    static const float closeToPointThreshold;
    static const float climbMove;

    MoveAlgo(Npc& unit);

    struct JumpStatus {
      AnimationSolver::Anim anim = AnimationSolver::Anim::Jump;
      float                 height = 0;
      };

    enum MvFlags {
      NoFlag   = 0,
      FaiMove  = 1,
      WaitMove = 1<<1,
      };

    static bool isClose(const Tempest::Vec3& w, const WayPoint& p);
    static bool isClose(float x,float y,float z,const WayPoint& p);
    static bool isClose(float x,float y,float z,const WayPoint& p,float dist);

    void    load(Serialize& fin);
    void    save(Serialize& fout) const;

    void    tick(uint64_t dt,MvFlags fai=NoFlag);

    void    multSpeed(float s){ mulSpeed=s; }
    void    clearSpeed();
    void    accessDamFly(float dx,float dz);

    bool    aiGoTo      (const Tempest::Vec3& p, float destDist);
    bool    aiGoToTarget(float destDist);

    bool    startClimb(JumpStatus ani);
    void    startDive();

    bool    isFaling()  const;
    bool    isSlide()   const;
    bool    isInAir()   const;
    bool    isJumpup()  const;
    bool    isClimb()   const;
    bool    isInWater() const;
    bool    isSwim()    const;
    bool    isDive()    const;

    uint8_t groundMaterial() const;
    auto    groundNormal() const -> Tempest::Vec3;

    auto    portalName() -> const char*;
    int32_t diveTime() const;

  private:
    void    tickMobsi  (uint64_t dt);
    bool    tickSlide  (uint64_t dt);
    void    tickGravity(uint64_t dt);
    void    tickSwim   (uint64_t dt);
    void    tickClimb  (uint64_t dt);
    void    tickJumpup (uint64_t dt);

    bool    tryMove    (float x, float y, float z);

    enum Flags : uint32_t {
      NoFlags = 0,
      InAir   = 1<<1,
      Faling  = 1<<2,
      Slide   = 1<<3,
      JumpUp  = 1<<4,
      ClimbUp = 1<<5,
      InWater = 1<<6,
      Swim    = 1<<7,
      Dive    = 1<<8,
      };

    void    setInAir   (bool f);
    void    setAsJumpup(bool f);
    void    setAsClimb (bool f);
    void    setAsSlide (bool f);
    void    setInWater (bool f);
    void    setAsSwim  (bool f);
    void    setAsDive  (bool f);

    bool    slideDir() const;
    void    onMoveFailed();
    void    applyRotation(Tempest::Vec3& out, const Tempest::Vec3& in) const;
    auto    animMoveSpeed(uint64_t dt) const -> Tempest::Vec3;
    auto    npcMoveSpeed (uint64_t dt, MvFlags moveFlg) -> Tempest::Vec3;
    auto    go2NpcMoveSpeed (const Tempest::Vec3& dp, const Npc &tg) -> Tempest::Vec3;
    auto    go2WpMoveSpeed  (Tempest::Vec3 dp, const Tempest::Vec3& to) -> Tempest::Vec3;
    bool    testSlide(const Tempest::Vec3& pos) const;
    void    implTick(uint64_t dt,MvFlags fai=NoFlag);

    float   stepHeight()  const;
    float   slideAngle()  const;
    float   slideAngle2() const;
    float   waterDepthKnee() const;
    float   waterDepthChest() const;
    bool    canFlyOverWater() const;
    void    takeFallDamage() const;

    void    rayMain  (const Tempest::Vec3& pos) const;
    float   dropRay  (const Tempest::Vec3& pos, bool& hasCol) const;
    float   waterRay (const Tempest::Vec3& pos, bool* hasCol = nullptr) const;
    auto    normalRay(const Tempest::Vec3& pos) const -> Tempest::Vec3;

    struct CacheLand : DynamicWorld::RayLandResult {
      float x=0, y=0, z=std::numeric_limits<float>::infinity();
      };
    struct CacheWater : DynamicWorld::RayWaterResult {
      float x=0, y=0, z=std::numeric_limits<float>::infinity();
      };

    Npc&                npc;
    mutable CacheLand   cache;
    mutable CacheWater  cacheW;
    Flags               flags=NoFlags;

    float               mulSpeed  =1.f;
    Tempest::Vec3       fallSpeed ={};
    float               fallCount=0.f;

    Tempest::Vec3       skipMove = {};

    uint64_t            climbStart=0;
    Tempest::Vec3       climbPos0={};
    float               climbHeight=0.f;
    uint8_t             jmp=0;

    uint64_t            diveStart=0;

    static const float   gravity;
    static const float   eps;
    static const float   epsAni;
    static const int32_t flyOverWaterHint;
  };
