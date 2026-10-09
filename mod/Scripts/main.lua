local source = debug.getinfo(1, "S").source:match("^@(.+)$")
local dir = assert(source and source:match("^(.*[/\\])"), "Cannot find mod directory")
local dll = dir .. "PlayerObstructionHoldFire.dll"
local function load(name) return assert(package.loadlib(dll, name)) end
local apply = load("luaopen_PlayerObstructionHoldFire")
local snapshot = load("luaopen_PlayerObstructionSnapshot")
local status = load("luaopen_PlayerObstructionStatus")
local beginMetric, endMetric = {}, {}
for _, name in ipairs({"Callback", "Lookup", "Capsule", "Write", "Queue"}) do
    beginMetric[name] = load("luaopen_MetricBegin" .. name)
    endMetric[name] = load("luaopen_MetricEnd" .. name)
end
apply()
local f = io.open(dir .. "PlayerObstructionHoldFire.log", "r")
local activation = f and f:read("*a") or "Missing native activation log"
if f then f:close() end
print("[PlayerObstructionHoldFire] " .. activation .. "\n")
if not activation:match("^ACTIVE:") then return end
local bridgeReset = load("luaopen_BridgeReset")
local bridgeDigits = {}
for digit in ("0123456789abcdef"):gmatch(".") do
    bridgeDigits[digit] = load("luaopen_Bridge" .. digit)
end
local cacheHit = load("luaopen_ControllerCacheHit")
local cacheRefresh = load("luaopen_ControllerCacheRefresh")
local invalidate = load("luaopen_InvalidatePlayerSnapshot")
local cachedController = nil
local cacheUses = 0
local function discardPlayer()
    cachedController = nil
    cacheUses = 0
    invalidate()
end
local queued = false
local ticks = 0
LoopAsync(50, function()
    if queued then return false end
    queued = true
    beginMetric.Queue()
    ExecuteInGameThread(function()
        endMetric.Queue()
        beginMetric.Callback()
        local ok, err = pcall(function()
            beginMetric.Lookup()
            local pc = cachedController
            -- Refresh every 100 uses as well as on invalidation, bounding retention
            -- across transitions where an old controller remains temporarily valid.
            if not pc or not pc:IsValid() or cacheUses >= 100 then
                cacheRefresh()
                pc = FindFirstOf("PlayerController")
                cachedController = pc
                cacheUses = 0
            else
                cacheHit()
            end
            cacheUses = cacheUses + 1
            endMetric.Lookup()
            if not pc or not pc:IsValid() then discardPlayer(); return end
            beginMetric.Capsule()
            local pawn = pc.Pawn
            if not pawn or not pawn:IsValid() then discardPlayer(); return end
            local capsule = pawn.CapsuleComponent
            if not capsule or not capsule:IsValid() then discardPlayer(); return end
            local r = capsule:GetScaledCapsuleRadius()
            local h = capsule:GetScaledCapsuleHalfHeight()
            endMetric.Capsule()
            beginMetric.Write()
            assert(r == r and h == h and r >= 1 and r <= 200 and h >= r and h <= 300, "Invalid capsule dimensions")
            local payload = string.format("%016x%04x%04x", pawn:GetAddress(), math.floor(r * 100 + 0.5), math.floor(h * 100 + 0.5))
            assert(#payload == 24, "Invalid bridge payload")
            bridgeReset()
            for i = 1, #payload do bridgeDigits[payload:sub(i, i)]() end
            endMetric.Write()
            snapshot()

        end)
        if not ok then discardPlayer() end
        if not ok and ticks == 0 then
            print("[PlayerObstructionHoldFire] Snapshot unavailable: " .. tostring(err) .. "\n")
        end
        endMetric.Callback()
        ticks = ticks + 1
        if ticks % 100 == 0 then status() end
        queued = false
    end)
    return false
end)
