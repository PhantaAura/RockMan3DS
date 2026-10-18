-- Read-only semantic scene/RNG boundaries paired with inherited global observer.
-- No memory/register/pad/save writes, no HTTP bridge or execution control.
assert(MML_RNG_REFERENCE_OBSERVER_01420 and MML_SCENE_RNG_TRACE and MML_RNG_ST00_PATH)
local global=MML_RNG_REFERENCE_OBSERVER_01420
local existing=io.open(MML_SCENE_RNG_TRACE,'r');if existing then existing:close();error('Trace exists')end
local output=assert(io.open(MML_SCENE_RNG_TRACE,'w'))
local obs={breakpoints={}};MML_SCENE_RNG_OBSERVER_01434=obs
local rows,loops,entries,current=0,0,0,nil
local function u(v)return tonumber(v)%4294967296 end
local function emit(line)if rows>=4096 then return false end;output:write(line..'\n');output:flush();rows=rows+1;return true end
local function bytes(address)
 local file=assert(io.open(MML_RNG_ST00_PATH,'rb'));assert(file:seek('set',2048+address-0x80100000));local b=assert(file:read(16));file:close();assert(#b==16);return b
end
local function wrapper(address,label,fn)
 local guard=bytes(address)
 local point=PCSX.addBreakpoint(address,'Exec',4,label,function()
  if rows>=4096 then return false end
  local ok,keep=pcall(function()
   local memory=PCSX.getMemoryAsFile();local matches=true
   for i=1,16 do if memory:readU8At(address+i-1)~=guard:byte(i)then matches=false;break end end
   if not matches then memory:close();return true end
   local result=fn(memory);memory:close();return result
  end)
  if not ok then emit('{"kind":"observer_error","read_only":true,"retail_parity":false}');print(keep);return false end
  return keep
 end)
 obs.breakpoints[#obs.breakpoints+1]=point
end
local function context(m,kind,pc)
 local s=global.status_readonly();local actor=u(m:readI32At(0x800c4c6c));local position={0,0,0};local actor_state=255
 local slot=(actor-0x800c3580)/1444;local valid=slot>=0 and slot<4 and slot==math.floor(slot)
 if valid then for i=0,2 do position[i+1]=m:readI16At(actor+20+i*2)end;actor_state=m:readU8At(actor+8)end
 local registers=PCSX.getRegisters().GPR.n
 local value={frame=u(m:readI32At(0x800c4c58)),seed=u(m:readI32At(0x8448)),ordinal=s.rand_ordinal,actor=actor,position=position,entry=entries}
 emit(string.format('{"kind":"%s","pc":%u,"loops":%u,"vsync":%u,"seed32":%u,"rand_ordinal":%u,"pending_rng":%s,"event_active":%u,"event_id":%u,"event_phase":%u,"event_frame":%u,"source_actor":%u,"actor_valid":%s,"actor_state08":%u,"position":[%d,%d,%d],"pool80_low":%u,"pool80_high":%u,"scratch_half2":%u,"scratch0":%u,"scratch1":%u,"main_phase":%u,"main_subphase":%u,"a0":%u,"callback_entry":%u,"read_only":true,"retail_parity":false}',kind,pc,loops,s.vsync,value.seed,value.ordinal,tostring(s.pending_return),m:readU8At(0x800c4c48),m:readU8At(0x800c4c49),m:readU8At(0x800c4c4c),value.frame,actor,tostring(valid),actor_state,position[1],position[2],position[3],u(m:readI32At(0x80098838)),u(m:readI32At(0x8009883c)),m:readI16At(0x1f800002)%65536,m:readU8At(0x1f800000),m:readU8At(0x1f800001),m:readU8At(0x800c1b10),m:readU8At(0x800c1b11),u(registers.a0),entries))
 return value
end
-- Exact loop listener only counts observations; it never supplies a seed.
obs.breakpoints[#obs.breakpoints+1]=PCSX.addBreakpoint(0x80011ce8,'Exec',4,'Read-only scene loop count',function()loops=loops+1;return rows<4096 end)
wrapper(0x80100efc,'Read-only phase5 scene constructor',function(m)
 local v=context(m,'scene_constructor',0x80100efc);return false
end)
wrapper(0x80100d6c,'Read-only scene RNG callback entry',function(m)
 entries=entries+1;current=context(m,'scene_rng_callback',0x80100d6c);return true
end)
for _,pc in ipairs({0x80100dd4,0x80100dec,0x80100e04,0x80100e40,0x80100e58,0x80100e70})do
 wrapper(pc,'Read-only scene rand return',function(m)
  if not current then return true end
  local s=global.status_readonly();local r=PCSX.getRegisters().GPR.n
  emit(string.format('{"kind":"scene_rng_return","pc":%u,"callback_entry":%u,"event_frame":%u,"rand_ordinal":%u,"seed32":%u,"v0_actual":%u,"pending_rng":%s,"read_only":true,"retail_parity":false}',pc,current.entry,current.frame,s.rand_ordinal,u(m:readI32At(0x8448)),u(r.v0),tostring(s.pending_return)))
  return true
 end)
end
function obs.close()
 for _,point in ipairs(obs.breakpoints)do point:remove()end
 if output then output:close();output=nil end
end
emit('{"kind":"scene_observer_start","read_only":true,"retail_parity":false}')
