-- Fixed bounded read-only scalar handler; no RAM/register/pad/save writes,
-- execution controls, automatic reset or game launch. Optional instrumentation
-- log is a new explicitly configured file outside source, capped at256rows.
-- Query GET /api/v1/lua/detection_globals_01419 in an explicitly selected
-- owner reference session. Launcher verifies full owner EXE hash first; this
-- observer additionally compares bounded live code with that read-only file.
local expected = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
assert(MML_OWNER_EXE_SHA256 == expected, 'Launcher must verify supported owner EXE SHA256')
assert(type(MML_OWNER_EXE_PATH) == 'string' and MML_OWNER_EXE_PATH:sub(1,1) == '/',
       'Provide explicit read-only owner EXE absolute path')
local owner = assert(io.open(MML_OWNER_EXE_PATH, 'rb'))
local header = assert(owner:read(32))
assert(header:sub(1,8) == 'PS-X EXE', 'Invalid owner EXE header')
local function le32(text, offset)
    return text:byte(offset+1) + text:byte(offset+2)*256 +
           text:byte(offset+3)*65536 + text:byte(offset+4)*16777216
end
local load = le32(header, 24)
local expectedRegions = {}
for _,range in ipairs({{0x80064718,0x80064794}, {0x80036478,0x800364D4},
                       {0x8005EBD4,0x8005EBE4}, {0x8005D4C0,0x8005D4D8}}) do
    assert(owner:seek('set', 2048 + range[1] - load))
    local bytes = assert(owner:read(range[2] - range[1]))
    assert(#bytes == range[2] - range[1], 'Truncated source identity range')
    expectedRegions[#expectedRegions+1] = {address=range[1], bytes=bytes}
end
owner:close()
local outputPath = MML_DETECTION_GLOBALS_TRACE_PATH
if outputPath ~= nil then
    assert(type(outputPath)=='string' and outputPath:sub(1,1)=='/', 'Log path must be absolute')
    local existing=io.open(outputPath,'r')
    if existing then existing:close(); error('Refusing to replace a globals trace') end
    assert(io.open(outputPath,'w')):close()
end
local vsync, loggedRows = 0, 0
local pad = PCSX.SIO0.slots[1].pads[1]
local function unsigned(v) return tonumber(v) % 4294967296 end
MML_DETECTION_GLOBALS_OBSERVER_01419 = {}
local function snapshot()
    local m = PCSX.getMemoryAsFile()
    local regs = PCSX.getRegisters()
    for _,region in ipairs(expectedRegions) do
        for i=1,#region.bytes do
            if m:readU8At(region.address+i-1) ~= region.bytes:byte(i) then
                m:close()
                return string.format('{"schema":"mml-reference-detection-globals/1",' ..
                    '"source_identity_matches":false,"failed_region":%u,"pc":%u}',
                    region.address, unsigned(regs.pc))
            end
        end
    end
    local actors = {}
    for slot = 0, 15 do
        local work = 0x8009F108 + slot * 0x3C0
        local active, marker, subtype = m:readU8At(work), m:readU8At(work + 1), m:readU8At(work + 2)
        -- The low five marker bits contain the slot; top bits select class20.
        if active % 2 == 1 and math.floor(marker / 32) == 1 and subtype == 3 then
            actors[#actors + 1] = string.format(
                '{"slot":%d,"work":%u,"active":%d,"class_slot_raw":%d,' ..
                '"subtype_raw":%d,"state08":%d,"action09":%d,"phase0A":%d,' ..
                '"stats_index0C":%d,"context_high0F":%d,"hp6E":%d,' ..
                '"profile352":%d,"phase353":%d,"countdown35A":%d,' ..
                '"sample32A":%d,"work80":%u,"position":[%d,%d,%d]}',
                slot, work, active, marker, subtype, m:readU8At(work + 8),
                m:readU8At(work + 9), m:readU8At(work + 10), m:readU8At(work + 12),
                m:readU8At(work + 15), m:readI16At(work + 0x6E),
                m:readU8At(work + 0x352), m:readU8At(work + 0x353),
                m:readU8At(work + 0x35A), m:readI16At(work + 0x32A) % 65536,
                unsigned(m:readI32At(work + 0x80)), m:readI16At(work + 0x14),
                m:readI16At(work + 0x16), m:readI16At(work + 0x18))
        end
    end
    local result = string.format(
        '{"schema":"mml-reference-detection-globals/1","source_identity_matches":true,' ..
        '"observer_vsync":%d,' ..
        '"pc":%u,"global800BBD96_i16":%d,"global800BBD84_u8":%d,' ..
        '"scratch1F800075_u8":%d,"raw_stats_selector800C1B6B_u8":%d,' ..
        '"stats_table_pointer80098844":%u,"player_active":%d,"player_state08":%d,' ..
        '"player_action09":%d,"player_phase0A":%d,"player_position":[%d,%d,%d],' ..
        '"actors_class20_03":[%s],"start_pressed":%s,"cross_pressed":%s,"retail_parity":false}',
        vsync, unsigned(regs.pc), m:readI16At(0x800BBD96), m:readU8At(0x800BBD84),
        m:readU8At(0x1F800075), m:readU8At(0x800C1B6B),
        unsigned(m:readI32At(0x80098844)), m:readU8At(0x800B51B0),
        m:readU8At(0x800B51B8), m:readU8At(0x800B51B9), m:readU8At(0x800B51BA),
        m:readI16At(0x800B51C4), m:readI16At(0x800B51C6), m:readI16At(0x800B51C8),
        table.concat(actors, ','), tostring(pad.getButton(3)), tostring(pad.getButton(14)))
    m:close()
    return result
end
MML_DETECTION_GLOBALS_OBSERVER_01419.vsync =
    PCSX.Events.createEventListener('GPU::Vsync', function()
        vsync = vsync + 1
        if outputPath and loggedRows < 256 and vsync % 120 == 0 then
            local ok, result = pcall(snapshot)
            if ok then
                local file=assert(io.open(outputPath,'a'))
                file:write(result..'\n'); file:close(); loggedRows=loggedRows+1
            else print('Globals observer error: '..tostring(result)) end
        end
    end)
PCSX.nextTick(function()
    PCSX.WebServer = PCSX.WebServer or {}
    PCSX.WebServer.Handlers = PCSX.WebServer.Handlers or {}
    PCSX.WebServer.Handlers.detection_globals_01419 = snapshot
    print('Read-only detection globals scalar handler installed; no game writes')
end)
