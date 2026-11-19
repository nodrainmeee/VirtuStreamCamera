# VirtuStream Virtual Microphone — composant à développer

## Architecture cible

Créer un endpoint Windows de capture audio nommé `VirtuStream Virtual Microphone`, à partir d'un pilote WDM/WaveRT. Le projet Microsoft SYSVAD est une référence de départ pour les miniports, INF et endpoints virtuels.

Le flux doit être fourni par l'application de contrôle via une interface user/kernel versionnée, puis consommé dans le chemin de capture du miniport. L'échantillon SYSVAD standard ne fait pas ce travail : il simule certaines données et ne constitue pas un pilote prêt à l'emploi pour notre cas.

## Exigences

- Endpoint de capture visible dans `mmsys.cpl` > Enregistrement.
- Nom clair et icône cohérente.
- Formats négociés documentés (commencer par 48 kHz, stéréo, PCM 16-bit).
- Tampon borné et faible latence.
- Protection contre les underruns, accès concurrents et fermetures brutales de l'application.
- Communication sécurisée avec le service user-mode.
- INF, catalogue, désinstallation et restauration propre des périphériques par défaut.

## Environnement de développement requis

- Visual Studio avec workload C++ desktop.
- Windows SDK compatible avec la cible.
- Windows Driver Kit (WDK) compatible.
- Machine virtuelle ou ordinateur secondaire pour installer/tester le pilote.
- Certificat de test pour tests isolés ; signature de production conforme aux exigences Microsoft avant distribution.

Ne pas désactiver durablement les protections de signature des pilotes sur le PC principal. Ne pas installer un pilote non testé.

## Statut de cette archive

Le pilote audio n'est pas encore implémenté ou compilé dans cette livraison. Aucun INF factice n'est fourni.
