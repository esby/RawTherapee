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
| `variable.*` | variables d'environnement (`RtVariable`) |
| `rtdef.h` | constantes : noms des boîtes, états, nombre d'onglets |
| `ttlog.h` | `TT_LOG()` / `TT_VERBOSE` : traces affichées seulement avec `Verbose=true` |
| `ttdep.*` | fonctions de tri des panneaux |
| `tt*.cc/h` | les outils de l'onglet Useful |
| `languages/default` | clés de traduction du fork, installées dans `<données>/esby/languages/` |
| `server/` | `esbywb.py` : serveur et outil en ligne de commande de la balance des blancs par série (Python) |

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
