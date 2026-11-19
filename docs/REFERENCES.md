# Références techniques

- Microsoft — `MFCreateVirtualCamera` : https://learn.microsoft.com/windows/win32/api/mfvirtualcamera/nf-mfvirtualcamera-mfcreatevirtualcamera
  - L'API officielle de caméra virtuelle Media Foundation est disponible à partir de Windows Build 22000.
- Microsoft — SYSVAD Virtual Audio Device Driver Sample : https://learn.microsoft.com/samples/microsoft/windows-driver-samples/sysvad-virtual-audio-device-driver-sample/
  - Base de référence WDM/WaveRT. Le sample standard simule une partie de la capture et n'est pas une implémentation finie du transport de notre média.
- Microsoft — Audio driver samples : https://learn.microsoft.com/windows-hardware/drivers/samples/audio-driver-samples
- Microsoft — DirectShow source filters : https://learn.microsoft.com/windows/win32/directshow/how-to-write-a-source-filter-for-directshow
  - Base de conception d'un filtre source push ; DirectShow est une API historique.
- Microsoft — `IAMStreamConfig` : https://learn.microsoft.com/windows/win32/api/strmif/nn-strmif-iamstreamconfig
- OBS — Video Capture Sources : https://obsproject.com/kb/video-capture-sources
  - Les sources de capture Windows d'OBS reposent sur la sortie DirectShow des périphériques.

Les choix et limites sont expliqués dans `ARCHITECTURE.md` et les fichiers `drivers/`.
