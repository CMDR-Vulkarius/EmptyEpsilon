-- Name: Damage Control
-- Description: Development scenario with a single player ship that has all systems damaged for testing repair mini-games. No enemies, no other objects. Use F5 in GM screen to copy layout.
-- Type: Development

--- Scenario
-- @script scenario_11_damage_control

function init()
    -- Create player ship with all systems damaged
    player = PlayerSpaceship():setFaction("Human Navy"):setTemplate("Atlantis"):setPosition(0, 0):setRotation(0)
    
    -- Damage all systems to various levels for testing
    player:setSystemHealth("reactor", -0.5)          -- Reactor: 50% damaged
    player:setSystemHealth("beamweapons", -0.6)      -- Beam Weapons: 60% damaged
    player:setSystemHealth("missilesystem", -0.4)    -- Missile System: 40% damaged
    player:setSystemHealth("maneuver", -0.7)         -- Maneuvering: 70% damaged
    player:setSystemHealth("impulse", -0.5)          -- Impulse: 50% damaged
    player:setSystemHealth("warp", -0.8)             -- Warp Drive: 80% damaged
    player:setSystemHealth("jumpdrive", -0.6)        -- Jump Drive: 60% damaged
    player:setSystemHealth("frontshield", -0.9)      -- Front Shield: 90% damaged
    player:setSystemHealth("rearshield", -0.9)       -- Rear Shield: 90% damaged
    
    -- Add some visual elements (optional, can be removed)
    local planet1 = Planet():setPosition(5000, 5000):setPlanetRadius(3000):setDistanceFromMovementPlane(-2000):setPlanetSurfaceTexture("planets/planet-1.png"):setPlanetCloudTexture("planets/clouds-1.png"):setPlanetAtmosphereTexture("planets/atmosphere.png"):setPlanetAtmosphereColor(0.2, 0.2, 1.0)
    local moon1 = Planet():setPosition(5000, 0):setPlanetRadius(1000):setDistanceFromMovementPlane(-2000):setPlanetSurfaceTexture("planets/moon-1.png"):setAxialRotationTime(20.0)
    local sun1 = Planet():setPosition(5000, 15000):setPlanetRadius(1000):setDistanceFromMovementPlane(-2000):setPlanetAtmosphereTexture("planets/star-1.png"):setPlanetAtmosphereColor(1.0, 1.0, 1.0)
    planet1:setOrbit(sun1, 40)
    moon1:setOrbit(planet1, 20.0)
    
    -- Add station for context (optional)
    SpaceStation():setPosition(10000, 0):setTemplate('Medium Station'):setFaction("Human Navy"):setRotation(random(0, 360)):setCallSign("Repair Dock")
    
    -- GM functions for testing
    addGMFunction(
        _("buttonGM", "Damage all systems again"),
        function()
            if player and player:isValid() then
                player:setSystemHealth("reactor", -0.5)
                player:setSystemHealth("beamweapons", -0.6)
                player:setSystemHealth("missilesystem", -0.4)
                player:setSystemHealth("maneuver", -0.7)
                player:setSystemHealth("impulse", -0.5)
                player:setSystemHealth("warp", -0.8)
                player:setSystemHealth("jumpdrive", -0.6)
                player:setSystemHealth("frontshield", -0.9)
                player:setSystemHealth("rearshield", -0.9)
            end
        end
    )
    
    addGMFunction(
        _("buttonGM", "Repair all systems"),
        function()
            if player and player:isValid() then
                player:setSystemHealth("reactor", 1.0)
                player:setSystemHealth("beamweapons", 1.0)
                player:setSystemHealth("missilesystem", 1.0)
                player:setSystemHealth("maneuver", 1.0)
                player:setSystemHealth("impulse", 1.0)
                player:setSystemHealth("warp", 1.0)
                player:setSystemHealth("jumpdrive", 1.0)
                player:setSystemHealth("frontshield", 1.0)
                player:setSystemHealth("rearshield", 1.0)
            end
        end
    )
    
    addGMFunction(
        _("buttonGM", "Random asteroid field"),
        function()
            cleanup()
            for n = 1, 1000 do
                Asteroid():setPosition(random(-50000, 50000), random(-50000, 50000)):setSize(random(100, 500))
                VisualAsteroid():setPosition(random(-50000, 50000), random(-50000, 50000)):setSize(random(100, 500))
            end
        end
    )
    
    addGMFunction(
        _("buttonGM", "Random nebula field"),
        function()
            cleanup()
            for n = 1, 50 do
                Nebula():setPosition(random(-50000, 50000), random(-50000, 50000))
            end
        end
    )
    
    addGMFunction(
        _("buttonGM", "Delete unselected"),
        function()
            local gm_selection = getGMSelection()
            for idx, obj in ipairs(getAllObjects()) do
                local found = false
                for idx2, obj2 in ipairs(gm_selection) do
                    if obj == obj2 then
                        found = true
                    end
                end
                if not found then
                    obj:destroy()
                end
            end
        end
    )
end

function cleanup()
    -- Clean up the current play field. Find all objects and destroy everything that is not a player.
    -- If it is a player, position him in the center of the scenario.
    for idx, obj in ipairs(getAllObjects()) do
        if obj.typeName == "PlayerSpaceship" then
            obj:setPosition(random(-100, 100), random(-100, 100))
        else
            obj:destroy()
        end
    end
end

function update(delta)
    -- No victory condition
end
