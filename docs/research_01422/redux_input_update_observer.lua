-- Read-only independent input/update chronology observer. No pad, RAM,
-- register, execution-control or save writes. Install beside the RNG observer.
assert(type(MML_INPUT_TRACE_PATH)=='string' and MML_INPUT_TRACE_PATH:sub(1,1)=='/')
assert(MML_OWNER_EXE_SHA256=='10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0')
local maximum=MML_INPUT_MAX_ROWS or 32768
assert(maximum>=16 and maximum<=65536)
local owner=assert(io.open(MML_OWNER_EXE_PATH,'rb'))
local header=assert(owner:read(32))
local function le32(s,o)
 return s:byte(o+1)+s:byte(o+2)*256+s:byte(o+3)*65536+s:byte(o+4)*16777216
end
assert(header:sub(1,8)=='PS-X EXE')
assert(owner:seek('set',2048+0x80016FD4-le32(header,24)))
local identity=assert(owner:read(32));owner:close()
local existing=io.open(MML_INPUT_TRACE_PATH,'r')
if existing then existing:close();error('Refusing to overwrite input/update trace') end
local output=assert(io.open(MML_INPUT_TRACE_PATH,'w'))
local rows,vsync,updates=0,0,0
local admitted,failed=false,false
local pad=PCSX.SIO0.slots[1].pads[1]
local function unsigned(v) return tonumber(v)%4294967296 end
local function emit(s)
 if failed or rows>=maximum then return false end
 output:write(s..'\n');output:flush();rows=rows+1
 return rows<maximum
end
local scope=MML_INPUT_CAPTURE_SCOPE or 'CURRENT_INSTALLATION'
assert(scope=='CURRENT_INSTALLATION' or scope=='MID_SESSION')
emit('{"kind":"input_observer_start","scope":"'..scope..'","source_exe_sha256":"'..MML_OWNER_EXE_SHA256..'","pad_override":false,"retail_parity":false}')
local observer={}
-- Read-only raw owners for the source type4/CD pipeline. These are observed
-- fields, not generated completion receipts. The current record is read only
-- when its exact pointer belongs to the proved sixteen-record queue.
local function command_snapshot(m)
 local current=unsigned(m:readI32At(0x80098A84))
 local physical=current%0x20000000
 local record='null'
 if physical>=0xA3A40 and physical<0xA3B40 and (physical-0xA3A40)%16==0 then
  local words={}
  for i=0,3 do words[#words+1]=tostring(unsigned(m:readI32At(current+i*4))) end
  record='['..table.concat(words,',')..']'
 end
 local response,mix={},{}
 for i=0,7 do response[#response+1]=m:readU8At(0x80098A98+i) end
 for i=0,3 do mix[#mix+1]=m:readU8At(0x800AD154+i) end
 return string.format(',"source_command_state":{"word_80098a84":%u,"word_800989a4":%u,"phase_800989c4":%u,"counter_80098828":%u,"word_800989b0":%u,"word_800989d8":%u,"byte_80098964":%d,"byte_8009896c":%d,"byte_800988c0":%d,"byte_800988d0":%d,"byte_800988ec":%d,"byte_80098b42":%d,"byte_80098ab8":%d,"half_800ad142":%d,"half_800ad14e":%d,"callback_80091ffc":%u,"callback_80092000":%u,"response_80098a98":[%s],"mix_800ad154":[%s],"current_record":%s}',
  current,unsigned(m:readI32At(0x800989A4)),unsigned(m:readI32At(0x800989C4)),unsigned(m:readI32At(0x80098828)),unsigned(m:readI32At(0x800989B0)),unsigned(m:readI32At(0x800989D8)),
  m:readU8At(0x80098964),m:readU8At(0x8009896C),m:readU8At(0x800988C0),m:readU8At(0x800988D0),m:readU8At(0x800988EC),m:readU8At(0x80098B42),m:readU8At(0x80098AB8),
  m:readI16At(0x800AD142)%65536,m:readI16At(0x800AD14E)%65536,unsigned(m:readI32At(0x80091FFC)),unsigned(m:readI32At(0x80092000)),table.concat(response,','),table.concat(mix,','),record)
end
observer.vsync=PCSX.Events.createEventListener('GPU::Vsync',function()vsync=vsync+1 end)
observer.update=PCSX.addBreakpoint(0x80016FD4,'Exec',4,'Read-only original actor update and player/input chronology',function()
 if failed or rows>=maximum then return false end
 local ok,result=pcall(function()
  local m=PCSX.getMemoryAsFile()
  local matches=true
  for i=1,#identity do
   if m:readU8At(0x80016FD4+i-1)~=identity:byte(i) then matches=false;break end
  end
  if not matches then
   m:close()
   if admitted then emit('{"kind":"input_observer_guard_failure","reason":"resident_dispatcher_identity"}');failed=true;return false end
   return true
  end
  admitted=true;updates=updates+1
  local held=0
  for button=0,15 do if pad.getButton(button) then held=held+2^button end end
  local p=0x800B51B0
  local actors={}
  for slot=0,15 do
   local w=0x8009F108+slot*0x3C0
   local flags00,flags06=m:readU8At(w),m:readU8At(w+6)
   local marker,subtype=m:readU8At(w+1),m:readU8At(w+2)
   if flags00%2==1 and flags06%2==1 then
    actors[#actors+1]=string.format('{"slot":%d,"work":%u,"flags00":%d,"flags06":%d,"marker01":%d,"subtype02":%d,"state08":%d,"action09":%d,"phase0A":%d,"position14":[%d,%d,%d],"heading56":%d}',slot,w,flags00,flags06,marker,subtype,m:readU8At(w+8),m:readU8At(w+9),m:readU8At(w+10),m:readI16At(w+0x14),m:readI16At(w+0x16),m:readI16At(w+0x18),m:readI16At(w+0x56)%65536)
   end
  end
  local flags={}
  for id=0,2047 do
   if math.floor(m:readU8At(0x800BE378+math.floor(id/8))/2^(7-id%8))%2==1 then flags[#flags+1]=id end
  end
  local event=0x800C4C48
  local text=string.format('{"kind":"source_update_input","update_ordinal":%d,"reference_vsync":%d,"dispatcher_identity_matches":true,"pad_held_buttons":%d,"player_work":%u,"player_flags00":%d,"player_marker01":%d,"player_state08":%d,"player_action09":%d,"player_phase0A":%d,"player_position14":[%d,%d,%d],"player_words20":[%d,%d,%d],"player_velocity44":[%d,%d,%d],"player_heading56":%d,"player_hp6E":%d,"player_held11C":%d,"admitted_actor_slots":[%s],"stage_byte800C1B60":%d,"transition_byte800C356C":%d,"busy_halfword80098A72":%d,"event_active00":%d,"event_id01":%d,"event_phase04":%d,"story_flag_ids":[%s],"complete_native_chronology_matches":false}',updates,vsync,held,p,m:readU8At(p),m:readU8At(p+1),m:readU8At(p+8),m:readU8At(p+9),m:readU8At(p+10),m:readI16At(p+0x14),m:readI16At(p+0x16),m:readI16At(p+0x18),m:readI32At(p+0x20),m:readI32At(p+0x24),m:readI32At(p+0x28),m:readI16At(p+0x44),m:readI16At(p+0x46),m:readI16At(p+0x48),m:readI16At(p+0x56)%65536,m:readI16At(p+0x6E),m:readI16At(p+0x11C)%65536,table.concat(actors,','),m:readU8At(0x800C1B60),m:readU8At(0x800C356C),m:readI16At(0x80098A72),m:readU8At(event),m:readU8At(event+1),m:readU8At(event+4),table.concat(flags,','))
  text=text:sub(1,-2)..command_snapshot(m)..'}'
  local rng=MML_RNG_REFERENCE_OBSERVER_01420
  if rng and rng.status_readonly then
   local anchor=rng.status_readonly()
   text=text:sub(1,-2)..string.format(',"rng_reference_vsync":%d,"rng_rand_ordinal":%d,"rng_pending_return":%s}',anchor.vsync,anchor.rand_ordinal,tostring(anchor.pending_return))
  end
  m:close();return emit(text)
 end)
 if not ok then emit('{"kind":"input_observer_guard_failure","reason":"lua_callback_error"}');failed=true;print('Input observer stopped: '..tostring(result));return false end
 return result
end)
function observer.close()
 observer.update:remove();observer.vsync:remove();output:close();failed=true
end
MML_INPUT_REFERENCE_OBSERVER_01422=observer
print('Independent read-only input/update observer installed')
