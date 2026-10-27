#include <aqua/aqua.h>
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <cmath>
#include <memory>
#include <unordered_map>
extern "C" {
#include "aq_json.h"
}
using namespace JPH;
namespace {
struct World {
    ObjectLayerPairFilterTable pairs{2};
    BroadPhaseLayerInterfaceTable broad{2,2};
    std::unique_ptr<ObjectVsBroadPhaseLayerFilterTable> objects;
    PhysicsSystem physics;
    TempAllocatorImpl allocator{16 * 1024 * 1024};
    JobSystemSingleThreaded jobs{2048};
    std::unordered_map<int,BodyID> bodies;
    World() {
        pairs.EnableCollision(0,1); pairs.EnableCollision(1,1);
        broad.MapObjectToBroadPhaseLayer(0,BroadPhaseLayer(0)); broad.MapObjectToBroadPhaseLayer(1,BroadPhaseLayer(1));
        objects=std::make_unique<ObjectVsBroadPhaseLayerFilterTable>(broad,2,pairs,2);
        physics.Init(4096,0,8192,4096,broad,*objects,pairs);
    }
    ~World() {
        auto &api=physics.GetBodyInterface();
        for(auto [id,body]:bodies) { (void)id; api.RemoveBody(body); api.DestroyBody(body); }
    }
};
struct Service { std::unordered_map<int,std::unique_ptr<World>> worlds; int next_world=1,next_body=1; };
static void *create(aqua_runtime *) {
    if(!Factory::sInstance) { RegisterDefaultAllocator(); Factory::sInstance=new Factory; RegisterTypes(); }
    return new(std::nothrow) Service;
}
static void destroy(aqua_runtime *,void *data) { delete static_cast<Service *>(data); }
static aj *at(aj *array,int index) { aj *v=array && array->type==AJ_ARR?array->child:nullptr; while(v && index-->0)v=v->next; return v; }
static bool number(aj *v,double &out) {
    if(!v || v->type!=AJ_NUM || !std::isfinite(v->num) || std::abs(v->num)>1000000) return false;
    out=v->num; return true;
}
static bool vector(aj *v,double *out,int count=3) {
    if(!v || v->type!=AJ_ARR || aj_count(v)!=count)return false;
    for(int i=0;i<count;i++)if(!number(at(v,i),out[i]))return false;
    return true;
}
static void vec(aq_buf &out,Vec3Arg v) { aq_buf_printf(&out,"[%.9g,%.9g,%.9g]",v.GetX(),v.GetY(),v.GetZ()); }
static void position(aq_buf &out,RVec3Arg v) { aq_buf_printf(&out,"[%.17g,%.17g,%.17g]",double(v.GetX()),double(v.GetY()),double(v.GetZ())); }
static void dispatch(aqua_call *call) {
    Service *service=static_cast<Service *>(aqua_state(call)); aqua_str input=aqua_arg_str(call,0);
    std::unique_ptr<aj,decltype(&aj_free)> request(aj_parse(input.data,input.length),aj_free);
    aj *req=request.get(), *id=at(req,0), *method=at(req,1), *args=at(req,2);
    aq_buf out={}; const char *error=nullptr; double values[4]={}, mass=0, scalar=0;
    if(!service || !id || id->type!=AJ_NUM || id->num<0 || id->num>2147483647 || id->num!=std::floor(id->num) ||
        !method || method->type!=AJ_STR || !args || args->type!=AJ_ARR) {
        aqua_fail(call,"Invalid Jolt request"); return;
    }
    const char *name=method->str;
#define IS(value) (!strcmp(name,value))
#define FAIL(message) do { error=message; goto finish; } while(0)
    if(IS("CreateWorld")) {
        if(service->worlds.size()>=128)FAIL("World capacity is 128");
        if(!vector(at(args,0),values))FAIL("Expected a finite 3-element gravity vector");
        auto world=std::make_unique<World>(); world->physics.SetGravity(Vec3(float(values[0]),float(values[1]),float(values[2])));
        int handle=service->next_world++; service->worlds.emplace(handle,std::move(world)); aq_buf_printf(&out,"%d",handle);
    } else {
        auto entry=service->worlds.find(int(id->num));
        if(entry==service->worlds.end()) { if(IS("Dispose"))aq_buf_puts(&out,"null"); else FAIL("Unknown or disposed Jolt world"); }
        else {
            World &world=*entry->second; auto &api=world.physics.GetBodyInterface();
            if(IS("Dispose")) { service->worlds.erase(entry); aq_buf_puts(&out,"null"); }
            else if(IS("CreateSphere") || IS("CreateBox")) {
                if(world.bodies.size()>=4096)FAIL("Jolt world capacity is 4096 bodies");
                if(!number(at(args,2),mass) || (mass!=0 && mass<.001))FAIL("Mass must be 0 or in 0.001..1000000 kilograms");
                double shape[3]={}; bool sphere=IS("CreateSphere");
                if(sphere) { if(!number(at(args,0),shape[0]))FAIL("Invalid sphere radius"); }
                else if(!vector(at(args,0),shape))FAIL("Expected 3 box half-extents");
                for(int i=0;i<(sphere?1:3);i++)if(shape[i]<.001 || shape[i]>10000)FAIL("Shape dimensions must be in 0.001..10000 meters");
                if(!vector(at(args,1),values))FAIL("Expected a finite 3-element position vector");
                RefConst<Shape> native_shape;
                if(sphere) native_shape=new SphereShape(float(shape[0]));
                else native_shape=new BoxShape(Vec3(float(shape[0]),float(shape[1]),float(shape[2])),float(std::min(.05,std::min({shape[0],shape[1],shape[2]})*.1)));
                BodyCreationSettings settings(native_shape,RVec3(values[0],values[1],values[2]),Quat::sIdentity(),mass>0?EMotionType::Dynamic:EMotionType::Static,mass>0?1:0);
                settings.mLinearDamping=0; settings.mAngularDamping=0; settings.mFriction=.2f; settings.mRestitution=0;
                settings.mMotionQuality=mass>0?EMotionQuality::LinearCast:EMotionQuality::Discrete;
                if(mass>0) { settings.mOverrideMassProperties=EOverrideMassProperties::CalculateInertia; settings.mMassPropertiesOverride.mMass=float(mass); }
                Body *body=api.CreateBody(settings); if(!body)FAIL("Jolt could not allocate a body");
                BodyID body_id=body->GetID(); api.AddBody(body_id,mass>0?EActivation::Activate:EActivation::DontActivate);
                int handle=service->next_body++; world.bodies.emplace(handle,body_id); aq_buf_printf(&out,"%d",handle);
            } else if(IS("GetBodyCount"))aq_buf_printf(&out,"%zu",world.bodies.size());
            else if(IS("GetGravity"))vec(out,world.physics.GetGravity());
            else if(IS("SetGravity")) {
                if(!vector(at(args,0),values))FAIL("Expected a finite 3-element vector");
                world.physics.SetGravity(Vec3(float(values[0]),float(values[1]),float(values[2])));
                for(auto [handle,body]:world.bodies) { (void)handle; if(api.GetMotionType(body)==EMotionType::Dynamic)api.ActivateBody(body); }
            } else if(IS("Step")) {
                if(!number(at(args,0),scalar) || scalar<.000001 || scalar>1 || !number(at(args,1),mass) || mass<1 || mass>128 || mass!=int(mass))FAIL("Invalid physics step");
                if(world.physics.Update(float(scalar),int(mass),&world.allocator,&world.jobs)!=EPhysicsUpdateError::None)FAIL("Jolt update failed");
            } else if(IS("OptimizeBroadPhase"))world.physics.OptimizeBroadPhase();
            else {
                aj *handle=at(args,0); if(!handle || handle->type!=AJ_NUM || handle->num<1 || handle->num>2147483647 || handle->num!=std::floor(handle->num))FAIL("Expected a live body from this Jolt world");
                auto body=world.bodies.find(int(handle->num)); if(body==world.bodies.end())FAIL("Expected a live body from this Jolt world");
                BodyID body_id=body->second;
                bool dynamic=IS("SetLinearVelocity") || IS("SetAngularVelocity") || IS("AddForce") || IS("AddImpulse") || IS("AddAngularImpulse");
                if(dynamic && api.GetMotionType(body_id)!=EMotionType::Dynamic)FAIL("Expected a dynamic body");
                if(IS("RemoveBody")) { api.RemoveBody(body_id); api.DestroyBody(body_id); world.bodies.erase(body); }
                else if(IS("GetPosition"))position(out,api.GetPosition(body_id));
                else if(IS("GetRotation")) { Quat q=api.GetRotation(body_id); aq_buf_printf(&out,"[%.9g,%.9g,%.9g,%.9g]",q.GetX(),q.GetY(),q.GetZ(),q.GetW()); }
                else if(IS("GetLinearVelocity"))vec(out,api.GetLinearVelocity(body_id));
                else if(IS("GetAngularVelocity"))vec(out,api.GetAngularVelocity(body_id));
                else if(IS("IsActive"))aq_buf_puts(&out,api.IsActive(body_id)?"true":"false");
                else if(IS("SetFriction") || IS("SetRestitution")) {
                    if(!number(at(args,1),scalar) || scalar<0 || scalar>1)FAIL("Expected a value in 0..1");
                    if(IS("SetFriction"))api.SetFriction(body_id,float(scalar)); else api.SetRestitution(body_id,float(scalar));
                } else {
                    if(!vector(at(args,1),values,IS("SetRotation")?4:3))FAIL("Expected a finite vector");
                    Vec3 v{float(values[0]),float(values[1]),float(values[2])};
                    if(IS("SetPosition"))api.SetPosition(body_id,RVec3(values[0],values[1],values[2]),EActivation::Activate);
                    else if(IS("SetRotation")) {
                        double length=std::sqrt(values[0]*values[0]+values[1]*values[1]+values[2]*values[2]+values[3]*values[3]);
                        if(length<1e-6)FAIL("Quaternion must be nonzero");
                        api.SetRotation(body_id,Quat(float(values[0]/length),float(values[1]/length),float(values[2]/length),float(values[3]/length)),EActivation::Activate);
                    } else if(IS("SetLinearVelocity"))api.SetLinearVelocity(body_id,v);
                    else if(IS("SetAngularVelocity")) { api.SetAngularVelocity(body_id,v); api.ActivateBody(body_id); }
                    else if(IS("AddForce")) { api.AddForce(body_id,v); api.ActivateBody(body_id); }
                    else if(IS("AddImpulse"))api.AddImpulse(body_id,v);
                    else if(IS("AddAngularImpulse"))api.AddAngularImpulse(body_id,v);
                    else FAIL("Unknown Jolt method");
                }
            }
        }
    }
finish:
    if(error) aqua_fail(call,"%s",error);
    else if(out.failed)aqua_fail(call,"Out of memory");
    else aqua_return_text(call,out.len?(char *)out.data:"null",out.len?out.len:4);
    aq_buf_free(&out);
#undef IS
#undef FAIL
}
}
AQUA_LIBRARY(jolt) {
    aqua_library_state(library,create,destroy);
    aqua_class *module=aqua_define_module(library,"JoltNative|物理原生");
    aqua_define_function(module,"Dispatch|呼叫",dispatch,"s");
}
