-- AUTHORED lifecycle harness: fake read-only PCSX / IO, no retail payload.
-- lua tests/test_player_lease_observer_lifecycle.lua /absolute/observer.lua
local source=MML_PLAYER_LEASE_HARNESS_SOURCE or (arg and arg[1])
assert(type(source)=='string' and #source>0,'Explicit observer source required')
local chunk=assert(loadfile(source))
local nativeOpen,nativePrint,nativePCSX=io.open,print,PCSX
local keys={'MML_OWNER_EXE_PATH','MML_OWNER_EXE_SHA256','MML_PLAYER_LEASE_TRACE_PATH',
            'MML_PLAYER_LEASE_MAX_ROWS','MML_PLAYER_LEASE_OBSERVER_01424'}
local prior={};for _,key in ipairs(keys) do prior[key]=_G[key] end
local ENTRY,BASE,CALLER=0x80035508,0x80010000,0x80044000
local function byte32(value,i) return math.floor(value/2^(8*i))%256 end
local function guard_byte(offset) return (offset*13+11)%256 end
local function scalar(value) return value>=0x80000000 and value-4294967296 or value end
local function count(lines,kind)
 local n=0;for _,line in ipairs(lines) do if line:find('"kind":"'..kind..'"',1,true) then n=n+1 end end
 return n
end
local current
local function install(name,maximum)
 MML_PLAYER_LEASE_OBSERVER_01424=nil
 local h={lines={},handles={},opened=0,closed=0,active=0,added=0,outputClosed=false,
          values={},pointers={},mask=0,ownerReads=0,registers={GPR={n={ra=CALLER}},CP2C={r={}},CP2D={r={}}}}
 current=h
 for i=0,31 do h.registers.CP2C.r[i]=scalar((0xfedcba98+i)%4294967296)
               h.registers.CP2D.r[i]=scalar((0x87654321+i)%4294967296) end
 for slot=0,23 do
  local pointer=0x80180000+slot*0x100;h.pointers[slot+1]=pointer
  h.values[pointer%0x20000000+0xbc]=scalar(0xffffffff-slot)
  h.values[pointer%0x20000000+0xc0]=scalar(0x80000000+slot)
 end
 MML_OWNER_EXE_PATH='/authored/owner-exe'
 MML_OWNER_EXE_SHA256='10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
 MML_PLAYER_LEASE_TRACE_PATH='/authored/lease/'..name
 MML_PLAYER_LEASE_MAX_ROWS=maximum or 8192
 io.open=function(path,mode)
  if path==MML_PLAYER_LEASE_TRACE_PATH then
   if mode=='r' then return nil end
   assert(mode=='w');return {
    write=function(_,text) assert(not h.outputClosed);h.lines[#h.lines+1]=text end,
    flush=function() assert(not h.outputClosed) end,
    close=function() h.outputClosed=true end}
  end
  assert(path==MML_OWNER_EXE_PATH and mode=='rb','Forbidden real file access')
  local position,closed=0,false
  return {
   read=function(_,size)
    assert(not closed and size==32);local bytes={}
    for i=0,size-1 do
     local offset=position+i;local value
     if offset<8 then value=('PS-X EXE'):byte(offset+1)
     elseif offset>=24 and offset<28 then value=byte32(BASE,offset-24)
     elseif offset<32 then value=0
     else value=guard_byte(offset) end
     bytes[#bytes+1]=string.char(value)
    end
    position=position+size;return table.concat(bytes)
   end,
   seek=function(_,origin,offset) assert(not closed and origin=='set');position=offset;return offset end,
   close=function() assert(not closed);closed=true end}
 end
 print=function() end
 PCSX={}
 function PCSX.addBreakpoint(address,kind,width,label,callback)
  assert(kind=='Exec' and width==4 and type(label)=='string' and type(callback)=='function')
  local bp={address=address,callback=callback,active=true,enabled=true}
  function bp:disable() if self.active then self.enabled=false end end
  function bp:enable() if self.active then self.enabled=true end end
  function bp:remove() if self.active then self.active=false;self.enabled=false;h.active=h.active-1 end end
  h.handles[#h.handles+1]=bp;h.active=h.active+1;h.added=h.added+1;return bp
 end
 function PCSX.getRegisters() return h.registers end
 function PCSX.getMemoryAsFile()
  h.opened=h.opened+1;local closed=false
  return {
   readU8At=function(_,address)
    assert(not closed and address>=ENTRY and address<ENTRY+32)
    return (guard_byte(2048+address-BASE)+(h.codeCorrupt and 1 or 0))%256
   end,
   readI32At=function(_,address)
    assert(not closed)
    if h.readError then error('Authored read exception') end
    if address>=0x1f800008 and address<0x1f800028 and address%4==0 then
     return scalar(0xf0000000+(address-0x1f800008)/4)
    end
    if address==0x80098798 then return scalar(h.mask) end
    if address>=0x800be478 and address<0x800be4d8 and address%4==0 then
     return scalar(h.pointers[1+(address-0x800be478)/4])
    end
    local physical=address%0x20000000
    assert(h.values[physical]~=nil,'Unmodeled owner read / attempted pointer dereference')
    h.ownerReads=h.ownerReads+1;return h.values[physical]
   end,
   close=function() assert(not closed);closed=true;h.closed=h.closed+1 end}
 end
 -- No write/reset/save/pad/break/resume/quit binding exists in the fake PCSX.
 function h:visit(address,width)
  self.registers.pc=address
  for _,bp in ipairs(self.handles) do
   if bp.active and bp.enabled and bp.address%0x20000000==address%0x20000000 then
    local keep=bp.callback(address,width or 4,'authored Exec')
    if keep==false then bp:remove() end
   end
  end
 end
 function h:enter(caller) self.registers.GPR.n.ra=caller or CALLER;self:visit(ENTRY) end
 function h:returnTo(caller)
  -- The harness, never the observer, simulates original source stores.
  for slot=0,23 do
   if math.floor(self.mask/2^(31-slot))%2==1 then
    local physical=self.pointers[slot+1]%0x20000000
    self.values[physical+0xbc]=0;self.values[physical+0xc0]=0
   end
  end
  for i=0,31 do self.registers.CP2C.r[i]=scalar(0x90000000+i)
               self.registers.CP2D.r[i]=scalar(0x80000000+i) end
  self:visit(caller or CALLER)
 end
 chunk();h.observer=MML_PLAYER_LEASE_OBSERVER_01424
 assert(h.active==1 and h.added==1 and count(h.lines,'lease_observer_start')==1)
 return h
end
local function close(h)
 h.observer.close();h.observer.close()
 assert(h.active==0 and h.outputClosed and h.opened==h.closed,'Observer leaked a handle')
 for _,bp in ipairs(h.handles) do assert(not bp.active) end
end
local function failed(h,reason)
 assert(h.observer.status_readonly().finished)
 assert(count(h.lines,'lease_observer_guard_failure')==1)
 assert(h.lines[#h.lines]:find('"reason":"'..reason..'"',1,true))
 local before=#h.lines;h:visit(CALLER);h:visit(ENTRY);assert(#h.lines==before)
 close(h)
end
local function run()
 local h=install('longrun')
 h.mask=0xffffffff
 for ordinal=1,4000 do
  local previous=#h.lines
  h:visit(CALLER);assert(#h.lines==previous,'Idle return incorrectly captured')
  h:enter();assert(h.observer.status_readonly().pending_return)
  h:visit(CALLER+4);assert(#h.lines==previous+1)
  h:returnTo();assert(not h.observer.status_readonly().pending_return)
  assert(h.observer.status_readonly().ordinal==ordinal and h.added==2 and h.active==2)
 end
 assert(count(h.lines,'player_lease_enter')==4000 and count(h.lines,'player_lease_return')==4000)
 assert(count(h.lines,'lease_observer_guard_failure')==0 and h.ownerReads==4000*24*4)
 assert(h.lines[2]:find('"words_bc_c0":[4294967295,2147483648]',1,true))
 assert(h.lines[2]:find('"cp2c":[4275878552,4275878553',1,true))
 assert(h.lines[3]:find('"cp2d":[2147483648,2147483649',1,true))
 close(h)
 local alias=install('alias-order');alias.mask=0x80000100
 alias.pointers[1]=0x00180000;alias.pointers[24]=0xa0180000
 alias:enter();assert(alias.ownerReads==4)
 assert(alias.lines[2]:find('"slot":0,"pointer":1572864,"physical":1572864,"alias_first_slot":0',1,true))
 assert(alias.lines[2]:find('"slot":23,"pointer":2685927424,"physical":1572864,"alias_first_slot":0',1,true))
 alias:returnTo();assert(alias.ownerReads==8)
 assert(alias.lines[3]:find('"words_bc_c0":[0,0]',1,true))
 close(alias)
 local inactive=install('inactive-invalid-pointers');inactive.mask=255
 for i=1,24 do inactive.pointers[i]=0x1f801234 end
 inactive:enter();inactive:returnTo();assert(inactive.ownerReads==0)
 assert(inactive.lines[2]:find('"selected_owners":[]',1,true));close(inactive)
 for _,pointer in ipairs({0,0x801ffff0,0x1f800000,0x1fc00000,0x80180001,0xc0180000,0x80200000}) do
  local bad=install('pointer-'..pointer);bad.mask=0x80000000;bad.pointers[1]=pointer
  bad:enter();failed(bad,'selected_owner_not_in_original_ram')
 end
 local changed=install('second-caller');changed:enter();changed:returnTo();changed:enter(CALLER+4)
 assert(changed.added==2 and not changed.observer.status_readonly().pending_return)
 failed(changed,'return_listener_capacity')
 local nested=install('nested');nested:enter();nested:enter();failed(nested,'unexpected_nested_lease_call')
 local corrupt=install('entry-identity');corrupt.codeCorrupt=true;corrupt:enter();failed(corrupt,'lease_code_identity')
 local returnCorrupt=install('return-identity');returnCorrupt:enter();returnCorrupt.codeCorrupt=true
 returnCorrupt:returnTo();failed(returnCorrupt,'lease_code_identity_at_return')
 local exception=install('read-exception');exception.readError=true;exception:enter()
 failed(exception,'lua_callback_error')
 local cp2=install('missing-cp2');cp2.registers.CP2C=nil;cp2:enter();failed(cp2,'lua_callback_error')
 local width=install('return-width');width:enter();width:visit(CALLER,2)
 failed(width,'unexpected_return_execution_width')
 local budget=install('budget',16)
 for i=1,6 do budget:enter();budget:returnTo() end
 budget:enter();assert(budget.observer.status_readonly().finished)
 assert(not budget.observer.status_readonly().pending_return and #budget.lines==14)
 assert(count(budget.lines,'lease_observer_budget_complete')==1);close(budget)
 local pending=install('close-pending');pending:enter();close(pending)
end
local ok,failure=xpcall(run,debug.traceback)
if current and current.observer then pcall(current.observer.close) end
io.open=nativeOpen;print=nativePrint;PCSX=nativePCSX
for _,key in ipairs(keys) do _G[key]=prior[key] end
assert(ok,failure)
nativePrint('{"schema":"mml-player-lease-observer-authored-harness/1",'..
 '"status":"PASS_AUTHORED_LIFECYCLE","longrun_returns":4000,"selected_owner_snapshots":192000,'..
 '"total_registrations":2,"return_listener_capacity":1,"aliased_owners_preserved":true,'..
 '"inactive_pointer_dereferences":0,"pointer_guard_cases":7,"budget_returns":6,'..
 '"failure_status":"UNKNOWN","failed_whole_trace_accepted":false,"close_active_listeners":0,'..
 '"synthetic_guards":true,"real_owner_files_used":false,"real_emulator_state_mutated":false,"retail_parity":false}')
