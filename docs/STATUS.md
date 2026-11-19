# État réel du projet (mis à jour : 19 nov. 2026)

Principe : une case n'est cochée que si elle a été exécutée. « Écrit » ≠ « compilé » ≠ « testé ».

| Composant | Écrit | Compilé | Testé | Où la preuve est produite |
|---|---|---|---|---|
| App Win32 + Media Foundation (0.1.0) | oui | **non vérifié** | non | build GitHub Actions |
| Correctif GetNativeVideoSize / macros | oui (vérifié dans le code) | **non vérifié** | non | build GitHub Actions |
| Transport vidéo `vs_video_ring.h` (ABI v1) | oui | oui (g++ Linux) | **oui** : 3 lecteurs + 1 écrivain, 3000 images, 0 déchirure ; rejet d'en-têtes hostiles | `tests/`, ctest en CI |
| Filtre caméra DirectShow | non | non | non | — |
| Pilote microphone WDM/WaveRT | non | non | non | — |
| Moteur Source Reader / NV12 / A-V sync | non | non | non | — |
| Nouvelle interface | non | non | non | — |
| Installateur / désinstallateur | non | non | non | — |

Environnement de travail de ce jalon : Linux sans réseau, sans compilateur Windows, sans WDK. Aucun test Windows n'a pu être exécuté.

## Limites qui ne dépendent pas du code
1. **Signature du pilote audio.** Windows 10 x64 refuse un pilote noyau non signé sans mode test. Distribution publique = compte Microsoft Partner Center + certificat EV + soumission Hardware Dev Center (attestation). Aucun code ne remplace cela.
2. **Tests matériels** (caméra vue par OBS/Teams/Chrome, micro dans mmsys.cpl, 720p30 sur i7-4600U, redémarrage, désinstallation) : à exécuter sur Windows ; je ne peux pas les faire pour vous.
3. **Versions d'actions GitHub** du workflow (`checkout@v7`, `upload-artifact@v7`, `setup-msbuild@v3`) : non vérifiables sans réseau ; si le build échoue au tout début, vérifier d'abord ces tags.

## Ordre de réalisation proposé (chaque étape validée par la CI avant la suivante)
1. Valider que le build 0.1.0 + ctest passent en CI.
2. Moteur Source Reader → NV12 → ring (mesures réelles de FPS/CPU).
3. Filtre DirectShow (DirectShow base classes récupérées par la CI) → test OBS.
4. Nouvelle interface (Direct2D/DirectWrite, thème anthracite).
5. Installateur Inno Setup (+ désinstallation, regsvr32 /u).
6. Pilote micro (fork de SYSVAD, licence MIT) en VM de test avec signature de test.
7. Signature de distribution.
