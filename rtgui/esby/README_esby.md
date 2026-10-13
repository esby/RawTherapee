# Fork esby de RawTherapee

Ce dossier contient le code propre au fork `esby/RawTherapee`. Ce document sert de
point d'entrée pour reprendre le projet après une pause : ce que fait le fork, où se
trouve le code, comment il se branche sur l'upstream et comment faire une mise à jour.

Dépôts :

- `origin` : `git@github.com:esby/RawTherapee.git` (ce fork)
- `reference` : `https://github.com/RawTherapee/RawTherapee.git` (l'upstream, ex-Beep6581)

Le détail de chaque changement est dans les messages de commit (`git log`), qui
expliquent le *pourquoi*.

## Fonctionnalités

- **Favoris** : chaque outil a un bouton favori ; l'onglet Favorites rassemble les outils
  marqués, dans un ordre propre.
- **Déplacement des outils** : boutons ↑ ↓ (dans l'onglet) et ← → (vers l'onglet voisin,
  en sautant les onglets cachés, Favorites et Trash).
- **Onglets Useful et Trash** : Useful contient les outils `tt*` ; Trash reçoit les outils
  mis à la corbeille.
- **Profils de disposition `.ttp`** (TTSaver) : favoris, corbeille, position de chaque
  outil et réglages des outils de Useful ; chargement automatique possible au démarrage.
- **Variables d'environnement** : EXIF complets (lus avec Exiv2), taille, nom de fichier,
  version pp3… exposés aux outils `tt*` (préfixe `rti:` pour les EXIF).
- **Outils `tt*`** (onglet Useful) :

| Outil | Rôle |
|---|---|
| TTSaver | enregistrer et charger les profils `.ttp` |
| TTIsoProfiler | appliquer un profil partiel selon l'ISO (`profiles/Partial/iso-*.pp3`) |
| TTTabHider | cacher des onglets |
| TTFavoriteColorChooser | couleurs des boutons favori et corbeille |
| TTPanelColorChooser | couleur des titres d'outils |
| TTUDLRHider | cacher les flèches de déplacement, verrouiller les favoris |
| TTLensCorrector | distorsion mémorisée par focale |
| TTTweaker | rotation automatique (Panasonic), fermeture après enregistrement… |
| TTVarDisplayer | afficher les variables, bouton de copie dans le presse-papier |
| TTSeriesWB | balance des blancs au flash : décalage de la série (partagé par TTShare, en mireds) et écart de chaque image avec la référence de sa série |
| TTSeriesExposure | écart d'exposition de chaque image avec la référence de sa série (pipette ou service), pour cette image seulement |

### Série et parent (tous les outils)

- **Série** d'une photo : en remontant depuis le dossier du RAW (5 niveaux au plus), le premier
  dossier qui contient un des fichiers de `SeriesMarkers` (`conf/fields.txt;.esby-series` : le
  `conf/fields.txt` d'un dossier numéroté, ou un `.esby-series` posé pour regrouper plusieurs
  dossiers) ; sinon le dossier numéroté (le dossier du RAW sans `pp/`, `pp/dpp/`).
- **Parent** d'une série (TTShare) : au-dessus d'elle (3 niveaux au plus), le premier dossier qui
  contient un des fichiers de `ParentMarkers` (`fields.conf` : la convention, ex. `sorted/`) ; sinon
  le dossier au-dessus de la série.
- `EsbySharedVariables::seriesFolder()` et `parentFolder()`, utilisés par TTShare, TTSeriesExposure
  et les variables partagées (TTSeriesWB ensuite).

### TTShare : modifications partagées par série

Une modification faite dans une image est partagée avec les images de sa série (voir ci-dessus) : tout de suite dans les autres instances qui en affichent une (dans la fenêtre active ;
les autres à leur prochain focus), plus tard dans les images ouvertes ensuite.

- Les paramètres sont comparés comme les clés du pp3 (`Section/Clé`), pour les sections partagées
  seulement (option `ShareSections`, `Exposure` par défaut ; cases à cocher de l'outil).
- Un nombre (compensation, contraste, noir...) est partagé comme un **décalage** : chaque image garde
  sa valeur, plus les décalages de la série, bornés aux limites du curseur. Le reste (courbe, choix,
  case à cocher) est partagé comme une **valeur**, qui remplace la précédente.
- Le serveur garde par série la somme des décalages de chaque clé (ou sa dernière valeur et sa
  version), et par image ce qu'elle a déjà reçu (`file_data`, espace `share`) : une image ouverte
  plus tard reçoit ce qui lui manque. `esbywb.py shares <dossier>` les affiche, `share-reset` les
  oublie.
- **Dossier parent** : une série utilisée pour la première fois part des modifications partagées
  de son dossier parent (une copie : le parent changé ensuite ne la change pas). « Send to the
  parent » remplace celles du parent par celles de la série ; « Reset from the parent » (avec
  confirmation) fait reprendre à la série celles du parent, ses images changent en conséquence.
- Le serveur garde aussi, par image, les valeurs de ses clés partagées telles que vues en dernier
  (`s:Section/Clé`). Une image rouverte avec d'autres valeurs (pp3 effacé : profil par défaut ;
  profil appliqué ailleurs) a perdu ce qu'elle avait reçu pour ces clés : elle reçoit à nouveau
  toutes les modifications de la série. Le bouton « Copy the status » (ou un clic sur le texte)
  copie l'état dans le presse-papier.
- « Share my changes » coupé (option `ShareChanges`) : la modification reste dans l'image. Les
  modifications des autres images sont toujours reçues tant que l'outil est actif.
- Une valeur posée par un outil pour cette image seulement (l'écart de TTSeriesExposure) n'est jamais partagée
  (`Environment::shareSuppressed`). Un retour dans l'historique n'est pas partagé.
- Autres sections partageables : `RGB Curves` (courbes R, V, B et mode luminosité, en valeur) et
  `Shadows & Highlights` (ombres, hautes lumières, largeurs tonales et rayon en décalage, bornés ;
  activation et mode Lab en valeur). Les courbes de l'outil Exposition sont dans `Exposure`.
- **Balance des blancs** (section `White Balance`, case « White balance ») : seules `Setting`,
  `Temperature`, `Green` et `Equal` sont partagées. La température est décalée en **mireds**
  (1 000 000 / K), la teinte et l'égaliseur comme des **facteurs** (logarithme). Une image en balance
  « Camera » compte avec les valeurs du boîtier : un décalage reçu la passe en « Custom » à partir
  d'elles. Une clé que l'image ne partage pas (`Environment::shareKeyFilter`, TTSeriesWB : photo sans
  flash) n'est ni envoyée ni reçue.
- Accroches : `ToolPanelCoordinator::panelChanged` et `profileChange` appellent `esbyParamsChanged`
  (changement d'un outil, ou nouvelle référence de comparaison) ; `esbyApplyParams` fait lire les
  paramètres reçus par les outils puis les traite comme un changement d'outil (entrée d'historique).

### TTSeriesExposure : écart de chaque image avec la référence de sa série

- La **série** d'une photo est son dossier numéroté (voir TTShare). La compensation commune à la
  série se règle avec l'outil Exposition lui-même, partagée par TTShare (« Share my changes »).
- Le curseur « Compensation of the series » montre la compensation de l'image sans son écart ; le
  bouger revient à bouger la compensation dans l'outil Exposition (partagé avec la série par
  TTShare si « Share my changes » est coché). Un clic sur le texte d'état le copie.
- TTSeriesExposure n'ajoute à chaque image que **son écart** (IL) avec la référence de la série, pour
  cette image seulement (jamais partagé) : curseur « Gap of this image », état **none** (pas d'écart),
  **computed** (mesuré) ou **manual** (curseur). « No gap » le remet à 0.
- **Référence de la série** : variable partagée `exposure.reference` posée sur le dossier de la série,
  texte JSON `{raw, comp, gap, target?, point?}` (compensation et écart de la référence quand elle a
  été posée). Ctrl+Maj+clic à la pipette sur la zone de référence (Ctrl+clic suffit tant que la série
  n'a pas de référence ; la luminosité devient la cible ; avec le service, le point désigne le
  visage), ou « Reference: this image » avec le service. Elle ne sert qu'à calculer les écarts.
- RawTherapee ne transmet les clics à un outil pipette qu'avec Ctrl (un clic simple déplace l'image).
- **Pipette** (Ctrl+clic sur une autre image) : écart = log2(cible / luminosité) − comp. de la référence + comp. de l'image +
  écart de la référence (la luminosité suit 2^compensation ; la courbe de l'outil Exposition rend le
  calcul approché : un deuxième clic l'affine). **Service** : écart = `delta_ev` + écart de la
  référence ; appliqué automatiquement à l'ouverture d'une image sans écart si les critères sont
  remplis (voir ci-dessous), sinon proposé.
- L'écart et son état sont gardés par le serveur (`file_data`, espace `exposure` : `gap`, `state`).
  Une image dont les valeurs ont été perdues (pp3 effacé) reçoit à nouveau son écart par TTShare,
  avec les modifications de la série. Sans serveur, le curseur fonctionne mais l'écart n'est pas
  gardé. Outil désactivé : rien n'est fait (les écarts déjà appliqués restent dans les images).
- La pipette lit la luminosité après la compensation d'exposition, la luminosité, le contraste et la
  compression des hautes lumières de l'outil Exposition, avant les courbes (`EUID_ToneCurve1`).

- **Images fermées** : rien n'est écrit dans leurs pp3 ; une image reçoit ce qu'elle a manqué à sa
  réouverture (son pp3 peut avoir été déplacé, ex. cache de rt_queue : la clé est son fichier d'origine).
- Une clé réelle (compensation, clip, teinte, égaliseur) reste réelle même écrite « 0 » dans le pp3.

### Information sur l'image partagée (`ui.showInfo`)

- Le bouton « i » de l'éditeur (informations de prise de vue sur l'image) est partagé par les instances :
  variable partagée globale `ui.showInfo` (booléen). L'activer dans une instance l'active dans les autres.
- Option `ShareShowInfo` (vraie par défaut). Sans serveur, chaque instance garde son bouton.
- Accroches : `EditorPanel` appelle `esbyInfoToggled` au clic et reçoit la valeur par `esbySetInfoListener`.

### TTSeriesWB : balance des blancs de la série et écart de chaque image

- Balance de l'image = balance du boîtier + **décalage de la série** + **écart de l'image**.
- Le décalage de la série (mireds, facteur de teinte, égaliseur bleu/rouge) est partagé par TTShare
  (section White balance) : changer la balance dans l'outil Balance des blancs, ou les curseurs
  « Series: » de TTSeriesWB (qui montrent la balance de l'image sans son écart), change toute la série.
- L'**écart** de l'image (mireds, jamais partagé) : **none**, **computed** ou **manual**, gardé par le
  serveur (`file_data`, espace `wb` : `gap`, `state`) et redonné par TTShare à une image qui a perdu ses
  valeurs. « No gap » le remet à 0, « Compute the gap » le recalcule même s'il était manuel.
- **Référence de la série** : variable `wb.reference` sur le dossier de la série, JSON
  `{raw, shutter, gap, k, hss?, sync}`. Écart calculé = écart de la référence + k × (vitesse − vitesse
  de la référence) ; en synchro haute vitesse (vitesse ≤ `sync`), écart = `hss`. « Model from this
  image », après un écart réglé à la main : donne `hss` (image en HSS) ou `k` (vitesse nettement
  différente de la référence). Le curseur Sync speed est gardé avec la référence.
- « Only for the photos taken with a flash » : une photo sans flash ne prend pas part à la balance de la
  série. L'interrupteur de l'outil l'active ; désactivé, il ne fait rien.
- Observations envoyées au serveur à l'enregistrement et à la fermeture (`saved-…`, `closed-…` avec
  l'état de l'écart ou `camera`) et à chaque « Model from this image » (`model`).
- Remplace l'ancien fonctionnement (valeur par dossier avec héritage, Learn, Apply, mode auto) ; les
  options `SeriesWBMired`, `SeriesWBGreen`, `SeriesWBEqual`, `SeriesWBMode`, `SeriesWBAuto`,
  `SeriesWBLearnTint` et `SeriesWBMarkers` n'existent plus. Les règles par dossier du serveur
  (`get`, `set`…) restent disponibles mais l'outil ne s'en sert plus.

### TTSeriesExposure : service externe de mesure d'exposition (optionnel)

Option `SeriesExpService` (`hôte:port`, vide par défaut) ; jeton éventuel dans la variable
d'environnement `ESBY_EXPOSURE_TOKEN`, envoyé en `Authorization: Bearer`. Le service n'est pas fourni :
il doit répondre en JSON (POST) à :

- `/reference {session, raw, point?, af_point?}` : le visage de référence de la séquence (`point` :
  clic de la pipette ; sinon le visage proche de `af_point`, ou le plus grand). Réponse : `status`,
  `choice` ;
- `/measure {session, raw, min_similarity, af_point?}` : `status` (`ok`, `no_face`, `no_match`,
  `no_reference`, `unmeasurable`, `unreadable`), `delta_ev` (exposition à ajouter par rapport à la
  référence, mesurée dans le RAW linéaire), `similarity`, `clipped_fraction`, et pour `no_face`
  éventuellement `fallback_af {delta_ev}`.

Positions en 0 à 1 de l'image affichée (orientation EXIF appliquée). Session : le dossier de la
série et le nom du RAW de la référence.

Application : écart = `delta_ev` + écart de la référence (absolu, donc idempotent).
Automatique si similarité ≥ `SeriesExpAutoSimilarity` (0,6), part écrêtée ≤ `SeriesExpMaxClipped`
(5 %), |écart| ≤ `SeriesExpMaxEv` (2 IL) et écart pas réglé à la main ; sinon proposée. Le repli sur la zone AF (`no_face`) n'est jamais
automatique ; `no_match` n'a pas de repli. `no_reference` (service redémarré ou session oubliée) :
nouvelle référence silencieuse à partir du RAW et du choix mémorisés. Exposition automatique ou
correspondance d'histogramme actives : aucune correction.

### Variables partagées

Le serveur `esbywb` garde aussi des variables nommées (nombre, texte ou booléen), posées sur une
séquence, un dossier (héritées en dessous) ou en global ; la plus proche l'emporte (voir
`server/README.md`). Pour l'image ouverte, elles sont copiées dans les variables de
l'environnement, avec leur portée et leur provenance : un outil les lit comme les autres
(`env->getVarAsDouble("exposure.offset")`). Une variable partagée ne remplace jamais une variable
interne ou Exif du même nom. La séquence est celle de TTSeriesExposure (quand il est actif) ;
sinon, le dossier numéroté de l'image.

Le panneau des variables (TTVarDisplayer) les affiche à part : valeur modifiable (Entrée : changée
là où elle est posée), provenance, bouton pour la retirer, et une ligne pour en ajouter une
(séquence, dossier, dossier parent ou global). Un changement fait dans une instance ou en ligne de
commande est vu aussitôt par toutes les instances ouvertes.

## Fichiers de ce dossier

| Fichier | Rôle |
|---|---|
| `esby.cmake` | sources du fork, chemins d'inclusion, installation des traductions |
| `environment.*` | état partagé : boîtes, panneaux, état favori/normal/corbeille, variables |
| `movabletoolpanel.*` | base de `ToolPanel` : boutons favori/corbeille/flèches, positions |
| `toolvboxdef.*` | boîtes d'onglet : nom, voisins ←/→, ajout et retrait de panneaux |
| `toolpanelcoordesby.cc` | méthodes de `ToolPanelCoordinator` ajoutées par le fork (constructeur, changement d'onglet, anneau ←/→, positions, EXIF, accroches d'`EditorPanel`) |
| `esbysettings.*` | options du fork (groupe `[TTP]` du fichier d'options), membre `Options::esby` |
| `esbyoptions.h` | `esbyOptions()` et `esbySettings()` : accès aux options |
| `esbypreferences.*` | onglet *Tools* des Préférences, membre `Preferences::esbyPanel` |
| `variable.*` | variables d'environnement (`RtVariable`) : valeur typée, portée, provenance |
| `esbysharedvars.*` | variables partagées de l'image ouverte, chargées depuis le serveur `esbywb` |
| `ttshare.*` | TTShare : modifications partagées par série (décalages et valeurs des clés du pp3) |
| `rtdef.h` | constantes : noms des boîtes, états, nombre d'onglets |
| `ttlog.h` | `TT_LOG()` / `TT_VERBOSE` : traces affichées seulement avec `Verbose=true` |
| `ttdep.*` | fonctions de tri des panneaux |
| `tt*.cc/h` | les outils de l'onglet Useful |
| `languages/default` | clés de traduction du fork, installées dans `<données>/esby/languages/` |
| `server/` | `esbywb.py` : serveur et outil en ligne de commande de la balance des blancs par série (Python) |
| `esbywbclient.*` | client du serveur `esbywb`, utilisé par TTSeriesWB et les variables partagées (GIO, asynchrone) |

## Branchement sur l'upstream

Le principe : le code upstream reste intact autant que possible, et le fork s'y branche
par de courtes **accroches**, marquées `esby-hook`. Pour les lister :

```bash
git grep -n "esby-hook" -- . ':!rtgui/esby'
```

Accroches actuelles :

- `rtgui/CMakeLists.txt` : `include(esby/esby.cmake)`
- `rtgui/options.h/.cc` : membre `EsbySettings esby`, lecture, sauvegarde, fichier de traduction
- `rtgui/preferences.h/.cc` : membre `esbyPanel`, onglet *Tools*, remplissage, enregistrement
- `rtgui/toolpanelcoord.cc` : appels `esby*()` dans le constructeur, dans `initImage()`, et
  `esbySetRawToolsSensitive()` dans `imageTypeChanged()` (les outils RAW sont grisés
  individuellement pour une image non RAW, au lieu de l'onglet Raw entier)
- `rtgui/editorpanel.cc` : ouverture d'image, événements, nom de fichier, enregistrement

Autres modifications du code upstream, plus profondes :

- `toolpanel.h/.cc` : `ToolPanel` hérite de `MovableToolPanel`
- `guiutils.h/.cc` : boutons dans l'en-tête des `MyExpander` (paramètre `ToolPanel*`, `nullptr` par défaut)
- `toolpanelcoord.h/.cc` : membres du fork, retouches dans `toolSelected`, `handleShortcutKey`,
  `updateVScrollbars`, `foldAllButOne`, `updateToolLocations`, `updateToolPanel`, et code
  upstream mis en commentaire dans le constructeur
- accesseurs utilisés par les outils `tt*` : `whitebalance` (`resetWBToCamera`),
  `distortion` (`get/setDistorValue`), `thumbnail` (`getpp3version`), `profilepanel` (`changeProfile`)
- `editorpanel.h`, `filepanel.h`, `rtwindow.*`, `main.cc` : option benchmark, `doDeployLate`
- correctifs à signaler à l'upstream : `filebrowser.cc` (crash quand le dossier de profils
  global manque), `profilestore.cc` (protection de `getPathFromId`)
- `CMakeLists.txt` (racine) : `-Wfatal-errors`

Pour voir tout ce que le fork modifie dans l'upstream :

```bash
git fetch reference
git difftool -d $(git merge-base HEAD reference/dev) HEAD
```

(`git config --global diff.tool meld` une fois pour toutes.)

## Conventions

- **Accroches** : une ligne par accroche, marquée `// esby-hook`. Le code va dans ce dossier.
- **Options** : passer par `esbyOptions()` et `esbySettings()` (qui s'appuient sur le
  singleton `App` de l'upstream), jamais directement par `App::get()`.
- **Includes** : `rtengine` inclut aussi des en-têtes de `rtgui` (ex : `labgrid.h` →
  `toolpanel.h`), donc `esby.cmake` ajoute `rtgui/` et `rtgui/esby/` aux chemins de `rtengine`.
  Après une mise à jour, prétraiter tous les fichiers de `rtengine` permet de le vérifier.
- **Traces** : `TT_LOG(...)` plutôt que `printf` ; les erreurs restent affichées en permanence.
- **Commentaires** : le code mis en commentaire est conservé ; quand une ligne est
  remplacée, l'ancienne reste en commentaire au-dessus.
- **Nouvel outil `tt*`** : l'ajouter dans `esby.cmake` (`ESBYSOURCEFILES`), le créer dans
  `esbyCreateUsefulTools()` (`toolpanelcoordesby.cc`), et ses clés de traduction dans
  `languages/default`.
- **Fichiers utilisateur** (hors dépôt) : `~/.config/RawTherapee5-esby/camconst.json`
  (entrée Panasonic DC-GH6).

## Mise à jour depuis l'upstream

```bash
git fetch reference
git config rerere.enabled true          # mémorise les résolutions de conflits
git checkout -b update-AAAA master
git merge reference/dev
```

Points d'attention :

1. **Conflits** : prendre la version upstream, puis y replacer les accroches `esby-hook`.
2. **Variables globales** : l'upstream regroupe peu à peu ses variables globales dans le
   singleton `App` (`App::get()`) ; les chercher dans `rtgui/esby/` si la compilation échoue.
3. **Arborescence** : les outils sont dans `rtgui/tools/`, les fenêtres dans `rtgui/windows/`,
   les widgets dans `rtgui/widgets/` ; corriger les `#include` du fork si un fichier déménage.
4. **Nouveaux outils upstream** : rien à faire, ils sont enregistrés depuis la table
   `PANEL_TOOLS` (`registerToolsFromLayout()`). Un outil déclaré comme sous-outil (ex : Crop
   Guide dans Crop) suit son outil parent, y compris dans Favorites.
5. **Fusionner souvent** : quelques dizaines de commits se fusionnent facilement, plusieurs
   centaines beaucoup moins (la mise à jour d'octobre 2026 couvrait 634 commits).

Après la fusion : compiler avec `brt_r new`, puis tester démarrage, déplacements, favoris,
profils `.ttp`, Préférences (*Tools*), ouverture d'image (TTVarDisplayer, TTTweaker) et
Batch Editor.

## Reste à faire

- Constructeur de `ToolPanelCoordinator` : repartir du constructeur upstream et le compléter
  par les méthodes `esby*()` (approche « B »), au lieu de garder le code upstream mis en
  commentaire. Pas urgent : la mise à jour d'octobre 2026 s'est fusionnée automatiquement.
- Profils `.ttp` : la position d'origine des outils mis à la corbeille n'est pas enregistrée.
- Signaler à l'upstream le crash de `FileBrowser::updateProfileList()` (dossier de profils
  global absent).
- `ToolParamBlock` converti en `ToolVBox*` (`toolvboxdef`, `registerToolsFromLayout`) :
  comportement indéfini qui fonctionne parce que les deux classes ont la même disposition.
- Message `Unable to load DCP profile ''` (code upstream, profil d'entrée « Embedded » sans
  nom de fichier) : à protéger et éventuellement à signaler.
- `issue encountered` dans `ProfileStore::getPathFromId()` : la protection évite le crash,
  mais la cause (un identifiant de dossier invalide) n'est pas trouvée.
