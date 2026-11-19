# VirtuStream Camera

Application Windows native dédiée à la prévisualisation et à la diffusion d'une source vidéo locale comme caméra virtuelle, avec un futur périphérique microphone virtuel.

> **État de cette livraison : fondation technique 0.1.0.** Elle contient une application Win32/C++ avec aperçu vidéo Media Foundation, sélection de fichier et commandes de lecture/boucle. Elle **ne contient pas encore** de filtre caméra DirectShow enregistré ni de pilote microphone WDM/WaveRT. L'interface l'indique explicitement : aucune caméra Windows n'est annoncée comme installée.

## Pourquoi repartir de zéro ?

L'ancien VirtuStream Browser remplaçait `navigator.mediaDevices.getUserMedia()` dans les pages Electron. Cela ne créait pas de périphérique vidéo au niveau de Windows. Son audio était routé vers une sortie existante (par exemple VB-CABLE), sans installer de microphone virtuel propre. La nouvelle application sépare le moteur média des périphériques système.

## Configuration cible

- Windows 10 x64 (cible initiale), compatibilité Windows 11 à conserver.
- Intel Core i7-4600U, 2 cœurs / 4 threads.
- 4 Go RAM.
- Intel HD Graphics.
- Cible de diffusion : 1280×720 à 30 i/s lorsque le fichier, le pilote graphique et l'application destinataire le permettent.
- 1920×1080 à tester ; aucune promesse de performance sans mesures.

## Arborescence

```text
VirtuStreamCamera/
├── CMakeLists.txt
├── CMakePresets.json
├── app/
│   ├── CMakeLists.txt
│   └── src/main.cpp
├── drivers/
│   ├── camera/README.md
│   └── audio/README.md
├── shared/FRAME_TRANSPORT_V1.md
└── docs/
    ├── ARCHITECTURE.md
    ├── BUILD_WINDOWS.md
    └── TEST_PLAN.md
```

## Construire l'application de prévisualisation

Voir [`docs/BUILD_WINDOWS.md`](docs/BUILD_WINDOWS.md).

## Périphériques virtuels : statut

- **Caméra Windows :** architecture cible DirectShow source filter pour compatibilité Windows 10, à développer et tester avec les applications qui énumèrent les sources DirectShow. La compatibilité n'est pas universelle ; certaines applications utilisent des API Media Foundation et peuvent nécessiter une autre intégration.
- **Microphone Windows :** pilote de capture WDM/WaveRT basé sur les exemples SYSVAD comme référence. Le chemin de transfert des échantillons depuis l'application doit être développé ; le sample SYSVAD standard n'est pas un pilote prêt à diffuser le son du fichier.
- **Windows 11 :** étudier l'API Media Foundation Virtual Camera (`MFCreateVirtualCamera`), qui n'est pas disponible sur Windows 10.

Ne pas installer de pilote expérimental sur le PC principal sans sauvegarde et procédure de récupération. Pour les pilotes de test, utiliser un environnement de test dédié et une signature de test explicite. Pour une distribution publique, prévoir les exigences de signature Microsoft.

## Principes de qualité

1. Mesurer la résolution et la cadence réellement produites par le moteur.
2. N'annoncer une résolution/FPS qu'après validation du flux.
3. Ne pas confondre aperçu local et qualité reçue par l'application distante : WebRTC et le réseau peuvent encore encoder/réduire le flux.
4. Limiter les copies CPU/GPU et éviter les conversions inutiles.
5. Ne pas utiliser une baisse de résolution comme correctif silencieux d'une saturation CPU.

## Compilation cloud (sans installer Visual Studio sur le PC)

Consulter [CLOUD_BUILD.md](CLOUD_BUILD.md). Le workflow GitHub Actions se trouve dans `.github/workflows/windows-build.yml` et produit un artefact Windows x64 téléchargeable.

Attention : le workflow actuel compile uniquement l'application de prévisualisation. Aucun pilote caméra ou microphone n'est encore présent dans cette préversion.
