# ENet Test Guide

## Files Creati

1. **NetworkManager.h/cpp** - Classe wrapper per ENet
   - `StartServer()` - Avvia un server
   - `ConnectToServer()` - Connetti a un server
   - `SendPacket()` - Invia dati
   - `Update()` - Processa eventi di rete

2. **ENetTest.cpp** - Test di verifica per ENet

## Come Testare ENet

### Opzione 1: Eseguire il test standalone
```
1. Nel file ENetTest.cpp, decommentare:
   int main() { TestBasicENet(); return 0; }

2. Creare un nuovo progetto console o usare questo per il test
3. Includere enet.lib nel linker
4. Compilare ed eseguire
```

### Opzione 2: Integrazione nel progetto principale
```cpp
#include "ENetTest.cpp"

// Nel file Game.cpp o test principale:
TestBasicENet();
```

## Cosa Verifica il Test

✓ Inizializzazione di ENet
✓ Creazione di un server host
✓ Creazione di un client host
✓ Connessione tra client e server
✓ Invio/ricezione pacchetti
✓ Cleanup risorse

## Output Atteso

```
=== ENet Basic Test ===

[Test 1] Initializing ENet...
SUCCESS: ENet initialized

[Test 2] Creating server host...
SUCCESS: Server created on port 5000

[Test 3] Creating client host...
SUCCESS: Client created

[Test 4] Client connecting to server...
SUCCESS: Connection initiated

[Test 5] Simulating network communication...
   [Server] Client connected!
   [Client] Connected to server!
   [Client] Sending test message...
   [Server] Received: Hello from ENet Client!
SUCCESS: Communication test passed!

[Test 6] Cleaning up...
SUCCESS: Cleanup completed

=== ENet Test Completed ===
```

## Prossimi Passi

Una volta confermato che ENet funziona:

1. Integrare `NetworkManager` nella classe `Game`
2. Implementare logica server/client per il multiplayer
3. Sincronizzare il movimento del player tramite rete
