# Plan de validation

## Application de prévisualisation (contenue dans cette archive)

- Démarrage sous Windows 10 x64.
- Sélection d'un fichier MP4 H.264/AAC.
- Prévisualisation dans la zone vidéo.
- Lecture, pause, boucle.
- Fermeture propre (pas de processus résiduel).
- Test d'un fichier 720p puis 1080p.

## Caméra virtuelle — critères d'acceptation du composant futur

- Le périphérique apparaît réellement dans les applications qui utilisent DirectShow.
- Les formats négociés correspondent au flux publié.
- Test 1280×720 / 30 fps sur la machine cible.
- Mesure de FPS produits/consommés et des frames perdues.
- Test OBS, navigateur Chromium et applications cibles une par une.
- Comparaison de l'aperçu local et du flux réellement capturé par l'application test.
- Mesure de la qualité après transport WebRTC séparée de la qualité du périphérique local.

## Microphone virtuel — critères d'acceptation du composant futur

- Endpoint visible dans le panneau Son > Enregistrement.
- Niveau de signal visible lors de la lecture de la vidéo.
- Sélectionnable dans une application de test.
- Vérification 48 kHz stéréo et latence.
- Mesure des underruns, silences, saturation et dérive A/V.
- Test de fermeture brutale/reconnexion de l'application de contrôle.

## Matériel cible

Intel Core i7-4600U, 4 Go RAM, Intel HD Graphics, SSD, Windows 10. Enregistrer les versions exactes de Windows, du pilote Intel et les codecs testés. Ne pas conclure sur les performances à partir d'une simple prévisualisation locale.
