# Notes de mise à jour de G-Confid

## 2026-Septembre-21 (Version `2.0.2`)

* Mettre à jour la version du sous-module jansonn à la version 2.15.0

## 2026-Juillet-17 (Version `2.0.1`)

- Supprimer la limite supérieure de version pour pyarrow
- Ajouter des tests pour Python 3.14
- Correction: La fonction Sensibilité identifie désormais correctement le groupe BY d'analyse dans le journal
- Toutes les instances du numéro de version de G-Confid ont été normalisées afin de supprimer les zéros de remplissage superflus.
- Mise à jour des dépendances :
  - Pandas >= 2.2.0 - un problème avec la version 2.1.4 de Pandas a été découvert, causant des erreurs dans G-Confid
- Définition explicite des types de données (dtypes) pour plusieurs champs de sortie de chaque procédure afin d'éviter un problème lors de la combinaison des résultats pour tout les groups BY

## 2026-Avril-30 (Version `2.00.000`)

- Le solveur PuLP par défaut a été remplacé par le solveur [HiGHS](https://highs.dev/) (voir la [documentation de PuLP](https://coin-or.github.io/pulp/technical/solvers.html#pulp.apis.HiGHS))
  - nécessite le paquet Python `highspy`
- Ajout de `highspy>=1.13.0` comme dépendance obligatoire
- Activation du multithreading (multifil) sur le solveur par défaut
- Audit:
  - Amélioration du rapport sommaire
  - Ajout du multitraitement pour répartir la charge de résolution sur les processeurs disponibles
   - Activé via le nouvel attribut `multiprocess`, défini à "False" par défaut
  - `use_shuttle` est maintenant défini à "False" par défaut
- Ajout de la prise en charge des fichiers Arrow IPC (`.arrow`)
- Les sorties sont maintenant enregistrées si un résolveur ne parvient pas à produire de solution valide
  - Au cours de l’une ou l’autre phase, si un résolveur renvoie un état non optimal ou si le nombre de cellules à traiter (cell_to_treat) est inchangé entre deux itérations, une erreur est consignée, les itérations supplémentaires du résolveur sont interrompues et les sorties standard sont générées à partir des résultats de la dernière itération réussie, puis placées dans deux nouveaux tableaux de sortie : `outsuppress_failed` et `outcomplements_failed` (uniquement si `outcomplements` est demandé)
    - Pour l’instant, disponible uniquement sous forme de fiche de données Pandas DataFrames
- Ajout de la validation des entrées avant exécution
  - Peut être ignorée à l’aide du paramètre `skip_validation`
- Ajout du paramètre `custom_solver` à Suppress, Audit et Opt_Round pour fournir un résolveur personnalisé à la place du résolveur par défaut PULP
- Le paramètre `p1_roundingbase` de Suppress a été renommé `size_roundingbase`
- Les paramètres `by_variable` ont été renommés en `by` pour correspondre au module de sensibilité
- Suppression des paramètres de type indicateur `no_*` et `reject_*`.
  - Voir le [Guide de Migration](/docs/FR/sas_migration_guide.md) pour la liste des paramètres et des modifications.
