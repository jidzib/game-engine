#include <engine/Player.hpp>
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace engine;
namespace {
void expect(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
bool near(float a,float b,float tolerance=0.0005f) { return std::abs(a-b)<=tolerance; }
bool near(Vec3 a,Vec3 b,float tolerance=0.0005f) { return near(a.x,b.x,tolerance)&&near(a.y,b.y,tolerance)&&near(a.z,b.z,tolerance); }
Vec3 axis(int i,float n) { return {i==0?n:0,i==1?n:0,i==2?n:0}; }
GameObject box(Vec3 position={},Vec3 scale={0.5f,0.5f,0.5f}) { return {"Test",position,scale,{},BoxCollider{}}; }
MoveResult move(const GameObject& p,Vec3 d,const GameObject& wall) {
    const GameObject* list[]={&wall}; return moveAndSlide(p,d,list);
}
template<class F> void rejects(F f) {
    bool rejected=false; try { f(); } catch(const std::invalid_argument&) { rejected=true; }
    expect(rejected,"Invalid collision data must throw");
}
}
int main() {
try {
    auto p=box();
    auto transformed=box({3,4,5},{2,3,4});
    transformed.boxCollider->offset={1,-2,0.5f};
    transformed.boxCollider->halfExtents={0.5f,2,3};
    auto b=*worldAabb(transformed);
    expect(near(b.center,{5,-2,7})&&near(b.halfSize,{1,6,12}),"World offset and nonuniform scale");
    for(int i=0;i<3;++i) for(float sign : {-1.0f,1.0f}) {
        Vec3 extent{10,10,10};
        if(i==0) extent.x=0.05f; if(i==1) extent.y=0.05f; if(i==2) extent.z=0.05f;
        auto wall=box(axis(i,sign*3),extent);
        auto hit=sweepAabb(*worldAabb(p),axis(i,sign*1000),*worldAabb(wall));
        expect(hit&&std::abs(hit->time-0.00245)<1e-8,"Thin wall sweep time");
        expect(hit->normals.size()==1&&near(hit->normals[0],axis(i,-sign)),"Outward axis normal");
        auto r=move(p,axis(i,sign*1000),wall);
        expect(near(r.position,axis(i,sign*(2.45f-collisionClearance))),"Both directions on all axes block at high speed");
        auto touching=p; touching.position=axis(i,sign*2.45f);
        // Use exactly representable contact geometry for boundary semantics.
        wall.position=axis(i,sign*3); wall.scale={10,10,10};
        if(i==0) wall.scale.x=0.5f; if(i==1) wall.scale.y=0.5f; if(i==2) wall.scale.z=0.5f;
        touching.position=axis(i,sign*2);
        expect(near(move(touching,axis(i,sign),wall).position,touching.position),"Touch inward blocked");
        expect(near(move(touching,axis(i,-sign),wall).position,touching.position+axis(i,-sign)),"Touch outward allowed");
        Vec3 tangent=axis((i+1)%3,2);
        expect(near(move(touching,tangent,wall).position,touching.position+tangent),"Touch tangent allowed");
        expect(near(move(touching,{},wall).position,touching.position),"Zero contact movement");
    }
    auto wall=box({3,0,0},{0.5f,10,10});
    auto slide=move(p,{5,2,3},wall);
    expect(near(slide.position,{2-collisionClearance,2,3}),"Slide retains full tangent without normalization");
    auto subdivided=p;
    for(int i=0;i<100;++i) subdivided.position=move(subdivided,{0.05f,0.02f,0.03f},wall).position;
    expect(near(slide.position,subdivided.position),"Large and subdivided sliding steps agree within 0.0005");
    auto second=box({0,0,4},{10,10,0.5f});
    std::vector<const GameObject*> obstacles{&wall,&second};
    auto corner=moveAndSlide(p,{6,1,6},obstacles);
    expect(near(corner.position,{2-collisionClearance,1,3-collisionClearance})&&corner.normals.size()==2,"Slide hits second wall in same move");
    std::reverse(obstacles.begin(),obstacles.end());
    expect(near(moveAndSlide(p,{6,1,6},obstacles).position,corner.position),"Corner order independent");
    second.position.z=3;
    auto simultaneous=moveAndSlide(p,{6,1,6},obstacles);
    expect(near(simultaneous.position,{2-collisionClearance,1,2-collisionClearance})&&simultaneous.normals.size()==2,"Simultaneous walls constrain both axes");
    std::reverse(obstacles.begin(),obstacles.end());
    expect(near(moveAndSlide(p,{6,1,6},obstacles).position,simultaneous.position),"Simultaneous order independent");
    auto singleCorner=box({3,0,3},{0.5f,10,0.5f});
    expect(move(p,{6,0,6},singleCorner).normals.size()==2,"Single-box corner supplies both normals");
    auto resting=p; resting.position=simultaneous.position;
    for(int i=0;i<100;++i) resting.position=moveAndSlide(resting,{0.1f,0,0.1f},obstacles).position;
    expect(near(resting.position,simultaneous.position),"Repeated corner pressure does not jitter");
    wall.boxCollider.reset();
    expect(near(move(p,{10,0,0},wall).position,{10,0,0}),"Missing obstacle ignored");
    wall.boxCollider.emplace(); wall.boxCollider->enabled=false;
    expect(near(move(p,{10,0,0},wall).position,{10,0,0}),"Disabled obstacle ignored");
    wall.boxCollider->enabled=true; p.boxCollider->enabled=false;
    expect(near(move(p,{10,0,0},wall).position,{10,0,0}),"Disabled player unrestricted");
    p.boxCollider.reset();
    expect(near(move(p,{10,0,0},wall).position,{10,0,0}),"Missing player unrestricted");
    p=box(); const GameObject* self[]={&p};
    expect(near(moveAndSlide(p,{1,2,3},self).position,{1,2,3}),"Self excluded");
    auto overlap=box(); auto recovered=move(p,{},overlap);
    expect(recovered.recovery==RecoveryStatus::Recovered&&near(recovered.position,{1+collisionClearance,0,0}),"Zero-input overlap recovery with deterministic X/+ tie");
    auto recoveredObject=p; recoveredObject.position=recovered.position;
    expect(move(recoveredObject,{},overlap).recovery==RecoveryStatus::NotNeeded,"Recovery removes overlap");
    auto left=box({-0.75f,0,0},{0.5f,100,100});
    auto right=box({0.75f,0,0},{0.5f,100,100});
    obstacles={&left,&right};
    auto trapped=moveAndSlide(p,{},obstacles);
    expect(trapped.recovery==RecoveryStatus::Unresolved,"Trapped recovery explicitly terminates");
    std::reverse(obstacles.begin(),obstacles.end());
    expect(near(moveAndSlide(p,{},obstacles).position,trapped.position),"Recovery order independent");
    auto invalid=box(); invalid.boxCollider->halfExtents.x=0;
    rejects([&]{(void)worldAabb(invalid);});
    invalid=box(); invalid.boxCollider->offset.y=std::numeric_limits<float>::infinity();
    rejects([&]{(void)worldAabb(invalid);});
    invalid=box(); invalid.scale.z=-1;
    rejects([&]{(void)worldAabb(invalid);});
    rejects([&]{(void)move(p,{std::numeric_limits<float>::quiet_NaN(),0,0},wall);});
    Player player;
    expect(player.object.boxCollider&&player.object.boxCollider->enabled,"Player collider explicitly enabled");
    expect(near(player.desiredDisplacement(0,0,1,1),{0,4,0}),"Vertical input maps to Y");
    expect(near(player.desiredDisplacement(0,1,0,1),{0,0,4}),"Forward input maps to Z");
    auto d=player.desiredDisplacement(1,1,1,1);
    expect(near(std::sqrt(dot(d,d)),4),"3D input normalization preserves speed");
    player.object=box(); obstacles={&wall};
    expect(near(player.move(1,0,0,1,obstacles).position,{2-collisionClearance,0,0}),"Player movement uses collision resolution");
    std::cout<<"Collision checks passed.\n"; return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
