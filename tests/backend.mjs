import initJolt from '../assets/vendor-jolt.mjs';
import {JoltWorld,vector} from '../src/physics.mjs';
import {readFileSync} from 'node:fs';
let initialized;
export class BrowserJoltBackend {
 constructor(){this.worlds=new Set();}
 async createWorld(gravity){vector(gravity);initialized??=initJolt({wasmBinary:readFileSync(new URL('../assets/vendor-jolt.wasm',import.meta.url))});const w=new JoltWorld(await initialized,gravity);this.worlds.add(w);return w;}
 dispose(){for(const w of this.worlds)w.dispose();this.worlds.clear();}
}
