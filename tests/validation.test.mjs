import test from 'node:test';
import assert from 'node:assert/strict';
import {number,vector,mass,dimension} from '../src/physics.mjs';
for(const value of [NaN,Infinity,-Infinity,1000001,-1000001,'0',null])test(`physics rejects invalid scalar ${String(value)}`,()=>assert.throws(()=>number(value)));
for(const value of [-1,.0000001,.000999999,1000001])test(`physics rejects invalid mass ${value}`,()=>assert.throws(()=>mass(value)));
for(const value of [0,.00001,-1,10001])test(`physics rejects invalid dimension ${value}`,()=>assert.throws(()=>dimension(value)));
test('static mass and minimum dynamic mass remain distinct',()=>{assert.equal(mass(0),0);assert.equal(mass(.001),.001);});
test('physics vector validates every component and dimension',()=>{assert.deepEqual(vector([1,2,3]),[1,2,3]);assert.throws(()=>vector([1,2]));assert.throws(()=>vector([1,2,Infinity]));});
