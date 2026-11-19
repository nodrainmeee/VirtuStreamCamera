# Compilation cloud avec GitHub Actions

Ce projet contient un workflow dans `.github/workflows/windows-build.yml`.

## Utilisation

1. Créer un dépôt GitHub nommé `VirtuStreamCamera`.
2. Extraire cette archive et téléverser **le contenu** du dossier `VirtuStreamCamera` à la racine du dépôt (y compris le dossier caché `.github`).
3. Placer le dépôt sur la branche `main`.
4. Ouvrir l'onglet **Actions** du dépôt.
5. Sélectionner **Build VirtuStream Camera (Windows x64)** et lancer **Run workflow**, ou faire un commit/push sur `main`.
6. Quand l'exécution est terminée avec le statut vert, ouvrir l'exécution et télécharger l'artefact `VirtuStreamCamera-preview-win-x64`.

La compilation est exécutée sur un runner Windows hébergé par GitHub. Visual Studio, MSVC et CMake ne sont pas installés sur le PC personnel pour ce processus.

## Limite importante de cette préversion

Le workflow compile uniquement l'application de prévisualisation 0.1.0 qui existe dans cette archive. Cette version permet de charger et prévisualiser un média avec Media Foundation, mais elle ne fournit pas encore de pilote de caméra virtuelle Windows ni de pilote de microphone virtuel. Les dossiers `drivers/camera` et `drivers/audio` ne contiennent que leur documentation d'architecture.

La compilation distante ne remplace pas les essais sur le PC cible : les périphériques devront être installés et testés sur Windows 10 réel. Les pilotes noyau 64 bits doivent aussi suivre la procédure de signature adaptée au test ou à la distribution.

## Confidentialité et coût

- Dépôt public : les runners GitHub standard Windows sont gratuits et illimités selon la documentation GitHub actuelle.
- Dépôt privé : les minutes des runners sont déduites du quota Actions du compte, puis peuvent être facturées selon le forfait.
- Garder le dépôt privé protège le code source du regard public. Ne publier le dépôt que si l'ouverture du code est souhaitée.
