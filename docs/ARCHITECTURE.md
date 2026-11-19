# Architecture technique proposée

## 1. Application de contrôle

Application native C++/Win32 légère. Media Foundation assure la lecture/prévisualisation. Le moteur de lecture ne doit pas dépendre d'un navigateur embarqué.

## 2. Pipeline vidéo cible

```text
Fichier local
    │
    ▼
Media Foundation Source Reader / Media Engine
    │  décodage matériel si le pilote Intel le permet
    ▼
Normalisation du format (NV12 privilégié pour le transport ; conversion uniquement si le consommateur l'exige)
    │
    ▼
Anneau partagé de 3 images + horodatages QPC / 100 ns
    │
    ▼
Filtre source caméra DirectShow (COM, user mode)
    │
    ▼
Applications compatibles avec l'énumération DirectShow
```

Le filtre DirectShow doit négocier plusieurs médias explicitement (au minimum NV12 et/ou RGB32), exposer `IAMStreamConfig`, utiliser des timestamps cohérents, publier les dimensions exactes et gérer proprement pause/arrêt/déconnexion. Les consommateurs lents doivent recevoir la dernière image disponible ou des images abandonnées de façon contrôlée plutôt que provoquer une file qui grandit sans limite.

DirectShow est une API historique. Le choix est motivé par le besoin de compatibilité Windows 10 et doit être validé avec chaque application cible. Une caméra DirectShow n'est pas automatiquement visible par tous les clients Media Foundation.

## 3. Pipeline audio cible

```text
Piste audio du fichier
    │
    ▼
Media Foundation / WASAPI (décodage et conversion PCM)
    │
    ▼
Anneau PCM partagé, timestamps et compteur de sous-alimentation
    │
    ▼
Pilote capture WDM/WaveRT (endpoint d'enregistrement Windows)
    │
    ▼
Applications qui sélectionnent "VirtuStream Virtual Microphone"
```

Le pilote doit exposer un véritable endpoint de capture. Le flux PCM envoyé par l'application doit arriver dans le buffer lu par le miniport de capture. Le sample SYSVAD est une référence architecturale, pas une implémentation terminée de ce transport.

## 4. Transport interprocessus

Voir `shared/FRAME_TRANSPORT_V1.md`. L'application et les composants de périphériques doivent utiliser une ABI versionnée, des tailles vérifiées, des événements nommés et des ACL restreintes à la session utilisateur. Ne pas exposer de commandes arbitraires depuis un navigateur ou une page web.

## 5. Différences Windows 10 / Windows 11

- Windows 10 : chemin de compatibilité caméra initial envisagé = filtre source DirectShow en mode utilisateur, avec tests par application. Microphone = pilote WDM/WaveRT.
- Windows 11 : évaluer en plus l'API officielle `MFCreateVirtualCamera` (Windows build 22000 minimum).
- Ne pas prétendre qu'une seule implémentation est visible dans tous les frameworks de capture.

## 6. Sécurité et stabilité

- Driver audio : tests sur une machine virtuelle ou machine secondaire avant installation sur le PC principal.
- Package signé pour un déploiement normal ; signature de test uniquement pour environnement de développement isolé.
- Aucun contournement des contrôles de sécurité ou de signature des pilotes.
- Désinstallation restaure les périphériques audio par défaut et retire proprement les composants enregistrés.
