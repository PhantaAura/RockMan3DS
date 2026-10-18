-- Installed Redux API shape test, isolated NO-GAME process only.
-- Registers/read-only breakpoint instrumentation; no emulator-memory writes,
-- pad override, reset, resume/pause or save access. Host launcher owns quit.
assert(PCSX and PCSX.addBreakpoint and PCSX.getRegisters)
local handles={}
local ok,failure=pcall(function()
 local registers=PCSX.getRegisters()
 for _,name in ipairs({'CP2C','CP2D'}) do
  for i=0,31 do
   local word=assert(tonumber(registers[name].r[i]),'Missing raw '..name..'.r')%4294967296
   assert(word>=0 and word<=4294967295)
  end
 end
 local addresses={0x80068000,0x000000a0,0x801022fc}
 for i=0,127 do addresses[#addresses+1]=0x80120000+i*4 end
 for _,address in ipairs(addresses) do
  local handle=PCSX.addBreakpoint(address,'Exec',4,
      'Read-only isolated observer narrow API test',function() return true end)
  handles[#handles+1]=handle
  assert(handle:isEnabled());handle:disable();assert(not handle:isEnabled())
  handle:enable();assert(handle:isEnabled());handle:disable()
 end
 assert(#handles==131,'Required retained callback capacity was not allocated')

end)
for _,handle in ipairs(handles) do handle:remove() end
assert(ok,failure)
print('{"schema":"mml-redux-observer-binding-test/1","status":"PASS_INSTALLED_API",'..
 '"raw_cp2_words_read":64,"retained_return_capacity_checked":128,"range_listeners_tested":0,"leaf_listeners_tested":131,'..
 '"listeners_removed":131,"no_game_loaded":true,"real_emulator_state_mutated":false,'..
 '"callback_execution_proved":false,"retail_parity":false}')
