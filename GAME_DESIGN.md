# TileRace — Game Design & Technical Reference

Questo documento descrive in modo esaustivo il gioco che stiamo costruendo, la sua architettura tecnica e la struttura dei file. È pensato per essere autosufficiente: chiunque (o qualsiasi AI) lo legga deve avere tutto il contesto per continuare lo sviluppo senza domande.

---

## 1. Cos'è TileRace

TileRace è un **platformer 2D multiplayer competitivo** visto di lato. Due o più giocatori gareggiano per raggiungere lo stesso traguardo su una mappa a tile. Vince chi arriva prima.

**Caratteristiche principali del gameplay:**
- Movimento orizzontale la cui fisica (inerzia, accelerazione, attrito) è ancora da definire e bilanciare
- Salto (SPAZIO), wall jump (SPAZIO mentre si è appoggiati a un muro), dash orizzontale (SHIFT) — le meccaniche avanzate vanno rivalutate durante i playtest
- Mappe a tile 32×32 px caricate da file `.txt`
- Partite sia online (server dedicato) che offline (server embedded nello stesso processo)
- Massimo 8 giocatori per partita

**Controlli:**
| Tasto / Pulsante | Gamepad (PS) | Azione |
|---|---|---|
| ← / A | Stick sin. / D-pad ← | Muovi sinistra |
| → / D | Stick sin. / D-pad → | Muovi destra |
| SPAZIO | Croce (×) | Salto / Wall jump |
| SHIFT | Quadrato (□) o Cerchio (○) | Dash (360°: stick analogico per direzione continua, tasti/dpad per 8 direzioni discrete) |

---

## 2. Architettura tecnica

### Principio fondamentale: simulazione deterministica

`Player::Simulate(InputFrame, World)` è **identica** su client e server. Stessi input + stesso stato iniziale = stesso risultato, sempre. Questo è il pilastro su cui poggia il multiplayer.

### Piattaforme supportate

Il gioco è **cross-platform** by design:
- **Windows** — compilatore w64devkit (GCC) o MSVC
- **Linux** — GCC o Clang, dipendenze di sistema: `libGL`, `libX11`, `libXrandr`, `libXi`, `libasound2`
- **macOS** — Clang, framework nativi (OpenGL/Metal via Raylib)

Raylib e ENet vengono scaricati e compilati da sorgente tramite `FetchContent`, quindi non esistono dipendenze binarie pre-compilate legate alla piattaforma. Il codice di gioco non usa API specifiche di Windows salvo nei link flags CMake (già condizionali con `if(WIN32)`).

### Stack tecnologico

| Componente | Libreria |
|---|---|
| Grafica / finestra / input / audio | Raylib 5.5 (via FetchContent) |
| Rete UDP | ENet (via FetchContent, mirror GitHub) |
| Build system | CMake 3.16+ con Ninja |
| Linguaggio | C++20 |
| Compilatore (Windows) | w64devkit (GCC) o MSVC |
| Compilatore (Linux/macOS) | GCC o Clang |

### Struttura CMake

```
common_logic    (static lib) ← Player.cpp, World.cpp
server_logic    (static lib) ← GameServer.cpp  [linka common_logic + enet]
TileRace_Server (exe)        ← server/main.cpp [linka server_logic]
TileRace_Client (exe)        ← client/main.cpp + ... [linka common_logic + server_logic + raylib + enet]
```

`server_logic` è linkato anche dal client perché `LocalServer` (modalità offline) istanzia `GameServer` direttamente in-process.

### Loop di gioco

**Server** (thread dedicato, 60Hz fisso):
```
while running:
    ProcessNetworkEvents()   // svuota coda ENet, non-blocking
    Tick()                   // Player::Simulate per ogni giocatore
    BroadcastGameState()     // PktGameState in UDP unreliable a tutti
    sleep_until(next_tick)
```

**Client** (fixed timestep + render senza cap):
```
while not closed:
    ProcessNetworkEvents()
    while accumulator >= 1/60s:
        Tick()               // prediction locale + invia input
        accumulator -= 1/60s
    alpha = accumulator / (1/60s)
    Render(alpha)            // interpolazione visiva tra tick
    input_handler_.Poll()   // DOPO BeginDrawing per catturare IsKeyPressed
```

### Client-Side Prediction e Reconciliation

> ⚠️ Questo è uno degli aspetti più delicati del progetto. L'implementazione precedente aveva problemi che richiedono una riscrittura completa. Va affrontato come un problema a sé, con attenzione al dettaglio, durante il passo 15-16.

L'obiettivo è che il giocatore locale non percepisca mai latenza di input. L'approccio generale:
1. Il client simula localmente **prima** di ricevere conferma dal server
2. Gli input inviati vengono tenuti in un buffer finché il server non li conferma
3. Quando lo stato autoritativo del server arriva, si confronta con la predizione — in caso di divergenza si corregge

I dettagli implementativi (soglie di errore, dimensione del buffer, gestione degli stati intermedi, comportamento con lag elevato) sono da decidere durante lo sviluppo di quella fase.

### Interpolazione visiva

Per eliminare il jitter visivo a FPS > 60Hz si salva lo stato del tick precedente e si interpola tra quel valore e quello corrente usando `alpha = accumulator / FIXED_DT`. I dettagli (quali campi interpolare, come gestire i remote players) sono da definire durante il passo 8 e poi rivisti durante la fase di rete.

### Meccaniche del salto avanzate

Queste tre tecniche sono standard nei platformer moderni e vanno implementate tutte insieme durante il passo 6 (o subito dopo), perché agiscono tutte su `PlayerState` e su `Simulate`. I valori delle costanti sono da calibrare ai playtest.

#### Jump buffer
Se il giocatore preme SPAZIO mentre è ancora in aria, il salto non viene ignorato: viene messo in memoria per un breve finestra di tick. Appena il player atterra (o tocca un muro), il salto viene eseguito automaticamente.
- `PlayerState` aggiunge `uint8_t jump_buffer_ticks` — decrementato ogni tick, azzerato quando il salto viene consumato.
- `Physics.h` aggiunge `JUMP_BUFFER_TICKS = 10` (≈ 167 ms a 60 Hz — da bilanciare).
- In `Simulate`: se `IsJump(input)` → `jump_buffer_ticks = JUMP_BUFFER_TICKS`. Subito dopo il check di atterraggio (cioè dopo `ResolveCollisionsY`): se `on_ground && jump_buffer_ticks > 0` → esegui il salto e azzera il buffer.
- **Wall jump buffer**: stesso meccanismo — se `jump_buffer_ticks > 0` e `on_wall_left/right` → esegui il wall jump.
- **Divieto doppio salto sullo stesso muro**: `PlayerState` aggiunge `int8_t last_wall_jump_dir` (`-1`=sinistra, `0`=nessuno, `1`=destra). Il wall jump scatta solo se `last_wall_jump_dir != lato_corrente`; viene resettato a `0` quando il player tocca terra o salta da terra. In questo modo si possono alternare muri a destra e sinistra liberamente, ma non rimbalzare ripetutamente sullo stesso muro senza toccare il pavimento.

#### Coyote time
Il player può saltare per qualche tick anche dopo essere camminato fuori da un bordo, purché non abbia già saltato. Elimina la sensazione frustrante di "ho premuto ma era troppo tardi".
- `PlayerState` aggiunge `uint8_t coyote_ticks` — decrementato ogni tick.
- `Physics.h` aggiunge `COYOTE_TICKS = 6` (100 ms — da bilanciare).
- In `Simulate`: ogni tick in cui `on_ground` era true nel tick precedente e ora non lo è più (transizione terra→aria **senza** aver saltato), setta `coyote_ticks = COYOTE_TICKS`.
- Il gate del salto diventa: `if (on_ground || coyote_ticks > 0)` invece di solo `on_ground`; quando il salto scatta azzera `coyote_ticks`.
- ⚠️ Attenzione alla serializzazione: `coyote_ticks` e `jump_buffer_ticks` devono far parte di `PlayerState` perché la simulazione sia deterministica (server e client devono avere lo stesso valore).

#### Salto variabile (hold to jump higher)
Tenere premuto SPAZIO produce un salto più alto; rilasciare prima taglia il salto. Si ottente moltiplicando `vel_y` per un fattore di taglio non appena il tasto viene rilasciato mentre il player sta ancora salendo.
- Non serve nessun campo aggiuntivo in `PlayerState`.
- `Physics.h` aggiunge `JUMP_CUT_MULTIPLIER = 0.45f` (da bilanciare).
- In `Simulate`: se `vel_y < 0` e `!IsJump(input)` (tasto non tenuto) → `vel_y *= JUMP_CUT_MULTIPLIER` (applicato una sola volta, non ogni tick; serve un flag `jump_released_` oppure il confronto con il `BTN_JUMP` del tick precedente).
- Alternativa più semplice: applicare una gravità più forte quando `vel_y < 0` e il tasto non è tenuto — meno preciso ma più facile da implementare. Decidere durante i playtest.

> **Stato implementazione salto avanzato:** tutte e quattro le meccaniche (jump buffer, coyote time, salto variabile, wall jump con divieto stesso lato) sono **già implementate** nei passi 5-6. I valori delle costanti in `Physics.h` sono da bilanciare durante i playtest.

#### Wall probe con lookahead
`on_wall_left` / `on_wall_right` si attivano già quando il player si trova entro `WALL_PROBE_REACH` pixel dal muro (attualmente `TILE_SIZE / 4 = 8 px`), non solo quando lo tocca fisicamente. Questo rende il wall jump più facile da eseguire senza richiedere un contatto pixel-perfetto con la parete. La costante è in `Physics.h` e può essere bilanciata ai playtest.

### Dash (passo 17)

Tasto SHIFT (tastiera) o Quadrato / Cerchio (gamepad PS). Il dash proietta il player a velocità `DASH_SPEED` in una direzione libera a 360°, con gravità sospesa per tutta la sua durata.

**Comportamento:**
- `RequestDash(float dx, float dy)` riceve il vettore di direzione grezzo (non normalizzato). Internamente calcola `len = sqrtf(dx²+dy²)` e memorizza il vettore unitario in `dash_dir_x`/`dash_dir_y`. Se `len < 0.001` (nessun input) usa come fallback `(last_dir, 0)` (dash orizzontale nell'ultima direzione).
- Scatta solo se `dash_cooldown_ticks == 0` e `dash_active_ticks == 0`: azzera `vel_x`/`vel_y` e setta `dash_active_ticks = DASH_ACTIVE_TICKS`.
- In `MoveX` con `dash_active_ticks > 0`: `total_dx = dash_dir_x × DASH_SPEED × FIXED_DT`; ignora input corrente e impulso wall jump.
- In `MoveY` con `dash_active_ticks > 0`: se `dash_dir_y != 0.f` applica `dy = dash_dir_y × DASH_SPEED × FIXED_DT`, imposta `vel_y = dy` (necessario perché `ResolveCollisionsY` controlla il segno di vel_y), poi risolve collisioni Y e resetta `vel_y = 0`; decrementa `dash_active_ticks`; se raggiunge 0 setta `dash_cooldown_ticks = DASH_COOLDOWN_TICKS`; return immediato (gravità e logica di salto sospese).
- Il wall jump è inibito durante il dash (`dash_active_ticks == 0` nel guard).
- `last_dir` viene aggiornato ogni tick in `MoveX` dalla direzione di input; se non c'è input, mantiene l'ultimo valore.

**Costanti in `Physics.h`** (da bilanciare ai playtest):
```cpp
inline constexpr float DASH_SPEED          = 750.f; // px/s durante il dash (1.5× il valore base)
inline constexpr int   DASH_ACTIVE_TICKS   =  12;   // durata (~200ms)
inline constexpr int   DASH_COOLDOWN_TICKS =  40;   // cooldown (~667ms)
```

**Campi in `PlayerState`:**
- `uint8_t dash_active_ticks` — tick di dash rimanenti; > 0 = dashing
- `uint8_t dash_cooldown_ticks` — tick di cooldown rimanenti
- `int8_t last_dir` — ultima direzione orizzontale non-zero (-1/+1); mai 0; usata come fallback quando si fa dash senza input
- `float dash_dir_x` — componente X del vettore unitario di direzione del dash corrente
- `float dash_dir_y` — componente Y del vettore unitario di direzione del dash corrente

**Input (client-side, `main.cpp`):** sticky flag `dash_pending`; al tick successivo campiona:
- **Tastiera:** ←/→/A/D → `raw_x ±1`; ↑/↓/W/S → `raw_y ±1` (può produrre diagonali discrete come (1,−1))
- **Gamepad stick:** se `||(ax, ay)||² > GP_DEADZONE²` usa il vettore analogico grezzo (360° continui), altrimenti cade sul D-pad (discreto). Il vettore grezzo sovrascrive il keyboard input.
- `RequestDash(raw_x, raw_y)` normalizza internamente; se (0,0) usa `last_dir`.

**Gamepad (layout PS, indice 0):** `GP_JUMP = GAMEPAD_BUTTON_RIGHT_FACE_DOWN` (Croce), `GP_DASH_A = GAMEPAD_BUTTON_RIGHT_FACE_LEFT` (Quadrato), `GP_DASH_B = GAMEPAD_BUTTON_RIGHT_FACE_RIGHT` (Cerchio). Movimento da `GAMEPAD_AXIS_LEFT_X/Y` (deadzone `GP_DEADZONE = 0.25`) + D-pad. Costanti e deadzone definiti in testa a `client/main.cpp`.

**Trail visivo (solo client, `main.cpp`):** durante il dash, ogni tick viene salvata la posizione del player in un ring buffer circolare (`TRAIL_LEN = 12` elementi). I ghost vengono disegnati prima del player come rettangoli 32×32 color azzurro (`{0, 180, 255}`) con alpha decrescente dalla testa alla coda; i ghost più vecchi si rimpiccioliscono leggermente. La trail si azzera appena il dash termina.

### Input sticky flags

`IsKeyPressed` di Raylib viene resettato da `BeginDrawing()` → `PollInputEvents()`. Se un frame di rendering va senza tick (FPS > 60), la pressione andrebbe persa. Soluzione:
- `InputHandler::Poll()` chiamato **dopo** `Render()` salva `jump_pending_` e `dash_pending_` come bool sticky
- `InputHandler::Sample()` li consuma al tick fisso successivo e li azzera

### Isolamento sessione offline

`LocalServer` genera un `session_token` a 32-bit casuale via `std::random_device`. Il server:
1. Binda su `127.0.0.1` (non raggiungibile da rete)
2. Mette ogni nuova connessione in `pending_auth_` senza assegnarle un player_id
3. Solo se `PktConnectRequest.session_token` combacia → promuove a giocatore
4. Altrimenti → `enet_peer_disconnect`

Il client locale riceve il token da `LocalServer::GetSessionToken()` e lo inserisce nel `PktConnectRequest`.

---

## 3. Struttura dei file

```
src/
 ├── common/          ← zero dipendenze esterne (solo STL)
 │   ├── Physics.h        costanti compile-time per la simulazione: TILE_SIZE, GRAVITY,
 │   │                    FIXED_DT, e tutte le costanti di movimento (velocità, forze di
 │   │                    salto, inerzia, dash, ecc.). I valori esatti sono da bilanciare
 │   │                    durante i playtest — questo file è il posto dove cambiarli.
 │   ├── InputFrame.h     struct InputFrame { uint32_t tick; uint8_t buttons; }
 │   │                    enum InputButtons: BTN_LEFT, BTN_RIGHT, BTN_JUMP, BTN_DASH
 │   │                    helpers: IsLeft(), IsRight(), IsJump(), IsDash()
 │   ├── PlayerState.h    POD serializzabile: x, y, vel_x, vel_y, on_ground,
 │   │                    on_wall_left, on_wall_right, last_dir (int8_t),
 │   │                    jump_buffer_ticks, coyote_ticks (uint8_t),
 │   │                    last_wall_jump_dir (int8_t: -1/0/1 — blocca doppio rimbalzo),
 │   │                    dash_active_ticks, dash_cooldown_ticks (uint8_t),
 │   │                    player_id (uint32_t), last_processed_tick (uint32_t)
 │   ├── GameState.h      MAX_PLAYERS=8; struct GameState { server_tick, player_count,
 │   │                    PlayerState players[8] }; FindPlayer(id) -> PlayerState*
 │   ├── Player.h/.cpp    classe Player: Simulate(InputFrame, World), SetState, GetState
 │   │                    `Simulate` è deterministica e identica su client e server.
 │   │                    L'implementazione interna (ordine delle operazioni, risoluzione
 │   │                    collisioni, logica di salto/dash) va definita e testata con cura.
 │   ├── World.h/.cpp     LoadFromFile(path): '0'=solido, 'E'=endpoint (non solido), ' '=aria, 'X'=spawn
 │   │                    IsSolid(tx,ty), GetWidth(), GetHeight()
 │   └── Protocol.h       pacchetti #pragma pack(1): PktConnectRequest (+ session_token),
 │                         PktPlayerAssigned, PktInput (InputFrame), PktGameState (GameState),
 │                         PktPlayerJoined, PktPlayerLeft, PktAudioEvent
 │
 ├── server/
 │   ├── GameServer.h/.cpp autorità 60Hz; Init(port, max_clients, map, local_only=false),
 │   │                      SetSessionToken(uint32_t), Run(), Stop() (atomic<bool>)
 │   │                      pending_auth_ (unordered_set<ENetPeer*>) per handshake token
 │   │                      players_ (map<id,Player>), peer_to_id_, pending_inputs_
 │   └── main.cpp           enet_initialize → GameServer::Init → Run → deinitialize
 │
 └── client/
     ├── main.cpp           InitWindow(1280,720) → LoadFont → loop → UnloadFont → CloseWindow
     │                      (futuro: enet_init → MainMenu → LocalServer → GameClient → cleanup)
     ├── GameClient.h/.cpp  Init(host,port,map,token), Run(); prediction_buffer_ (deque 128),
     │                      prev_world_state_ per interpolazione remoti, prev_x_/prev_y_
     ├── InputHandler.h/.cpp Poll() dopo Render, Sample(tick) nel Tick fisso
     │                       jump_pending_, dash_pending_ (bool sticky)
     ├── Renderer.h/.cpp     Init(w,h,title) skip se finestra già aperta,
     │                       Draw(GameState, World, local_id, cam_x, cam_y)
     │                       Camera2D zoom=2, palette neon (sfondo {5,5,15})
     ├── AudioManager.h/.cpp Init(assets_dir), PlayEvent(PktAudioEvent), Update()
     │                       event-based: il server decide QUANDO, il client decide COME
     ├── MainMenu.h/.cpp     Show() → MenuResult { MenuChoice, server_ip, port }
     │                       due schermate: Main (Offline/Online) e OnlineInput (campo IP)
     └── LocalServer.h/.cpp  Start(port,map) genera token, GameServer in std::thread,
                              bind 127.0.0.1, sleep 150ms; Stop() = server.Stop+join
                              GetSessionToken() → uint32_t
```

### Asset
```
assets/
 ├── fonts/    font Roboto (Thin, Light, Medium, Bold) in formato .ttf
 └── levels/   mappe .txt, 40 colonne × N righe
               '0'=muro, 'E'=endpoint (non solido — il player ci passa attraverso), ' '=aria, 'X'=spawn
```
Gli asset vengono copiati in `build/<config>/bin/assets/` ad ogni build tramite `add_custom_target(copy_assets ALL ...)` nel CMakeLists.txt radice.

---

## 4. Flusso di rete (schema ad alto livello)

> ⚠️ Questo schema descrive l'intenzione generale, non un'implementazione validata. I dettagli vanno rivisti quando si costruisce la fase di rete.

```
CLIENT                                   SERVER
─────────────────────────────────────────────────────────────
Sampling input al tick N
Player::Simulate locale (prediction)
SendInput(PktInput)  ──────────────────────────────────────→
                                           Raccoglie input per ogni client
                                           Simula il mondo in modo autoritativo
                                           BroadcastGameState()
                        ←────────────────── PktGameState (stato autoritativo)
Reconciliazione con lo stato ricevuto
(dettagli di implementazione TBD)
```

---

## 5. Aspetti ancora da costruire (TODO)

- [ ] **Fisica del movimento**: tuning completo di velocità, inerzia (se presente), salto, wall jump e dash — i valori del POC non erano soddisfacenti, tutto da ribilanciare durante i playtest
- [x] **Jump buffer**: preme SPAZIO in aria → memorizza il salto per `JUMP_BUFFER_TICKS` tick; eseguito appena atterra o tocca un muro (vale anche per wall jump)
- [x] **Coyote time**: permette di saltare per `COYOTE_TICKS` tick dopo aver camminato fuori da un bordo; richiede `coyote_ticks` in `PlayerState`
- [x] **Salto variabile**: rilasciare SPAZIO durante la salita taglia `vel_y` di `JUMP_CUT_MULTIPLIER`
- [x] **Wall jump con divieto stesso lato**: `last_wall_jump_dir` in `PlayerState`; resettato a terra; impedisce doppio rimbalzo sullo stesso muro senza toccare il pavimento
- [ ] **Client prediction + server reconciliation**: riscrittura da zero — l'implementazione precedente aveva bug strutturali; va progettata con attenzione durante la fase 5
- [ ] Logica di vittoria: raggiungere il tile `'E'` termina la partita
- [ ] Schermata fine partita con classifica e tempo
- [ ] Spawn multipli (`'X'`) assegnati per indice player_id
- [ ] Audio effetti: jump, land, dash, victory (file .wav in assets/sounds/)
- [ ] Musica di sottofondo (gestita autonomamente dal client, non event-based)
- [ ] Più livelli / selezione livello nel menu
- [ ] Parametri di connessione configurabili da linea di comando per il server standalone
- [ ] HUD in-game (nome player, posizione in gara, timer)
- [ ] Build e test su Linux e macOS (verificare CMakeLists.txt senza blocchi WIN32)

---

## 6. Scaletta di sviluppo (passo per passo)

Ecco la scaletta. Ogni passo produce qualcosa che **gira e puoi testare** prima di andare avanti.

> **Stato attuale:** completati i passi 1–16 e 17, 19 e 20 (finestra + tilemap + World + Player + gravità + collisioni + jump buffer/coyote/salto variabile/wall jump + fixed timestep + camera + dash + InputFrame/Simulate + sticky flags + server ENet + `common` lib + client→server InputFrame + server→client GameState broadcast + **GameState multi-player + prediction + reconciliation** + **menu iniziale con modalità Offline/Online + nome player + nome sopra ogni player + LocalServer in thread**).



## Fase 1 — Fondamenta (niente rete, niente fisica)

**1. Finestra vuota**
Progetto CMake con Raylib. `InitWindow`, loop con `BeginDrawing/EndDrawing`, `CloseWindow`.

**2. Tilemap statica**
Carica un file `.txt` dove `0` = muro e spazio = aria. Disegna rettangoli colorati per ogni tile solido. Niente classi, tutto in `main`.

**3. Separazione World**
Estrai una classe `World` con `LoadFromFile()`, `IsSolid(tx, ty)`, `GetWidth/Height`. Rifai il render usando quella.

---

## Fase 2 — Fisica locale (niente rete)

**4. Player che si muove**
Un rettangolo `32×32`. Frecce = sposta la posizione direttamente (senza velocità). Dividi subito in `PlayerState` (dati POD) e `Player` (logica).

**5. Gravità e salto**
Aggiungi `vel_x`, `vel_y` a `PlayerState`. Ogni frame: `vel_y += GRAVITY * dt`, `y += vel_y * dt`. Spazio = `vel_y = -JUMP_FORCE` se `on_ground`.

**6. Collisioni separate X/Y**
Prima muovi X e risolvi solo X (snap al bordo tile). Poi muovi Y e risolvi solo Y (setta `on_ground`). Questo è il passo più delicato — testalo a lungo.
Dopo che le collisioni funzionano, aggiungere nell'ordine:
- **Jump buffer** (`jump_buffer_ticks` in `PlayerState`, costante `JUMP_BUFFER_TICKS`)
- **Coyote time** (`coyote_ticks` in `PlayerState`, costante `COYOTE_TICKS`)
- **Salto variabile** (costante `JUMP_CUT_MULTIPLIER`, nessun campo extra in `PlayerState`)
- **Wall jump buffer** (riutilizza `jump_buffer_ticks` — scatta se `on_wall_left/right`)

Vedi la sezione *Meccaniche del salto avanzate* in §2 per i dettagli implementativi.

**7. Fixed timestep**
Sostituisci il `dt` variabile con un accumulator a 60Hz. Il rendering gira a FPS liberi. Verifica che la fisica sia identica a 30fps e 144fps.

**8. Camera** ✓
`Camera2D` di Raylib che segue il player. Interpolazione visiva: alla fine del tick loop si calcola `alpha = accumulator / FIXED_DT` e si lerpa la posizione tra il tick precedente (`prev_x/prev_y`) e quello corrente (`state.x/state.y`). Il rendering world-space (tilemap, trail, player) è wrappato in `BeginMode2D/EndMode2D`; HUD e debug overlay restano in screen-space. Camera offset = centro schermo, zoom = 1 (40 tile × 32 px = 1280 px, esattamente la larghezza finestra).

---

## Fase 3 — Input robusto

**9. InputFrame** ✓
Crea `struct InputFrame { uint32_t tick; uint8_t buttons; float dash_dx; float dash_dy; }` con bit flag (BTN_LEFT, BTN_RIGHT, BTN_JUMP, BTN_JUMP_PRESS, BTN_DASH, BTN_UP, BTN_DOWN). `Player::Simulate(const InputFrame&, const World&)` è l'unico punto dove si aggiorna lo stato: gestisce edge rising (BTN_JUMP_PRESS → jump buffer, BTN_DASH → RequestDash), salto variabile (rilascio BTN_JUMP → CutJump), sterzata dash e movimento orizzontale. Il campo `prev_jump_held_` vive in `Player` (non in `PlayerState`). `main` costruisce l'InputFrame da Raylib ogni tick fisso e chiama `Simulate`.

**10. Sticky flag per il salto** ✓
`jump_pressed` bool impostato da `IsKeyPressed` prima del loop di render; consumato all'interno del tick fisso come `BTN_JUMP_PRESS` nell'`InputFrame`. Identico per `dash_pending`. Garantisce che l'input non vada mai perso anche a frame rate molto alti.

---

## Fase 4 — Introduzione alla rete (senza prediction)

**11. Server headless minimo** ✓
Nuovo eseguibile `TileRace_Server` (target CMake separato in `src/server/`). ENet in ascolto su UDP porta 7777 (costante `SERVER_PORT` in `Protocol.h`). Riceve pacchetti di testo `"ping"`, risponde `"pong"`. `Protocol.h` in `src/common/` contiene `SERVER_PORT`, `MAX_CLIENTS`, `CHANNEL_COUNT`, `MSG_PING/PONG`. Nessuna logica di gioco ancora.

**12. Struttura `common`** ✓
`common_logic` è già una static lib compilata separatamente da `src/common/CMakeLists.txt` e linkata da entrambi `TileRace_Client` e `TileRace_Server`. Nessuna modifica necessaria: `World`, `Player`, `PlayerState`, `InputFrame`, `Physics`, `Protocol` sono tutti in `common` senza dipendenze da Raylib o ENet.

**13. Input → server → stato → client** ✓
Il client invia `PktInput` (ENet unreliable sequenced, canale 0) al server ogni tick fisso. Il server chiama `Player::Simulate(pkt.frame, world)` e rimanda `PktPlayerState`. Il client riceve lo stato e lo usa per il rendering con interpolazione visiva (`prev_x/prev_y` + `alpha`). `Protocol.h` definisce `PktInput`, `PktPlayerState`, `PktType`. `TileRace_Client` linka `enet` + `ws2_32`. **Latenza visibile ma funzionante.**

**14. Più giocatori** ✓
`GameState.h` in `common/`: `struct GameState { uint32_t count; PlayerState players[MAX_PLAYERS]; }`. `PlayerState` ha ora `last_processed_tick` (usato per la reconciliation). Il server assegna player_id progressivi, invia `PktWelcome` alla connessione, poi dopo ogni `Simulate` fa `enet_host_broadcast` di `PktGameState` a tutti i peer. Il client renderizza gli altri giocatori (arancio) con posizione autoritativa dal server; il proprio con prediction + interpolazione.

---

## Fase 5 — Client-Side Prediction

**15. Prediction** ✓
Il client chiama `Player::Simulate` localmente con lo stesso `InputFrame` inviato al server, prima di ricevere la risposta. Il rendering usa lo stato predetto localmente: nessun lag visivo anche con 100ms di latenza. Ring buffer `input_history[128]` indicizzato per `tick % 128` archivia ogni frame inviato.

**16. Reconciliation** ✓
Quando arriva `PktGameState`, il client cerca il proprio `PlayerState` (per `player_id`), usa `last_processed_tick` per sapere fino a dove il server è autorevole, resetta il player locale a quello stato e ri-simula tutti gli input in `input_history` da `last_processed_tick+1` fino al `sim_tick` corrente. Il rendering rimane fluido perché usa sempre lo stato post-reconciliation.

---

## Fase 6 — Mosse avanzate

**17. Dash** ✓
`BTN_DASH`, sticky flag su Shift, logica in `Simulate`. Implementato: `RequestDash()`, gravity suspend in `MoveY`, `last_dir` tracking, cooldown. Vedi §2 *Dash*.

**18. Wall jump**
Rilevazione `on_wall_left/right` in `ResolveCollisionsX`. Salto via dal muro usando i flag del tick precedente.

---

## Fase 7 — Robustezza e UX

**19. Menu iniziale** ✔
Schermata Raylib con due pulsanti: Offline / Online. Online = campo testo per IP.
Implementato in `MainMenu.h/.cpp`: schermata 1 (TILE RACE, campo nome, pulsanti OFFLINE/ONLINE),
schermata 2 (ONLINE, campo IP, pulsanti CONNETTI/INDIETRO). Restituisce `MenuResult { choice, server_ip, username, port }`.
Nome player salvato in `PlayerState::name[16]` e visibile sopra ogni player (DrawPlayer world-space).

**20. Modalità offline** ✔
`LocalServer`: avvia `GameServer` in un `std::thread` background. Client si connette a `127.0.0.1` come se fosse online.
Implementato: `ServerLogic.h/.cpp` contiene `RunServer()` condiviso tra `TileRace_Server` e `LocalServer`.
`server_logic` static lib linkato sia dal server standalone che dal client.

**21. Token di sessione**
`LocalServer` genera un uint32 casuale. Server lo controlla nel `PktConnectRequest`. Client locali non autorizzati vengono disconnessi.

---

## Ordine delle cose da imparare

```
Raylib base → tilemaps → fisica 2D → fixed timestep
→ ENet base → architettura client/server → prediction → reconciliation
```

I passi **6** (collisioni), **7** (fixed timestep) e **16** (reconciliation) sono i tre punti dove la maggior parte del tempo andrà spesa. La fisica in generale (passi 5-8) è da trattare come un lavoro iterativo: non basta che "funzioni", deve anche *sentirsi bene* — e questo richiede playtest e aggiustamenti continui dei valori.