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

## Note
- Tile `spawn` (`X`) e `end` (`E`) li gestisci tu nei chunk `start` / `end`.
- Il loader NON inserisce spawn/end automaticamente.
- Il loader aggiunge automaticamente una cornice solida esterna (1 tile) alla composizione finale per evitare cadute nel vuoto ai bordi globali.
- Il generatore usa sempre: `1 start` + `N mid` + `1 end`.
- `N` è casuale e varia con la difficoltà del livello: da ~5 (facile) fino a ~20 (difficile).
- I chunk `mid` sono pescati solo da chunk che NON sono `start` né `end`.
- Mantieni tutti i chunk con stessa tile size (32) e stesso tileset.
