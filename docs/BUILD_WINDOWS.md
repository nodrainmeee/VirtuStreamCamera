# Construction sous Windows 10/11 x64

## Pré-requis

1. Windows 10 ou Windows 11 x64.
2. Visual Studio 2022 avec **Desktop development with C++**.
3. CMake 3.24 ou supérieur.
4. Windows 10 SDK / Windows SDK compatible avec le système cible.

Le pilote audio nécessitera en plus une installation séparée du WDK compatible. Il n'est pas requis pour compiler l'application de prévisualisation.

## Compiler la préversion de l'application

Depuis PowerShell, dans le dossier `VirtuStreamCamera` :

```powershell
cmake --preset windows-x64-release
cmake --build --preset windows-x64-release
```

Le binaire se trouvera dans :

```text
out/build/windows-x64-release/app/Release/VirtuStreamCamera.exe
```

Selon la génération CMake / Visual Studio, le chemin exact peut différer légèrement ; rechercher `VirtuStreamCamera.exe` dans le dossier `out/build`.

## État de la préversion

Cette préversion permet de sélectionner un fichier vidéo, d'afficher l'aperçu Media Foundation, de lire/mettre en pause et d'activer la boucle. Elle ne diffuse pas encore vers une caméra Windows, et ne fournit pas de microphone virtuel. Ces fonctions nécessitent les composants natifs documentés dans `drivers/`.

## Dépannage

- Si CMake ne trouve pas MSVC, ouvrir le terminal développeur Visual Studio 2022.
- Si `MF_MEDIA_ENGINE_PLAYBACK_HWND` n'est pas disponible, vérifier la version du Windows SDK utilisée.
- Si une vidéo ne s'ouvre pas, tester un fichier MP4 H.264/AAC standard ; la disponibilité des codecs dépend du système.
- Le décodage matériel dépend du pilote Intel et du codec. Ne pas forcer le mode matériel s'il est instable.
