# Confronto pilota: dieci partite da 3 e da 10 minuti

Il batch da 10 minuti comprende le dieci directory da
`match_20260922_180935` a `match_20260922_195210` in
`out/build-vs2026-x86/output/datasets/soccerreplay1988_10min_pilot`.
Tutte le partite sono complete e l'audit automatico non rileva anomalie nei
timestamp o conflitti fra gli eventi controllati.

Il confronto usa il secondo batch da 3 minuti, generato dopo le modifiche al
comportamento del simulatore. In entrambi i casi seed e squadre sono casuali:
si tratta quindi di un confronto fra gruppi, non di dieci coppie identiche.
Non sono stati esportati fotogrammi; le clip sono candidate ricavate dai
timestamp e non esempi già validati visivamente.

## Risultato principale

| Metrica | 10 partite da 3 min | 10 partite da 10 min | Rapporto 10/3 |
| --- | ---: | ---: | ---: |
| Durata reale totale, min | 38,36 | 112,93 | 2,94x |
| Annotazioni grezze | 567 | 1.418 | 2,50x |
| Eventi potenzialmente utili | 191 | 392 | 2,05x |
| Eventi utili per partita | 19,1 | 39,2 | 2,05x |
| Eventi utili per minuto reale | 4,98 | 3,47 | 0,70x |
| Finestre valide da 30 s | 135 | 346 | 2,56x |
| Finestre non sovrapposte | 34 | 107 | 3,15x |
| Finestre non sovrapposte per partita | 3,4 | 10,7 | 3,15x |
| Finestre non sovrapposte per minuto reale | 0,89 | 0,95 | 1,07x |
| Efficienza temporale delle finestre valide | 32,7% | 41,4% | +8,7 punti |
| Copertura del video con l'unione delle finestre | 57,5% | 63,5% | +6,0 punti |
| Label canoniche osservate nel batch | 18/24 | 18/24 | uguale |
| Classi valide medie per partita | 5,8 | 9,9 | 1,71x |

Le partite da 10 minuti producono molte più clip distinte per partita. Il
vantaggio normalizzato per minuto di video è però contenuto: circa il 7% in
più nel limite massimo di finestre non sovrapposte. Il beneficio principale è
quindi avere meno partite, finestre meno concentrate e più classi presenti
nella stessa partita, non una crescita proporzionale degli eventi.

I 346 eventi validi generano 10.380 secondi nominali di finestre, ma la loro
unione misura 4.300,16 secondi. La quota duplicata è quindi 58,6%, contro il
67,3% nelle partite da 3 minuti. La compressione minore riduce la ridondanza,
ma non la elimina.

## Distribuzione delle classi

| Label | 3 min: eventi | 10 min: eventi | 3 min: eventi/min | 10 min: eventi/min |
| --- | ---: | ---: | ---: | ---: |
| clearance | 33 | 100 | 0,86 | 0,89 |
| saved by goal-keeper | 37 | 87 | 0,96 | 0,77 |
| ball out of play | 29 | 44 | 0,76 | 0,39 |
| goal | 12 | 31 | 0,31 | 0,27 |
| free kick | 5 | 21 | 0,13 | 0,19 |
| corner | 19 | 20 | 0,50 | 0,18 |
| lead to corner | 18 | 16 | 0,47 | 0,14 |
| throw in | 5 | 15 | 0,13 | 0,13 |
| off-side | 3 | 10 | 0,08 | 0,09 |
| foul (no card) | 1 | 9 | 0,03 | 0,08 |
| yellow card | 4 | 9 | 0,10 | 0,08 |
| penalty | 3 | 7 | 0,08 | 0,06 |
| shot off target | 2 | 3 | 0,05 | 0,03 |
| substitution | 20 | 20 | 0,52 | 0,18 |

`ball possession` passa da 326 a 976 eventi, ma la frequenza resta simile:
8,50 contro 8,64 al minuto reale. Questo conteggio segnala cambi di possesso;
non misura la durata o la sterilità del possesso.

Tutti i 16 eventi `lead to corner` sono seguiti da un `corner` dopo 4,41–4,60
secondi. Tutti i 10 `off-side` sono seguiti da un `free kick` dopo 4,52–4,57
secondi. Causa e ripresa descrivono quindi la stessa sequenza e non devono
essere trattate come esempi video indipendenti.

Le sei label mai osservate sono ancora `injury`, `own goal`, `penalty missed`,
`red card`, `second yellow card` e `var`. Dieci minuti aumentano i conteggi
delle classi presenti, ma in questo campione non ampliano la copertura totale
delle label rispetto alle partite da 3 minuti.

## Variabilità

Nelle partite da 10 minuti gli eventi utili vanno da 24 a 55, con deviazione
standard 9,62. Le finestre non sovrapposte vanno da 8 a 13, con deviazione
standard 1,77 e media 10,7. Il rendimento per partita è più stabile rispetto
al secondo batch da 3 minuti, nel quale le finestre andavano da 1 a 7.

## Dimensionamento preliminare

La tabella usa 10,7 finestre non sovrapposte per partita da 10 minuti e 3,4
per partita da 3 minuti. La durata grezza usa i tempi effettivamente osservati,
comprese le interruzioni. Non corregge ancora per validità visiva, equilibrio
delle classi o diversità semantica.

| Target | Partite da 3 min | Ore grezze 3 min | Partite da 10 min | Ore grezze 10 min |
| --- | ---: | ---: | ---: | ---: |
| 1.000 clip | 295 | 18,9 | 94 | 17,7 |
| 2.500 clip | 736 | 47,1 | 234 | 44,0 |
| 5.000 clip | 1.471 | 94,1 | 468 | 88,1 |
| 10.000 clip | 2.942 | 188,1 | 935 | 176,0 |
| 25.000 clip | 7.353 | 470,1 | 2.337 | 439,9 |
| 50.000 clip | 14.706 | 940,3 | 4.673 | 879,6 |
| 100.000 clip | 29.412 | 1.880,5 | 9.346 | 1.759,2 |

## Interpretazione per la tesi

Fra le due durate misurate, 10 minuti è la scelta migliore se il criterio è
ottenere finestre da 30 secondi temporalmente indipendenti: produce circa il
7% in più per minuto grezzo e riduce la quota di sovrapposizione di 8,7 punti.
Riduce inoltre di circa tre volte il numero di partite da gestire per uno
stesso target di clip.

Non è migliore secondo ogni criterio. La densità complessiva di eventi utili
scende del 30%, la copertura delle 24 label non aumenta e varie azioni
(`corner`, `lead to corner`, `ball out of play`, parate e gol) sono meno
frequenti per minuto reale. Ne segue che inviare partite complete a Cosmos
sarebbe inefficiente. Conviene selezionare prima le finestre, unire gli eventi
correlati della stessa azione e assegnare priorità alle classi rare.

Questo confronto consente una conclusione provvisoria fra 3 e 10 minuti, ma
non stabilisce ancora l'ottimo fra 2, 3, 5 e 10 minuti e non misura la reale
diversità visiva all'interno della stessa label.
