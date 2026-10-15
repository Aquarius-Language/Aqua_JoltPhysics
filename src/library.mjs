import initJolt from '../assets/vendor-jolt.mjs';
import {JoltWorld} from './physics.mjs';
import wasm from '../assets/vendor-jolt.wasm';
export function aqua_library_abi(){return 1;}
export function aqua_create_library(){
  let initialized, nextWorld=1, nextBody=1, closed=false;const worlds=new Map();
  const decode=new TextDecoder(),encode=new TextEncoder();
  const J=()=>initialized??=initJolt({instantiateWasm(imports,receive){const instance=new WebAssembly.Instance(new WebAssembly.Module(wasm),imports);receive(instance);return instance.exports;}});
  return {
    async dispatch(bytes){
      if(closed)throw Error('Jolt library disposed');
      const [id,method,args]=JSON.parse(decode.decode(bytes));let result=null;
      if(method==='CreateWorld'){
        if(worlds.size>=128)throw Error('World capacity is 128');
        const world=new JoltWorld(await J(),args[0]);result=nextWorld++;worlds.set(result,{world,bodies:new Map()});
      }else{
        const entry=worlds.get(id);if(!entry){if(method==='Dispose')return encode.encode('null');throw Error('Unknown or disposed Jolt world');}
        const {world,bodies}=entry;
        if(method==='Dispose'){world.dispose();worlds.delete(id);}
        else if(method==='CreateSphere'||method==='CreateBox'){const body=world.create(method==='CreateSphere'?'sphere':'box',...args);result=nextBody++;bodies.set(result,body);}
        else if(method==='Step')world.step(...args);
        else if(method==='GetBodyCount'){world.live();result=world.bodies.size;}
        else if(method==='GetGravity')result=world.getGravity();
        else if(method==='SetGravity')world.setGravity(...args);
        else if(method==='OptimizeBroadPhase'){world.live();world.system.OptimizeBroadPhase();}
        else {
          const body=bodies.get(args[0]);if(!body)throw Error('Expected a live body from this Jolt world');
          if(method==='RemoveBody'){world.remove(body);bodies.delete(args[0]);}
          else if(['GetPosition','GetRotation','GetLinearVelocity','GetAngularVelocity','IsActive'].includes(method))result=world.get(method,body);
          else if(['SetPosition','SetRotation','SetLinearVelocity','SetAngularVelocity','AddForce','AddImpulse','AddAngularImpulse','SetFriction','SetRestitution'].includes(method))world.set(method,body,args[1]);
          else throw Error('Unknown Jolt method');
        }
      }
      return encode.encode(JSON.stringify(result));
    },
    aqua_dispose(){if(closed)return;closed=true;for(const {world}of worlds.values())world.dispose();worlds.clear();}
  };
}
