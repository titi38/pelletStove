# Périodes de présence — configuration par défaut

Le fichier `config/presence-schedule.default.json` est un **modèle versionné**, avec
sept tableaux dans l'ordre lundi → dimanche. **Aucune période n'est activée**
par défaut ; la consigne du mode automatique est 20 °C.

Au démarrage, AutoMode utilise :

1. `PELLET_SCHEDULE_FILE` (sinon `presence-schedule.json`) lorsqu'il existe ;
2. à défaut, `PELLET_DEFAULT_SCHEDULE_FILE` (sinon
   `config/presence-schedule.default.json`) ;
3. à défaut ou si le JSON est invalide, une programmation vide.

Le fichier modèle est **lu sans être modifié**. Les changements enregistrés
par l'interface sont sauvegardés séparément dans `PELLET_SCHEDULE_FILE`.
La configuration sauvegardée invalide n'est pas remplacée automatiquement ;
les périodes restent vides et un avertissement est journalisé.

Définir des chemins absolus dans systemd, par exemple :

```
Environment=PELLET_SCHEDULE_FILE=/var/lib/pelletStove/presence-schedule.json
Environment=PELLET_DEFAULT_SCHEDULE_FILE=/opt/pelletStove/config/presence-schedule.default.json
```

`/var/lib/pelletStove/` doit être inscriptible par l'utilisateur du service.
L'emplacement du modèle doit correspondre à ton installation réelle.

Attention : les protections antigel du code existant peuvent encore démarrer
le poêle même si aucune période de présence n'est programmée.

Les intervalles sont des périodes **de présence** avec début inclus et fin
exclue (HH:MM). Exemple à ajouter par l'interface :
`{"start":"17:00","end":"22:00"}` dans le tableau du jour choisi.
