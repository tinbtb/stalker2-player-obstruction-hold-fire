local source = debug.getinfo(1, "S").source:match("^@(.+)$")
local dir = assert(source and source:match("^(.*[/\\])"), "Cannot find mod directory")
local dll = dir .. "PlayerObstructionHoldFire.dll"
local function load(name) return assert(package.loadlib(dll, name)) end
local apply = load("luaopen_PlayerObstructionHoldFire")
local snapshot = load("luaopen_PlayerObstructionSnapshot")
local status = load("luaopen_PlayerObstructionStatus")
apply()
local f = io.open(dir .. "PlayerObstructionHoldFire.log", "r")
local activation = f and f:read("*a") or "Missing native activation log"
if f then f:close() end
print("[PlayerObstructionHoldFire] " .. activation .. "\n")
if not activation:match("^ACTIVE:") then return end
local queued = false
local ticks = 0
LoopAsync(50, function()
    if queued then return false end
    queued = true
    ExecuteInGameThread(function()
        local ok, err = pcall(function()
            local pc = FindFirstOf("PlayerController")
            if not pc or not pc:IsValid() then return end
            local pawn = pc.Pawn
            if not pawn or not pawn:IsValid() then return end
            local capsule = pawn.CapsuleComponent
            if not capsule or not capsule:IsValid() then return end
            local r = capsule:GetScaledCapsuleRadius()
            local h = capsule:GetScaledCapsuleHalfHeight()
            local out = assert(io.open(dir .. "player_snapshot.txt", "w"))
            out:write(string.format("%x %.6f %.6f\n", pawn:GetAddress(), r, h))
            out:close()
            snapshot()
            ticks = ticks + 1
            if ticks % 100 == 0 then status() end
        end)
        if not ok and ticks == 0 then
            print("[PlayerObstructionHoldFire] Snapshot unavailable: " .. tostring(err) .. "\n")
            ticks = -1
        end
        queued = false
    end)
    return false
end)
