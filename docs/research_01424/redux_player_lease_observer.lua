-- Bounded read-only original80035508 / resident GTE lease observation.
-- No emulated RAM/register/pad writes, save accesses or execution controls.
-- Launcher must verify the owner EXE hash before setting these globals.
local EXE_SHA = '10d2008f2837c466b8a3c3f56ff9b0946096ef29d7f26f09010d78ed4b2568f0'
local ENTRY = 0x80035508
assert(MML_OWNER_EXE_SHA256 == EXE_SHA, 'Hash-verified owner EXE required')
local function absolute(value) return type(value)=='string' and value:sub(1,1)=='/' end
assert(absolute(MML_OWNER_EXE_PATH) and absolute(MML_PLAYER_LEASE_TRACE_PATH),
       'Explicit absolute owner EXE and new trace paths required')
local maximum = MML_PLAYER_LEASE_MAX_ROWS or 8192
assert(maximum>=16 and maximum<=8192 and maximum==math.floor(maximum))
local function unsigned(value)
    local number = assert(tonumber(value), 'Missing numeric observed register')
    return number % 4294967296
end
local function le32(bytes, offset)
    return bytes:byte(offset+1)+bytes:byte(offset+2)*256+
           bytes:byte(offset+3)*65536+bytes:byte(offset+4)*16777216
end
local exe = assert(io.open(MML_OWNER_EXE_PATH,'rb'))
local header = assert(exe:read(32))
assert(#header==32 and header:sub(1,8)=='PS-X EXE','Invalid owner executable header')
local base = le32(header,24)
assert(ENTRY>=base,'Lease function outside owner executable')
assert(exe:seek('set',2048+ENTRY-base))
local codeGuard = assert(exe:read(32));exe:close()
assert(#codeGuard==32,'Truncated lease function guard')
local existing = io.open(MML_PLAYER_LEASE_TRACE_PATH,'r')
if existing then existing:close();error('Refusing to replace a player lease trace') end
if MML_PLAYER_LEASE_OBSERVER_01424 then MML_PLAYER_LEASE_OBSERVER_01424.close() end
local output = assert(io.open(MML_PLAYER_LEASE_TRACE_PATH,'w'))
local rows, ordinal, finished, pending = 0, 0, false, nil
local observer = {return_listener=nil,return_address=nil}
MML_PLAYER_LEASE_OBSERVER_01424 = observer
local function append(text)
    if finished or rows>=maximum then return false end
    output:write(text..'\n');output:flush();rows=rows+1
    return rows<maximum
end
local function stop(reason)
    append('{"kind":"lease_observer_guard_failure","reason":"'..reason..
           '","status":"UNKNOWN","whole_trace_accepted":false}')
    finished=true
    if observer.return_listener then observer.return_listener:disable() end
    return false
end
local function guarded(callback)
    return function(...)
        if finished or rows>=maximum then return false end
        local ok, keep=pcall(callback,...)
        if not ok then
            print('Read-only player lease observer stopped: '..tostring(keep))
            return stop('lua_callback_error')
        end
        return keep
    end
end
local function code_matches(memory)
    for i=1,#codeGuard do
        if memory:readU8At(ENTRY+i-1)~=codeGuard:byte(i) then return false end
    end
    return true
end
local function ram_span(address, bytes)
    if address%4~=0 then return nil end
    -- Only original 2 MiB RAM and its documented physical/KSEG aliases;
    -- scratch, ROM, RAM mirrors, KSEG2 and hardware are not owner pointers.
    local segment=math.floor(address/0x20000000)
    if segment~=0 and segment~=4 and segment~=5 then return nil end
    local physical=address%0x20000000
    if physical+bytes>0x00200000 then return nil end
    return physical
end
local function words_json(words)
    local fields={}
    for i,word in ipairs(words) do fields[i]=string.format('%u',word) end
    return '['..table.concat(fields,',')..']'
end
local function cp2_snapshot(registers, name)
    local bank=assert(registers[name], 'Missing resident '..name)
    local values={}
    for i=0,31 do values[i+1]=unsigned(bank.r[i]) end
    return values
end
local function with_memory(callback)
    local memory=PCSX.getMemoryAsFile()
    local ok,first,second=pcall(callback,memory)
    memory:close()
    if not ok then error(first) end
    return first,second
end
local function memory_words(memory,address,count)
    local values={}
    for i=0,count-1 do values[i+1]=unsigned(memory:readI32At(address+4*i)) end
    return values
end
local function selected_owners(memory, mask, pointers)
    local selected, aliases={},{}
    for slot=0,23 do
        -- The source tests the sign bit, then shifts left once per slot.
        if math.floor(mask/2^(31-slot))%2==1 then
            local pointer=pointers[slot+1]
            local physical=ram_span(pointer,0xc4)
            if pointer==0 or not physical then return nil,'selected_owner_not_in_original_ram' end
            if aliases[physical]==nil then aliases[physical]=slot end
            selected[#selected+1]={slot=slot,pointer=pointer,physical=physical,
                alias_first_slot=aliases[physical],
                words=memory_words(memory,pointer+0xbc,2)}
        end
    end
    return selected
end
local function owners_json(selected)
    local fields={}
    for i,owner in ipairs(selected) do
        fields[i]=string.format('{"slot":%d,"pointer":%u,"physical":%u,'..
            '"alias_first_slot":%d,"words_bc_c0":%s}',owner.slot,owner.pointer,
            owner.physical,owner.alias_first_slot,words_json(owner.words))
    end
    return '['..table.concat(fields,',')..']'
end
local function observe_return(address,width)
    local captured=pending
    if not captured or unsigned(address)~=captured.caller then return true end
    if tonumber(width)~=4 then return stop('unexpected_return_execution_width') end
    local registers=PCSX.getRegisters()
    local snapshot,reason=with_memory(function(memory)
        if not code_matches(memory) then return nil,'lease_code_identity_at_return' end
        local after={}
        -- Read the exact pointers admitted at entry in source slot order. Do
        -- not follow a changed table, dereference BC/C0 or merge aliases.
        for i,owner in ipairs(captured.selected) do
            after[i]={slot=owner.slot,pointer=owner.pointer,physical=owner.physical,
                alias_first_slot=owner.alias_first_slot,
                words=memory_words(memory,owner.pointer+0xbc,2)}
        end
        return {selected=after,mask=unsigned(memory:readI32At(0x80098798)),
                pointers=memory_words(memory,0x800be478,24)}
    end)
    if not snapshot then return stop(reason) end
    local after,mask,pointers=snapshot.selected,snapshot.mask,snapshot.pointers
    local controls,data=cp2_snapshot(registers,'CP2C'),cp2_snapshot(registers,'CP2D')
    append(string.format('{"kind":"player_lease_return","ordinal":%d,"caller_ra":%u,'..
        '"mask_80098798":%u,"owner_pointers_800be478":%s,"selected_owners":%s,'..
        '"cp2c":%s,"cp2d":%s,"retail_parity":false}',
        captured.ordinal,captured.caller,mask,words_json(pointers),owners_json(after),
        words_json(controls),words_json(data)))
    pending=nil;observer.return_listener:disable()
    return true
end
append(string.format('{"kind":"lease_observer_start","source_exe_sha256":"%s",'..
    '"source_function":%u,"max_rows":%d,"ram_bytes":2097152,'..
    '"boot_chronology_proved":false,"retail_parity":false}',EXE_SHA,ENTRY,maximum))
observer.entry_listener=PCSX.addBreakpoint(ENTRY,'Exec',4,
    'Read-only original player GTE lease entry',guarded(function()
    if pending then return stop('unexpected_nested_lease_call') end
    if rows>=maximum-3 then
        append(string.format('{"kind":"lease_observer_budget_complete","ordinal":%d,'..
            '"pending_return":false}',ordinal))
        finished=true
        if observer.return_listener then observer.return_listener:disable() end
        return false
    end
    local registers=PCSX.getRegisters()
    local caller=unsigned(registers.GPR.n.ra)
    if not ram_span(caller,4) or caller==ENTRY then return stop('unsupported_return_address') end
    if observer.return_address and observer.return_address~=caller then
        return stop('return_listener_capacity')
    end
    local snapshot,reason=with_memory(function(memory)
        if not code_matches(memory) then return nil,'lease_code_identity' end
        local camera=memory_words(memory,0x1f800008,8)
        local mask=unsigned(memory:readI32At(0x80098798))
        local pointers=memory_words(memory,0x800be478,24)
        local selected,ownerReason=selected_owners(memory,mask,pointers)
        if not selected then return nil,ownerReason end
        return {camera=camera,mask=mask,pointers=pointers,selected=selected}
    end)
    if not snapshot then return stop(reason) end
    local camera,mask,pointers,selected=snapshot.camera,snapshot.mask,
        snapshot.pointers,snapshot.selected
    local controls,data=cp2_snapshot(registers,'CP2C'),cp2_snapshot(registers,'CP2D')
    ordinal=ordinal+1
    pending={ordinal=ordinal,caller=caller,selected=selected}
    append(string.format('{"kind":"player_lease_enter","ordinal":%d,"caller_ra":%u,'..
        '"scratch_camera_1f800008":%s,"mask_80098798":%u,'..
        '"owner_pointers_800be478":%s,"selected_owners":%s,'..
        '"cp2c":%s,"cp2d":%s,"source_identity_matches":true,"retail_parity":false}',
        ordinal,caller,words_json(camera),mask,words_json(pointers),owners_json(selected),
        words_json(controls),words_json(data)))
    if not observer.return_listener then
        observer.return_address=caller
        observer.return_listener=PCSX.addBreakpoint(caller,'Exec',4,
            'Read-only original player GTE lease retained return',guarded(observe_return))
    else observer.return_listener:enable() end
    return true
end))
function observer.status_readonly()
    return {rows=rows,ordinal=ordinal,finished=finished,pending_return=pending~=nil,
        return_listeners=observer.return_listener and 1 or 0,return_listener_capacity=1,
        return_address=observer.return_address,retail_parity=false}
end
function observer.close()
    if output then
        observer.entry_listener:remove()
        if observer.return_listener then observer.return_listener:remove() end
        output:close();output=nil;finished=true
    end
end
print('Read-only original player GTE lease observer installed')
