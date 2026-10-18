-- AUTHORED harness only. No actual BIOS/EXE/overlay, pads, emulated memory,
-- process control or saves are used. Execution evidence is recorded separately
-- in docs/validation_01424/rng-observer-lifecycle.json.
-- lua tests/test_rng_observer_lifecycle.lua /absolute/path/to/observer.lua
-- Redux test bootstrap may instead set MML_OBSERVER_HARNESS_SOURCE.
local source = MML_OBSERVER_HARNESS_SOURCE or (arg and arg[1])
assert(type(source) == 'string' and #source > 0, 'Explicit observer source path is required')
local observerChunk = assert(loadfile(source))
local nativeOpen, nativePrint, nativePCSX = io.open, print, PCSX
local keys = {'MML_RNG_BIOS_PATH','MML_RNG_BIOS_SHA256','MML_OWNER_EXE_PATH',
 'MML_OWNER_EXE_SHA256','MML_RNG_ST00_PATH','MML_RNG_OVERLAY_SHA256',
 'MML_RNG_TRACE_PATH','MML_RNG_MAX_ROWS','MML_RNG_CAPTURE_SCOPE',
 'MML_RNG_ENABLE_HTTP_STATUS','MML_RNG_REFERENCE_OBSERVER_01420'}
local prior = {}
for _,key in ipairs(keys) do prior[key] = _G[key] end
local entry, load, callerBase = 0x80068000, 0x80010000, 0x80120000
local function byte32(value, index) return math.floor(value / 2^(8*index)) % 256 end
local function syntheticByte(profile, offset)
 if profile == 'exe' and offset < 32 then
  if offset < 8 then return ('PS-X EXE'):byte(offset+1) end
  if offset >= 16 and offset < 20 then return byte32(entry, offset-16) end
  if offset >= 24 and offset < 28 then return byte32(load, offset-24) end
  return 0
 end
 if profile == 'st00' and offset < 80 then
  if offset >= 4 and offset < 8 then return byte32(141056, offset-4) end
  if offset >= 12 and offset < 16 then return byte32(0x80100000, offset-12) end
  return 0
 end
 -- Deliberately authored synthetic guard bytes; no real instruction payload.
 return (offset*17 + (profile == 'bios' and 3 or profile == 'exe' and 5 or 7)) % 256
end
local function countKind(lines, kind)
 local count = 0
 for _,line in ipairs(lines) do if line:find('"kind":"'..kind..'"',1,true) then count=count+1 end end
 return count
end
local current
local function install(name, maximum)
 MML_RNG_REFERENCE_OBSERVER_01420=nil
 local h={lines={},byAddress={},handles={},events={},seed=0,
  active=0,peak=0,added=0,eventActive=0,memoryOpened=0,memoryClosed=0,
  outputClosed=false,enabled=0,enabledPeak=0,registers={GPR={n={t1=0x2f,ra=callerBase,a0=0,v0=0,s0=0}}}}
 current=h
 MML_RNG_BIOS_PATH='/authored/bios';MML_OWNER_EXE_PATH='/authored/exe'
 MML_RNG_ST00_PATH='/authored/st00';MML_RNG_TRACE_PATH='/authored/trace/'..name
 MML_RNG_BIOS_SHA256='713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d'
 MML_OWNER_EXE_SHA256='10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
 MML_RNG_OVERLAY_SHA256='9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
 MML_RNG_MAX_ROWS=maximum or 65536;MML_RNG_CAPTURE_SCOPE='CURRENT_INSTALLATION';MML_RNG_ENABLE_HTTP_STATUS=false
 local paths={['/authored/bios']='bios',['/authored/exe']='exe',['/authored/st00']='st00'}
 io.open=function(path, mode)
  if path==MML_RNG_TRACE_PATH then
   if mode=='r' then return nil end
   assert(mode=='w','Harness forbids other output modes')
   return {
    write=function(_,text) assert(not h.outputClosed);h.lines[#h.lines+1]=text end,
    flush=function() assert(not h.outputClosed) end,
    close=function() h.outputClosed=true end}
  end
  local profile=assert(paths[path],'Harness forbids real owner-file access')
  assert(mode=='rb');local position,closed=0,false
  return {
   seek=function(_,origin,offset) assert(origin=='set' and not closed);position=offset;return offset end,
   read=function(_,size)
    assert(not closed and size<=2048);local bytes={}
    for i=0,size-1 do bytes[#bytes+1]=string.char(syntheticByte(profile,position+i)) end
    position=position+size;return table.concat(bytes)
   end,
   close=function() closed=true end}
 end
 print=function() end -- The harness emits one result after restoring globals.
 PCSX={Events={}}
 function PCSX.addBreakpoint(address, kind, width, label, callback)
  assert(kind=='Exec' and (width==4 or width==0x200000 or width==0x80000) and
         type(callback)=='function' and type(label)=='string')
  local bp={address=address,width=width,callback=callback,active=true,enabled=true}
  function bp:disable()
   if self.active and self.enabled then self.enabled=false;h.enabled=h.enabled-1 end
  end
  function bp:enable()
   if self.active and not self.enabled then
    self.enabled=true;h.enabled=h.enabled+1;h.enabledPeak=math.max(h.enabledPeak,h.enabled)
   end
  end
  function bp:isEnabled() return self.active and self.enabled end
  function bp:remove()
   if self.active then self:disable();self.active=false;h.active=h.active-1 end
  end
  h.byAddress[address]=h.byAddress[address] or {};table.insert(h.byAddress[address],bp)
  h.handles[#h.handles+1]=bp;h.added=h.added+1;h.active=h.active+1;h.peak=math.max(h.peak,h.active)
  h.enabled=h.enabled+1;h.enabledPeak=math.max(h.enabledPeak,h.enabled)
  return bp
 end
 function PCSX.Events.createEventListener(name, callback)
  assert(name=='GPU::Vsync');local event={callback=callback,active=true}
  function event:remove() if self.active then self.active=false;h.eventActive=h.eventActive-1 end end
  h.events[#h.events+1]=event;h.eventActive=h.eventActive+1;return event
 end
 function PCSX.getRegisters() return h.registers end
 function PCSX.getMemoryAsFile()
  h.memoryOpened=h.memoryOpened+1;local closed=false
  return {
   readU8At=function(_,address)
    assert(not closed)
    if address>=0xbfc060e4 and address<0xbfc060e4+56 then
     return (syntheticByte('bios',address-0xbfc00000)+(h.biosCorrupt and 1 or 0))%256
    end
    if address>=entry and address<entry+32 then return syntheticByte('exe',2048+address-load) end
    if address>=0x8010226c and address<0x8010226c+148 then return syntheticByte('st00',2048+address-0x80100000) end
    error('Harness forbids unmodeled memory reads')
   end,
   readI32At=function(_,address) assert(not closed and address==0x8448);return h.seed end,
   close=function() assert(not closed);closed=true;h.memoryClosed=h.memoryClosed+1 end}
 end
 -- No write/reset/save/pad/break/resume binding exists in this fake PCSX.
 local function normalize(address)
  local physical=address%0x20000000
  if physical<0x800000 then physical=physical%0x200000 end
  return physical
 end
 function h:visit(address, width)
  self.registers.pc=address
  local accessed=normalize(address)
  -- The documented invoker receives the original execution address, even
  -- though Redux uses normalized addresses to find intersecting listeners.
  for _,bp in ipairs(self.handles) do
   local start=normalize(bp.address)
   if bp.active and bp.enabled and accessed>=start and accessed<start+bp.width then
    local keep=bp.callback(address,width or 4,'authored Exec')
    if keep==false then bp:remove() end
   end
  end
 end
 function h:enter(caller, service)
  self.registers.GPR.n.t1=service or 0x2f;self.registers.GPR.n.ra=caller;self:visit(0xa0)
 end
 function h:returnTo(caller, scalar)
  -- Authored observed values only; this harness tests listener lifecycle,
  -- not a second implementation of the selected BIOS recurrence.
  self.seed=self.seed+1;self.registers.GPR.n.v0=scalar;self:visit(caller)
 end
 observerChunk();h.observer=MML_RNG_REFERENCE_OBSERVER_01420
 assert(h.active==3 and h.enabled==3 and h.eventActive==1 and countKind(h.lines,'observer_start')==1)
 h:visit(entry);assert(h.observer.status_readonly().game_entries==1)
 return h
end
local function closeAndCheck(h)
 h.observer.close();h.observer.close()
 assert(h.active==0 and h.enabled==0 and h.eventActive==0 and h.outputClosed,'Close leaked retained listeners')
 assert(h.memoryOpened==h.memoryClosed,'Read-only memory handle leaked')
 for _,bp in ipairs(h.handles) do assert(not bp.active) end
end
local results
local function disabled(h)
 for _,bp in pairs(h.observer.returnBreakpoints) do assert(not bp:isEnabled()) end
end
local function failed(name, caller, reason)
 local h=install(name)
 h:enter(caller)
 local status=h.observer.status_readonly()
 assert(status.finished and status.pending_return and status.return_listeners==0)
 assert(countKind(h.lines,'bios_rng_enter')==1 and countKind(h.lines,'bios_rng_return')==0)
 assert(countKind(h.lines,'observer_guard_failure')==1)
 assert(h.lines[#h.lines]:find('"reason":"'..reason..'"',1,true))
 disabled(h)
 local rows=status.rows;h:visit(callerBase);h:visit(0xa0)
 assert(h.observer.status_readonly().rows==rows,'Failed capture continued recording')
 closeAndCheck(h)
end
local function run()
 local totalReturns=0
 for batch=1,4 do
  local h=install('longrun-'..batch)
  for ordinal=1,25000 do
   -- 128 exact callers, including physical/KSEG0/KSEG1 aliases. Retain each
   -- once; registration count remains131 after first128 calls, never percall.
   local index=(ordinal-1)%128
   local physical=0x120000+math.floor(index/3)*4
   local caller=physical+({0,0x80000000,0xa0000000})[1+index%3]
   local before=h.observer.status_readonly()
   h:visit(caller);assert(h.observer.status_readonly().rows==before.rows)
   h:enter(caller)
   assert(h.observer.status_readonly().pending_return)
   h:visit(caller+4)
   h:visit(physical+(caller>=0x80000000 and 0 or 0x80000000))
   assert(h.observer.status_readonly().pending_return)
   assert(h.observer.status_readonly().rows==before.rows+1)
   h:returnTo(caller,ordinal%32768)
   local status=h.observer.status_readonly()
   assert(not status.pending_return and not status.finished and status.rand_ordinal==ordinal)
   assert(status.return_listeners==math.min(ordinal,128) and h.added==3+math.min(ordinal,128))
   disabled(h)
  end
  assert(h.peak==131 and h.enabledPeak==4 and h.added==131)
  assert(countKind(h.lines,'bios_rng_return')==25000 and countKind(h.lines,'observer_guard_failure')==0)
  closeAndCheck(h);totalReturns=totalReturns+25000
 end
 local capacity=install('capacity')
 for ordinal=1,128 do
  local caller=callerBase+(ordinal-1)*4
  capacity:enter(caller);capacity:returnTo(caller,ordinal)
 end
 capacity:enter(callerBase+128*4)
 local full=capacity.observer.status_readonly()
 assert(full.finished and full.pending_return and full.return_listeners==128)
 assert(capacity.added==131 and capacity.peak==131 and countKind(capacity.lines,'bios_rng_return')==128)
 assert(capacity.lines[#capacity.lines]:find('"reason":"return_listener_capacity"',1,true))
 disabled(capacity);closeAndCheck(capacity)
 local domains=install('domains')
 for _,caller in ipairs({0,0x001ffffc,0x80000000,0x801ffffc,
                         0xa0000000,0xa01ffffc,0x1fc00000,0x1fc7fffc,
                         0x9fc00000,0x9fc7fffc,0xbfc00000,0xbfc7fffc}) do
  domains:enter(caller)
  assert(domains.observer.status_readonly().pending_return)
  domains:returnTo(caller,42);disabled(domains)
 end
 domains.registers.GPR.n.a0=1234
 local priorOrdinal=domains.observer.status_readonly().rand_ordinal
 domains:enter(callerBase,0x30);domains:returnTo(callerBase,99)
 assert(domains.observer.status_readonly().rand_ordinal==priorOrdinal,'srand incremented rand ordinal')
 assert(countKind(domains.lines,'bios_rng_return')==13)
 closeAndCheck(domains)
 for _,caller in ipairs({0x1f800000,0x1f000000,0x00200000,0x1fc80000,callerBase+1,0xc0120000,0x20120000,0xdfc00000}) do
  failed('unsupported-'..caller,caller,'unsupported_return_address')
 end
 local budget=install('budget',16)
 for ordinal=1,5 do budget:enter(callerBase);budget:returnTo(callerBase,ordinal) end
 budget:enter(callerBase)
 local stopped=budget.observer.status_readonly()
 assert(stopped.finished and not stopped.pending_return and stopped.rand_ordinal==5)
 assert(stopped.rows<=16 and countKind(budget.lines,'bios_rng_return')==5)
 assert(countKind(budget.lines,'observer_budget_complete')==1)
 disabled(budget);closeAndCheck(budget)
 local nested=install('nested')
 nested:enter(callerBase);nested:enter(callerBase+4)
 assert(nested.observer.status_readonly().finished and nested.observer.status_readonly().pending_return)
 assert(nested.lines[#nested.lines]:find('unexpected_nested_random_service',1,true))
 disabled(nested);closeAndCheck(nested)
 local corrupt=install('return-identity')
 corrupt:enter(callerBase);corrupt.biosCorrupt=true;corrupt:returnTo(callerBase,1)
 assert(corrupt.observer.status_readonly().finished and countKind(corrupt.lines,'bios_rng_return')==0)
 assert(corrupt.lines[#corrupt.lines]:find('bios_rng_identity_at_return',1,true))
 disabled(corrupt);closeAndCheck(corrupt)
 local width=install('return-width')
 width:enter(callerBase);width:visit(callerBase,2)
 assert(width.observer.status_readonly().finished and countKind(width.lines,'bios_rng_return')==0)
 assert(width.lines[#width.lines]:find('unexpected_return_execution_width',1,true))
 disabled(width);closeAndCheck(width)
 local pending=install('close-pending')
 pending:enter(callerBase);assert(pending.observer.status_readonly().pending_return)
 closeAndCheck(pending)
 results={longrun_returns=totalReturns,longrun_batches=4,distinct_callers=128,return_listeners=128,
  total_registrations=131,peak_active=131,peak_enabled=4,domain_returns=13,
  unsupported_domains=8,budget_returns=5,close_active_listeners=0}
end
local ok, failure=xpcall(run,debug.traceback)
if current and current.observer then pcall(current.observer.close) end
io.open=nativeOpen;print=nativePrint;PCSX=nativePCSX
for _,key in ipairs(keys) do _G[key]=prior[key] end
assert(ok,failure)
nativePrint(string.format('{"schema":"mml-rng-observer-authored-harness/2","status":"PASS_AUTHORED_LIFECYCLE",'..
 '"longrun_returns":%d,"distinct_callers":%d,"return_strategy":"bounded_retained_exact_callers",'..
 '"return_listeners":%d,"total_registrations":%d,"peak_active":%d,"peak_enabled":%d,'..
 '"capacity_complete_returns":128,"capacity_guard_129":true,"longrun_batches":4,"domain_returns":%d,"unsupported_domains":%d,"budget_returns":%d,'..
 '"failure_status":"UNKNOWN","failed_whole_trace_accepted":false,"close_active_listeners":0,'..
 '"synthetic_guards":true,"real_owner_files_used":false,"real_emulator_state_mutated":false,"retail_parity":false}',
 results.longrun_returns,results.distinct_callers,results.return_listeners,results.total_registrations,
 results.peak_active,results.peak_enabled,results.domain_returns,results.unsupported_domains,results.budget_returns))
