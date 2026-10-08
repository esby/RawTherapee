# esbywb : serveur esby (balance des blancs par série, variables partagées)

Étape 2 de `../SPEC_series_wb.md`, étendu aux variables partagées entre toutes les instances de
RawTherapee. Python 3, bibliothèque standard uniquement.

## Démarrage

```bash
./esbywb.py serve                  # au premier plan
```

ou en service systemd utilisateur, voir `esbywb.service`.

- socket : `$XDG_RUNTIME_DIR/esby-wb.sock` (lisible par l'utilisateur seul)
- état : `~/.local/share/esby-wb/state.json` (règles par dossier, balances appliquées par fichier)

## Commandes

```bash
./esbywb.py set "~/photos/Lucca 2026" 15 --comment "halls, lumière chaude"
./esbywb.py set "~/photos/Lucca 2026/Samedi - Hall 3" 22
./esbywb.py set "~/photos/Lucca 2026/Dimanche - extérieur" 0
./esbywb.py get "~/photos/Lucca 2026/Vendredi"      # +15, source : Lucca 2026
./esbywb.py list
./esbywb.py unset "~/photos/Lucca 2026/Samedi - Hall 3"
./esbywb.py move "~/photos/Lucca 2026" "~/photos/2026 - Lucca"
./esbywb.py orphans
```

`set` accepte `--green` (facteur de teinte, 1.0 par défaut), `--equal` (facteur de l'égaliseur
bleu/rouge, 1.0 par défaut) et `--all` (aussi pour les photos sans flash). Un décalage positif
refroidit le rendu.

## Observations

Avec le serveur, TTSeriesWB envoie une **observation** à chaque Learn, à chaque enregistrement
d'image depuis l'éditeur, et à la fermeture de chaque image (quand son profil est enregistré) :
balance de l'image et du boîtier, décalage, exposition (ISO, ouverture, vitesse, `light_value`),
boîtier, objectif, flash, et nature de l'observation :

- `learn` : décalage appris sur une image corrigée à la main ;
- `learn-unchanged` : Learn sur une image qui avait déjà la balance de la série (sans information) ;
- `saved-…` ou `closed-…`, suivi de `manual` (retouche à la main), `series` (balance de la série)
  ou `camera` (balance du boîtier).

Une seule observation est gardée par fichier, la plus récente. Elles servent à vérifier si le
décalage suit l'exposition, avant toute correction automatique ; les plus instructives sont les
retouches à la main (`learn`, `…-manual`).

```bash
./esbywb.py observations > obs.csv            # CSV
./esbywb.py observations --exif > obs.csv     # + balises lues par exiftool dans chaque fichier
./esbywb.py observations --exif "Model,LightValue,PanasonicRaw_CameraIFD_0x1300" > obs.csv
```

## Variables partagées

Une variable a un nom (lettres, chiffres, `. _ : -`) et une valeur (nombre, texte ou booléen).
Elle est posée sur une **séquence** d'un dossier, sur un **dossier** (hérité par les dossiers en
dessous) ou en **global**. Pour une image, la plus proche l'emporte : séquence, dossier de
l'image, dossiers parents, global. Chaque changement est notifié aux instances abonnées
(événement `variable_changed`).

```bash
./esbywb.py var-set global SeriesExpGap 10
./esbywb.py var-set "~/photos/Lucca 2026" exposure.offset 0.3
./esbywb.py var-set "~/photos/Lucca 2026/042" exposure.offset -0.2 --sequence 2
./esbywb.py var-unset "~/photos/Lucca 2026/042" exposure.offset --sequence 2
./esbywb.py vars                                      # tout ce qui est posé, et où
./esbywb.py show "~/photos/Lucca 2026/042" --sequence 2   # ce qui s'applique, et d'où ça vient
```

Protocole : `var_set` (`path`, `sequence` optionnel, `name`, `value`), `var_unset`, `var_get`
(`path`, `sequence` et `name` optionnels : variables résolues avec `value`, `scope`, `origin`),
`var_list` (tout ce qui est posé). `path` vaut `global` (ou `*`) pour les variables globales ;
une séquence est stockée sous la clé `dossier#séquence`. `move` déplace aussi les variables.

## Tests

```bash
python3 -m unittest discover tests
```
