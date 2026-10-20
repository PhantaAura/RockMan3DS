-- Research probe: pad-driven walk of the original tutorial route.
-- Reads RAM and logs event/flag/area changes. Never writes emulated RAM.
local OUT = assert(io.open(MML_OUT, 'w'))
local mem = PCSX.getMemPtr()
local function u8(a) return mem[a - 0x80000000] end
local function s16(a) local v = u8(a) + u8(a+1)*256; if v >= 32768 then v = v - 65536 end; return v end
local function u16(a) return u8(a) + u8(a+1)*256 end
local function u32(a) return u16(a) + u16(a+2)*65536 end
local B = (PCSX.CONSTS or PCSX.CONSTANTS).PAD.BUTTON
local pad = PCSX.SIO0.slots[1].pads[1]
local held = {}
local function press(btn, on) if on and not held[btn] then pad.setOverride(btn); held[btn]=true elseif (not on) and held[btn] then pad.clearOverride(btn); held[btn]=false end end
local function release_all() for _,b in ipairs({B.UP,B.DOWN,B.LEFT,B.RIGHT,B.CROSS,B.CIRCLE,B.START,B.SQUARE,B.R2}) do press(b,false) end end
local P = 0x800B51B0
local frame = 0
local function log(s) OUT:write(string.format('f=%d %s\n', frame, s)); OUT:flush() end
local function hex(a,n) local t={} for i=0,n-1 do t[#t+1]=string.format('%02x',u8(a+i)) end return table.concat(t) end
local function pos() return s16(P+0x14), s16(P+0x16), s16(P+0x18) end
local shot_n = 0
local function snap(tag)
  local ok, err = pcall(function()
    local ss = PCSX.GPU.takeScreenShot()
    shot_n = shot_n + 1
    local f = assert(io.open(string.format('%s/shot_%03d_%s_%dx%d_%d.raw', MML_DIR, shot_n, tag, tonumber(ss.width), tonumber(ss.height), tonumber(ss.bpp)), 'wb'))
    f:write(tostring(ss.data)); f:close()
  end)
  log((ok and 'SNAP ' or 'SNAPFAIL ')..tag..' '..tostring(err))
end
-- call hooks (Exec breakpoints); arguments from registers
MML_BPS = {}
local function reg(n) local ok,r = pcall(function() return PCSX.getRegisters().GPR.n[n] end); return ok and r or -1 end
local function hook(addr, name)
  local ok, bp = pcall(PCSX.addBreakpoint, addr, 'Exec', 4, name, function()
    if name == 'door_event_80050C2C' and reg('a2') == 0 then return end
    local x,y,z = pos()
    log(string.format('CALL %s a0=%08x a1=%08x a2=%08x a3=%08x ra=%08x pos=%d:%d:%d', name, reg('a0'), reg('a1'), reg('a2'), reg('a3'), reg('ra'), x,y,z))
  end)
  if ok then MML_BPS[#MML_BPS+1] = bp else log('HOOKFAIL '..name..' '..tostring(bp)) end
end
hook(0x8001DDE4, 'event_init_8001DDE4')
-- Writes to the byte holding flag 1552 (0x800BE378 + 194).
do local ok, bp = pcall(PCSX.addBreakpoint, 0x800BE43A, 'Write', 1, 'flag1552', function()
  local r = PCSX.getRegisters(); log(string.format('WRITE flag_byte_194 pc=%08x ra=%08x', tonumber(r.pc), tonumber(r.GPR.n.ra))) end)
  if ok then MML_BPS[#MML_BPS+1] = bp else log('HOOKFAIL flagwrite '..tostring(bp)) end end
hook(0x80063BC8, 'message_request_80063BC8')
hook(0x80050C2C, 'door_event_80050C2C')
function dump_pool(tag)
  for i = 0, 15 do local a = 0x8009F108 + i*0x3C0
    if u8(a) ~= 0 then log(string.format('POOL20 %s s%d head=%s pos=%d:%d:%d', tag, i, hex(a,16), s16(a+0x14), s16(a+0x16), s16(a+0x18))) end end
end
-- state snapshots
local last = {}
local flags_prev = nil
local function watch()
  local s = string.format('stage=%d area=%d trans=%d busy=%d', u8(0x800C1B60), u8(0x800C1B61), u8(0x800C356C), s16(0x80098A72))
  if s ~= last.s then log('STATE '..s); last.s = s end
  local e = hex(0x800C4C48, 16)
  if e ~= last.e then log('EVENT '..e); last.e = e end
  local f = {}
  for i=0,255 do f[i]=u8(0x800BE378+i) end
  if flags_prev then
    for i=0,255 do if f[i] ~= flags_prev[i] then
      for b=0,7 do local m = 128 >> b
        local was = (flags_prev[i] & m) ~= 0; local now = (f[i] & m) ~= 0
        if was ~= now then log(string.format('FLAG %d %s', i*8+b, now and 'SET' or 'CLEAR')) end
      end end end
  end
  flags_prev = f
end
-- route (original coordinates, from a manual Area 1 walk)
local route = {{2822,-8112},{2736,-8415},{2593,-8660},{2771,-9105},{2777,-9647},{2641,-9770},{2228,-9820},
  {1625,-9894},{1151,-9884},{541,-9761},{198,-9815},{-151,-9974},{-365,-9889},{-324,-9505},{-257,-8931},
  {-281,-8513},{-219,-8290},{-143,-7462},{154,-6981},{267,-6758},{-139,-5784},{-256,-5650},{-256,-5300}}
local phase, wp, base_pos, move_at = 'boot', 1, nil, nil
local turn_sign, last_gap, stuck, last_xz, cross_t = 1, -1, 0, nil, 0
local door_t, area2_t, stuck_h, use_circle, cycle, last_h = 0, 0, 0, false, 0, -1
local function steer(tx, tz)
  local x,_,z = pos()
  local h = u16(P+0x56) & 0xfff
  local dx, dz = tx - x, tz - z
  if dx*dx + dz*dz < 60*60 then return true end
  local want = math.floor(math.atan2(-dx, -dz) * 4096 / (2*math.pi) + 0.5) % 4096
  local gap = (want - h) % 4096; if gap > 2048 then gap = gap - 4096 end
  local mag = math.abs(gap)
  if last_gap >= 0 and mag > last_gap + 2 and (held[B.LEFT] or held[B.RIGHT]) then turn_sign = -turn_sign end
  last_gap = mag
  local left = (gap > 0) == (turn_sign > 0)
  press(B.LEFT, mag > 64 and left); press(B.RIGHT, mag > 64 and not left)
  press(B.UP, mag < 160)
  return false
end
MML_PROBE_LISTENER = PCSX.Events.createEventListener('GPU::Vsync', function()
  frame = frame + 1
  watch()
  if frame % 300 == 0 then local x,y,z = pos(); log(string.format('POS %d:%d:%d h=%d ph=%s wp=%d p44d=%d p44e=%d', x,y,z,u16(P+0x56)&0xfff, phase, wp, u8(P+0x44d), u8(P+0x44e))) end
  if phase == 'boot' then
    local k = frame % 80; local slot = math.floor(frame / 80) % 2
    press(B.START, frame < 2100 and k < 3 and slot == 0)
    press(B.CROSS, k < 3 and (slot == 1 or frame >= 2100))
    if u8(P) == 3 and frame % 400 == 0 then base_pos = s16(P+0x18); move_at = frame end
    if move_at then
      press(B.UP, frame - move_at < 24)
      if frame - move_at == 40 then
        if s16(P+0x18) ~= base_pos then phase = 'route'; release_all(); log('CONTROL'); snap('control'); dump_pool('control') end
        move_at = nil
      end
    end
  elseif phase == 'route' then
    local x,_,z = pos()
    local xz = x..':'..z
    local hh = u16(P+0x56) & 0xfff
    if xz == last_xz and (hh == last_h or stuck > 30) then stuck = stuck + 1 else stuck = 0 end
    last_h = hh
    last_xz = xz
    -- A freeze while walking is usually a radio call: tap CROSS to advance it.
    -- Freeze: alternate CROSS and CIRCLE every 45 frames; snapshot the screen.
    -- Freeze: a dialogue or a tutorial prompt. Cycle CROSS, SQUARE, CIRCLE
    -- every 45 frames and snapshot each new screen.
    -- Run 1 completed the corridor sequence with CROSS taps every 90 frames.
    -- CROSS taps every 90 frames, each preceded by RIGHT so the
    -- "Read instructions again? Yes/No" prompt takes No.
    if stuck > 30 and stuck % 90 == 1 then cross_t = 10 end
    if stuck == 40 or (stuck > 0 and stuck % 150 == 0) then snap('stuck_wp'..wp) end
    local frozen = stuck > 30
    if frozen then press(B.UP, false); press(B.LEFT, false); press(B.RIGHT, true) end
    press(B.CROSS, cross_t > 0 and cross_t <= 4)
    if cross_t > 0 then cross_t = cross_t - 1 end
    if stuck > 0 and stuck % 600 == 0 then dump_pool('stuck') end
    -- Skip only when blocked by geometry: heading changes but position does not.
    local h = u16(P+0x56) & 0xfff
    if stuck == 1 then stuck_h = h end
    if stuck > 400 and h ~= stuck_h then log('SKIP wp='..wp); wp = wp + 1; stuck = 0 end
    if wp > #route then phase = 'door'; release_all(); log('AT_DOOR')
    elseif not frozen and steer(route[wp][1], route[wp][2]) then wp = wp + 1; last_gap = -1 end
  elseif phase == 'door' then
    door_t = door_t + 1
    press(B.UP, true); press(B.CIRCLE, door_t > 40 and door_t % 40 < 4)
    if u8(0x800C1B61) == 2 or door_t > 900 then phase = 'area2'; release_all(); log('AREA2_OR_TIMEOUT') end
  elseif phase == 'area2' then
    area2_t = area2_t + 1
    press(B.CROSS, area2_t % 120 < 3)
    if area2_t > 2400 then log('END'); OUT:close(); PCSX.quit(0) end
  end
  if frame > 60*60*20 then log('TIMEOUT'); OUT:close(); PCSX.quit(1) end
end)
log('PROBE_START')
