# TileRace — Project Context

> AI reference document. Read this alongside `utils/Skeleton.h` (interface snapshot of all headers)
> for full project context. `Skeleton.h` tells you **what** exists; this file tells you **why** and **how**.

---

## 1. What the Game Is

TileRace is a real-time 2D side-scrolling platformer (cross-platform) supporting two game modes:

- **Co-op** (default online): 2–8 players work together to reach an exit tile. A level is cleared as soon as at least one player reaches the exit (team win). Shared checkpoints, player-to-player collisions, and magnet grab are active.
- **Race** (default offline): players race individually to the exit. No player collisions, no checkpoints in generated levels. Each player's finish time is tracked independently.

The **session leader** (first connected player) can switch between modes via the pause menu's "Lobby Settings" option.

**Core mechanics (all implemented):**

- Horizontal movement with acceleration/deceleration inertia
- Jump, wall-jump, variable-height jump (hold = higher), coyote time, jump buffer
- Dash (360°) with suspended gravity, cooldown, and post-dash enhanced jump
- Dash push: dashing into another player slams them with 0.8× dash velocity
- Sprint: hold Right Ctrl / R2 for 2× horizontal speed (disabled during dash)
- Magnet grab: hold Alt / Circle to grab and carry a nearby player (MAGNET_RANGE, GRAB_OFFSET_X)
- Drawing trails: hold P / R1 to leave persistent coloured spline marks on the map
- Kill tiles ('K') that trigger a death animation and respawn
- Ready/Go! overlay displayed at the start of each level

**Controls:**
| Input | Gamepad (PS) | Action |
|---|---|---|
| ← / → / A / D | Left stick / D-pad | Move |
| SPACE | Cross (×) | Jump / wall-jump |
| SHIFT | Square (□) | Dash (direction from stick/WASD/D-pad) |
| P | R1 (right bumper) | Draw trail on map (hold) |
| Right Ctrl | R2 (right trigger) | Sprint (hold) |
| Alt | Circle (○) | Magnet grab — grab and carry a nearby player (hold) |
| Backspace | Triangle (△) | Restart from last checkpoint |
| Delete | — | Restart from spawn (clears checkpoint) |
| ESC | Start / Options | Pause menu |

---

## 2. Technology Stack

| Component               | Library / Tool                     |
| ----------------------- | ---------------------------------- |
| Graphics, window, input | Raylib 5.5 (FetchContent)          |
| UDP networking          | ENet (FetchContent, GitHub mirror) |
| Build system            | CMake 3.16+ + Ninja                |
| Language                | C++20                              |
| Compiler (Windows)      | w64devkit (GCC) or MSVC            |
| Compiler (Linux/macOS)  | GCC or Clang                       |

Build presets (in `CMakePresets.json`):

- `run-scc-debug` — debug build, runs from `build/debug/bin/`
- `run-scc-release` — optimised build, runs from `build/release/bin/`

Assets are copied to `build/<config>/bin/assets/` by a CMake `POST_BUILD` step.

---

## 3. Architecture

### Module layout

```
src/common/     Pure logic — no Raylib, no ENet. Shared between client and server.
src/server/     Authoritative server. ENet + session state. Links common + enet.
src/client/     Rendering, input, game loop. Links common + server_logic + raylib + enet.
```

CMake targets:

```
common_logic     (static lib)  ← Player.cpp, World.cpp
server_logic     (static lib)  ← ServerLogic.cpp, LevelManager.cpp, ServerSession.cpp, ChunkStore.cpp, LevelGenerator.cpp, LevelValidator.cpp
TileRace_Server  (exe)         ← server/main.cpp
TileRace         (exe)         ← client/main.cpp + all client .cpp files (including mode-specific HudCoop/HudRace, LevelResultsCoop/LevelResultsRace, SessionResultsCoop/SessionResultsRace)
```

`server_logic` is linked into the client because `LocalServer` (offline mode) runs the server in a background thread inside the same process.

### SOLID design applied throughout

The codebase was refactored to follow SOLID and DRY principles:

| File                                        | Responsibility                                                                                                      |
| ------------------------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| `GameSession`                               | One play session: physics tick, reconciliation, render coordination                                                 |
| `NetworkClient`                             | ENet abstraction; `<enet/enet.h>` never appears outside NetworkClient.cpp                                           |
| `InputSampler`                              | All keyboard + gamepad sampling; sticky flags for rising-edge events                                                |
| `Renderer`                                  | All Raylib draw calls; no other file calls DrawXxx / BeginDrawing. Dispatches mode-specific rendering by `GameMode` |
| `HudCoop` / `HudRace`                       | Mode-specific HUD overlay (co-op: standard; race: adds "Race Mode" label top-right)                                 |
| `LevelResultsCoop` / `LevelResultsRace`     | Mode-specific end-of-level results screen                                                                           |
| `SessionResultsCoop` / `SessionResultsRace` | Mode-specific session-end global results screen                                                                     |
| `UIWidgets`                                 | Stateless Raylib UI helpers (buttons, text fields, CTRL+V paste support)                                            |
| `VisualEffects`                             | Client-only effect structs (trail, death particles, PauseState enum incl. LOBBY_SETTINGS) — no Raylib draw calls    |
| `LocalServer`                               | Wraps server thread for offline mode                                                                                |
| `ServerSession`                             | Full server session state machine; ENet-loop-agnostic. Manages leader election and game mode                        |
| `LevelManager`                              | Load maps, compute spawn, generate levels from chunks                                                               |
| `ChunkStore`                                | Loads all chunk TMJ files at startup; classifies into start/mid/end pools                                           |
| `LevelGenerator`                            | Composes playable levels from chunks with difficulty-curve-based selection                                          |
| `LevelValidator`                            | AI agent: BFS over ground tiles using real Player::Simulate to verify completability                                |
| `SpawnFinder.h`                             | Header-only; shared between GameSession and LevelManager                                                            |
| `PlayerReset.h`                             | Header-only; shared reset helper for kill/restart/level-change events                                               |
| `SoundPool`                                 | Pool of N sound variants; random pitch ±7 %; 2-D spatial audio (volume + stereo pan)                                |
| `SfxManager`                                | Owns jump + dash SoundPools; mute toggle; local vs. spatialized remote play                                         |
| `GameMode.h`                                | Header-only enum `GameMode { COOP, RACE }` — shared between client and server                                       |

---

## 4. Simulation Model (critical — read carefully)

### Determinism

`Player::Simulate(InputFrame, World)` is **byte-for-byte identical** on client and server.
Same `PlayerState` + same `InputFrame` → same result, always. This is the foundation of multiplayer correctness.

### Fixed timestep

Both client and server run physics at exactly **60 Hz** (`FIXED_DT = 1/60 s`).

Client loop:

```
Poll();                        // capture rising-edge input events once per render frame
accumulator += GetFrameTime();
while (accumulator >= FIXED_DT) {
    TickFixed(net);            // build InputFrame, send to server, simulate locally
    accumulator -= FIXED_DT;
}
// sub-frame interpolation for rendering using alpha = accumulator / FIXED_DT
```

Server loop (inside `enet_host_service` with 16 ms timeout):

```
OnReceive → HandleInput → Player::Simulate → BroadcastGameState
```

### Client-side prediction + reconciliation

1. Client simulates locally the moment `InputFrame` is built (before server reply).
2. Every sent `InputFrame` is archived in `input_history_[tick % 128]`.
3. When `PktGameState` arrives, find own `PlayerState` by `player_id`.
4. Take server's state (authoritative up to `last_processed_tick`).
5. Re-simulate all `InputFrame`s from `last_processed_tick + 1` up to `sim_tick_`.
6. Render the post-reconciliation state — always smooth, zero input lag.

### Collision resolution (split-axis)

`MoveX` then `MoveY` independently — standard AABB tile-based approach.
Corner correction: when the player's head clips a corner by ≤ `CORNER_CORRECTION_PX` (8 px = TILE_SIZE/4), the player is nudged horizontally instead of being blocked.

**Anti-tunnelling:** both `MoveX` and `MoveY` clamp the per-frame displacement to `TILE_SIZE − 1` px (31 px). This guarantees that `ResolveCollisionsX/Y` always sees the solid tile the player would penetrate, even under extreme velocities (e.g. dash push at 960 px/s → 16 px/frame clamped to 31).

**Player-vs-player world clamp:** after `ResolvePlayerCollisions` separates overlapping AABBs, each player is run through `ClampToWorld` which iteratively resolves any overlap with solid tiles using the minimum-penetration-axis method (up to 4 passes).

**Race mode:** player-to-player collisions (`ResolvePlayerCollisions`), magnet grab (`ApplyMagnetGrab`), and checkpoint activation are all skipped entirely. Each player races independently.

---

## 5. Player Mechanics Detail

### Jump mechanics

| Mechanic        | Implementation                                                                          |
| --------------- | --------------------------------------------------------------------------------------- |
| Basic jump      | `vel_y = -JUMP_FORCE` when `on_ground` (or coyote) and `jump_buffer_ticks > 0`          |
| Jump buffer     | `BTN_JUMP_PRESS` sets `jump_buffer_ticks = 10`; consumed on next valid surface          |
| Coyote time     | `coyote_ticks = 6` set when leaving ground without jumping                              |
| Variable height | `vel_y *= JUMP_CUT_MULTIPLIER (0.45)` when jump key released while rising               |
| Wall jump       | Jumps off wall when `on_wall_left/right` and `last_wall_jump_dir != current_wall`       |
| Dash jump       | Within `DASH_JUMP_WINDOW_TICKS = 10` after dash ends, jump force = 1150 instead of 1000 |

`on_wall_left/right` activate within `WALL_PROBE_REACH = 8 px` of a wall (not only on contact).

### Dash mechanics

- Direction: normalised from `(dash_dx, dash_dy)` in `InputFrame`; fallback = `(last_dir, 0)`
- During active dash: gravity and directional input both suspended
- `dash_ready` resets to true on landing (air dashes limited to one per airborne phase)
- Cooldown: `DASH_COOLDOWN_TICKS = 25` after dash ends
- Visual: `TrailState` ring buffer (12 points) rendered as fading ghost segments

### Dash push

- When a dashing player overlaps a non-dashing player, the target receives an impulse of
  `DASH_SPEED × DASH_PUSH_MULTIPLIER` (960 px/s) in the dasher's direction.
- The target's `vel_x`, `vel_y` are overwritten and `move_vel_x` is zeroed.
- When both players are dashing into each other, both dashes are cancelled and both receive
  the mutual push impulse.
- Position separation (from `ResolvePlayerCollisions`) still applies after the velocity push.

### Sprint

- Hold Right Ctrl (keyboard) or R2 / right trigger (gamepad, threshold > 0.3) to sprint.
- `BTN_SPRINT` (bit 8) is set in InputFrame; `Player::Simulate` multiplies `MOVE_SPEED` by `SPRINT_MULTIPLIER` (2.0).
- Sprint is disabled while a dash is active (`dash_active_ticks > 0`).
- `PlayerState::sprinting` flag is set for potential animation/visual use.
- `InputFrame::buttons` is `uint16_t` to accommodate 10 bits (PROTOCOL_VERSION = 10).

### Magnet grab

- Hold Alt (keyboard) or Circle / ○ (gamepad) to activate the magnet.
- `BTN_MAGNET` (bit 9) is set in InputFrame; `Player::Simulate` sets `PlayerState::magneting = true` (disabled during dash).
- Server-side: `ApplyMagnetGrab(ENetPeer* break_free)` runs each tick between `UpdateZone` and `ResolvePlayerCollisions`.
  When a magneting player has no grab target, the closest non-magneting player within `MAGNET_RANGE`
  is grabbed: `PlayerState::grabbed = true`, and the target’s physics are fully suspended (`Player::Simulate` early-returns).
- While grabbed, the target’s position is snapped **one tile above** the grabber (`target.y = grabber.y - TILE_SIZE`, `target.x = grabber.x`).
  Velocity is zeroed. The grabber moves normally and carries the target.
- Grab is released when: the grabber releases the magnet button, either player dies/respawns/finishes,
  either player disconnects, the **grabbed player presses jump (`BTN_JUMP_PRESS`) or dash (`BTN_DASH`)** (break-free),
  or the **grabbed player is pushed against a horizontal wall** (detected by a horizontal x-shift after `ClampToWorld`).
- After a break-free via jump/dash, the freed player is excluded from re-grabbing for the rest of that tick
  (via the `break_free` parameter passed from `HandleInput` to `ApplyMagnetGrab`).
- Grabbed players are excluded from `ResolvePlayerCollisions` (no push/separation applies to them).
- `grab_targets_` map in `ServerSession` tracks active grabber→target relationships; cleared on level change.
- `PlayerReset.h` resets `magneting = false` and `grabbed = false` on spawn and checkpoint resets.

### Drawing trails

- Player holds P (keyboard) or L2 / left trigger (gamepad) to enter draw mode.
- `BTN_DRAW` (bit 7) is set in InputFrame; `Player::Simulate` sets `PlayerState::drawing = true`.
- Client-side recording: each player accumulates strokes as `vector<Vector2>` (one stroke per press cycle).
  Points are sampled every tick, but a new point is only added when the distance from the last is ≥ 8 px.
- Max 4000 points per player per level; cleared on level change.
- **Optimised rendering:** Catmull-Rom splines are tessellated incrementally into cached polylines
  (`DrawStroke::tessellated`). Only new/tail segments are recomputed when points are added
  (6 subdivisions per segment). `Renderer::DrawMapTrails` draws the pre-computed polyline
  with `DrawLineEx` — no per-frame spline math.
  Colour matches the player (local = cyan, remote = orange), alpha 200.

### Kill tiles and respawn

- Touching a 'K' tile → server sets `kill_respawn_ticks = 60` (death animation, ~1 s)
- After that countdown: `respawn_grace_ticks = 60` is set → triggers Ready/Go! overlay on client
- Input is blocked while either countdown is > 0

---

## 5b. Game Modes and Leader Election

### Game modes

`GameMode` (defined in `src/common/GameMode.h`) determines per-session rules:

| Mode   | Enum value | Collisions | Checkpoints | Magnet grab | Default for |
| ------ | ---------- | ---------- | ----------- | ----------- | ----------- |
| `COOP` | 0          | ✅ Yes     | ✅ Yes      | ✅ Yes      | Online      |
| `RACE` | 1          | ❌ No      | ❌ No       | ❌ No       | Offline     |

`GameState` broadcasts `game_mode` and `leader_id` every tick so all clients stay in sync.

In **race mode**:

- `ResolvePlayerCollisions` and `ApplyMagnetGrab` are skipped (players pass through each other).
- Checkpoint activation is skipped in `HandleInput`.
- `World::StripCheckpoints()` replaces all 'C' tiles with air (' ') after level generation.

### Mode-specific rendering

Each of these rendering areas is split into a co-op file and a race file:

| Feature         | Co-op file               | Race file                |
| --------------- | ------------------------ | ------------------------ |
| HUD overlay     | `HudCoop.cpp`            | `HudRace.cpp`            |
| Level results   | `LevelResultsCoop.cpp`   | `LevelResultsRace.cpp`   |
| Session results | `SessionResultsCoop.cpp` | `SessionResultsRace.cpp` |

`Renderer` dispatches to the mode-specific implementation via `GameMode` parameter.
Race mode variants are currently identical to co-op but include a "Race Mode" header label.
Both HUD variants show a top-right mode label ("Co-op Mode" / "Race Mode"). Race mode HUD timers differ from co-op: the **top centre (48 px)** shows the player's current level completion time (`level_ticks` → `MM:SS.cc`), while the **top right (24 px)** shows the level expiry countdown directly below the "Race Mode" label (at y=36 px). Co-op mode shows only the time limit countdown at top centre.
Race mode session results show a **leaderboard sorted by wins descending**: each row displays rank (#1, #2, …), player name, and win count. First place uses the accent colour.

### Leader election

- **First connected:** when the server has no players and a new player connects, `leader_id_` is set to the new player's ID.
- **Promotion:** if the leader disconnects, `ElectLeader()` promotes the remaining player with the lowest `player_id` (earliest connected).
- **Reset:** when all players disconnect, `leader_id_` resets to 0 and `game_mode_` resets to `COOP`.

### Leader powers (lobby only)

- **Toggle mode:** leader opens the pause menu and selects "Lobby Settings" → "Mode" to toggle between Co-op and Race. The client sends `PKT_SET_GAME_MODE`; server validates sender is leader.
- **Session length:** leader opens "Lobby Settings" → "Levels" and changes generated levels per session (`PKT_SET_MAX_LEVELS`).
  Supported range is `1..20`, default `5`. Controls: left/right, mouse wheel, D-pad left/right, right stick left/right.
- **Start game:** the game starts automatically when all players reach the exit zone 'E'. The `PKT_START_GAME` packet is still supported server-side (for potential future UI use) but is no longer sent by the client (the G key binding was removed).

### Visual indicators

- Leader name is rendered in **bold** (`font_bold_`) above the player sprite. Other players use `font_hud_`.
- Lobby options panel (`DrawLobbyOptions`) shows: current mode and leader name. The panel no longer displays control hints (TAB/G were removed); lobby settings are accessed via the pause menu instead.

---

## 6. Game Flow

```
main()
  └─ ShowSplashScreen()            → blocks until any key/button pressed
  └─ ShowMainMenu()               → MenuResult { OFFLINE | ONLINE | QUIT, username, ip }
       └─ [OFFLINE] LocalServer::Start(skip_lobby=true, GameMode::RACE)
       └─ NetworkClient::Connect()
       └─ GameSession::Tick() loop
            ├─ InputSampler::Poll()
            ├─ HandlePauseInput()  (includes lobby settings for leader)
            ├─ TickFixed() × N    (60 Hz physics + network send)
            ├─ PollNetwork()      (ENet events → HandlePacket)
            └─ DoRender()
       └─ [session end] DrawSessionEndScreen for 3 s
  └─ back to ShowMainMenu()
```

### Level progression (server-side)

```
Online mode:
  Lobby (_Lobby.tmj)  [file-based]
    → all players in zone 'E' for 3 s → DoLevelChange()
        → Generated Level 1, 2, … MAX_GENERATED_LEVELS (chunk-based)
            → all players finish OR 2-min time limit → SendResults() (15 s or all ready)
                → DoLevelChange() → next generated level
                    → last level complete → SendGlobalResults() (25 s or all ready)
                        → FinishSession() → PKT_LOAD_LEVEL(is_last=1) → ResetToInitial() → back to lobby

Offline mode (skip_lobby, GameMode::RACE):
  Level 1 generated immediately (no lobby, race mode) → same progression as above
```

Online mode starts at the lobby; offline mode skips it and generates level 1
immediately. Levels are procedurally generated from chunk pieces stored in
`assets/levels/chunks/` (scanned recursively, including subdirectories).
The server loads all chunks into memory at startup via `ChunkStore`, then generates
each level on-the-fly using `LevelGenerator`.
Each generated level is wrapped in a 5-tile solid border for safety.
After generation, `LevelValidator` runs a BFS agent that tries ~17 macro-actions
(walk, jump, dash, wall-jump, dash-jump combos) from every reachable ground tile
using the real `Player::Simulate` physics. If the agent cannot reach any 'E' tile,
the level is discarded and regenerated with a different seed (up to 10 retries).
**Validation is enabled in offline mode**; unsolved variants are discarded.
Online mode skips validation (`validate=false`) for faster generation.
Generated levels are transmitted to clients via `PKT_LEVEL_DATA` (variable-size packet
containing the full tile grid, including the `level` number for HUD display).
Generated levels per session are leader-configurable in lobby (`1..20`, default `5`).

**Difficulty curve:** the generator maps `level_num / total_levels` to a `[0,1]` progression
(`t`), then selects mid chunks whose `difficulty` property falls within a band centred on
`min_d + t * (max_d - min_d)` (±0.5). Level length grows aggressively with `t`: the first
level has 4 mid chunks and the last has 28 (≈ double the previous max of 14).
This ensures all difficulty tiers are used evenly.

**Chunk-based checkpoint placement:** at startup, `ChunkStore` classifies mid chunks
into two sub-pools: `mid_checkpoint_` (contain 'C' tiles) and `mid_normal_` (no 'C' tiles).
During level generation, the generator picks checkpoint chunks at regular intervals:

- Levels with < 8 mid chunks: no checkpoint chunks inserted.
- Levels with ≥ 8 mid chunks: every 5th mid slot uses a checkpoint chunk instead of a normal one.
  For branching levels, each branch independently continues the counter from the pre-fork section.
- 'C' tiles from chunk data are preserved (no longer stripped during finalisation).

**Shared checkpoints (cooperative):** when _any single player_ reaches a checkpoint tile,
the server activates that checkpoint for _all_ players (`ServerSession::activated_checkpoints_`)
and immediately resets every player to start from that newly activated checkpoint.
Once a checkpoint has been activated, passing over it again does not trigger it a second time.
This prevents a player that backtracks from resetting everyone's checkpoint to an earlier position.

The server tracks cooperative level clears in `coop_cleared_levels_`.
At session end, `PKT_GLOBAL_RESULTS` broadcasts the team's clear count to all clients.

### Packet types

| Packet                 | Direction | Event                                                                  |
| ---------------------- | --------- | ---------------------------------------------------------------------- |
| `PKT_INPUT`            | C → S     | One `InputFrame` per tick                                              |
| `PKT_GAME_STATE`       | S → C     | Full `GameState` broadcast after every input                           |
| `PKT_WELCOME`          | S → C     | On connect: `player_id` + `session_token`                              |
| `PKT_PLAYER_INFO`      | C → S     | After welcome: `name` + `protocol_version`                             |
| `PKT_LOAD_LEVEL`       | S → C     | Load next map from file (lobby) or `is_last=1` → return to menu        |
| `PKT_LEVEL_DATA`       | S → C     | Generated level tile grid (variable-size: header + width×height chars) |
| `PKT_LEVEL_RESULTS`    | S → C     | End-of-level sorted leaderboard                                        |
| `PKT_GLOBAL_RESULTS`   | S → C     | Session-end win leaderboard (after last level)                         |
| `PKT_RESTART`          | C → S     | Respawn at last checkpoint (or spawn if none); Backspace / Triangle    |
| `PKT_RESTART_SPAWN`    | C → S     | Respawn always at level spawn, clearing checkpoint; Delete             |
| `PKT_READY`            | C → S     | Skip results screen early                                              |
| `PKT_VERSION_MISMATCH` | S → C     | Incompatible `PROTOCOL_VERSION` — disconnect follows                   |
| `PKT_EMOTE`            | C → S     | Emote selection (emote_id 0-7)                                         |
| `PKT_EMOTE_BROADCAST`  | S → C     | Broadcast emote to all clients                                         |
| `PKT_GENERATING`       | S → C     | Server is generating the next level; client shows loading overlay      |
| `PKT_SET_GAME_MODE`    | C → S     | Leader sets the game mode (coop / race); lobby only                    |
| `PKT_START_GAME`       | C → S     | Leader starts the game from the lobby                                  |

---

## 7. Rendering

`Renderer` owns all Raylib calls. No other file may call `DrawXxx`, `BeginDrawing`, `BeginMode2D`, etc. Replacing Raylib means rewriting only `Renderer.cpp`.

**Mode-specific rendering:** `DrawHUD`, `DrawResultsScreen`, and `DrawGlobalResultsScreen` accept a `GameMode` parameter and dispatch to the mode-specific implementation (e.g. `DrawHudModeCoop` vs `DrawHudModeRace`). Race mode variants display a "Race Mode" header. See section 5b for the file breakdown.

**Timer (`DrawTimer`):** accepts a `GameMode` parameter. In **co-op mode**: time-limit countdown at top centre (48 px). In **race mode**: level completion timer (`level_ticks` → `MM:SS.cc`) at top centre (48 px), level expiry countdown at top right (24 px, under the "Race Mode" label).

**Pause menu (`DrawPauseMenu`):** accepts `show_lobby_settings` and `lobby_mode` parameters. When the local player is the session leader in the lobby, a "Lobby Settings" item appears in the menu. The "Lobby Settings" sub-menu (`LOBBY_SETTINGS` state) shows "Mode: CO-OP / RACE" (toggle) and "Back". The quit confirmation dialog (`CONFIRM_QUIT`) now uses `GetPauseItemRect(i, 2)` for consistent click hit-testing.

**Leader display:** `DrawPlayer` accepts an `is_leader` parameter. Leader names are rendered in `font_bold_`; other players use `font_hud_`.

**Lobby options panel:** `DrawLobbyOptions` shows a semi-transparent panel (top-right) with the current game mode and leader name. Control hints (TAB/G) have been removed; the leader changes settings via the pause menu's "Lobby Settings" sub-menu instead.

**Font sizes:**

- `font_small_` — 20 px — minor HUD text
- `font_hud_` — 28 px — HUD, pause menu, results
- `font_timer_` — 72 px — in-game timer display
- `font_bold_` — **200 px** — Ready?/Go! overlay (loaded large to avoid upscaling blur; all draw sizes are downscales)

**Camera:**

- `Camera2D` follows the local player with smooth lerp
- Camera shake triggered on death (`TriggerShake`)
- Sub-frame visual interpolation: render position = `lerp(prev_xy, curr_xy, alpha)`

**Colour palette:** all colours defined in `Colors.h` as `CLRS_<AREA>_<ROLE>` constants.\n\n**Level indicator (`DrawLevelIndicator`):** shows \"Level N\" at bottom-centre using `font_hud_` 24 px. Visible only when `current_level_ > 0` (i.e. during generated levels, not in the lobby).

**Off-screen player indicators (`DrawOffscreenArrows`):**

- Called in `GameSession::DoRender` after `EndWorldDraw` and before any HUD draw call.
- For each remote player whose screen-space position (via `GetWorldToScreen2D`) lies outside the viewport:
  - Computes direction from screen centre to the player's screen position.
  - Intersects the ray with the inset viewport rectangle (margin = 64 px from each edge).
  - Draws a filled orange circle (diameter 16 px, `CLRS_PLAYER_REMOTE`) with a `CLRS_PLAYER_REMOTE_NAME` outline.
  - Draws the player's name (`font_small_`, 14 px) centred above the dot.
- Skipped while the player is in the death-respawn animation (`kill_respawn_ticks > 0`).

---

## 8. Persistence

`SaveData` (`save.dat` in CWD): stores `username`, `last_ip`, and `sfx_muted`. Serialised as JSON (`{"u":"…","i":"…","m":"0/1"}`), XOR-obfuscated on disk with a 16-byte rotating key. Loaded at startup, saved:

- Before returning from `ShowMainMenu` (username, IP, mute).
- Immediately on each SFX toggle (main menu M key / pause menu item).

Old save files without `"m"` key default to unmuted (backward-compatible).

---

## 9. Offline Mode

`LocalServer` starts `RunServer()` in a `std::thread`. The client connects to `127.0.0.1:58721` exactly as it would online. This allows single-player practice without a separate server process.

**Offline defaults to Race mode:** `LocalServer::Start` is called with `skip_lobby=true` and `GameMode::RACE`. The server generates level 1 immediately with checkpoints stripped.

Online mode starts at the lobby (`_Lobby.tmj`), defaulting to Co-op mode. The leader can switch to Race mode from the pause menu's "Lobby Settings" option.

---

## 10. Network Constants

```cpp
SERVER_PORT        = 58291   // online / dedicated server
SERVER_PORT_LOCAL  = 58721   // in-process LocalServer (offline mode)
PROTOCOL_VERSION   = 10      // increment on any breaking change
MAX_PLAYERS        = 8
CHANNEL_RELIABLE   = 0
CHANNEL_COUNT      = 1
LOBBY_MAP_PATH     = "assets/levels/tilemaps/_Lobby.tmj"
```

Disconnect reason codes are sent as the `data` field of `enet_peer_disconnect()`:

- `DISCONNECT_GENERIC = 0`
- `DISCONNECT_VERSION_MISMATCH = 1` (bits[31:16] = server version)
- `DISCONNECT_SERVER_BUSY = 2`

---

## 11. Key Physics Constants (all in `Physics.h`)

```cpp
TILE_SIZE            = 32       // px per tile
FIXED_DT             = 1/60 s
MOVE_SPEED           = 300      // px/s max
MOVE_ACCEL           = 8000     // px/s²
MOVE_DECEL           = 6000     // px/s²
GRAVITY              = 2000     // px/s²
JUMP_FORCE           = 1000     // upward impulse
DASH_JUMP_FORCE      = 1150     // post-dash enhanced jump
MAX_FALL_SPEED       = 1400     // px/s terminal velocity
DASH_SPEED           = 1200     // px/s during dash
DASH_ACTIVE_TICKS    = 12       // ~200 ms
DASH_COOLDOWN_TICKS  = 25       // ~417 ms
DASH_PUSH_MULTIPLIER = 0.8      // pushed player gets 0.8× dash velocity
SPRINT_MULTIPLIER    = 2.0      // sprint horizontal speed factor
MAGNET_RANGE         = 256      // px — magnet grab radius
GRAB_OFFSET_X        = 36       // px — horizontal offset for carried player (TILE_SIZE + 4)
DASH_JUMP_WINDOW     = 10       // post-dash jump boost window
JUMP_BUFFER_TICKS    = 10       // ~167 ms
COYOTE_TICKS         = 6        // ~100 ms
JUMP_CUT_MULTIPLIER  = 0.45
CORNER_CORRECTION_PX = 8        // = TILE_SIZE/4
WALL_PROBE_REACH     = 8        // px beyond edge to detect adjacent walls
```

---

## 12. Asset Layout

```
assets/
  fonts/                    SpaceMono-Bold.ttf (used for all in-game text)
  levels/
    TileSet.tsx             Tiled tileset definition (properties: type, solid)
    TileSet.png             Tileset spritesheet
    chunks/                 Chunk .tmj files for procedural level generation (recursive scan)
      _Start01.tmj          Start chunk (has spawn + exit)
      _End01.tmj            End chunk (entry + end tiles)
      01/                   Difficulty 1 mid chunks (Flat)
      02/                   Difficulty 2 mid chunks (Simple)
      03/                   Difficulty 3 mid chunks (Medium)
      04/                   Difficulty 3 mid chunks (Hard)
      __README.md           Chunk authoring guidelines
    tilemaps/
      _Lobby.tmj            Waiting room — triggers level progression when all are in zone 'E'
    _old/                   Legacy ASCII maps (no longer loaded at runtime)
```

Map format: **Tiled JSON** (`.tmj`), 32 px per tile, uncompressed orthogonal.

Tileset (`TileSet.tsx`) tile properties:

| Tile ID | `type`        | `solid` | Legacy char | Behaviour                                             |
| ------- | ------------- | ------- | ----------- | ----------------------------------------------------- |
| 0       | `spawn`       | false   | `X`         | Player start position                                 |
| 1       | `platform`    | true    | `0`         | Solid wall / floor                                    |
| 2       | `kill`        | true    | `K`         | Death + respawn on touch                              |
| 3       | `end`         | false   | `E`         | Exit / finish tile                                    |
| 4       | `checkpoint`  | false   | `C`         | Saves a respawn position; player dies → respawns here |
| 5       | `chunk_entry` | false   | `I`         | Chunk entry connector (stripped during generation)    |
| 6       | `chunk_exit`  | false   | `O`         | Chunk exit connector (stripped during generation)     |
| 8       | `chunk_entry` | false   | `I`         | Chunk entry (alt tile ID)                             |
| 9       | `chunk_exit`  | false   | `O`         | Chunk exit (alt tile ID)                              |

On death (kill tile) the player respawns at their last shared checkpoint (`PlayerState::checkpoint_x/y`); if no checkpoint has been activated they respawn at the level spawn. `SpawnReset` clears the checkpoint; `CheckpointReset` preserves it.
Checkpoints are shared: one player reaching a 'C' tile group activates the spawn position for every connected player and all players are reset to that checkpoint instantly.

**Restart keys:**

- Backspace / Triangle: restart at last checkpoint (or spawn if none)
- Delete: restart at level spawn (clears checkpoint)

---

## 14. Audio System

### Overview

`InitAudioDevice()` / `CloseAudioDevice()` are called in `main.cpp` (after `InitWindow` / before `CloseWindow`). All audio is owned by `SfxManager`, which is a member of `GameSession` and **loaded once at construction** (never re-read at runtime).

### SoundPool

`SoundPool` manages N `Sound` instances loaded from explicit file paths:

- `Play(pitch_variance)` — picks a variant round-robin (avoids same-sound repetition), applies random pitch in `[1 − var, 1 + var]`, centre pan, full volume.
- `PlayAt(source, listener, max_dist, pitch_variance)` — same as `Play` but:
  - Linear volume: `1 − dist / max_dist` (silent beyond `max_dist`).
  - Stereo pan: `0.5 + dx / (max_dist × 0.5)` clamped to `[0, 1]`.
  - Sounds beyond `max_dist` are culled without calling Raylib.

### SfxManager

Owns two `SoundPool`s: `jump_pool_` (3 variants) and `dash_pool_` (3 variants), loaded from `assets/sfx/jump_01-03.wav` and `assets/sfx/dash_01-03.wav`.

API:

- `PlayJump()` / `PlayDash()` — local player, full volume, centre.
- `PlayJumpAt(sx, sy, lx, ly)` / `PlayDashAt(sx, sy, lx, ly)` — remote player, spatialized (`max_dist = 1400 px`).
- `SetMuted(bool)` / `IsMuted()` — all four play methods are no-ops when `muted_ == true`.

Initial mute state is read from `SaveData::sfx_muted` in `GameSession`'s constructor.

### Event detection

**Local player** (in `GameSession::TickFixed`, after `Player::Simulate`):

- **Jump**: `pre_vy > −500 && post_vy ≤ −500` → `sfx_.PlayJump()`
- **Dash**: `pre_dash == 0 && post_dash > 0` → `sfx_.PlayDash()`

**Remote players** (in `GameSession::HandlePacket` → `PKT_GAME_STATE`, gated by `last_processed_tick != prev_tick`):

- Same velocity / dash threshold checks as above.
- First appearance: prev-state is initialised to the current snapshot **without** firing sounds (prevents false triggers on join).
- Player in death/grace period: prev-state is updated without firing (prevents false triggers on respawn).
- Sounds play via `PlayJumpAt` / `PlayDashAt` using the local player's centre as the listener.

### Mute controls

| Location   | Control                                                        | Behaviour                       |
| ---------- | -------------------------------------------------------------- | ------------------------------- |
| Main menu  | **M** key                                                      | Toggles mute, saves immediately |
| Main menu  | Click **"SFX: ON [M]" / "SFX: OFF [M]"** label (bottom-left)   | Same                            |
| Pause menu | Navigate to **"SFX: ON" / "SFX: OFF"** (middle item) + confirm | Same                            |

Pause menu now has **3 or 4 items** depending on context: Resume / [Lobby Settings (leader only, lobby)] / SFX toggle / Quit to Menu. When the local player is the session leader and in the lobby, the "Lobby Settings" item appears between Resume and SFX toggle (4 items total, navigation wraps mod-4); otherwise it is hidden (3 items, wraps mod-3).

---

## 15. Build Output / Install Path

The install step writes to a **per-platform `deploy/` subdirectory** (not `dist/`):

| Platform | Install prefix  |
| -------- | --------------- |
| Windows  | `deploy/win/`   |
| macOS    | `deploy/mac/`   |
| Linux    | `deploy/linux/` |

Set in `CMakeLists.txt` via `CMAKE_INSTALL_PREFIX … CACHE PATH … FORCE` before the `install()` rules. The `installDir` field has been removed from `CMakePresets.json`. The prefix can still be overridden at install time:

```bash
cmake --install build/release --component TileRaceRuntime --prefix /custom/path
```

After changing `CMakeLists.txt`, run `cmake --preset release` once to apply the new prefix to the cache.

| Feature                                                           | Status                    |
| ----------------------------------------------------------------- | ------------------------- |
| Tilemap loading and rendering                                     | ✅                        |
| Tiled `.tmj` map format + TSX property parsing                    | ✅                        |
| Shared checkpoints ('C') + cooperative activation                 | ✅                        |
| Chunk-based checkpoint placement (interspersed mid pool)          | ✅                        |
| Player physics (move, accel, gravity, collision)                  | ✅                        |
| Jump buffer, coyote time, variable height, wall jump              | ✅                        |
| Dash (360°, air limit, cooldown, dash-jump)                       | ✅                        |
| Dash push (slam other players with 2× dash force)                 | ✅                        |
| Sprint (Right Ctrl / R2, 1.5× horizontal speed)                   | ✅                        |
| Drawing trails (P / L2, cached Catmull-Rom splines)               | ✅                        |
| Fixed timestep (60 Hz)                                            | ✅                        |
| Camera follow + shake + sub-frame interpolation                   | ✅                        |
| ENet client/server (online + offline)                             | ✅                        |
| Client-side prediction + server reconciliation                    | ✅                        |
| Multiple players, remote player rendering                         | ✅                        |
| Off-screen player indicators (compass arrows)                     | ✅                        |
| Kill tiles + death animation + Ready/Go! overlay                  | ✅                        |
| Race timer, live leaderboard                                      | ✅                        |
| End-of-level results screen                                       | ✅                        |
| Session-end global leaderboard (win counts per player)            | ✅                        |
| Chunk-based procedural level generation                           | ✅                        |
| AI-driven level validation (physics BFS, offline only)            | ✅                        |
| Lobby → level progression loop                                    | ✅                        |
| 2-minute time limit per level                                     | ✅                        |
| Restart from checkpoint (PKT_RESTART) / spawn (PKT_RESTART_SPAWN) | ✅                        |
| Magnet grab mechanic (Alt / Circle, grab & carry players)         | ✅                        |
| Main menu (offline/online, username, IP)                          | ✅                        |
| Splash screen (title + press any button)                          | ✅                        |
| Persistent save data (username, last IP, sfx mute)                | ✅                        |
| Pause menu with quit confirmation + lobby settings                | ✅                        |
| Colour palette (Colors.h)                                         | ✅                        |
| Visual trail (dash), death particles                              | ✅                        |
| Personal best time tracking                                       | ✅                        |
| Ready mechanic (skip results screen early)                        | ✅                        |
| Version mismatch detection + graceful error                       | ✅                        |
| Server-busy rejection                                             | ✅                        |
| Win32 taskbar icon                                                | ✅                        |
| SOLID refactoring (client + server)                               | ✅                        |
| Audio SFX (jump, dash) — local + 2-D spatialized remote           | ✅                        |
| SFX mute toggle (main menu + pause menu, persisted)               | ✅                        |
| Per-platform deploy folder (deploy/win\|mac\|linux)               | ✅                        |
| Game modes: Co-op (default online) / Race (default offline)       | ✅                        |
| Session leader election + promotion on disconnect                 | ✅                        |
| Leader lobby controls (toggle mode via pause menu)                | ✅                        |
| Mode-specific rendering (HUD, results, session results)           | ✅                        |
| Bold leader name display                                          | ✅                        |
| Race mode: no collisions, no checkpoints, stripped 'C' tiles      | ✅                        |
| CTRL+V paste in online menu text fields                           | ✅                        |
| Race HUD: level timer (top centre) + expiry countdown (top right) | ✅                        |
| Race session results: ranked leaderboard sorted by wins           | ✅                        |
| Multiple spawn assignment by player_id                            | ❌ (all use center spawn) |
| Linux / macOS build verification                                  | ❌                        |
