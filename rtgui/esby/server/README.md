# esbywb : serveur de balance des blancs par série

Étape 2 de `../SPEC_series_wb.md`. Python 3, bibliothèque standard uniquement.

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

Avec le serveur, TTSeriesWB envoie une **observation** à chaque Learn et à chaque enregistrement
d'image : balance de l'image et du boîtier, décalage, nature (`learn`, `saved-manual`,
`saved-series`, `saved-camera`), exposition (ISO, ouverture, vitesse, `light_value`), boîtier,
objectif, flash. Une seule observation est gardée par fichier, la plus récente. Elles servent à
vérifier si le décalage suit l'exposition, avant toute correction automatique.

```bash
./esbywb.py observations > obs.csv            # CSV
./esbywb.py observations --exif > obs.csv     # + balises lues par exiftool dans chaque fichier
./esbywb.py observations --exif "Model,LightValue,PanasonicRaw_CameraIFD_0x1300" > obs.csv
```

## Tests

```bash
python3 -m unittest discover tests
```
