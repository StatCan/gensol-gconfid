# Notes de mise à jour de G-Confid

## 2026-XXXX-YY (Version `2.0.2b1`)

* Mise à jour de la version de jansson à 2.15.0

## 2026-Juillet-17 (Version `2.0.1`)

* Version de production

## 2026-Juillet-15 (Version `2.0.1b2`)

* Supprimer la limite supérieure de version pour pyarrow
* Ajouter des tests pour Python 3.14
* Correction: La fonction Sensibilité identifie désormais correctement le groupe BY d'analyse dans le journal

## 2026-Mai-13 (Version `2.0.1b1`)

- Toutes les instances du numéro de version de G-Confid ont été normalisées afin de supprimer les zéros de remplissage superflus.
- Mise à jour des dépendances :
  - PyArrow < 25 - les nouvelles versions de PyArrow (22-24) sont désormais disponibles et testées avec G-Confid
  - Pandas >= 2.2.0 - un problème avec la version 2.1.4 de Pandas a été découvert, causant des erreurs dans G-Confid
- Définition explicite des types de données (dtypes) pour plusieurs champs de sortie de chaque procédure afin d'éviter un problème lors de la combinaison des résultats pour tout les groups BY

## 2026-Avril-30 (Version `2.00.000`)

- Version de production
- Correction d'un bug mineur sur les valeurs affichées dans le rapport d'audit

## 2026-Avril-28 (Version `2.00.000b16`)

- Suppress: Correction d'un plantage survenant si toutes les cellules de `incell` sont marquées comme sensibles
- Audit:
  - Correction d'un bogue dans la création d'ensembles binaires qui entraînait la sortie de valeurs minimales et maximales incorrectes dans certaines circonstances
  - Amélioration du rapport sommaire
  - Amélioration du paramétrage du champ "ProblemIndicator" après la résolution

## 2026-Avril-09 (Version `2.00.000b15`)

- Le solveur PuLP par défaut a été remplacé par le solveur [HiGHS](https://highs.dev/) (voir la [documentation de PuLP](https://coin-or.github.io/pulp/technical/solvers.html#pulp.apis.HiGHS))
  - nécessite le paquet Python `highspy`
- Ajout de `highspy>=1.13.0` comme dépendance obligatoire
- Activation du multithreading (multifil) sur le solveur par défaut
- Suppress: Désactivation temporaire du paramètre `multiprocess`, nécessite un développement supplémentaire
- Audit:
  - Avec `multiprocess=True`: un solveur personnalisé (`custom_solver`) avec un chemin de journalisation (logfile ou logPath) produira désormais correctement un journal cumulatif de tous les journaux de processus individuels à cet emplacement
  - Amélioration des journaux pour signaler la progression durant la résolution

## 2026-Mars-23 (Version `2.00.000b14`)

- Suppress: Ajout du multitraitement pour répartir la charge de résolution sur les processeurs disponibles
  - Activé via le nouvel attribut `multiprocess`, défini à "False" par défaut

## 2026-Mars-17 (Version `2.00.000b13`)

- Audit:
  - Ajout de l'identification et de l'utilisation d'ensembles binaires pour améliorer l'efficacité du solveur
  - Ajout du multitraitement pour répartir la charge de résolution sur les processeurs disponibles
   - Activé via le nouvel attribut `multiprocess`, défini à "False" par défaut
  - `use_shuttle` est maintenant défini à "False" par défaut

## 2026-Février-20 (Version `2.00.000b12`)

- Mise à jour de la fonction d'audit pour reproduire plus fidèlement le comportement original de SAS
- Mise à jour pour assurer la compatibilité avec Pandas 3.0
  - Mise à jour de la limite supérieure de la dépendance Pandas à la version 4 dans pyproject.toml
- Mise à jour des scripts de construction pour fonctionner avec Visual Studio 2022
- Conversion des procédures Python natives pour utiliser des tableaux PyArrow au lieu de jeux de données Pandas
- Mise à jour du type de sortie par défaut vers PyArrow
- Mise à jour de la dépendance Pandas pour ajouter la prise en charge de la version 3
- Fin de la prise en charge de Python 3.10
- Amélioration de l'efficacité des calculs de limites de Suppression
- OptRound permet désormais de laisser l'un des champs "CellUB" ou "CellLB" vide, ou les deux
- Ajout de la prise en charge des fichiers Arrow IPC (`.arrow`)

## 2025-Octobre-23 (Version `2.00.000b11`)

- Mises à jour mineures de la documentation et nettoyage interne en vue de la diffusion publique.

## 2025-Octobre-14 (Version `2.00.000b10`)

- G-Confid a désormais été mis à l’essai pour utilisation avec Python 3.13
- Les types de sortie par défaut peuvent désormais être définis individuellement sur chaque procédure (y compris la sensibilité) ou globalement pour toutes les procédures, comme indiqué dans le guide de l’utilisateur
- L’option `split_blocks` de la méthode `to_pandas()` de PyArrow est désormais définie à `False` par défaut
  - Les sorties Pandas issues de Sensitivity ne nécessitent plus d’appliquer `.copy()` pour désactiver le mode lecture seule
- Correction du rapport de suppression pour compter correctement le nombre de cellules complémentaires
- Correction d’un bogue lié à la définition incorrecte des bornes supérieure et inférieure dans les champs du résolveur
- Mise à jour de la dépendance PyArrow pour autoriser la version 21.0.0

## 2025-Août-18 (Version `2.00.000b9`)

- Les sorties sont maintenant enregistrées si un résolveur ne parvient pas à produire de solution valide
  - Au cours de l’une ou l’autre phase, si un résolveur renvoie un état non optimal ou si le nombre de cellules à traiter (cell_to_treat) est inchangé entre deux itérations, une erreur est consignée, les itérations supplémentaires du résolveur sont interrompues et les sorties standard sont générées à partir des résultats de la dernière itération réussie, puis placées dans deux nouveaux tableaux de sortie : `outsuppress_failed` et `outcomplements_failed` (uniquement si `outcomplements` est demandé)
    - Pour l’instant, disponible uniquement sous forme de fiche de données Pandas DataFrames

## 2025-Juillet-17 (Version `2.00.000b8`)

- Mise en œuvre de l’ensemble de données OutComplement
  - Ajout d’un nouveau paramètre à la procédure de suppression `outcomplement`
    - Comme `outsuppress`, accepte un chemin d’accès vers un emplacement de sortie souhaité
    - Accepte également une expression booléenne si vous ne souhaitez pas enregistrer sur disque, mais souhaitez simplement générer la fiche de données DataFrame
    - Par défaut à `False`, car l’ensemble de données peut être très volumineux
- Met en œuvre `generate_suppress_report` pour consigner un rapport des cellules supprimées après chaque étape de la procédure de suppression
- Correction d’un bogue de validation entraînant l’exclusion de noms de champs fournis par l’utilisateur dans les schémas de validation

## 2025-Juin-30 (Version `2.00.000b7`)

- Ajout de la validation des entrées avant exécution
  - Peut être ignorée à l’aide du paramètre `skip_validation`
- Mise en œuvre complète de toutes les conditions `suppress_order` dans Suppress
- Mise en œuvre de la macro `VerifyNeedRunSuppressCells`
  - Ignore le traitement et produit un Outsuppress valide si incell contient uniquement des cellules sensibles, aucune cellule sensible, ou seulement des cellules sensibles et des cellules avec le statut 'X' ou 'P'
- `MINRESP` et `MINRESPW` sont désormais ignorés lors du calcul de la sensibilité pour une cellule si tous les enregistrements de cette cellule sont accompagnés d’une renonciation
- Correction d’un bogue provoquant la génération d’un fichier outround par OptRound avec des CellIds mal appariés

## 2025-Avril-03 (Version `2.00.000b5`)

- `report_level` consigne désormais correctement le rapport d’audit lorsqu’il est défini, même si le niveau de journalisation est INFO
- Ajout du paramètre `custom_solver` à Suppress, Audit et Opt_Round pour fournir un résolveur personnalisé à la place du résolveur par défaut PULP

## 2025-Février-28 (Version `2.00.000b4`)

- Le paramètre `p1_roundingbase` de Suppress a été renommé `size_roundingbase`
- L’algorithme Shuttle est disponible pour la vérification (Audit) et peut être utilisé avec le paramètre `use_shuttle`

## 2025-Février-24 (Version `2.00.000b3`)

- Création d’une nouvelle classe d’enveloppeur `gconfid.Auditing()`, `gconfid.Opt_Rounding()`
- L’alias `gconfid.sensitivity()` est renommé `gconfid.Sensitivity()`
- Le résolveur par défaut de PULP est désormais utilisé afin que l’utilisateur puisse modifier le résolveur ou les options avant d’appeler G-Confid
- Les ensembles de données d’entrée sont davantage insensibles à la casse; G-Confid tente d’ajuster la casse pour correspondre aux noms attendus
- Les paramètres `by_variable` ont été renommés en `by` pour correspondre au module de sensibilité
- Améliorations apportées à la documentation et aux exemples

## 2025-Janvier-31 (Version `2.00.000b2`)

- Nouvel attribut `capture_text` disponible dans `gconfid.Suppression()` et `gconfid.sensitivity()`.
  - Lorsque `capture=True` est précisé, cet attribut contient une copie de la sortie console.
- Nouvel enveloppeur de suppression `gconfid.Suppression()`
  - Nouvel enveloppeur de test `gconfid.testing.Suppression()`
- Version initiale de la documentation de migration.
  - Voir le [Guide de Migration](/docs/FR/sas_migration_guide.md) pour plus de détails sur la procédure de sensibilité.
  - Voir le [Tutoriel sur la Migration](/docs/FR/sas_migration_tutorial.md) pour une démonstration d’appel de la sensibilité.
- Suppression des paramètres de type indicateur `no_*` et `reject_*`.
  - Voir le [Guide de Migration](/docs/FR/sas_migration_guide.md) pour la liste des paramètres et des modifications.
