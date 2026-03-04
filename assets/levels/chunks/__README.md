# Chunk authoring (Tiled)

Questa cartella contiene i chunk `.tmj` usati dal generatore livelli.

## Dove mettere i file
- Metti qui i chunk: `assets/levels/chunks/*.tmj`
- Puoi usare sottocartelle (`easy/`, `medium/`, `hard/`) se vuoi.

## Proprietà mappa da impostare in Tiled
Apri **Map Properties** del chunk e aggiungi:

- `difficulty` (int, 1..10)
- `chunk_role` (string: `start`, `mid`, `end`, oppure `any`)
- `weight` (int opzionale, default 1, più alto = più probabile)

## Entry / Exit tra chunk
Usa tile con proprietà `type` nel `TileSet.tsx`:

- `chunk_entry`  (char interno `I`)
- `chunk_exit`   (char interno `O`)

Regole pratiche:
- chunk `start`: 1 `chunk_exit`, `chunk_entry` opzionale
- chunk `mid`:   1 `chunk_entry` + 1 `chunk_exit`
- chunk `end`:   1 `chunk_entry`, `chunk_exit` opzionale

Il loader sovrappone `entry(N+1)` su `exit(N)` e poi rimuove entrambi.

## Diramazioni (fork / bivi)

Il generatore supporta percorsi che si dividono e si riuniscono.
Il server rileva automaticamente i chunk con più entry o più exit all'avvio:

- **Fork-start** (inizio bivio): chunk con **più di 1 `chunk_exit`** (`O`).
  Ogni exit diventa l'inizio di un ramo parallelo.
- **Fork-end** (fine bivio): chunk con **più di 1 `chunk_entry`** (`I`).
  Ogni entry riceve un ramo che si riunisce.

### Come creare un chunk fork-start
1. Disegna il chunk con 1 entry (`I`) e 2+ exit (`O`).
2. Posiziona le exit distanziate orizzontalmente (almeno ~10 tile di distanza
   consigliata) per dare spazio ai rami paralleli.
3. Imposta `chunk_role` a `mid` (o `any`).

### Come creare un chunk fork-end
1. Disegna il chunk con 2+ entry (`I`) e 1 exit (`O`).
2. Le entry devono corrispondere ai rami in arrivo.
3. Imposta `chunk_role` a `mid` (o `any`).

### Regole di branching basate sulla difficoltà
- **Livello facile** (t ≈ 0): fino a 5 strade in contemporanea, fino a 4 arrivi.
- **Livello difficile** (t ≈ 1): massimo 2 strade in contemporanea, 1-2 arrivi.
- Tra percorsi paralleli vengono garantiti almeno **2 tile** di distanza.
- Se non esistono chunk fork, il percorso resta lineare (compatibilità totale).

## Note
- Tile `spawn` (`X`) e `end` (`E`) li gestisci tu nei chunk `start` / `end`.
- Il loader NON inserisce spawn/end automaticamente.
- Il loader aggiunge automaticamente una cornice solida esterna (5 tile) alla composizione finale per evitare cadute nel vuoto ai bordi globali.
- In modalità lineare il generatore usa sempre: `1 start` + `N mid` + `1 end`.
- In modalità branching: `1 start` + `mid pre-fork` + `fork_start` + `N rami di mid` + `fork_end o end per ramo`.
- `N` è casuale e varia con la difficoltà del livello: da ~4 (facile) fino a ~14 (difficile).
- I chunk `mid` sono pescati solo da chunk che NON sono `start` né `end`.
- Mantieni tutti i chunk con stessa tile size (32) e stesso tileset.
