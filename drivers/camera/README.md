# VirtuStream Virtual Camera — composant à développer

## Cible Windows 10

Le chemin de compatibilité initial envisagé est un filtre source DirectShow COM enregistré dans `CLSID_VideoInputDeviceCategory`. Il doit exposer un flux vidéo temps réel alimenté par l'anneau partagé défini dans `shared/FRAME_TRANSPORT_V1.md`.

Le filtre doit implémenter proprement :

- `IBaseFilter` / pin de sortie (les DirectShow Base Classes `CSource` et `CSourceStream` peuvent servir de base) ;
- `IAMStreamConfig` et négociation de types de média ;
- horodatages et cadence de présentation ;
- gestion de pause, reprise, arrêt et déconnexion ;
- verrouillage/refcount/threading COM corrects ;
- enregistrement et désenregistrement de la catégorie vidéo.

Formats de départ à tester : NV12 et RGB32, 1280×720 à 30 fps. Ajouter 1920×1080 seulement après mesure.

## Limite de compatibilité

DirectShow est une technologie historique. Certaines applications modernes utilisent Media Foundation et peuvent ne pas énumérer ce filtre. Chaque application doit être testée. Pour Windows 11, étudier séparément `MFCreateVirtualCamera` et son modèle de source logicielle.

## Pas de faux périphérique

Ne pas modifier `navigator.mediaDevices.enumerateDevices()` pour afficher un nom fictif. Le succès se vérifie dans le Gestionnaire de périphériques / les listes de caméras des applications et par la réception effective des images.

## Statut de cette archive

Le filtre caméra n'est pas encore implémenté ou compilé dans cette livraison. Ce dossier documente son contrat et les points d'acceptation. Aucune commande `regsvr32` ne doit être exécutée tant que la DLL réelle n'existe pas et n'a pas été testée.
