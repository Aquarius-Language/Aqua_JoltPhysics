import {build} from 'esbuild';
await build({entryPoints:[new URL('./src/library.mjs',import.meta.url).pathname.replace(/^\/(\w:)/,'$1')],bundle:true,format:'esm',platform:'neutral',external:['module'],loader:{'.wasm':'binary'},outfile:new URL('./assets/jolt.mjs',import.meta.url).pathname.replace(/^\/(\w:)/,'$1'),minify:true});


