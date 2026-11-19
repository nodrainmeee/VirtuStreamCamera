# Contrat de transport vidéo/audio — proposition v1

Ce document définit le contrat à implémenter entre le service de lecture et les composants de sortie. Ce n'est pas encore du code de pilote.

## Vidéo

- Identifiant : `Local\\VirtuStreamCamera.FrameRing.v1`
- Événement nouveau frame : `Local\\VirtuStreamCamera.FrameReady.v1`
- Événement espace libre : `Local\\VirtuStreamCamera.FrameConsumed.v1`
- Trois slots maximum (triple buffering).
- Format de transport conseillé : NV12, stride/planes décrits explicitement.
- Chaque frame contient : signature, version ABI, numéro de séquence, largeur, hauteur, format, strides, taille des plans, timestamp QPC, timestamp de présentation 100 ns, index de slot.
- Les métadonnées sont écrites avant publication atomique de l'index de frame.
- Le consommateur valide les limites avant toute lecture et ignore les séquences périmées.
- 720p NV12 ≈ 1,32 Mio par image ; 3 slots ≈ 4 Mio. 1080p NV12 ≈ 2,97 Mio par image ; 3 slots ≈ 9 Mio (hors alignement).

## Audio

- Identifiant : `Local\\VirtuStreamCamera.AudioRing.v1`
- PCM de départ : 48 kHz, 2 canaux, 16-bit PCM (ou float32 si le miniport et l'endpoint négocient ce format).
- Le contrat doit inclure la fréquence, le nombre de canaux, le format, le nombre de frames audio, le timestamp, le compteur de séquence et le compteur de sous-alimentation.
- Le pilote ne doit jamais bloquer le thread temps réel en attente d'une vidéo ou d'une UI.

## Synchronisation

- Utiliser QPC comme horloge monotone commune côté user mode.
- Convertir les timestamps vers les unités 100 ns exigées par DirectShow/WASAPI lorsque nécessaire.
- En cas de retard, abandonner les données les plus anciennes selon une politique documentée ; ne jamais accumuler indéfiniment.
- Mesurer FPS produits, FPS consommés, frames abandonnées, latence, underruns audio et dérive A/V.

## Sécurité

- Noms d'objets locaux et ACL limités à l'utilisateur/session qui a lancé l'application.
- Pas de mémoire partagée globale accessible à tous les utilisateurs.
- Version ABI obligatoire et validation stricte de chaque champ.
