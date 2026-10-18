-- Fixed bounded observations only. No RAM writes, pad overrides, or save API.
local base='/Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/pass-20261004-worldalive/ps1-contact-reference/'
local vsync=0
local pad=PCSX.SIO0.slots[1].pads[1]
local startFrames,crossFrames,startLast,crossLast=0,0,0,0
local function pressed(button) return pad.getButton(button) end
local function unsigned32(v) return tonumber(v) % 4294967296 end
local function snapshot()
 local m=PCSX.getMemoryAsFile()
 local r=PCSX.getRegisters()
 local player={}
 for i=0,2 do player[#player+1]=m:readI16At(0x800B51C4+2*i) end
 local idx={}
 for _,i in ipairs({78,94,123}) do
  local address=0x8015C000+12*i
  local raw6=m:readI16At(address+6)%65536
  local pointer=unsigned32(m:readI32At(address+8))
  local default=m:readI32At(({[78]=0x80194630,[94]=0x801947B0,[123]=0x80194600})[i]+8)
  idx[#idx+1]=string.format('{"index":%d,"raw6":%d,"descriptor_word":%u,"default_contact_pointer":%u}',i,raw6,pointer,unsigned32(default))
 end
 local actors={}
 for slot=0,15 do
  local work=0x8009F108+slot*0x3C0
  local active=m:readU8At(work)
  local class=m:readU8At(work+1)
  local subtype=m:readU8At(work+2)
  if active%2==1 and math.floor(class/32)==1 and subtype==3 then
   actors[#actors+1]=string.format('{\"slot\":%d,\"work\":%u,\"active\":%d,\"state\":%d,\"stats_index\":%d,\"context_high_byte\":%d,\"hp\":%d}',slot,work,active,m:readU8At(work+8),m:readU8At(work+0x0C),m:readU8At(work+0x0F),m:readI16At(work+0x6E))
  end
 end
 local result=string.format('{"vsync":%d,"pc":%u,"gp":%u,"ram_u32_at_0x00000008":%u,"ram_u32_at_0x80000008":%u,"raw_stats_selector_at_0x800c1b6b":%d,"stats_table_pointer_at_0x80098844":%u,"map_idx_pointer_at_0x1f80004c":%u,"player_class_slot_raw":%d,"player_state":%d,"player_position":[%s],"idx":[%s],"actors_class20_03":[%s],"pad_start_pressed":%s,"pad_cross_pressed":%s,"pad_start_frames_observed":%d,"pad_cross_frames_observed":%d,"pad_start_last_vsync":%d,"pad_cross_last_vsync":%d}',vsync,unsigned32(r.pc),unsigned32(r.GPR.n.gp),unsigned32(m:readI32At(8)),unsigned32(m:readI32At(0x80000008)),m:readU8At(0x800C1B6B),unsigned32(m:readI32At(0x80098844)),unsigned32(m:readI32At(0x1F80004C)),m:readU8At(0x800B51B1),m:readU8At(0x800B51B8),table.concat(player,','),table.concat(idx,','),table.concat(actors,','),tostring(pressed(3)),tostring(pressed(14)),startFrames,crossFrames,startLast,crossLast)
 m:close()
 return result
end
reduxContactObserverVsync=PCSX.Events.createEventListener('GPU::Vsync',function()
 vsync=vsync+1
 if pressed(3) then startFrames=startFrames+1;startLast=vsync end
 if pressed(14) then crossFrames=crossFrames+1;crossLast=vsync end
 if vsync%60==0 and vsync<=12000 then
  local file=io.open(base..'observations-r4.jsonl','a');file:write(snapshot()..'\n');file:close()
 end
end)
PCSX.nextTick(function()
 PCSX.WebServer=PCSX.WebServer or {}
 PCSX.WebServer.Handlers=PCSX.WebServer.Handlers or {}
 PCSX.WebServer.Handlers.scalar=snapshot
 PCSX.WebServer.Handlers.pause=function() PCSX.pauseEmulator();return 'paused' end
 PCSX.WebServer.Handlers.resume=function() PCSX.resumeEmulator();return 'resumed' end
 PCSX.WebServer.Handlers.quit=function() PCSX.quit();return 'quitting' end
 print('Bounded read-only contact and pressed-pad observer ready')
end)
