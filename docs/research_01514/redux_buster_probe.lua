-- Research probe: drives the pad to reach gameplay, then fires the Buster
-- and logs player pose points and class-0x10 shot objects. Private output.
local ffi = require('ffi')
local OUT = MML_OUT
local out = assert(io.open(OUT, 'w'))
local mem = PCSX.getMemPtr()
local function u8(a) return mem[a - 0x80000000] end
local function s16(a) local v = u8(a) + u8(a+1)*256; if v >= 32768 then v = v - 65536 end; return v end
local function xyz(a) return s16(a)..':'..s16(a+2)..':'..s16(a+4) end
local B = (PCSX.CONSTS or PCSX.CONSTANTS).PAD.BUTTON
local pad = PCSX.SIO0.slots[1].pads[1]
local held = {}
local function press(btn, on) if on and not held[btn] then pad.setOverride(btn); held[btn]=true elseif (not on) and held[btn] then pad.clearOverride(btn); held[btn]=false end end
local P = 0x800B51B0
local frame, stable, phase, fire_start = 0, 0, 'boot', nil
local function shots()
  local t = {}
  for i = 0, 15 do
    local a = 0x800BFB20 + i*0x10C
    if u8(a) ~= 0 then
      t[#t+1] = string.format('s%d[b0=%d st=%d sub=%02x t=%d e=%d p14=%s p2c=%s c4=%02x%02x%02x%02x]', i, u8(a), u8(a+8), u8(a+2), u8(a+11), u8(a+14), xyz(a+0x14), xyz(a+0x2c), u8(a+0xc7),u8(a+0xc6),u8(a+0xc5),u8(a+0xc4))
    end
  end
  return table.concat(t, ' ')
end
local function hex(a,n) local t={} for i=0,n-1 do t[#t+1]=string.format('%02x',u8(a+i)) end return table.concat(t) end
local function line(tag)
  out:write(string.format('%s f=%d ph=%s p0=%d p8=%d p9=%d pA=%d pos=%s h=%d m3c8=%s m3e0=%s e400=%s e418=%s x420=%s w170=%d w171=%d %s\n',
    tag, frame, phase, u8(P), u8(P+8), u8(P+9), u8(P+10), xyz(P+0x14), s16(P+0x56), xyz(P+0x3c8), xyz(P+0x3e0), xyz(P+0x400), xyz(P+0x418), xyz(P+0x420), u8(P+0x170), u8(P+0x171), shots()))
  out:flush()
end
local base_pos, move_at = nil, nil
local fire_buttons = {B.SQUARE, B.CIRCLE, B.SQUARE}
MML_PROBE_LISTENER = PCSX.Events.createEventListener('GPU::Vsync', function()
  frame = frame + 1
  if phase == 'boot' then
    local k = frame % 80
    local slot = math.floor(frame / 80) % 2
    press(B.START, frame < 2100 and k < 3 and slot == 0)
    press(B.CROSS, k < 3 and (slot == 1 or frame >= 2100))
    if frame % 120 == 0 then line('S') end
    -- movement probe every 400 frames once the player object is live
    if u8(P) == 3 and frame % 400 == 0 then base_pos = xyz(P+0x14); move_at = frame end
    if move_at then
      press(B.UP, frame - move_at < 24)
      if frame - move_at == 40 then
        if xyz(P+0x14) ~= base_pos then phase = 'settle'; press(B.CROSS,false); press(B.UP,false); fire_start = frame + 60; line('G') end
        move_at = nil
      end
    end
    if frame > 60*60*15 then line('TIMEOUT'); out:close(); PCSX.quit(1) end
  elseif phase == 'settle' then
    if frame >= fire_start then phase = 'fire' end
  elseif phase == 'fire' then
    local n = frame - fire_start
    local btn = fire_buttons[math.floor(n / 120) + 1]
    for _, b in ipairs({B.SQUARE, B.CIRCLE}) do press(b, b == btn and n % 40 < 4) end
    line('F')
    if n == 1 then out:write('STATS p160='..hex(P+0x160,0x20)..' p4f0='..hex(P+0x4f0,0x18)..' g5f80='..hex(0x800B5F80,0x10)..' g5fdc='..hex(0x800B5FDC,0x10)..'\n') end
    for i=0,15 do local a=0x800BFB20+i*0x10C; if u8(a)~=0 and u8(a+8)==1 and u8(a+11)==1 then out:write(string.format('SHOT s%d r98=%d v44=%d:%d:%d c4=%s b0=%s\n',i,s16(a+0x98),s16(a+0x44),s16(a+0x46),s16(a+0x48),hex(a+0xc4,8),hex(a,0x10))) end end
    if n > 360 then press(B.SQUARE,false); press(B.CIRCLE,false); out:close(); PCSX.quit(0) end
  end
end)
