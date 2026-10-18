-- Read-only, bounded scalar observer for an explicitly configured reference run.
-- Requires PCSX-Redux debugger + interpreter and MML_RNG_TRACE_PATH set before
-- loading. Does not boot, resume, reset, write RAM, change pads, or access saves.
assert(type(MML_RNG_TRACE_PATH) == 'string' and
       string.sub(MML_RNG_TRACE_PATH, 1, 1) == '/',
       'Set MML_RNG_TRACE_PATH to a new absolute output filename first')
local existing = io.open(MML_RNG_TRACE_PATH, 'r')
if existing then existing:close(); error('Refusing to overwrite RNG trace') end
local output = assert(io.open(MML_RNG_TRACE_PATH, 'w'))
local maximum, rows, vsync, randomOrdinal = 4096, 0, 0, 0
local function unsigned(v) return tonumber(v) % 4294967296 end
local function append(value)
    if rows >= maximum then return false end
    output:write(value .. '\n'); output:flush(); rows = rows + 1
    return rows < maximum
end
MML_RNG_OBSERVER_01419 = {}
MML_RNG_OBSERVER_01419.vsync = PCSX.Events.createEventListener('GPU::Vsync', function()
    vsync = vsync + 1
end)
MML_RNG_OBSERVER_01419.gateway = PCSX.addBreakpoint(0xA0, 'Exec', 4,
    'Read-only BIOS RNG service chronology', function()
        local ok, keep = pcall(function()
            local registers = PCSX.getRegisters()
            local r = registers.GPR.n
            local id = unsigned(r.t1)
            if id ~= 0x2F and id ~= 0x30 then return rows < maximum end
            if id == 0x2F then randomOrdinal = randomOrdinal + 1 end
            return append(string.format(
                '{"kind":"bios_gateway","vsync":%d,"rand_ordinal":%d,' ..
                '"pc":%u,"function_id":%u,"caller_ra":%u,"a0_argument":%u,' ..
                '"s0":%u,"seed_argument_valid":%s}',
                vsync, randomOrdinal, unsigned(registers.pc), id,
                unsigned(r.ra), unsigned(r.a0), unsigned(r.s0), tostring(id == 0x30)))
        end)
        if not ok then print('RNG observer stopped: ' .. tostring(keep)); return false end
        return keep
    end)
MML_RNG_OBSERVER_01419.sample = PCSX.addBreakpoint(0x801022FC, 'Exec', 4,
    'Read-only class03 original RNG result before32A store', function()
        local ok, keep = pcall(function()
            local registers = PCSX.getRegisters()
            local r = registers.GPR.n
            local m = PCSX.getMemoryAsFile()
            local work = unsigned(r.s0)
            -- The proved class20 pool has16 records of960 bytes. Refuse unrelated
            -- overlay/address reuse rather than reading an unbounded pointer.
            local slot = (work - 0x8009F108) / 0x3C0
            if slot < 0 or slot >= 16 or slot ~= math.floor(slot) then m:close(); return rows < maximum end
            local class, subtype = m:readU8At(work + 1), m:readU8At(work + 2)
            -- Low five marker bits contain the source pool slot.
            if math.floor(class / 32) ~= 1 or subtype ~= 3 then m:close(); return rows < maximum end
            local state, action, phase = m:readU8At(work + 8), m:readU8At(work + 9), m:readU8At(work + 10)
            m:close()
            return append(string.format(
                '{"kind":"class03_sample_store","vsync":%d,"rand_ordinal":%d,' ..
                '"pc":%u,"work":%u,"slot":%d,"state08":%d,"action09":%d,' ..
                '"phase0A":%d,"v0_return":%u,"sample32A_to_store":%u}',
                vsync, randomOrdinal, unsigned(registers.pc), work, slot, state, action,
                phase, unsigned(r.v0), unsigned(r.v0) % 65536))
        end)
        if not ok then print('RNG observer stopped: ' .. tostring(keep)); return false end
        return keep
    end)
function MML_RNG_OBSERVER_01419.close()
    MML_RNG_OBSERVER_01419.gateway:remove()
    MML_RNG_OBSERVER_01419.sample:remove()
    output:close()
end
print('Bounded RNG scalar observer installed; recording starts at current reference state')
