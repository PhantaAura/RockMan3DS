-- Research probe: at control start, log the terrain family-C back-face test
-- (0x80025E8C: v0 = scratch 0x1F8001C0, t0 = depth, s1 = primitive word) and
-- the positive mode byte 0x1F800075 (0x80023C18, v1). Never writes RAM.
local OUT = assert(io.open(MML_OUT, 'w'))
local mem = PCSX.getMemPtr()
local function u8(a) return mem[a - 0x80000000] end
local function s16(a) local v = u8(a) + u8(a+1)*256; if v >= 32768 then v = v - 65536 end; return v end
local B = (PCSX.CONSTS or PCSX.CONSTANTS).PAD.BUTTON
local pad = PCSX.SIO0.slots[1].pads[1]
local held = {}
local function press(btn, on) if on and not held[btn] then pad.setOverride(btn); held[btn]=true elseif (not on) and held[btn] then pad.clearOverride(btn); held[btn]=false end end
local P = 0x800B51B0
local frame, control_at, window = 0, nil, false
local drew_at = nil
local function log(s) OUT:write(string.format('f=%d %s\n', frame, s)); OUT:flush() end
local function R() return PCSX.getRegisters().GPR.n end
local counts = {culled=0, black=0, bit24=0}
local thr_seen = {}
MML_BPS = {}
local ok, bp = pcall(PCSX.addBreakpoint, 0x80025E8C, 'Exec', 4, 'famC_back', function()
  if not window then return end
  local r = R(); local thr = tonumber(r.v0); local t0 = tonumber(r.t0); local s1 = tonumber(r.s1)
  thr_seen[thr] = true
  local skip = not (thr < t0)          -- slt v0,v0,t0 ; beq v0,zero -> skip
  if skip then counts.culled = counts.culled + 1
  elseif (s1 & 0x01000000) ~= 0 then counts.bit24 = counts.bit24 + 1
  else counts.black = counts.black + 1 end
  if counts.culled + counts.black + counts.bit24 <= 40 then
    log(string.format('BACK thr=%d t0=%d s1=%08x -> %s', thr, t0, s1, skip and 'CULLED' or (((s1 & 0x01000000) ~= 0) and 'SKIP_BIT24' or 'BLACK')))
  end
end)
if ok then MML_BPS[#MML_BPS+1] = bp else log('HOOKFAIL famC '..tostring(bp)) end
local ok2, bp2 = pcall(PCSX.addBreakpoint, 0x80023C18, 'Exec', 4, 'mode75', function()
  if window then log(string.format('MODE75 v1=%d', tonumber(R().v1))) end
end)
if ok2 then MML_BPS[#MML_BPS+1] = bp2 end
local fam = {}
for _,a in ipairs({0x800240D0,0x80025160,0x80025B28,0x80026D2C,0x80021E74,0x800226A4,0x80025E70}) do
  fam[a]=0
  local okf, bpf = pcall(PCSX.addBreakpoint, a, 'Exec', 4, string.format('n%08x',a), function() if window then fam[a]=fam[a]+1 end end)
  if okf then MML_BPS[#MML_BPS+1]=bpf else log('HOOKFAIL '..string.format('%08x',a)..' '..tostring(bpf)) end
end
MML_LISTENER = PCSX.Events.createEventListener('GPU::Vsync', function()
  frame = frame + 1
  if not control_at then
    local k = frame % 80; local slot = math.floor(frame / 80) % 2
    press(B.START, frame < 2100 and k < 3 and slot == 0)
    press(B.CROSS, k < 3 and (slot == 1 or frame >= 2100))
    if u8(P) == 3 and s16(P+0x14) == 2822 and s16(P+0x18) == -4833 then control_at = frame; press(B.START,false); press(B.CROSS,false)
      log(string.format('CONTROL pos=%d:%d:%d', s16(P+0x14), s16(P+0x16), s16(P+0x18))) end
  else
    local t = frame - control_at
    window = true
    local total = 0; for _,n in pairs(fam) do total = total + n end
    if total > 0 and not drew_at then drew_at = frame; log('FIRST_TERRAIN_FRAME') end
    if (drew_at and frame - drew_at >= 30) or t > 1200 then
      log(string.format('SUMMARY culled=%d black=%d skip_bit24=%d', counts.culled, counts.black, counts.bit24))
      for k,_ in pairs(thr_seen) do log('THRESHOLD '..k) end
      for a,n in pairs(fam) do log(string.format('HITS %08x %d', a, n)) end
      local okp, err = pcall(function() local ss = PCSX.GPU.takeScreenShot()
        local f = assert(io.open(MML_DIR..'/shot_control_'..tonumber(ss.width)..'x'..tonumber(ss.height)..'_'..tonumber(ss.bpp)..'.raw','wb')); f:write(tostring(ss.data)); f:close() end)
      log('SNAP '..tostring(okp)..' '..tostring(err))
      log('END'); OUT:close(); PCSX.quit(0)
    end
  end
  if frame > 60*60*3 then log('TIMEOUT'); OUT:close(); PCSX.quit(1) end
end)
log('PROBE_START')
