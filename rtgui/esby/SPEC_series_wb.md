# Balance des blancs par série : spécification

Statut : étapes 1 (`ttserieswb.*`), 2 (`server/esbywb.py`) et 3 (`esbywbclient.*`) implémentées.

## 1. Objectif

Corriger automatiquement et de façon cohérente la balance des blancs des photos prises au
flash, série par série, sans avoir à régler chaque dossier un par un.

Constat de départ (photo P2655548, GH5, 1/60 s, f/2.8, 200 ISO, flash, balance du boîtier en
automatique) : la balance « Camera » (5 044 K) est corrigée à la main vers 4 679 K, soit un
décalage d'environ +15 mireds. La lumière ambiante, chaude, éclaire aussi le sujet à ces
réglages ; le décalage dépend donc du lieu et des réglages, d'où une valeur par série.

Le système comporte trois parties indépendantes :

- un **serveur** local qui garde les valeurs par dossier ;
- un **outil `tt*`** du fork (`TTSeriesWB`) qui les applique dans RawTherapee ;
- un **outil en ligne de commande** (`esbywb`) pour gérer les valeurs en masse.

## 2. Vocabulaire

- **Règle** : une valeur déclarée pour un dossier.
- **Valeur résolue** : la valeur qui s'applique à un dossier, obtenue par héritage.
- **Source** : le dossier dont vient la valeur résolue.
- **Décalage** : l'écart à appliquer à la balance du boîtier.
- **Mired** : unité de température de couleur, `mired = 1 000 000 / kelvins`. Un même écart en
  mireds produit le même effet visuel quelle que soit la température de départ, ce qui n'est
  pas le cas d'un écart en kelvins.

## 3. Règles et héritage

Une règle associe à un dossier :

| Champ | Type | Défaut | Rôle |
|---|---|---|---|
| `mired` | réel | 0 | décalage de température en mireds |
| `green` | réel | 1.0 | facteur appliqué à la teinte (`Green` de RawTherapee) |
| `equal` | réel | 1.0 | facteur appliqué à l'égaliseur bleu/rouge (`Equal`, 1.0 pour le boîtier) |
| `flash_only` | booléen | true | n'appliquer qu'aux photos dont le flash s'est déclenché |
| `comment` | texte | vide | note libre (lieu, éclairage…) |

**Résolution** : pour un dossier, on remonte l'arborescence jusqu'au premier dossier qui a une
règle. Si aucun n'en a, on prend la règle globale (`mired = 0`, donc aucune correction).

```
~/photos/                              règle globale : 0
└── Lucca 2026/                        règle : +15
    ├── Vendredi - Palazzo Pfanner/    résolu : +15 (source : Lucca 2026)
    ├── Samedi - Hall 3/               règle : +22
    │   └── Groupe Genshin/            résolu : +22 (source : Samedi - Hall 3)
    └── Dimanche - extérieur/          règle : 0
```

**Chemins** : ils sont normalisés avant usage (chemin absolu, liens symboliques résolus,
sans `/` final). Une règle reste attachée à un chemin : si un dossier est déplacé ou renommé,
sa règle devient orpheline (voir la commande `move` de `esbywb`).

**Sens du décalage** : `mired = mired(cible) − mired(boîtier)`. Un décalage positif baisse la
température réglée dans RawTherapee, donc refroidit le rendu.
Exemple : 5 044 K (198,3 mireds) + 15,4 mireds = 213,7 mireds, soit 4 679 K.

## 4. Calcul de la balance cible

À partir de la balance du boîtier (méthode « Camera » de RawTherapee : température `T0`,
teinte `G0`) :

```
T = 1 000 000 / (1 000 000 / T0 + mired)
G = G0 × green
```

`T` et `G` sont ramenés dans les limites de l'outil White Balance de RawTherapee. La balance
de l'image passe en méthode « Custom » avec ces valeurs.

Le calcul part **toujours** de la balance du boîtier, jamais de la valeur courante : appliquer
deux fois la même règle donne le même résultat.

## 5. Suivi de série et retouches manuelles

Une image **suit la série** tant que sa balance est celle que l'outil lui a appliquée. Dès que
la balance est modifiée à la main, l'image a une **balance manuelle**, et l'outil n'y touche
plus.

- Le serveur mémorise, pour chaque fichier traité, la balance appliquée (`T`, `G`) et la règle
  utilisée. RawTherapee réécrit le `.pp3` sans conserver les clés qu'il ne connaît pas : cette
  information ne peut donc pas être stockée dans le `.pp3`.
- À l'ouverture d'une image :
  - **jamais traitée, sans `.pp3`** ou avec la méthode « Camera » : l'outil applique la valeur
    résolue (si la condition flash est remplie) ;
  - **jamais traitée, avec une autre méthode** dans son `.pp3` : balance manuelle, on n'y touche pas ;
  - **déjà traitée** : si la balance du `.pp3` correspond à la dernière balance appliquée
    (à une tolérance près), l'image suit la série ; sinon, elle est manuelle.
- L'outil propose « rattacher à la série » pour revenir à la valeur de la série, et « détacher »
  pour la conserver telle quelle.

## 6. Le serveur

- Processus local sur moria, lancé à la demande ou par l'utilisateur (service systemd
  utilisateur possible). Langage : Rust ou Python.
- Socket Unix : `$XDG_RUNTIME_DIR/esby-wb.sock`. Rien n'est exposé sur le réseau.
- Stockage : `~/.local/share/esby-wb/state.json` (règles et fichiers traités), écrit de façon
  atomique (fichier temporaire, puis renommage).
- Plusieurs clients simultanés (plusieurs instances de RawTherapee, `esbywb`).

## 7. Protocole

Le serveur refuse les valeurs hors des plages de l'outil : décalage de −100 à +100 mireds, facteurs
de teinte et d'égaliseur de 0,5 à 2,0.

Messages JSON, un par ligne, sur la socket. Chaque requête porte un identifiant `id`, repris
dans la réponse. Les événements du serveur n'ont pas d'`id`.

| Requête | Paramètres | Réponse |
|---|---|---|
| `hello` | `version` | version du serveur |
| `get` | `path` | valeur résolue, `source` |
| `set` | `path`, `mired`, `green`, `flash_only`, `comment` | ok |
| `unset` | `path` | ok (le dossier revient à l'héritage) |
| `list` | — | toutes les règles |
| `move` | `from`, `to` | ok (règles et fichiers suivis déplacés) |
| `applied` | `file`, `T`, `G`, `source` | ok (mémorise une application) |
| `file_state` | `file` | dernière balance appliquée, ou rien |
| `subscribe` | — | ok, puis événements |

Événement : `rule_changed` avec `path`. Un client concerné est un client dont l'image ouverte
se trouve dans `path` ou dans un de ses sous-dossiers.

Exemple :

```json
{"id": 1, "op": "get", "path": "/home/keby/photos/Lucca 2026/Samedi - Hall 3"}
{"id": 1, "ok": true, "mired": 22, "green": 1.0, "flash_only": true, "source": "/home/keby/photos/Lucca 2026/Samedi - Hall 3"}
{"event": "rule_changed", "path": "/home/keby/photos/Lucca 2026"}
```

## 8. L'outil `TTSeriesWB` (fork)

**Affichage** : valeur résolue et source (« +15 mireds, hérité de Lucca 2026 »), état de l'image
(suit la série / manuelle / non concernée car sans flash), état du serveur.

**Actions** :

- définir la valeur pour le dossier de l'image ;
- définir la valeur pour la racine de la série : le premier dossier, en remontant depuis celui de
  l'image, qui contient un fichier repère (option `SeriesWBMarkers`, par défaut
  `fields.conf;.esby-series`) ; à défaut, le dossier parent ;
- apprendre de cette image : `mired = mired(balance actuelle) − mired(boîtier)` ; avec le serveur,
  la valeur apprise devient celle de la racine de la série (fichier repère), ou du dossier de l'image
  à défaut ;
- revenir à l'héritage (supprime la règle du dossier) ;
- rattacher l'image à la série / la détacher.

**Comportement** :

- réagit à l'ouverture d'une image (`FakeEvPhotoLoaded`), après la transmission des EXIF ;
- lit `rti:Exif:Flash` (ou `rti:Exif:MakerNote:FlashFired`) pour la condition flash ;
- réapplique la valeur à l'image ouverte quand un `rule_changed` la concerne et qu'elle suit
  la série ;
- toutes les communications sont asynchrones (intégrées à la boucle GLib) : l'interface
  n'attend jamais le serveur ;
- serveur indisponible : l'outil l'indique et n'applique rien (pas de valeur inventée) ;
- toute application passe par les paramètres de traitement normaux : elle apparaît dans
  l'historique, s'annule, et est écrite dans le `.pp3` (nécessaire pour l'export par lots).

## 9. L'outil `esbywb`

```
esbywb list                         # règles, avec les dossiers sources
esbywb get  <dossier>               # valeur résolue et source
esbywb set  <dossier> <mired> [--green G] [--all] [--comment "…"]
esbywb unset <dossier>
esbywb move <ancien> <nouveau>      # après un renommage de dossier
esbywb orphans                      # règles dont le dossier n'existe plus
```

`--all` désactive la condition flash pour cette règle.

## 10. Cas limites

- **Ordre des outils** : TTTweaker peut remettre la balance du boîtier (`resetWBToCamera`) et
  TTIsoProfiler peut appliquer un profil partiel contenant une balance. `TTSeriesWB` doit passer
  après eux, ou ces réglages doivent être exclusifs.
- **Images sans EXIF de flash** (JPEG retouchés, scans) : non concernées si `flash_only`.
- **Batch Editor** : à définir. Proposition : appliquer à chaque image de la sélection selon
  son propre dossier.
- **Même image ouverte dans deux instances** : la dernière application l'emporte, comme pour
  toute modification dans RawTherapee.
- **Disques externes** : le chemin dépend du point de montage ; une règle déclarée sur un disque
  monté ailleurs ne sera pas retrouvée (`move` permet de corriger).
- **Copie locale avant ouverture** (ex : `rt_queue`, qui copie le RAW dans `~/Images/raws`) : le
  lanceur passe le chemin complet du fichier d'origine dans la variable d'environnement
  `ESBY_ORIGIN`. TTSeriesWB l'utilise comme dossier de série (et comme nom de fichier pour le
  serveur) si l'image ouverte porte le même nom de fichier.

## 11. Étapes de réalisation

1. `TTSeriesWB` sans serveur : décalage réglé dans l'outil, application à l'ouverture, suivi de
   série en mémoire. Permet de valider le calcul et le comportement. **Fait.** Le décalage est
   enregistré dans le profil `.ttp` ; le suivi de série ne dure que le temps de la session.
2. Le serveur et le protocole, avec `esbywb`. **Fait** : `server/esbywb.py` (Python 3,
   bibliothèque standard), tests dans `server/tests/`, service systemd dans `server/esbywb.service`.
3. Le branchement de `TTSeriesWB` sur le serveur (lecture, apprentissage, notifications).
   **Fait** : `esbywbclient.*` (GIO, asynchrone, reconnexion toutes les 10 secondes, cJSON).
   Les curseurs de l'outil affichent la valeur du dossier de l'image ; les modifier est un
   aperçu, envoyé au serveur par « définir pour ce dossier » ou « pour le dossier parent ».
   Sans serveur, l'outil applique ses propres valeurs (comportement de l'étape 1).

## 11 bis. Observations (collecte avant une modulation automatique)

Les tests sur le GH5 et le S5 II (octobre 2026) montrent que la puissance du flash et la correction
TTL ne sont enregistrées nulle part ; seul le déclenchement l'est (`Exif:Flash`). En TTL, le S5 II
indique si le flash a servi (`Panasonic_0x8007` = 2) et sa balance automatique tient compte du
préflash. Le GH5 enregistre une mesure de la scène non identifiée (`PanasonicRaw_CameraIFD_0x1300`).

Pour savoir si le décalage d'une série suit l'exposition (`LightValue`, ou `0x1300`), TTSeriesWB
envoie au serveur une observation à chaque Learn et à chaque enregistrement (voir
`server/README.md`). Une modulation automatique ne sera ajoutée que si l'analyse de ces
observations montre une relation nette.

## 11 ter. Mode Auto (octobre 2026)

Mesures à la pipette sur une feuille blanche (S5 II, X3 et AD200 en manuel) :

- l'ouverture et l'ISO ne changent pas la bonne balance (ils agissent autant sur le flash que sur
  l'ambiance), alors que la balance automatique du boîtier dérive ;
- sous la vitesse de synchronisation, la bonne balance se réchauffe proportionnellement au temps de
  pose (l'ambiance s'accumule, l'éclair ne dure qu'un instant) : environ 82 mireds par seconde dans
  la pièce du test ;
- en HSS (dès 1/250 s avec ce matériel), la bonne balance est constante, plus chaude et plus verte.

Le mode Auto de TTSeriesWB en découle : une balance de référence pour l'éclair normal (température,
teinte, égaliseur, vitesse), un coefficient de pose (mireds par seconde) et une balance propre au HSS
au-delà d'un seuil de synchronisation réglable. Le Learn choisit ce qu'il met à jour selon la vitesse
de l'image : la balance HSS, la référence normale, ou le coefficient de pose (seconde référence à une
vitesse nettement différente). Sans valeur pour le régime d'une image, l'outil ne la modifie pas.
Sur les mesures du test, trois Learn suffisent à reproduire les autres photos à environ 1 mired près.

Le mode et le modèle sont stockés par dossier sur le serveur (`mode`, `auto`) et dans les options.
Le mode « Shift » (décalage par rapport au boîtier) reste disponible.

## 12. Questions ouvertes

- Faut-il moduler le décalage selon la part de lumière ambiante (vitesse, ouverture, ISO,
  puissance du flash) ? Une fois des mesures sur plusieurs séries disponibles, on pourra voir
  si le décalage varie avec ces réglages.
- Le serveur doit-il aussi gérer d'autres réglages par série (exposition, profil partiel) ?
  Le protocole s'y prêterait sans changement de principe.
- Faut-il démarrer le serveur automatiquement depuis RawTherapee s'il ne tourne pas ?
