-- Bounded, read-only installed OpenBIOS / original game RNG observer.
-- No RAM/register/pad writes, resets, execution controls or save accesses.
-- Launcher must hash all supplied files before setting these globals.
-- Install before game entry for a boot chronology, or label it MID_SESSION.
local BIOS_SHA = '713ea2aed58606282b3ff5e91d77f8c5892ea36f5adcd279d2c7dd26604a625d'
local EXE_SHA = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
local OVERLAY_SHA = '9490c200edc3a8045af4629e983bed83cab56a9a53b178ac067ea76af3ce413c'
assert(MML_RNG_BIOS_SHA256 == BIOS_SHA, 'Hash-verified installed BIOS required')
assert(MML_OWNER_EXE_SHA256 == EXE_SHA, 'Hash-verified owner EXE required')
assert(MML_RNG_OVERLAY_SHA256 == OVERLAY_SHA, 'Hash-verified tutorial overlay required')
local function absolute(value)
    return type(value) == 'string' and value:sub(1,1) == '/'
end
assert(absolute(MML_RNG_BIOS_PATH) and absolute(MML_OWNER_EXE_PATH) and
       absolute(MML_RNG_ST00_PATH) and absolute(MML_RNG_TRACE_PATH),
       'Explicit absolute file paths are required')
local function read_range(path, offset, size)
    local file = assert(io.open(path, 'rb'))
    assert(file:seek('set', offset))
    local bytes = assert(file:read(size))
    file:close()
    assert(#bytes == size, 'Truncated profile range')
    return bytes
end
local function le32(bytes, offset)
    return bytes:byte(offset+1) + bytes:byte(offset+2)*256 +
           bytes:byte(offset+3)*65536 + bytes:byte(offset+4)*16777216
end
local exeHeader = read_range(MML_OWNER_EXE_PATH, 0, 32)
assert(exeHeader:sub(1,8) == 'PS-X EXE', 'Invalid owner executable header')
local exeLoad = le32(exeHeader, 24)
local entry = le32(exeHeader, 16)
assert(entry == 0x80068000, 'Unsupported game entry')
local stHeader = read_range(MML_RNG_ST00_PATH, 0, 80)
assert(le32(stHeader, 0) == 0 and le32(stHeader, 4) == 141056 and
       le32(stHeader, 12) == 0x80100000, 'Unsupported tutorial archive layout')
local biosGuard = read_range(MML_RNG_BIOS_PATH, 0x60E4, 56)
local exeGuard = read_range(MML_OWNER_EXE_PATH, 2048 + entry - exeLoad, 32)
local callbackGuard = read_range(MML_RNG_ST00_PATH, 2048 + 0x226C, 148)
local existing = io.open(MML_RNG_TRACE_PATH, 'r')
if existing then existing:close(); error('Refusing to replace a reference trace') end
local output = assert(io.open(MML_RNG_TRACE_PATH, 'w'))
local maximum = MML_RNG_MAX_ROWS or 16384
assert(maximum >= 16 and maximum <= 65536 and maximum == math.floor(maximum))
local rows, ordinal, vsync, gameEntries = 0, 0, 0, 0
local pending, finished = nil, false
local callbackAdmitted, skippedCallbackVisits = false, 0
local captureScope = MML_RNG_CAPTURE_SCOPE or 'CURRENT_INSTALLATION'
assert(captureScope == 'CURRENT_INSTALLATION' or captureScope == 'MID_SESSION')
if MML_RNG_REFERENCE_OBSERVER_01420 then MML_RNG_REFERENCE_OBSERVER_01420.close() end
local observer = {breakpoints={}, returnBreakpoints={}}
-- Lazy retained exact-caller cache. 128 is a bounded engineering capacity,
-- supported by 93 aligned owner EXE/tutorial rand-call candidates plus the
-- 32 prior completed live callers; neither count proves full call coverage.
-- Wide disabled ranges still trigger costly C++ lookup/dispatch per instruction
-- in this installed debugger, so no RAM/ROM execution range is registered.
local returnCount, returnCapacity = 0, 128
local function supported_return(address)
    if address % 4 ~= 0 then return false end
    local segment = math.floor(address / 0x20000000)
    if segment ~= 0 and segment ~= 4 and segment ~= 5 then return false end
    local physical = address % 0x20000000
    return physical < 0x00200000 or
           (physical >= 0x1FC00000 and physical < 0x1FC80000)
end
local function disable_returns()
    for _,breakpoint in pairs(observer.returnBreakpoints) do breakpoint:disable() end
end
MML_RNG_REFERENCE_OBSERVER_01420 = observer
local function unsigned(value) return tonumber(value) % 4294967296 end
local function append(text)
    if finished or rows >= maximum then return false end
    output:write(text .. '\n'); output:flush(); rows = rows + 1
    return rows < maximum
end
local function guard(memory, address, bytes)
    for index = 1, #bytes do
        if memory:readU8At(address + index - 1) ~= bytes:byte(index) then return false end
    end
    return true
end
local function fail(reason)
    append('{"kind":"observer_guard_failure","reason":"' .. reason .. '"}')
    finished = true
    disable_returns()
    return false
end
local function guarded(callback)
    return function(...)
        if finished or rows >= maximum then return false end
        local ok, keep = pcall(callback, ...)
        if not ok then
            print('Read-only RNG observer stopped: ' .. tostring(keep))
            return fail('lua_callback_error')
        end
        return keep
    end
end
local function add(address, label, callback)
    local breakpoint = PCSX.addBreakpoint(address, 'Exec', 4, label, guarded(callback))
    observer.breakpoints[#observer.breakpoints+1] = breakpoint
    return breakpoint
end
do
    local memory = PCSX.getMemoryAsFile()
    local matches = guard(memory, 0xBFC060E4, biosGuard)
    local state = unsigned(memory:readI32At(0x8448))
    memory:close()
    assert(matches, 'Selected BIOS does not match the installed RNG profile')
    append(string.format('{"kind":"observer_start","bios_sha256":"%s",' ..
        '"source_exe_sha256":"%s","overlay_sha256":"%s",' ..
        '"scope":"%s","seed32":%u,"max_rows":%d,' ..
        '"game_boot_parity":false,"retail_parity":false}',
        BIOS_SHA, EXE_SHA, OVERLAY_SHA, captureScope, state, maximum))
end
observer.vsync = PCSX.Events.createEventListener('GPU::Vsync', function()
    vsync = vsync + 1
end)
add(entry, 'Read-only original game entry boundary', function()
    local memory = PCSX.getMemoryAsFile()
    local matches = guard(memory, entry, exeGuard)
    local seed = unsigned(memory:readI32At(0x8448))
    memory:close()
    if not matches then return fail('game_entry_identity') end
    gameEntries = gameEntries + 1
    return append(string.format('{"kind":"game_entry","vsync":%d,' ..
        '"game_entry_count":%d,"rand_ordinal":%d,"seed32":%u,' ..
        '"source_identity_matches":true}', vsync, gameEntries, ordinal, seed))
end)
add(0xA0, 'Read-only shared BIOS RNG call order', function()
    local registers = PCSX.getRegisters()
    local r = registers.GPR.n
    local service = unsigned(r.t1)
    if service ~= 0x2F and service ~= 0x30 then return true end
    if pending then return fail('unexpected_nested_random_service') end
    -- Reserve enter/return/admission/store plus a final marker. Stop before
    -- starting another observed service, never between a call and its return.
    if rows >= maximum - 5 then
        append(string.format('{"kind":"observer_budget_complete",' ..
            '"rand_ordinal":%d,"rows_before":%d,"pending_return":false}',ordinal,rows))
        finished=true
        disable_returns()
        return false
    end
    local memory = PCSX.getMemoryAsFile()
    local matches = guard(memory, 0xBFC060E4, biosGuard)
    local seed = unsigned(memory:readI32At(0x8448))
    memory:close()
    if not matches then return fail('bios_rng_identity') end
    if service == 0x2F then ordinal = ordinal + 1 end
    pending = {service=service, ordinal=ordinal, caller=unsigned(r.ra),
               before=seed, argument=unsigned(r.a0), vsync=vsync}
    append(string.format('{"kind":"bios_rng_enter","vsync":%d,' ..
        '"rand_ordinal":%d,"game_entry_count":%d,"service":%u,' ..
        '"caller_ra":%u,"seed_before32":%u,"a0_argument":%u}',
        vsync, ordinal, gameEntries, service, pending.caller, seed, pending.argument))
    -- Reuse a bounded listener for each observed caller. Recreating a Lua/C++
    -- callback for every rand exhausted the installed binding during r2.
    -- A retained listener ignores unrelated visits to its address; it reads
    -- a return only when the shared pending call names this exact caller.
    local caller = pending.caller
    if not supported_return(caller) then return fail('unsupported_return_address') end
    if not observer.returnBreakpoints[caller] then
        if returnCount >= returnCapacity then return fail('return_listener_capacity') end
        observer.returnBreakpoints[caller] = PCSX.addBreakpoint(caller, 'Exec', 4,
            'Read-only BIOS RNG cached caller return', guarded(function(address,width)
            local capture = pending
            if not capture or capture.caller ~= caller or unsigned(address) ~= caller then return true end
            if tonumber(width) ~= 4 then return fail('unexpected_return_execution_width') end
            local returned = PCSX.getRegisters()
            local mem = PCSX.getMemoryAsFile()
            local matches = guard(mem, 0xBFC060E4, biosGuard)
            local after = unsigned(mem:readI32At(0x8448))
            mem:close()
            if not matches then return fail('bios_rng_identity_at_return') end
            append(string.format('{"kind":"bios_rng_return","vsync":%d,' ..
                '"rand_ordinal":%d,"service":%u,"caller_ra":%u,' ..
                '"seed_before32":%u,"seed_after32":%u,"v0_actual":%u}',
                vsync, capture.ordinal, capture.service, capture.caller,
                capture.before, after, unsigned(returned.GPR.n.v0)))
            pending = nil
            observer.returnBreakpoints[caller]:disable()
            return true
        end))
        returnCount = returnCount + 1
    else observer.returnBreakpoints[caller]:enable() end
    return rows < maximum
end)
add(0x801022FC, 'Read-only original class03 random sample store', function()
    local registers = PCSX.getRegisters()
    local r, memory = registers.GPR.n, PCSX.getMemoryAsFile()
    local matches = guard(memory, 0x8010226C, callbackGuard)
    local work = unsigned(r.s0)
    local slot = (work - 0x8009F108) / 0x3C0
    if not matches then
        memory:close()
        if callbackAdmitted then return fail('tutorial_callback_identity_after_admission') end
        skippedCallbackVisits = skippedCallbackVisits + 1
        if skippedCallbackVisits == 1 or skippedCallbackVisits % 120 == 0 then
            return append(string.format('{"kind":"class03_overlay_not_admitted",' ..
                '"vsync":%d,"rand_ordinal":%d,"skipped_visits":%d,' ..
                '"overlay_identity_matches":false,"actor_fields_read":false}',
                vsync, ordinal, skippedCallbackVisits))
        end
        return true
    end
    if slot < 0 or slot >= 16 or slot ~= math.floor(slot) then
        memory:close(); return fail('class03_work_not_in_original_pool')
    end
    local marker, subtype = memory:readU8At(work+1), memory:readU8At(work+2)
    if math.floor(marker / 32) ~= 1 or subtype ~= 3 then
        memory:close(); return fail('class03_pool_identity')
    end
    if not callbackAdmitted then
        callbackAdmitted = true
        append(string.format('{"kind":"class03_overlay_admitted","vsync":%d,' ..
            '"rand_ordinal":%d,"overlay_identity_matches":true,' ..
            '"source_overlay_sha256":"%s"}', vsync, ordinal, OVERLAY_SHA))
    end
    local state, action, phase = memory:readU8At(work+8),
        memory:readU8At(work+9), memory:readU8At(work+10)
    local stats, context = memory:readU8At(work+12), memory:readU8At(work+15)
    local x, y, z = memory:readI16At(work+0x14), memory:readI16At(work+0x16),
        memory:readI16At(work+0x18)
    local heading = memory:readI16At(work+0x56) % 65536
    memory:close()
    return append(string.format('{"kind":"class03_sample_store","vsync":%d,' ..
        '"rand_ordinal":%d,"slot":%d,"work":%u,"state08":%d,' ..
        '"action09":%d,"phase0A":%d,"stats_index0C":%d,"context_high0F":%d,' ..
        '"position14":[%d,%d,%d],"heading56":%d,"v0_actual":%u,' ..
        '"sample32A_to_store":%u,"overlay_identity_matches":true}',
        vsync, ordinal, slot, work, state, action, phase,
        stats, context, x, y, z, heading, unsigned(r.v0), unsigned(r.v0) % 65536))
end)
function observer.status_readonly()
    return {vsync=vsync,rand_ordinal=ordinal,game_entries=gameEntries,
            rows=rows,finished=finished,pending_return=pending~=nil,
            return_listeners=returnCount,return_listener_capacity=returnCapacity,
            return_strategy='bounded_retained_exact_callers'}
end
function observer.close()
    if output then
        for _,breakpoint in ipairs(observer.breakpoints) do breakpoint:remove() end
        for _,breakpoint in pairs(observer.returnBreakpoints) do breakpoint:remove() end
        observer.vsync:remove()
        output:close(); output = nil; finished = true
    end
end
-- HTTP is opt-in. The installed reference process terminated after its first
-- status HTTP request; the two prior scalar rows remain valid observations.
-- File/event capture does not depend on the HTTP bridge.
if MML_RNG_ENABLE_HTTP_STATUS == true then
    PCSX.nextTick(function()
        PCSX.WebServer = PCSX.WebServer or {}
        PCSX.WebServer.Handlers = PCSX.WebServer.Handlers or {}
        PCSX.WebServer.Handlers.rng_reference_01420 = function()
            return string.format('{"schema":"mml-rng-observer-status/1",' ..
                '"rows":%d,"rand_ordinal":%d,"vsync":%d,"game_entries":%d,' ..
                '"finished":%s,"pending_return":%s,"retail_parity":false}',
                rows, ordinal, vsync, gameEntries, tostring(finished), tostring(pending ~= nil))
        end
    end)
end
print('Read-only RNG reference observer installed; no seed is assumed or written')
