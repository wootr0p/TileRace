# Modalità di gioco

TileRace offre due modalità: **Co-op** e **Race**.

---

## Co-op

I giocatori (da 2 a 8) collaborano come squadra per completare ogni livello. L'obiettivo è raggiungere la casella di uscita: basta che **un solo giocatore** la tocchi per far avanzare l'intera squadra al livello successivo.

### Azioni disponibili

- **Muoversi** a sinistra e a destra.
- **Saltare** (anche lungo le pareti verticali grazie al wall-jump). Tenere premuto il tasto di salto allunga il salto.
- **Scattare** (dash) in qualsiasi direzione per attraversare rapidamente lo spazio.
- **Scattare** sui compagni per spostarli con una spinta.
- **Correre** (sprint) per raddoppiare la velocità orizzontale.
- **Afferrare** un compagno vicino con la calamita e trasportarlo.
- **Tornare all'ultimo checkpoint** salvato, oppure ripartire dal punto di partenza.
- **Lasciare tracce colorate** sulla mappa come segnali per i compagni.

### Checkpoint

Disseminati nel livello, i checkpoint si attivano al passaggio e aggiornano il punto di respawn di **tutti i giocatori contemporaneamente**. Muorendo su una trappola ci si respawna all'ultimo checkpoint raggiunto.

### Fine livello

Non appena almeno un giocatore tocca l'uscita, il livello è completato per tutti. Se il tempo a disposizione (2 minuti) scade senza che nessuno raggiunga l'uscita, il livello viene perso.

### Risultati di sessione

Al termine di tutti i livelli, viene mostrato quanti livelli la squadra è riuscita a completare.

---

## Race

Ogni giocatore corre in modo indipendente per raggiungere l'uscita il più velocemente possibile. Non c'è cooperazione: l'obiettivo è arrivare primi.

### Azioni disponibili

- **Muoversi** a sinistra e a destra.
- **Saltare** (con wall-jump e salto variabile in altezza).
- **Scattare** in qualsiasi direzione.
- **Correre** (sprint).
- **Tornare al punto di partenza** in qualsiasi momento.
- **Lasciare tracce colorate** sulla mappa.

### Differenze rispetto al Co-op

- I giocatori non si collidono e non possono trascinarsi a vicenda.
- Non esistono checkpoint nei livelli race.
- Ogni giocatore termina in proprio: il primo che tocca l'uscita ottiene il posto più alto in classifica, ma gli altri continuano a giocare finché non finiscono o scade il tempo.

### Fine livello

Il livello si chiude solo quando tutti i giocatori hanno completato il percorso (o il tempo è scaduto). I tempi individuali vengono confrontati per stilare la classifica.

### Risultati di sessione

Al termine di tutti i livelli, la classifica mostra il numero di vittorie (primi posti) ottenute da ciascun giocatore.

---

## Validazione dei livelli

Prima di ogni partita, il server verifica automaticamente che il livello generato sia risolvibile: simula il percorso con le stesse regole fisiche del gioco (salti, dash, wall-jump) e controlla che esista almeno un cammino praticabile dal punto di partenza all'uscita. Solo i livelli superabili vengono messi in gioco.
