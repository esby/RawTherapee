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
| TTSeriesWB | décalage de la balance des blancs au flash, en mireds (voir `SPEC_series_wb.md`) |
| TTSeriesExposure | exposition par séquence : cible apprise à la pipette (Ctrl+clic) sur une référence, appliquée d'un clic aux autres photos |

### TTSeriesExposure : séquences

- La séquence d'une photo est son **dossier numéroté** (celui qui contient `pp/`), qu'elle soit dans
  `pp/` ou déjà dans `pp/dpp/`. `ESBY_ORIGIN` (voir `rt_queue`) donne le chemin d'origine.
- Si le `fields.conf` de ce dossier ne crédite qu'**un modèle** (une ligne `credit_cosplayer:` qui
  commence par `model:`), un écart de plus de 10 minutes (option `SeriesExpGap`) entre deux photos
  démarre une nouvelle séquence. Sans `fields.conf`, le nombre de modèles est lu dans le nom du
  dossier (`NNN - jour - crédits - …`) ; crédit vide ou inconnu : le dossier entier.
- La pipette lit la luminosité après la compensation d'exposition, la luminosité, le contraste et la
  compression des hautes lumières de l'outil Exposition, avant les courbes (`EUID_ToneCurve1`).
- Les cibles sont dans `~/.config/RawTherapee5-esby/esby-exposure-targets.ini`.
- **Décalage de séquence** (curseur, variable partagée `exposure.offset`) : préférence de rendu
  ajoutée à la compensation mesurée (référence + écart + décalage). Le curseur la pose sur la
  séquence ; elle peut aussi être posée sur un dossier, hérité par ses séquences (panneau des
  variables ou `esbywb.py var-set`). Les cibles, les références et les mesures sont gardées sans
  décalage. La compensation appliquée par l'outil et le décalage qu'elle contient sont notés par
  image (`[applied]`, `[applied_offset]`) : une image qui les a encore reçoit le nouveau décalage,
  tout de suite si elle est ouverte, sinon à sa réouverture, avec un bouton ancienne/nouvelle
  valeur ; une image retouchée à la main n'est pas changée (message seulement). Il faut le serveur.

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

Positions en 0 à 1 de l'image affichée (orientation EXIF appliquée). Session : la clé de séquence.

Application : compensation = compensation de la référence + `delta_ev` (absolue, donc idempotente).
Automatique si similarité ≥ `SeriesExpAutoSimilarity` (0,6), part écrêtée ≤ `SeriesExpMaxClipped`
(5 %), |`delta_ev`| ≤ `SeriesExpMaxEv` (2 IL) et image non retouchée (compensation appliquée par
l'outil, ou 0 sans trace) ; sinon proposée. Le repli sur la zone AF (`no_face`) n'est jamais
automatique ; `no_match` n'a pas de repli. `no_reference` (service redémarré ou session oubliée) :
nouvelle référence silencieuse à partir du RAW et du choix mémorisés. Exposition automatique ou
correspondance d'histogramme actives : aucune correction.

### Variables partagées

Le serveur `esbywb` garde aussi des variables nommées (nombre, texte ou booléen), posées sur une
séquence, un dossier (héritées en dessous) ou en global ; la plus proche l'emporte (voir
`server/README.md`). Pour l'image ouverte, elles sont copiées dans les variables de
l'environnement, avec leur portée et leur provenance : un outil les lit comme les autres
(`env->getVarAsDouble("SeriesExpGap")`). Une variable partagée ne remplace jamais une variable
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
