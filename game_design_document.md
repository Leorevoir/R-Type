### **R-Type 2.5D - Game Design Document**

**1. High Concept**

A tight, modern reimagining of the classic R-Type's first level, this project is a 2.5D horizontal scrolling shooter. Players will pilot the iconic R-9 "Arrowhead" ship, utilizing its signature "Force" device to battle through waves of biomechanical Bydo enemies and defeat a final boss.

**2. Vision Statement**

Our goal is to deliver a short, polished, and intensely replayable experience that captures the strategic and methodical essence of the R-Type series. The focus is squarely on the core mechanics: masterful use of the Force for both offense and defense, the charge-and-release tension of the Wave Cannon, and survival against iconic enemy patterns. By limiting the scope to a single, perfectly crafted level, we will create a challenging and satisfying vertical slice that feels like a true, modern R-Type game.

**3. Key Features**

*   **2.5D Visuals:** 3D models and environments presented on a fixed 2D gameplay plane, creating a sense of depth with parallax scrolling backgrounds.
*   **The R-9 Arrowhead:** The legendary player ship, equipped with its standard vulcan and powerful, chargeable Wave Cannon.
*   **The "Force" Mechanic:** Command the indestructible, detachable Force unit. Attach it to the front or rear for shielded firepower, or detach it to attack enemies independently.
*   **Iconic First Level:** A single, continuous level inspired by the design and flow of the original R-Type's Stage 1, culminating in a battle against the boss Dobkeratops.
*   **Strategic, Methodical Gameplay:** A slower-paced shooter that emphasizes pattern recognition and tactical positioning over pure reflexes.

**4. Gameplay Mechanics**

*   **Player Ship: R-9 "Arrowhead"**
    *   **Movement:** 8-directional control.
    *   **Weapons:**
        *   **Standard Shot:** A rapid-fire, low-damage vulcan for clearing weaker enemies.
        *   **Wave Cannon:** Hold the fire button to charge a meter. Releasing it fires a devastating beam that penetrates multiple enemies. The size and damage of the beam will depend on the charge level.
    *   **Durability:** The R-9 is destroyed in a single hit.

*   **The Force**
    *   The Force is an indestructible orb that blocks most standard enemy projectiles.
    *   **Attached:** Can be attached to the front or back of the R-9. It acts as a shield and fires alongside the main ship.
    *   **Detached:** Can be launched from the ship. It will attack enemies independently with its own vulcan cannons. The player can recall the Force to re-attach it. Good use of a detached Force is critical for clearing enemies in hard-to-reach places.

*   **Enemies (Limited Set for Scope)**
    *   **Bydo Drones:** Small, weak enemies that appear in formations and follow set flight paths.
    *   **Turrets:** Stationary enemies attached to the environment that fire periodically.
    *   **Boss: Dobkeratops:** The Stage 1 boss. It will feature a multi-phase pattern:
        *   **Phase 1:** The Dobkeratops attacks with its sweeping tail while its weak point on its abdomen remains closed.
        *   **Phase 2:** The abdomen opens, revealing a weak point that fires projectiles. This is the only time the boss is vulnerable to damage.

**5. Art and Audio**

*   **Art Style:** 2.5D (3D assets on a 2D plane). The aesthetic will be a modern take on the classic biomechanical and industrial horror of the Bydo Empire. Backgrounds will feature multiple layers of parallax scrolling to create a strong sense of depth.
*   **Audio:**
    *   **Sound Effects:** Punchy, satisfying sounds for weapons, impacts, and explosions are a priority to provide strong player feedback.
    *   **Music:** A single, high-energy electronic track that builds in intensity as the player progresses through the level, culminating in a dramatic boss theme.

**6. Scope**

To ensure a polished final product, the scope is aggressively limited. Power-ups (speed-ups, missiles, different lasers) are **out of scope** for this first prototype.

*   **Week 1: Core Mechanics & Player Control**
    *   **Goal:** A playable ship in a test environment.
    *   **Tasks:**
        *   Implement R-9 ship movement (8-way).
        *   Implement standard vulcan shot and the charge/release Wave Cannon.
        *   Implement the Force mechanics: attaching, detaching, recalling, and basic collision/blocking.
        *   Import the final 3D model for the R-9 Arrowhead.

*   **Week 2: Level Construction & Enemies**
    *   **Goal:** A fully playable, unpopulated level with functioning enemy types.
    *   **Tasks:**
        *   Block out the Stage 1 layout, including open areas and tight corridors.
        *   Create 3D assets for the environment and implement parallax scrolling.
        *   Implement the Bydo Drone and Turret enemies with their basic AI and attack patterns.
        *   Begin placing enemies in the level to establish game flow and difficulty curve.

*   **Week 3: Boss Battle & Polish**
    *   **Goal:** A complete, polished gameplay loop from start to finish.
    *   **Tasks:**
        *   Implement the Dobkeratops boss with its two-phase attack pattern.
        *   Integrate music and sound effects.
        *   Create a simple UI (Lives, Score).
        *   Implement a main menu ("Start", "Quit") and a game-over screen.
