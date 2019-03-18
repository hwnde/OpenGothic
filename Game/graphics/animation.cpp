#include "animation.h"

#include <Tempest/Log>

#include <zenload/modelAnimationParser.h>
#include <zenload/zenParser.h>

#include "resources.h"

using namespace Tempest;

Animation::Animation(ZenLoad::ModelScriptBinParser &p,const std::string& name) {
  while(true) {
    ZenLoad::ModelScriptParser::EChunkType type=p.parse();
    switch (type) {
      case ZenLoad::ModelScriptParser::CHUNK_EOF: {
        setupIndex();
        return;
        }
      case ZenLoad::ModelScriptParser::CHUNK_ANI: {
        // p.ani().m_Name;
        // p.ani().m_Layer;
        // p.ani().m_BlendIn;
        // p.ani().m_BlendOut;
        // p.ani().m_Flags;
        // p.ani().m_LastFrame;
        // p.ani().m_Dir;

        auto& ani      = loadMAN(name+'-'+p.ani().m_Name+".MAN");
        ani.flags      = Flags(p.ani().m_Flags);
        ani.nextStr    = p.ani().m_Next;
        ani.firstFrame = uint32_t(p.ani().m_FirstFrame);
        ani.lastFrame  = uint32_t(p.ani().m_LastFrame);
        if(ani.nextStr==ani.name)
          ani.animCls=Loop;
        ani.sfx = std::move(p.sfx());
        ani.tag = std::move(p.tag());

        /*
        for(auto& sfx : p.sfxGround())
          animationAddEventSFXGround(anim, sfx);
        p.sfxGround().clear();

        for(auto& pfx:p.pfx())
          animationAddEventPFX(anim, pfx);
        p.pfx().clear();

        for(auto& pfxStop:p.pfxStop())
          animationAddEventPFXStop(anim, pfxStop);
        p.pfxStop().clear();
        */
        break;
        }

      case ZenLoad::ModelScriptParser::CHUNK_EVENT_SFX: {
        //animationAddEventSFX(anim, p.sfx().back());
        p.sfx().clear();
        break;
        }
      case ZenLoad::ModelScriptParser::CHUNK_EVENT_SFX_GRND: {
        // animationAddEventSFXGround(anim, p.sfx().back());
        p.sfxGround().clear();
        break;
        }
      case ZenLoad::ModelScriptParser::CHUNK_EVENT_PFX: {
        // animationAddEventPFX(anim, p.pfx().back());
        p.pfx().clear();
        break;
        }
      case ZenLoad::ModelScriptParser::CHUNK_EVENT_PFX_STOP: {
        // animationAddEventPFXStop(anim, p.pfxStop().back());
        p.pfxStop().clear();
        break;
        }
      case ZenLoad::ModelScriptParser::CHUNK_ERROR:
        throw std::runtime_error("animation load error");
      default:
        // Log::d("not implemented anim tag");
        break;
      }
    }
  }

const Animation::Sequence* Animation::sequence(const char *name) const {
  auto it = std::lower_bound(sequences.begin(),sequences.end(),name,[](const Sequence& s,const char* n){
    return s.name<n;
    });

  if(it!=sequences.end() && it->name==name)
    return &(*it);
  return nullptr;
  }

void Animation::debug() const {
  for(auto& i:sequences)
    Log::d(i.name);
  }

Animation::Sequence& Animation::loadMAN(const std::string& name) {
  sequences.emplace_back(name);
  return sequences.back();
  }

void Animation::setupIndex() {
  // for(auto& i:sequences)
  //   Log::i(i.name);

  std::sort(sequences.begin(),sequences.end(),[](const Sequence& a,const Sequence& b){
    return a.name<b.name;
    });

  for(auto& i:sequences) {
    for(auto& r:sequences)
      if(r.name==i.nextStr){
        i.next = &r;
        break;
        }
    }
  }

Animation::Sequence::Sequence(const std::string &name) {
  if(!Resources::hasFile(name))
    return;

  const VDFS::FileIndex& idx = Resources::vdfsIndex();
  ZenLoad::ZenParser            zen(name,idx);
  ZenLoad::ModelAnimationParser p(zen);

  while(true) {
    ZenLoad::ModelAnimationParser::EChunkType type = p.parse();
    switch(type) {
      case ZenLoad::ModelAnimationParser::CHUNK_EOF:{
        setupMoveTr();
        return;
        }
      case ZenLoad::ModelAnimationParser::CHUNK_HEADER: {
        this->name      = p.getHeader().aniName;
        this->fpsRate   = p.getHeader().fpsRate;
        this->numFrames = p.getHeader().numFrames;
        this->layer     = p.getHeader().layer;

        if(this->name.size()>1){
          if(this->name.find("_2_")!=std::string::npos)
            animCls=Transition;
          else if(this->name[0]=='T' && this->name[1]=='_')
            animCls=Transition;
          else if(this->name[0]=='R' && this->name[1]=='_')
            animCls=Transition;
          else if(this->name[0]=='S' && this->name[1]=='_')
            animCls=Loop;

          //if(this->name=="S_JUMP" || this->name=="S_JUMPUP")
          //  animCls=Transition;
          }
        break;
        }
      case ZenLoad::ModelAnimationParser::CHUNK_RAWDATA:
        nodeIndex = std::move(p.getNodeIndex());
        samples   = p.getSamples();
        break;
      case ZenLoad::ModelAnimationParser::CHUNK_ERROR:
        throw std::runtime_error("animation load error");
      }
    }
  }

bool Animation::Sequence::isFinished(uint64_t t) const {
  return t>=totalTime();
  }

float Animation::Sequence::totalTime() const {
  return numFrames*1000/fpsRate;
  }

ZMath::float3 Animation::Sequence::speed(uint64_t at,uint64_t dt) const {
  ZMath::float3 f={};

  auto a = translateXZ(at+dt), b=translateXZ(at);
  f.x = a.x-b.x;
  f.y = a.y-b.y;
  f.z = a.z-b.z;

  return f;
  }

ZMath::float3 Animation::Sequence::translateXZ(uint64_t at) const {
  if(numFrames==0 || tr.size()==0) {
    ZMath::float3 n={};
    return n;
    }

  uint64_t fr     = uint64_t(fpsRate*at);
  float    a      = (fr%1000)/1000.f;
  uint64_t frameA = fr/1000;
  uint64_t frameB = frameA+1;

  auto  mA = frameA/tr.size();
  auto  pA = tr[frameA%tr.size()];

  auto  mB = frameB/tr.size();
  auto  pB = tr[frameB%tr.size()];

  float m = mA+(mB-mA)*a;
  ZMath::float3 p=pA;
  p.x += (pB.x-pA.x)*a;
  p.y += (pB.y-pA.y)*a;
  p.z += (pB.z-pA.z)*a;

  p.x += m*moveTr.position.x;
  p.y += m*moveTr.position.y;
  p.z += m*moveTr.position.z;
  return p;
  }

void Animation::Sequence::setupMoveTr() {
  size_t sz = nodeIndex.size();

  if(samples.size()>0 && samples.size()>=sz) {
    auto& a = samples[0].position;
    auto& b = samples[samples.size()-sz].position;
    moveTr.position.x = b.x-a.x;
    moveTr.position.y = b.y-a.y;
    moveTr.position.z = b.z-a.z;

    tr.resize(samples.size()/sz);
    for(size_t i=0,r=0;i<samples.size();i+=sz,++r){
      auto& p  = tr[r];
      auto& bi = samples[i].position;
      p.x = bi.x-a.x;
      p.y = bi.y-a.y;
      p.z = bi.z-a.z;
      }
    }

  if(samples.size()>0){
    translate=samples[0].position;
    }
  }
