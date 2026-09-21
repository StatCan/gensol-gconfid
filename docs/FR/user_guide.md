# Aperçu

G-Confid est un système généralisé de Statistique Canada qui offre une méthodologie conçue pour empêcher la diffusion de données confidentielles. Il est composé de quatre modules; le module de Sensibilité (`Sensitivity`) est utilisé pour déterminer le caractère délicat des cellules de tableau (et des combinaisons de cellules). Le module de `Suppression` sert à établir un modèle de suppression, c’est-à-dire un ensemble de cellules complémentaires, nécessaires pour protéger les cellules sensibles (ou les combinaisons de cellules). La troisième composante, le module d’Audit (`Auditing`), sert à vérifier la validité d’un schéma de suppression. En plus de ces trois modules, le module d’Arrondissement optimisé (`OptimizedRounding`) fournit un arrondissement à la fois contrôlé et additif.

Ce projet contient la version Python de G-Confid adaptée à partir de la version SAS existante. G-Confid est mis à l’essai avec les versions 3.10, 3.11, 3.12 et 3.13 de Python.

## Table des matières

-   [Sensibilité](#sensibilité)
    -   [Paramètres](#paramètres-de-sensibilité)
    -   [Tableaux d’entrée](#tableaux-dentrée-de-la-sensibilité)
    -   [Tableaux de sortie](#tableaux-de-sortie-de-la-sensibilité)
-   [Suppression](#suppression)
    -   [Paramètres](#paramètres-de-suppression)
    -   [Tableaux d’entrée](#tableaux-dentrée-de-la-suppression)
    -   [Tableaux de sortie](#tableaux-de-sortie-de-la-suppression)
-   [Audit](#audit)
    -   [Paramètres](#auditing-parameters)
    -   [Tableaux d’entrée](#tableaux-dentrée-de-laudit)
    -   [Tableaux de sortie](#tableaux-sortie-de-laudit)
-   [Arrondissement optimisé](#arrondissement-optimisé)
    -   [Paramètres](#paramètres-darrondissement-optimisé)
    -   [Tableaux d’entrée](#tableaux-dentrée-de-larrondissement-optimisé)
    -   [Tableaux de sortie](#tableaux-sortie-de-larrondissement-optimisé)
-   [Guide technique](#guide-technique)
    -   [Noms des variables dans les tableaux d’entrée](#noms-des-variables-dans-les-tableaux-dentrée)
    -   [Journal G-Confid](#journal-g-confid)
        -   [Configuration de la langue du journal](#configuration-de-la-langue-du-journal)
            -   [Niveau de verbosité du journal de Python (`trace=`)](#niveau-de-verbosité-du-journal-python-trace)
            -   [Suppression et dépannage des messages du journal (`capture=`)](#suppression-et-dépannage-des-messages-de-journal-capture)
            -   [Utilisation de votre propre enregistreur (`logger=`)](#utilisation-de-votre-propre-enregistreur-logger)
    -   [Spécification des tableaux d’entrées et de sorties](#spécification-des-tableaux-dentrée-et-de-sortie)
        -   [Formats pris en charge](#formats-pris-en-charge)
        -   [Spécification des tableaux d’entrée](#spécification-des-tableaux-dentrée)
        -   [Spécification des tableaux de sortie](#spécification-des-tableaux-sortie)
        -   [Personnalisation des spécifications de sortie par défaut](#personnaliser-la-spécification-de-sortie-par-défaut)
    -   [Accès aux tablauxs de sortie](#accès-aux-tablauxs-de-sortie)
    -   [Configuration du résolveur](#configuration-du-résolveur)
    -   [Autres](#autres)
        -   [Caractères d’échappement et chemins d’accès aux fichiers](#caractères-déchappement-et-chemins-daccès-aux-fichiers)
        -   [Erreurs et exceptions](#erreurs-et-exceptions)
        -   [Utilisation des fichiers SAS en Python](#utilisation-des-fichiers-sas-en-python)
        -   [Considérations relatives au rendement](#considérations-relatives-au-rendement)
        -   [Multitraitement](#multitraitement)

## Sensibilité

Ce module traite un fichier de microdonnées en agrégeant les données en cellules de tableau, conformément aux spécifications de l’utilisateur. Il calcule ensuite la sensibilité des cellules du tableau.

### Paramètres de sensibilité

| Paramètre       | Type python  |  Description                |
| ----------------| -------------| --------------------------- |
| indata|[Ensemble de données d’entrée](#spécification-des-tableaux-dentrée-et-de-sortie)|Précise le fichier de microdonnées d’entrée. Obligatoire.|
| outconstraint|[Ensemble de données de sortie](#spécification-des-tableaux-dentrée-et-de-sortie)|Contient les contraintes. Obligatoire.|
| outcell| [Ensemble de données de sortie](#spécification-des-tableaux-dentrée-et-de-sortie)|Contient les cellules sensibles Obligatoire.|
| outlargest|[Ensemble de données de sortie](#spécification-des-tableaux-dentrée-et-de-sortie)|Contient des renseignements sur les principaux contributeurs à une cellule. Facultatif.|
| outpairs|[Ensemble de données de sortie](#spécification-des-tableaux-dentrée-et-de-sortie)|Contient les paires d’observations les plus sensibles dans chaque cellule. Facultatif.|
| outtargets|[Ensemble de données de sortie](#spécification-des-tableaux-dentrée-et-de-sortie)|Contient des renseignements sur les combinaisons d’observations les plus sensibles dans chaque cellule.|
| unit_id|str| Indique le nom de la variable qui constitue la clé de l’ensemble de données d’entrée.|
| by| str | Calcule la sensibilité pour chaque groupe BY. |
| var | str | Indique le nom de la variable contenant les données des répondants.  |
| shadow| str | Indique le nom de la variable transitoire.  |
| dimension| str | Indique les noms des variables représentant les dimensions hiérarchiques. |
| waiver| str | Indique le nom de la variable contenant l’indicateur de renonciation complète. |
| p_waiver| str| Indique le nom de la variable contenant l’indicateur de renonciation partielle. |
| proxy_size | str | Indique le nom de la variable auxiliaire de taille utilisée dans le calcul. |
| weight| str | Indique la variable représentant le poids d’estimation.|
| s_rule| str | Précise la règle de sensibilité à utiliser.|
| hierarchy | str  | Précise la hiérarchie utilisée pour chaque dimension. |
| code_range | str| Précise l’intervalle des codes associé au niveau le plus bas de la hiérarchie.|
| m | float | Représente le nombre maximal de cellules non sensibles que l’utilisateur permet à G-Confid d’inclure dans un ensemble de cellules sensibles que G-Confid combine pour former un agrégat. Ce nombre est facultatif et a une valeur par défaut de 5. M doit être >= 0 et <= 10.                                                                                                                                                       |
| x | float | Précise le paramètre X permettant d’ajuster le nombre de combinaisons de cellules. `x` représente la différence relative minimale entre le total d’une cellule non sensible et la variation nette de la sensibilité de l’agrégat (si cette cellule y est incluse). Si la différence relative est inférieure à `x`, la cellule non sensible n’est pas incluse dans l’agrégat. Facultatif, doit être >= 0, par défaut = 0.                |
| y | float | Précise le paramètre Y permettant d’ajuster le nombre de combinaisons de cellules. `y` représente le ratio minimal entre le total d’une cellule non sensible et la sensibilité de l’agrégat sans cette cellule (pour que cette cellule soit incluse dans l’agrégat). Si le ratio est inférieur à `y`, la cellule non sensible n’est pas incluse dans l’agrégat. Facultatif, par défaut = 0 |
| z | float| Précise le paramètre Z permettant d’ajuster le nombre de combinaisons de cellules. `z` représente le ratio minimal entre la valeur du plus grand contributeur à un agrégat et le total d’une cellule non sensible (avant l’inclusion de cette cellule dans l’agrégat). Si le ratio est inférieur à `z`, la cellule non sensible n’est pas incluse dans l’agrégat. Facultatif, doit être <= 100, par défaut = 0                          |
| tolerance | float | Précise la valeur de la borne supérieure en dessous de laquelle la sensibilité est jugée trop faible (sous cette valeur, la sensibilité est fixée à 0). Par exemple, si la tolérance = 0,05, toute valeur de sensibilité comprise entre 0 et 0,05 sera ramenée à zéro. Ce nombre est facultatif et a une valeur par défaut de 0,01. La tolérance doit être >= 0 et <= 1000.                                                         |
| min_resp | int| Précise le nombre minimal de répondants dont la valeur n’est pas nulle dans une cellule. |
| proxy_ratio | float | Cette option précise quand et comment intégrer la variable `proxy_size` lors du calcul de la sensibilité. Lorsque cette valeur est précisée, elle sert à déterminer le seuil à partir duquel les observations sont remplacées par la variable de substitution. Elle est facultative et doit être comprise entre 0,0 et 1,0. `accept_negative=True` et `proxy_size` doivent tous deux être précisés lors de l’utilisation de cette option. |
| proxy_percentile    |   | Précise le pourcentage d’observations remplacées par la variable de substitution. |
| weight_prot_level   | str  | Précise le niveau de protection souhaité en présence de poids (s’applique uniquement lorsque l’instruction WEIGHT est précisée). Facultatif. Les valeurs valides sont LINEAR, STEP et EXACT. La valeur par défaut est LINEAR.|
| min_resp_w          | float | Précise le nombre minimal pondéré de répondants ayant une valeur non nulle dans une cellule. |
| debug_file_prefix   |    | Précise un préfixe pour les fichiers de débogage.|
| debug_work_dir_path |    | Précise un dossier de débogage.   |
| verbose | Booléen| Affiche des renseignements supplémentaires dans le journal. |
| timer | Booléen| Ajoute des renseignements temporels dans le journal. |
| print_codes | Booléen | Précise si les hiérarchies et les intervalles doivent être affichés ou non dans le journal.|
| limit_warnings | Booléen | Précise que l’affichage des avertissements générés lors de la lecture des microdonnées doit être limité ou non |
| additive_noise| Booléen | Détermine comment le système calcule les variables de sensibilité au niveau des cellules marginales (ou agrégées).|
| proxy_diag| Booléen  | Détermine s’il faut produire un rapport diagnostique lié à l’utilisation d’une variable de substitution.|
| weight_diag| Booléen| Détermine s’il faut produire un rapport diagnostique lié à l’utilisation d’une variable de pondération.|
| accept_negative | Booléen| Inclut des valeurs négatives lors du calcul de la sensibilité. Cette option est obligatoire lorsque proxy_size est précisé.|
| presort|Booléen| Si `presort = True`, trie automatiquement toutes les tableaux d’entrée précisés. L’utilisateur peut désactiver cette fonction en précisant `presort = False.`|
| skip_validation|Booléen|Désactive la validation pré‑traitement des champs attendus dans vos tables d’entrée. La valeur par défaut est False.|
| trace | Booléen ou int | Voir l’[option trace](#niveau-de-verbosité-du-journal-python-trace) |
| capture| Booléen | Voir [Suppression et dépannage des messages du journal](#suppression-et-dépannage-des-messages-de-journal-capture)|
| logger|logging.Logger| Voir [Utilisation de votre propre enregistreur](#utilisation-de-votre-propre-enregistreur-logger)|

### Tableaux d’entrée de la sensibilité

#### **indata**

Précise l’ensemble de données d’entrée contenant les microdonnées sur les répondants. Les variables associées aux instructions `dimensio` et `unit_id` doivent être des variables de type caractère. Toutes les autres variables doivent être numériques, à l’exception des variables `by` qui peuvent être de l’un ou l’autre type.

### Tableaux de sortie de la sensibilité

#### **outcell**

| Variable | Contenu  |
| -------- | ---------|
| CellId | Identifiant de cellule généré par G-Confid. Cet identifiant est utilisé pour établir un lien avec les ensembles de données `outconstaint` et `outlargest`. |
| NbAnonym | Nombre de répondants ayant contribué au total de la cellule, mais pour lesquels il n’y avait pas de valeur dans la variable `unit_id`. |
| AnonymTotalVar | Valeur totale des répondants anonymes pour la cellule. G-Confid considère que le répondant est anonyme si la valeur de l’observation pour la variable unit_id est manquante dans le fichier de microdonnées d’entrée. |
| NbRespondents | Nombre de répondants ayant contribué au total de la cellule (à l’exclusion des répondants anonymes). Ce nombre correspond au nombre de répondants distincts.                                                          |
| WeightedNbRespondents| Nombre pondéré de répondants. Cette variable existe si une variable `weight` est précisée. |
| TotalVar| Valeur totale de la cellule |
| Sensitivity | Sensibilité de la cellule. Lorsque l’instruction `waiver` ou `p_waiver` est précisée, il s’agit de la sensibilité des cellules corrigée en fonction des exemptions.|
| Status | L’état de la cellule. Si la cellule est sensible : `S`. Si la cellule n’est pas sensible : `V`.  |
| Type | Le type de cellule. Si la cellule est une cellule de tableau : `C`. Si la cellule est un agrégat : `A`.  |
| TotalNoise | La quantité totale de protection qu’une cellule peut fournir contre la divulgation résiduelle. Cette variable est requise par `SUPPRESSION`. En l’absence de valeurs négatives ou de pondérations, cette valeur est égale à Total.|
| TotalProxySize|Valeur totale de la cellule pour la variable auxiliaire spécifiée par PROXYSIZE. Cette variable n’est calculée que lorsque l’instruction PROXYSIZE est définie.|
| SensitivityBeforeWaivers | Lorsque `waiver` ou `p_waiver` est précisé, cette variable contient la sensibilité de la cellule avant l’ajustement pour les exemptions.|
| FavCost | Lorsque `waiver` ou `p_waiver` est précisé, cette variable est ajoutée et contient le calcul des coûts (FavCost) liés au recours à des exemptions.|
| ShadowTotal | Si une variable `shadow` est précisée, cette variable est ajoutée et contient la valeur totale de la variable transitoire. |
| {by variables} | Les variables précisées dans le paramètre `by` sont incluses dans l’ensemble de données, le cas échéant. |

#### **outconstraint**

| Variable | Contenu |
| -------- | --------|
| ConstraintId   | Identifiant d’une contrainte (équation linéaire). Entier de 1 à N, où N est le nombre de contraintes définies par G-Confid. |
| CellId         | Identifiant de cellule généré par G-Confid.  |
| Coefficient    | Coefficient d’une cellule dans une contrainte. Si la valeur est = 1, la cellule se trouve du côté gauche de l’équation. Si la valeur est = -1, la cellule est un total. |
| {by variables} | Les variables précisées dans le paramètre `by` sont incluses dans l’ensemble de données, le cas échéant. |

#### **outlargest**

Précise l’ensemble de données de sortie contenant les renseignements sur les principaux contributeurs d’une cellule.

| Variable | Contenu |
| -------- | --------|
| CellId | Identifiant de cellule généré par G-Confid.|
| {unit_id} | La variable précisée dans le paramètre unit_id est incluse dans l’ensemble de données. Les valeurs spéciales `Anon` et `Pool` sont utilisées pour représenter les répondants anonymes et le reste des répondants.|
| NbRespondents | Nombre de répondants ayant contribué au total de la cellule (en excluant les répondants anonymes, mais en incluant les répondants dont les contributions sont nulles). Ce nombre correspond au nombre de répondants distincts. |
| TotalVar | Valeur totale des données du répondant, des répondants anonymes ({unit_id}="Anon") ou du reste des répondants ({unit_id}="Pool")). |
| TotalPercent | Proportion de la valeur totale des cellules que ce répondant représente.|
| WaiverFlag | Lorsque `waiver` ou `p_waiver` est précisé, l’indicateur WaiverFlag de l’ensemble de données d’entrée est ajouté. |
| ShadowTotal  | Si une variable transitoire est précisée, la valeur totale de la variable transitoire pour les répondants, les contributeurs anonymes (`Anon`) ou le reste des répondants (`Pool`). |
| ShadowPercent  |  |
| {by variables} | Les variables précisées dans le paramètre by sont incluses dans l’ensemble de données, le cas échéant. |

#### **outpairs**

Précise l’ensemble de données de sortie contenant des renseignements sur les paires d’observations les plus sensibles dans chaque cellule. Cet ensemble de données ne peut être généré que lorsqu’un `weight` est précisé avec `s_rule="nk"` et une valeur `weight_prot_level` autre que "EXACT".

| Variable | Contenu |
| -------- | --------|
| CellId | Identifiant de cellule généré par G-Confid. |
| TargetId | Identifiant de la cible (FT1 ou FT2), selon la paire produisant une sensibilité plus élevée. L’observation déterminée comme la cible dans la paire cible/attaquant la plus sensible de la cellule. |
| TargetPt  | Valeur du seuil de précision (PT) pour la cible déterminée. |
| AttackerId | L’observation déterminée comme l’attaquant dans la paire cible/attaquant la plus sensible de la cellule. S’il n’y a qu’un répondant dans une cellule, `AttackerId`est vide. |
| AttackerSn | Valeur de bruit propre (SN) pour l’attaquant déterminé. S’il n’y a qu’un répondant dans une cellule, `AttackerSn` est vide. |
| RemainderCount | Nombre d’unités restantes (autres que cible/l’attaquant). S’il y a deux répondants ou moins, `RemainderCount=0.` |
| RemainderN     | Bruit total (N) pour les unités restantes. S’il y a deux répondants ou moins, `RemainderN=0.` |

#### **outtargets**

Précise l’ensemble de données de sortie contenant des renseignements sur la combinaison d’observations la plus sensible dans chaque cellule. Cette option ne peut être générée que lorsqu’un `weight` est précisé avec `s_rule="nk"` et une valeur `weight_prot_level` autre que "EXACT".

| Variable | Contenu |
| -------- | --------|
| CellId | Identifiant de cellule généré par G-Confid. |
| Id | Comprend l’identifiant des n premières observations et la valeur “Remainder“ pour la dernière observation. Comprend l’identifiant des n observations (précisées par le paramètre de règle nk) contenues dans la combinaison de cibles la plus sensible de la cellule, ainsi que la valeur “Remainder“ pour toutes les observations restantes. Si la cellule contient n ou moins d’observations, aucune valeur “Remainder“ n’est incluse. |
| PtnVariable | Précise la variable PTN incluse dans la ligne : seuil de précision (PT) pour les cibles et bruit cumulatif (N) pour les unités restantes.|
| Value | Valeur de la variable précisée dans la colonne PtnVariable correspondante.|

## Suppression

Ce module indique les cellules à supprimer dans un tableau, en plus des cellules sensibles, afin d’empêcher la divulgation de données confidentielles. Les données agrégées sont traitées au moyen d’un résolveur de programmation linéaire en utilisant des contraintes générées par le module de [Sensibilité](#sensitivity).

### Paramètres de suppression

| Paramètre | Type python | Description |
| ----------------| -------------| --------------------------- |
| incell | [Ensemble de données d’entrée](#spécification-des-tableaux-dentrée-et-de-sortie) | Voir [`incell`](#incell) pour plus de détails. |
| inconstraint | [Ensemble de données d’entrée](#spécification-des-tableaux-dentrée-et-de-sortie) | Voir [`inconstraint`](#inconstraint) pour plus de détails.|
| outsuppress | [Ensemble de données de sortie](#spécification-des-tableaux-dentrée-et-de-sortie) | Voir [`outsuppress`](#outsuppress) pour plus de détails.|
| outcomplement | [Ensemble de données de sortie](#spécification-des-tableaux-dentrée-et-de-sortie) | Voir [`outcomplement`](#outcomplement) pour plus de détails.|
| cost_function1      | string | Précise le nom de la fonction de coût à utiliser à la phase 1 du processus de suppression. Valeur par défaut = "Size".|
| cost_function2      | string | Précise le nom de la fonction de coût à utiliser à la phase 2 du processus de suppression. Facultatif.|
| cost_var1           | string | Précise le nom de la variable de coût à utiliser à la phase 1 du processus de suppression. Il doit s’agir d’une variable numérique présente dans l’ensemble de données utilisé pour `incell`. Cette variable doit uniquement contenir des valeurs positives; les valeurs manquantes ne sont pas autorisées. Lorsque ce paramètre est fourni, la variable précisée est utilisée pour calculer les coefficients de fonction de coût, au lieu de `TotalNoise`. |
| cost_var2           | string | Précise le nom de la variable de coût à utiliser à la phase 2 du processus de suppression. Il doit s’agir d’une variable numérique présente dans l’ensemble de données utilisé pour `incell`. Cette variable doit uniquement contenir des valeurs positives; les valeurs manquantes ne sont pas autorisées. Lorsque ce paramètre est fourni, la variable précisée est utilisée pour calculer les coefficients de fonction de coût, au lieu de `TotalNoise`. |
| scale_cost          | string | Précise la méthode utilisée pour réduire les coefficients de fonction de coût. Les valeurs possibles sont `None`, `MEAN` et `SCALE`. Voir [Scale Cost](#scale-cost) pour plus de détails . |
| p1_roundingbase     | int    | Il s’agit de la base d’arrondissement à utiliser conjointement avec la fonction de coût `ROUNDEDSIZE`.|
| sen_roundingbase    | int    | Il s’agit de l’arrondissement effectué pour arrondir les valeurs de sensibilité. Facultatif; des valeurs brutes sont utilisées par défaut. |
| total_roundingbase  | int    | Il s’agit de l’arrondissement utilisé pour arrondir les valeurs `TotalNoise`. Facultatif; des valeurs brutes sont utilisées par défaut. |
| constraint_scale    | int    | Défaut = 2 |
| suppress_order      | int    | Défaut = 0 |
| ambiguity_tolerance | float  | Défaut = 0,00001 |
| by                  | string |
| custom_solver       |pulp.LpSolver|Un solveur PuLP personnalisé optionnel à utiliser à la place de celui fourni par défaut.|
|skip_validation|Booléen|Désactive la validation préalable des champs attendus dans tes tables d’entrée. Valeur par défaut = False.|
| trace               |Booléen ou int | Voir [l’option trace](#niveau-de-verbosité-du-journal-python-trace) |
| capture             |Booléen | Voir [Suppression et dépannage des messages du journal](#suppression-et-dépannage-des-messages-de-journal-capture) |
| logger              |Booléen | Voir [Utilisation de votre propre enregistreur](#utilisation-de-votre-propre-enregistreur-logger) |
<!-- |multiprocess|bool|Enables the use of multiple processors to distribute the work of the solver. Defaults to False. See [Multiprocessing](user_guide.md#Multiprocessing). *NOTE: May be unstable. Does not work with any custom_solver that logs to disk*| -->

#### **Cost Functions**

Les fonctions de coût suivantes sont disponibles.

| Nom de la fonction | Description                                                                     |
| ------------------ | ------------------------------------------------------------------------------- |
| CONSTANT           | Traite toutes les cellules de la même façon (coût = 1 pour toutes les valeurs). |
| SIZE               | Utilise la variable de coût directement.                                        |
| DIGITS             | $\log_{10}(cost\_var+1)$                                                        |
| INFORMATION        | $\frac{\log_{10}(cost\_var+1)}{cost\_var+1}$                                    |
| INVERSE            | Utilise la valeur réciproque de la variable de coût.                            |
| ROUNDEDSIZE        | Arrondissement appliqué à la variable de coût.                                  |
| SCALEDINFORMATION  | $\text{Round}(\text{mean}(cost\_var) * \frac{\log(cost\_var + 1)}{cost\_var+ 1}, 3)$|
| SCALEDINVERSE      | $\text{Round}(\text{mean}(cost\_var) * INVERSE(cost\_var), 3)$|


- *par défaut, la variable de coût est `TotalNoise` (la valeur de la cellule).

#### **Scale Cost**

Le paramètre scale_cost précise la méthode utilisée pour réduire les coefficients de fonction de coût. Les valeurs possibles sont None, `MEAN` et `SCALE`.

Lorsque scale_cost=None, les coefficients de la fonction de coût du problème LP sont utilisés sans modification.

Lorsque scale_cost="SCALE", les coefficients de la fonction des coûts du problème LP sont mis à l’échelle selon l’algorithme suivant :

$$
y = \frac{(b - a)(x - \text{min})}{\text{max} - \text{min}}
$$

où *b* et *a* sont les bornes supérieure et inférieure de l’intervalle de mise à l’échelle, *max* et *min* sont les valeurs maximales et minimales des coefficients de la fonction de coût, *x* est la valeur du coefficient actuel à mettre à l’échelle et *y* est la valeur du coefficient mis à l’échelle.

Lorsque scale_cost="MEAN", les coefficients de la fonction de coût du problème LP sont réduits selon l’algorithme suivant :

$$
y = \frac{\sum_{i=1}^{n} x_i}{n}
$$

où *x* est la valeur du coefficient, *y* est la valeur du coefficient de réduction et *n* est le nombre de coefficients de la fonction de coût.

Ce paramètre est facultatif et la valeur par défaut est None.

### Tableaux d’entrée de la suppression

#### **incell**

Cet ensemble de données contient les cellules réelles du tableau ainsi que les agrégats sensibles. Les variables suivantes sont nécessaires dans cet ensemble de données :

-   CellID
-   TotalNoise
-   Sensitivity
-   Status
-   Type

Pour obtenir de plus amples renseignements sur ces variables, voir [outcell](#outcell) dans les tableaux de sortie du module Sensibilité, qui décrivent ces colonnes.

Veuillez noter que cet ensemble de données nécessite des variables `BY` ou des variables de coût supplémentaires.

#### **inconstraint**

Ensemble de données contenant les coefficients de contraintes linéaires. Voir [outconstraint](#outconstraint) dans les tableaux de sortie du module Sensibilité pour obtenir de plus amples renseignements sur cet ensemble de données.

### Tableaux de sortie de la suppression

#### **outsuppress**

En plus des champs contenus dans l’ensemble de données incell, ce tableau comprend les deux nouveaux champs suivants :

| Variable     | Description                                                                                                                                                                                                                                                                                                                                                                    |
| ------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| OutStatus    | Indique l’état de la cellule. `P` indique les cellules publiables et `X` indique les cellules à supprimer.                                                                                                                                                                                                                                                                         |
| NetVariation | Variation nette en valeur absolue de la cellule, requise pour protéger les cellules sensibles. Une cellule sensible présente toujours une variation nette non nulle. Une variation nette non nulle pour une cellule non sensible indique que cette cellule a été sélectionnée comme complément d’une cellule sensible. Une cellule publiée présente une variation nette nulle. |

#### **outcomplement**

Ensemble de données facultatif. S’il n’est pas indiqué comme devant être enregistré ou créé, aucun tableau ne sera créé. <!-- As well, if `multiprocess=True` and you only provide a `cost_function1` but no `cost_function2` then no complements are able to be produced and therefore no table will be created. -->

| Variable         | Description                                                                                  |
| ---------------- | -------------------------------------------------------------------------------------------- |
| Phase            | Indique la phase du processus de suppression, 1 ou 2.                                        |
| ProtectionNumber | Ordre dans lequel les cellules sensibles ont été protégées                                   |
| CellId           | Identifiant unique de la cellule sensible pour laquelle les compléments sont répertoriés.    |
| ComplementId     | Répertorie les CELLID de tous les compléments de la cellule sensible, pour une phase donnée. |

#### **outsuppress\_failed et outcomplement\_failed**

Nouvelle fonctionnalité dans la version Python de G-Confid. Lorsqu’une itération du résolveur échoue, une erreur est consignée, mais au lieu d’écarter les données des itérations précédentes réussies, celles-ci sont désormais utilisées pour produire les sorties habituelles, lesquelles sont toutefois consignées dans ces deux nouveaux tableaux de sortie. outsuppress\_failed est toujours créé si le résolveur échoue, tandis que outcomplement\_failed est uniquement créée si le tableau outcomplement avait été demandé initialement. Ces deux tableaux sont identiques à leurs équivalents outsuppress et outcomplement; toutefois, ils ne sont créés qu’en cas d’échec du résolveur et ne sont pas produits autrement.

## Audit

Vérifie la validité d’un schéma de suppression. En pratique, l’audit devrait être utilisé lorsque l’on décide de modifier un schéma de suppression fourni par le module de suppression. G-Confid ne déterminant pas nécessairement le meilleur schéma de suppression en ce qui concerne la perte d’information, il peut être utile d’apporter des modifications au schéma de suppression retenu afin de réduire cette perte d’information lorsque des cellules supplémentaires sont supprimées. Toutefois, en apportant ces modifications, on risque de choisir un schéma de suppression qui ne garantit plus la confidentialité des données. Pour vérifier la validité d’un schéma de suppression modifié, on peut utiliser le module d’audit.

### Paramètres d’audit

| Paramètre    | Type python                   | Description                                                                                                                                                 |
| ------------ | ----------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| incell       | [Ensemble de données d’entrée](#spécification-des-tableaux-dentrée-et-de-sortie)|                                                                                                                                                             |
| inconstraint | [Ensemble de données d’entrée](#spécification-des-tableaux-dentrée-et-de-sortie)|                                                                                                                                                             |
| outaudit     | [Ensemble de données de sortie](#spécification-des-tableaux-dentrée-et-de-sortie)|                                                                                                                                                             |
| lb_factor    | float                         | Valeur numérique du facteur de borne inférieure, comprise entre 0 et 1 inclusivement. Facultatif; valeur par défaut = 0,5.                                  |
| ub_factor    | float                         | Valeur numérique du facteur de borne supérieure, comprise entre 1 et 10 inclusivement. Facultatif; valeur par défaut = 1,5.                                 |
| use_shuttle  | Booléen    |Utilise l’algorithme shuttle pour calculer les bornes inférieure et supérieure de tes cellules. Valeur par défaut = False.|
| report_level | int                           | Détermine si un rapport est généré à la fin du traitement. Indiquer 1 pour générer le rapport, 0 pour ne pas en générer. Facultatif, valeur par défaut = 0. |
| by           | str                           | Liste de noms de variables séparés par des espaces, utilisés pour créer par groupes By.                                                                     |
| custom_solver| pulp.LpSolver|Solveur PuLP personnalisé optionnel à utiliser à la place du solveur par défaut. |
|multiprocess|Booléen|Active le multiprocessing pour paralléliser l’exécution du solveur. Valeur par défaut = False. See [Multiprocessing](user_guide.md#multiprocessing). *NOTE: Peut être instable. Ne fonctionne pas avec un custom_solver qui écrit des logs sur disque. *|
|skip_validation|Booléen|Disables the pre-process validation of expected fields on your input tables. Valeur par défaut = False.|
| trace               |Booléen ou int | Voir [l’option trace](#niveau-de-verbosité-du-journal-python-trace) |
| capture             |Booléen | Voir [Suppression et dépannage des messages du journal](#suppression-et-dépannage-des-messages-de-journal-capture) |
| logger              |Booléen | Voir [Utilisation de votre propre enregistreur](#utilisation-de-votre-propre-enregistreur-logger) |

### Tableaux d’entrée de l’audit

#### **`incell` pour l’audit**

Cet ensemble de données aura la même structure que [outsuppress](#outsuppress) du module de suppression.

#### **`inconstraint` pour l’audit**

Cet ensemble de données aura la même structure que [outconstraint](#outconstraint) du module de sensibilité.

### Tableaux de sortie de l’audit

#### **outaudit**

L’ensemble de données de sortie du module d’audit contient toutes les colonnes de l’ensemble de données d’entrée `incell`, plus les colonnes suivantes :

| Variable         | Description                                                                                                                                                                                                                                                                                                                                      |
| ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| LBound           | Valeur de la borne inférieure de la cellule (lb_factor*TotalNoise).                                                                                                                                                                                                                                                                              |
| LTolerance       | Tolérance inférieure de la cellule (Total–Sensitivity/2).                                                                                                                                                                                                                                                                                        |
| MinValue         | Valeur minimale de la cellule, calculée par le résolveur LP (devrait être comprise entre LBound et TotalNoise).                                                                                                                                                                                                                                  |
| MidPoint         | Valeur médiane de la cellule ((MinValue+MaxValue)/2).                                                                                                                                                                                                                                                                                            |
| UTolerance       | Tolérance supérieure de la cellule (Total+sensibilité/2).                                                                                                                                                                                                                                                                                        |
| MaxValue         | Valeur maximale de la cellule, calculée par le résolveur LP (devrait être comprise entre TotalNoise et UBound).                                                                                                                                                                                                                                  |
| UBound           | Valeur de la borne supérieure de la cellule (ub_factor*TotalNoise).                                                                                                                                                                                                                                                                              |
| IntervalWidth    | Largeur d’intervalle de la cellule en pourcentage (100*( MaxValue - MinValue)/Total).                                                                                                                                                                                                                                                            |
| MaxMinusMin      | Différence entre la valeur maximale et la valeur minimale.                                                                                                                                                                                                                                                                                       |
| ProblemIndicator | Indicateur de problème pour la cellule. Pour les cellules ou les agrégats sensibles, la valeur est 0 pour une `protection adéquate`, 1 pour une `non atteinte` et 2 pour une `divulgation exacte`. Pour les compléments, la valeur est 0 pour `good protection` et 2 pour `exact disclosure`. |

## Arrondissement optimisé

Ce module est utilisé pour arrondir des tableaux multidimensionnels. À partir d’un ensemble de données d’entrée, le système tente de produire un tableau arrondi ayant la même structure que le tableau initial et aussi proche que possible de celui-ci. Les valeurs arrondies sont additives et contrôlées; le contrôle est maintenu en évitant d’arrondir de plus d’un multiple de la base au-delà de la valeur initiale. L’arrondissement sert à protéger les fréquences en créant une incertitude quant aux valeurs exactes.

### Paramètres d’arrondissement optimisé

| Paramètre      | Type python                   | Description                                           |
| -------------- | ----------------------------- | ----------------------------------------------------- |
| incell         | [Ensemble de données d’entrée](#spécification-des-tableaux-dentrée-et-de-sortie)||
| additive_con   | [Ensemble de données d’entrée](#spécification-des-tableaux-dentrée-et-de-sortie)||
| additive_bound | [Ensemble de données d’entrée](#spécification-des-tableaux-dentrée-et-de-sortie)||
| outround       | [Ensemble de données de sortie](#spécification-des-tableaux-dentrée-et-de-sortie)||
| base           | int                           | Base d’arrondissement à utiliser.                     |
|multiprocess|Booléen|Active le multiprocessing pour paralléliser l’exécution du solveur. Valeur par défaut = False. See [Multiprocessing](user_guide.md#multiprocessing). *NOTE: Peut être instable. Ne fonctionne pas avec un custom_solver qui écrit des logs sur disque. *|
|skip_validation|Booléen|Disables the pre-process validation of expected fields on your input tables. Valeur par défaut = False.|
| trace               |Booléen ou int | Voir [l’option trace](#niveau-de-verbosité-du-journal-python-trace) |
| capture             |Booléen | Voir [Suppression et dépannage des messages du journal](#suppression-et-dépannage-des-messages-de-journal-capture) |
| logger              |Booléen | Voir [Utilisation de votre propre enregistreur](#utilisation-de-votre-propre-enregistreur-logger) |

### Tableaux d’entrée de l’arrondissement optimisé

#### **`incell` pour l’arrondissement optimisé**

| Variable | Description                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| CellId   | Identifiant unique de chaque cellule. Cette colonne clé doit être unique pour chaque ligne. Elle est obligatoire et doit contenir des nombres réels.                                                                                                                                                                                                                                                                                                    |
| Total    | Valeur de la cellule à arrondir. Elle est obligatoire et doit contenir des nombres réels. Si aucune colonne de poids n’est fournie, tous les poids seront établis par défaut à 1,0. Les cellules ayant un poids plus élevé sont plus susceptibles d’être arrondies à la valeur la plus proche multiple de la base d’arrondissement lorsque les contraintes d’additivité de la solution sont assouplies (bornes de contrainte manquantes ou non nulles). |
| Weight   | Poids appliqué par l’algorithme d’arrondissement. Il est obligatoire et doit contenir des nombres réels positifs.                                                                                                                                                                                                                                                                                                                                       |
| CellLB   | Valeur de la borne inférieure de la cellule, utilisée pour forcer l’arrondissement d’une cellule à la hausse. Pour chaque ligne, la valeur de CellLB doit être inférieure ou égale à la valeur de CellUB (sauf si l’une des deux est manquante). Cette colonne est facultative et, si elle n’est pas fournie, toutes les valeurs seront considérées comme manquantes par défaut.                                                                        |
| CellUB   | Valeur de la borne supérieure de la cellule, utilisée pour arrondir une cellule à la baisse. Pour chaque ligne, la valeur de CellUB doit être inférieure ou égale à la valeur de CellLB (sauf si l’une des deux est manquante). Cette colonne est facultative et, si elle n’est pas fournie, toutes les valeurs seront considérées comme manquantes par défaut.                                                                                         |

#### **additive\_con**

Cet ensemble de données a la même structure que [outconstraint](#outconstraint) du module de sensibilité.

#### **additive\_bound**

| Variable     | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| ------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| ConstraintId | Identifiant unique pour chaque contrainte. Obligatoire.                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| ConstraintLB | La borne inférieure de la contrainte utilisée pour assouplir l’exigence d’additivité de certains totaux. Pour chaque ligne, la valeur de ConstraintLB doit être inférieure ou égale à la valeur de ConstraintUB. Si la colonne ConstraintLB n’est pas fournie, une valeur de zéro est utilisée comme borne inférieure pour chaque cellule (ce qui indique que le Total ne peut pas être inférieur à la SOMME). Facultatif; peut contenir des nombres réels ou des valeurs manquantes.    |
| ConstraintUB | Borne supérieure de la contrainte, utilisée pour assouplir l’exigence d’additivité de certains totaux. Pour chaque ligne, la valeur de ConstraintUB doit être supérieure ou égale à la valeur de ConstraintLB. Si la colonne ConstraintUB n’est pas fournie, une valeur de zéro est utilisée comme borne inférieure pour toutes les cellules (ce qui indique que le Total ne peut pas être supérieur à la SOMME). Facultatif; peut contenir des nombres réels ou des valeurs manquantes. |

### Tableaux de sortie de l’arrondissement optimisé

#### **outround**

| Variable               | Description                                                                                                              |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------ |
| CellId                 | Identifiant unique de la cellule du tableau.                                                                             |
| UpperResidual          | Upper residual. Contient des nombres réels compris entre 0 et 1.                                                         |
| PositiveShiftIndicator | Positive_shift_indicator. Contient 0 ou 1.                                                                               |
| PositiveBaseShift      | Positive_base_shift. Contient des entiers positifs.                                                                      |
| PositiveShift          | Positive_shift. Contient des nombres réels positifs.                                                                     |
| LowerResidual          | Lower_residual. Contient des nombres réels compris entre 0 et 1.                                                         |
| NegativeShiftIndicator | Négative_shift_indicator. Contient 0 ou 1.                                                                               |
| NegativeBaseShift      | Négative_base_shift. Contient des entiers positifs.                                                                      |
| NegativeShift          | Négative_shift. Contient des nombres réels positifs.                                                                     |
| ResidualIndicator      | Contient 0 ou 1.                                                                                                         |
| Shift                  | Différence entre la valeur arrondie et non arrondie.                                                                     |
| AbsoluteShift          | Valeur absolue de la différence entre la valeur arrondie et la valeur non arrondie. Contient des nombres réels positifs. |
| CellLowerBound         | Contient des nombres réels ou des valeurs manquantes.                                                                    |
| CellUpperBound         | Contient des nombres réels ou des valeurs manquantes.                                                                    |
| Weight                 | Contient des nombres réels positifs.                                                                                     |
| Total                  | Valeur totale initiale.                                                                                                  |
| RoundedTotal           | Total après application de l’arrondissement.                                                                             |

## Guide technique

### Exécution des modules G-Confid

Pour exécuter une procédure GConfid dans un script Python, nous importons d’abord le progiciel G-Confid ainsi que tous les autres progiciels que nous prévoyons utiliser :

```python
import gconfid  
import pandas as pandas
```

Lors de l’exécution d’un module G-Confid, nous créons un nouvel objet avec le nom de notre choix (dans cet exemple, "my_sens_result") :

```python
my_sens_result = gconfid.sensitivity(
    indata=indata,
    outcell="pandas,
    outconstraint="pandas,
    outlargest="pandas,
    hierarchy="""Tot_Reg R1 R2;
                Tot_Cla C1 C2 C3;""",
    s_rule="pq 0.2",
    proxy_ratio=0.2,
    proxy_diag=True,
    accept_negative=True,
    unit_id="ident",
    var="val",
    dimension="reg cla",
    proxy_size="proxyvar",
    p_waiver="WaiverFlag",
)
```

-   Les modules doivent être appelés en utilisant le nom du progiciel G-Confid, c.-à-d.`gconfid.sensitivity()`.
-   Les paramètres (p. ex.`hierarchy`) et les tableaux (p. ex. `indata`) sont définis sous forme de paires de clé-valeur séparées par des virgules et peuvent apparaître dans n’importe quel ordre.

Les sorties peuvent être conservées en mémoire ou enregistrées sur disque (voir [cette section](#spécification-de-tableaux-de-sortie) pour plus de détails). Si elles sont conservées en mémoire, elles sont stockées dans l’objet créé par l’utilisateur (p. ex. `my_sens_result` lors de l’exécution de la procédure :

```python
print(my_sens_result.outlargest) # Affiche le tableau outlargest  
sens_outlargest = my_sens_result.outlargest # Enregistre outlargest en tant que nouveau tableau PyArrow appelé « sens_outlargest »
```

### Noms des variables dans les tableaux d’entrée

Les paramètres G-Confid font référence à des variables (c.-à-d. des colonnes) provenant des tableaux d’entrée, comme **`indata`**. Toute variable référencée par un module G-Confid doit être composée d’une seule chaîne de caractères, sans espaces ni caractères spéciaux, à l’exception du trait de soulignement (\_). Par exemple, "first_name" est un nom de variable acceptable, tandis que "first name" et "first-name$" ne le sont pas. Pour s’assurer la compatibilité des tableaux d’entrée avec G-Confid, les utilisateurs peuvent devoir modifier les noms de variables. Contraintes supplémentaires :

-   Les noms de variables ne doivent pas dépasser 64 caractères.
-   Les noms des variables doivent être uniques au sein d’un même tableau d’entrée. (La même variable peut apparaître dans plusieurs tableaux d’entrée sans problème.)

Les paramètres qui font référence à des variables comprennent notamment `by`, `var`, `weight` et `cost_var1`. Ils sont simplement indiqués par leur nom de variable, et les listes de deux variables ou plus sont séparées par un seul espace. Par exemple :

-   Variable unique _weight = "wgt"_
-   Liste de variables : _by = "province industry_"

Veuillez noter que les noms de variables ne sont pas sensibles à la casse dans G-Confid; si un tableau d’entrée contient deux colonnes ou plus qui ne diffèrent que par la casse, il n’est pas clair quelle colonne sera utilisée lors du traitement. (Il est fortement recommandé aux utilisateurs d’attribuer des noms distincts — insensibles à la casse — à toutes les colonnes des tableaux d’entrée.)

### Journal G-Confid

Les messages de journal sont, par défaut, écrits dans le terminal pendant l’exécution. Ces messages proviennent de deux sources : le code Python (applicable à tous les modules) et code C (applicable uniquement au module de sensibilité). Les messages peuvent être affichés en français ou en anglais.

#### **Configuration de la langue du journal**

Le progiciel _g-confid_ produit des messages de journal en anglais ou en français. Il tente de détecter la langue à partir de son environnement hôte lors de l’importation. Utilisez la fonction _g-confid.set\_language()_ pour définir la langue au moment de l’exécution, en indiquant un membre de l’énumération gconfid.SupportedLanguage (c.-à-d.en ou .fr).

> **Exemple** : réglage de la langue à Français
> 
> ```python
> import gconfid
> gconfid.set_language(gconfid.SupportedLanguage.fr)
> res = gconfid.Auditing(...)
> ```

#### **Messages de journal Python**

Python gère la journalisation à l’aide du module normalisé [_logging_](https://docs.python.org/3/library/logging.html). Tous les messages du journal Python ont un niveau de journal ([_log level_](https://docs.python.org/3/library/logging.html#logging-levels)) associé, comme _ERROR_ (erreur), _WARNING_ (avertissement), _INFO_ (information) et _DEBUG_ (dégogage). Les messages provenant de Python sont généralement d’une seule ligne et sont précédés d’un horodatage et du niveau.

Par défaut, seuls les messages d’avertissement et d’erreur s’affichent. Utilisez le paramètre trace pour modifier les niveaux de journal affichés.

##### **Niveau de verbosité du journal Python (trace=)**

Utilisez le paramètre trace pour contrôler les niveaux de journal s’affichant en précisant l’un des niveaux de journal suivants :

-   gconfid.log\_level.ERROR
-   gconfid.log\_level.WARNING
-   gconfid.log\_level.INFO
-   gconfid.log\_level.DEBUG

Les messages correspondant au niveau de journal précisé et aux niveaux supérieurs s’afficheront; les niveaux inférieurs ne le seront pas.

Par souci de commodité, définir _trace=True_ active l’ensemble des messages de journal.

#### **Suppression et dépannage des messages de journal (capture=)**

Le paramètre capture peut être utilisé pour supprimer toute sortie de journal (en utilisant capture=None). Cette option est désactivée par défaut (capture=Faux).

Préciser Capture=True fera en sorte que les messages de journal s’affichent en bloc à la fin de l’exécution de la procédure, plutôt qu’immédiatement tout au long de l’exécution. Cette différence peut n’être perceptible que pour des procédures longues. Cette option peut améliorer le rendement dans certains cas, comme lors du traitement d’un très grand nombre de groupes BY.

> _Notebooks Jupyter et messages de journal manquants_  
> Lors de l’exécution du module de sensibilité dans des notebooks Jupyter, les messages de journal générés par le code C peuvent être absents, en particulier lors de l’utilisation du code Visual Studio sous Windows.  
> Pour corriger ce problème, essayez d’utiliser l’option **_capture=True_**.
> 
> -   Vous pouvez également [utiliser votre propre enregistreur](#utilisation-de-votre-propre-enregistreur-logger).

En raison de la façon dont les notebooks Jupyter gèrent les sorties terminal de Python, les messages provenant de procédures en C peuvent ne pas s’afficher. Pour résoudre ce problème, préciser capture=True dans l’appel de procédure lors de l’exécution dans un notebook Jupyter.

#### **Utilisation de votre propre enregistreur (logger=)**

Utilisez le paramètre logger pour préciser un [**enregistreur**](https://docs.python.org/3/library/logging.html#logger-objects) que vous avez créé. Tous les messages provenant de Python et de C seront envoyés à cet enregistreur. Cela permet de personnaliser les préfixes des messages, d’écrire dans un fichier, etc.

Veuillez noter que les messages issus des procédures en C sont envoyés à l’enregistreur sous la forme d’un seul message Python de niveau _INFO_.

> **_Exemple_** : écriture de journaux dans un fichier
> 
> ```python
> import gconfid
> import logging
> my_logger = logging.getLogger(__name__)
> logging.basicConfig(filename='example.log', encoding='utf-8', level=logging.DEBUG)
> 
> sensitivity_call = gconfid.sensitivity(
>   logger=my_logger,
>    indata=indata,
>    outlargest=True,
> ...
> ```

### Spécification des tableaux d’entrée et de sortie

Pour les tableaux d’entrée comme pour les tableaux de sortie, les utilisateurs peuvent préciser des objets en mémoire ou des fichiers sur disque. Plusieurs formats sont pris en charge pour les deux types. Les objets sont associés à des identifiants (p. ex. « pandas dataframe ») tandis que les fichiers sont associés à des extensions (p. ex.« filename.parquet »); voir le tableau ci-dessous pour plus de détails. Il convient de noter que certains sont recommandés à des fins d’essai seulement et que tous les formats ne sont pas pris en charge pour les tableaux de sortie.

#### **Formats pris en charge**

| Format                            | Type    | Identifiants ou extensions pris en charge       | Notes                                                                             |
| --------------------------------- | ------- | ----------------------------------------------- | --------------------------------------------------------------------------------- |
| Tableau PyArrow                   | Objet   | `"pyarrow"`, `"table"`, `"pyarrow table"`       | Format recommandé pour les objets en mémoire.                                     |
| Fiche de données Pandas           | Objet   | `"pandas"`, `"dataframe"`, `"pandas dataframe"` |                                                                                   |
| Apache Parquet                    | Fichier | `.parquet`, `.parq`                             | Utilisation minimale de RAM, bon rendement avec de grands tableaux.               |
| Apache Arrow IPC                  | Fichier | `.arrow`                                        | Utilise le moins de RAM et offre de bonnes performances avec de grandes tables.   |
| Apache Feather                    | Fichier | `.feather`                                      | Utilisation de RAM la plus faible, bon rendement avec de grands tableaux.         |
| Ensemble de données SAS           | Fichier | `.sas7bdat`                                     | À des fins d’essai uniquement, en entrée seulement; non recommandé en production. |
| Valeurs séparées par des virgules | Fichier | `.csv`                                          | À des fins d’essai uniquement; non recommandé en production.                      |

Pour obtenir des conseils relatifs aux chemins d’accès aux fichiers en Python, voir [Caractères d’échappement et chemins d’accès aux fichiers](#caractères-déchappement-et-chemins-daccès-aux-fichiers).

#### **Spécification des tableaux d’entrée**

Pour utiliser un objet en mémoire en entrée, il suffit de faire référence au nom de l’objet directement dans l’appel de procédure. Cette procédure permet de détecter automatiquement le type d’objet parmi les types pris en charge.

```python
    suppression_call = gconfid.suppression(
        incell=df, # where df is a Pandas DataFrame previously generated
        incontraint=table, # where table is a PyArrow Table previously generated
        ... # etc. (parameters, output tables)
        )
```

Pour préciser une entrée à partir du fichier, indiquez un chemin d’accès au fichier relatif ou complet :

```python
    suppression_call = gconfid.suppression(
        incell="./input_data.parquet", # Parquet file with local reference
        incontraint=r"C:\temp\input_constraints.feather", # Feather file with Windows reference
        ... # etc. (parameters, output datasets)
        )
```

Les utilisateurs peuvent également combiner les deux types d’entrées :

```python
    suppression_call  = gconfid.suppression(
        incell="./input_data.parquet", # Parquet file with local reference
        incontraint=table, # where table is a PyArrow Table previously generated
        ... # etc. (parameters, output tables)
        )
```

#### **Spécification de tableaux de sortie**

Les procédures Banff créent automatiquement plusieurs tableaux de sortie. Certaines sont facultatives et peuvent être désactivées en indiquant False. (Indiquer False pour une sortie facultative, empêchera sa production; ce qui peut réduire l’utilisation de la mémoire. Indiquer False pour une sortie obligatoire entraînera une erreur.) Le format par défaut des tableaux de sortie est Pandas DataFrame en mémoire. Pour produire la sortie dans un autre format en mémoire, indiquez l’_identifiant_ correspondant sous forme de chaîne. Pour écrire les sorties dans des fichiers, indiquer un chemin d’accès au fichier avec une _extension_ prise en charge.

Consultez le [tableau des formats pris en charge](#formats-pris-en-charge) pour obtenir une liste des identifiants et extensions.

L’exemple suivant comprend des sorties obligatoires et facultatives, enregistrées sous forme de combinaison d’objets en mémoire et de fichiers.

```python
    result = gconfid.sensitivity(
        indata=my_micro_data,
        outcell=True, # Mandatory output saved as a PyArrow Table (due to defaults)
        outconstraint="pandas", # Mandatory output saved as a Pandas DataFrame
        outlargest="./out_largest.parquet", # Optional output, saved as a parquet file
        outpairs=False, # Optional output disabled
        outtargets="pandas" # Optional output saved as a Pandas DataFrame
        ... # etc. (parameters, output tables)
        )
```

**REMARQUE : Les sorties remplaceront automatiquement les objets et fichiers existants portant le même nom.**

#### **Personnaliser la spécification de sortie par défaut**

Pour déterminer le format actuel du tableau de sortie global par défaut :

```python
>>> gconfid.get_default_output_spec()
'pyarrow'
```

-   cela correspond à **`pyarrow.Table`**

La valeur par défaut peut être définie à l’aide de n’importe quel _identifiant_ figurant dans le [tableau des formats pris en charge](#formats-pris-en-charge).

> **Exemple** : remplacer le format de sortie par défaut par `pandas.DataFrame`
>
> ```python
> gconfid.set_default_output_spec('pandas')
> ```

Pour définir des formats de sortie par défaut propres à une procédure (proc), la même fonction peut être appelée sur les modules de procédure individuels.

> **Exemple** : changer le format de sortie par défaut uniquement pour la sensibilité vers `pandas.DataFrame`
>
> ```python
> gconfid.sensitiv.set_default_output_format('pandas')
> ```

Le paramétrage le plus détaillé a préséance en cas de conflit de format. Si un tableau particulier d’une procédure est demandé dans un certain format, les valeurs par défaut au niveau de la procédure et au niveau global sont ignorées pour ce tableau. De même, si un format de sortie par défaut propre à une procédure est précisé, il remplacera le format de sortie par défaut global pour cette procédure seulement; sinon, la valeur globale est utilisée.

#### **Accès aux tablauxs de sortie**

Pour les objets enregistrés en mémoire, on peut y accéder à l’aide des attributs de l’objet correspondant aux tableaux de sortie :

```python
    result = gconfid.sensitivity(
        indata=my_micro_data,
        outcell=True,
        outconstraint=True, 
        outlargest="pandas",
        ... # etc.
        )
    print(result.outcell) # Print outcell to the terminal
    my_table = result.outconstraint # Save outconstraint as a new object called my_table
```

*Remarque : `outcell` et `outconstraint` étant des sorties obligatoires, ils demeurent accessibles sous_ `result.outcell` et `result.outconstraint` même s’ils ne sont pas explicitement précisés au moyen d’instructions **`True`**.*

### Configuration du résolveur

Les modules de suppression, d’audit et d’arrondissement optimisé utilisent résolveur de problèmes linéaires. Le progiciel PULP sert d’interface générique pour divers résolveurs, tant en source ouverte que commerciaux. G-Confid fera appel au résolveur par défaut de PULP `pulp.LpSolverDefault`,  que nous préconfigurons pour utiliser le résolveur [`HiGHS`](https://highs.dev/). Consulte [la documentation PuLP](https://coin-or.github.io/pulp/technical/solvers.html#pulp.apis.HiGHS). Cela exige que le paquet `highspy` soit installé — il fait partie des dépendances de G‑Confid et devrait donc être présent automatiquement si vous installez G‑Confid via pip ou un autre gestionnaire de paquets. Si, pour une raison quelconque, G‑Confid est installé sans accès à `highspy`, le solveur PULP_CBC_CMD fourni avec PuLP sera utilisé à la place. Vous pouvez également choisir de configurer votre propre résolveur personnalisé et l’utiliser au lieu du résolveur par défaut lors de l’appel du module souhaité.

```python
import pulp
  
# Affiche la liste des résolveurs disponibles  
solver_list = pulp.listSolvers(onlyAvailable=True)
print(solver_list) 
  
# SCIP_PY est un autre résolveur en source ouverte pouvant être utilisé s’il est installé dans votre environnement  
my_solver = pulp.SCIP_PY(mip=False, msg = True, logPath="c:/temp/scip.log", timeLimit=600)
  
res = gconfid.suppression(
    incell=df,
    incontraint=table,
    custom_solver=my_solver, # Provide the solver we want to use instead of the default
    ... # etc. (parameters, output tables)
  )
```

Les options disponibles varient selon le résolveur. La documentation des options propres à chaque résolveur est disponible [ici](https://coin-or.github.io/pulp/technical/solvers.html).

#### **SAS avec PULP**

À partir de la version 3 de PULP, SAS 9.4 et SAS Viya sont désormais disponibles comme résolveurs. Par conséquent, G-Confid est en mesure d’utiliser SAS, si l’utilisateur dispose d’une installation SAS sur son système ou d’un accès à un serveur SAS.

Lors de l’utilisation du résolveur SAS94, le progiciel saspy est utilisé et doit donc être installé et configuré. Une fois configuré, le résolveur SAS94 devrait être disponible dans PULP.

La configuration de saspy comprend la création d’un fichier sascfg\_personal.py dans le chemin de recherche Python. Si SAS est installé localement dans un environnement Windows, le contenu du fichier pourrait être le suivant :

```python
SAS_config_names=['default']
default = {
    'provider': 'sas.iomprovider',
    'encoding': 'windows-1252'}
```

Une fois saspy installé et configuré, le résolveur par défaut de PULP peut être défini comme SAS94 :

```python
import pulp

solver = pulp.apis.SAS94(msg=True)
pulp.LpSolverDefault = solver
```

Pour obtenir de plus amples renseignements sur la configuration de saspy, veuillez consulter sa [documentation](https://sassoftware.github.io/saspy/install.html). SAS Viya peut également être utilisé; il repose sur la bibliothèque swat.

### Autres

#### **Validation des données d’entrée**

Par défaut, lors de l’exécution d’un module G-Confid, les noms de colonnes de vos tableaux de données d’entrée sont normalisés quant à leur casse (majuscules/minuscules) selon le format attendu, et leurs colonnes ainsi que leur contenu sont validés. Un schéma normalisé est utilisé pour toutes les colonnes obligatoires de chaque fichier d’entrée, tandis que les noms des champs fournis comme paramètres pour chaque module sont ajoutés de façon dynamique au schéma normalisé. Par conséquent, la casse des noms de champs passés en paramètres (c.-à-d. variables BY) doit correspondre exactement à celle utilisée dans le fichier de données où ils sont attendus. La validation et la normalisation peuvent être désactivées en passant le paramètre **_skip\_validation=True_** lors de l’appel du module concerné.

#### **Caractères d’échappement et chemins d’accès aux fichiers**

Dans Windows, le caractère barre oblique inverse (`\`) est généralement utilisé pour séparer les dossiers et les fichiers dans un chemin d’accès.

-   Exemple `"C:\users\stc_user\documents\dataset.csv"`

Dans Python, toutefois, le caractère `\` est un *"caractère d’échappement*" et reçoit un traitement particulier. Fournir un chemin d’accès de fichier dans l’exemple ci-dessus peut entraîner une erreur lors de l’exécution. Pour désactiver ce comportement, utilisez une chaîne brute "*raw string*" en ajoutant le préfixe r :

- `r"C:\users\stc_user\documents\dataset.csv"`

Autres solutions :

-   doubles barres obliques inverses : `C:\\users\\stc_user\\documents\\dataset.csv`
-   barres obliques normales : `C:/users/stc_user/documents/dataset.csv`

#### **Erreurs et exceptions**

En Python, les erreurs d’exécution sont généralement gérées en "levant une exception". Cette approche est adoptée par le progiciel gconfid. Chaque fois qu’une erreur se produit, une exception est levée. Cela peut se produire lors du chargement ou du prétraitement des données d’entrée, de l’exécution d’une procédure ou de l’écriture des données de sortie.

En général, les exceptions contiennent un message d’erreur utile. Elles sont souvent "chaînées" pour fournir un contexte supplémentaire.

#### **Utilisation des fichiers SAS en Python**

Le progiciel G-Confid fournit plusieurs fonctions utiles pour lire des fichiers SAS en mémoire ou les convertir dans un autre format.

Pour utiliser ces fonctions, votre programme doit importer gconfid `import gconfid`.

| Function                                                         | Description                                                                                 |
| ---------------------------------------------------------------- | ------------------------------------------------------------------------------------------- |
| `gconfid.io_util.SAS_file_to_arrowipc_file(file_path, destination)`| Lit un ensemble de données SAS situé à `file_path` et l’écrit au format *Arrow IPC* à destination |
| `gconfid.io_util.SAS_file_to_feather_file(file_path, destination)` | Lit un ensemble de données SAS situé à `file_path` et l’écrit au format *feather* à destination |
| `gconfid.io_util.SAS_file_to_parquet_file(file_path, destination)` | Lit un ensemble de données SAS situé à `file_path` et l’écrit au format *parquet* à destination |
| `gconfid.io_util.DF_from_sas_file(file_path)`                      | Lit un ensemble de données SAS situé à `file_path` et le retourne sous forme *pandas.DataFrame* |
| `gconfid.io_util.PAT_from_sas_file(file_path)`                     | Lit un ensemble de données SAS situé à `file_path` et le retourne sous forme *pyarrow.Table*    |

#### **Considérations relatives au rendement**

Les formats utilisés pour les ensembles de données d’entrée et de sortie ont une incidence sur le rendement.

En cas de de mémoire vive (RAM) insuffisante (en raison d’une faible capacité ou des grands ensembles de données), les données doivent être stockées sur disque. Le format de fichier choisi a un effet sur le rendement. Les formats Apache Parquet (`.parquet`), Apache Arrow IPC (`.arrow`) et Apache Feather (`.feather`) offrent actuellement le meilleur rendement lors de l’utilisation de fichiers pour des ensembles de données d’entrée ou de sortie.

Feather utilise généralement le moins de mémoire vive; ce qui le rend idéal pour de grands ensembles de données ou des environnements d’exécution disposant de peu de RAM; il est recommandé pour les fichiers temporaires. Parquet offre généralement la plus petite taille de fichier, tout en fournissant un excellent rendement de lecture et d’écriture dans des environnements multiprocesseurs et utilisation raisonnablement faible de mémoire vive; il est recommandé pour le stockage de données à moyen et long terme.

L’utilisation du format d’ensemble de données SAS pour de grands ensembles de données d’entrée peut entraîner une dégradation du rendement, en particulier dans les environnements avec peu de RAM. Ce format est uniquement recommandé pour de petits ensembles de données (moins de quelques centaines de Mo). De manière générale, l’utilisation du format SAS est déconseillée, au profit des formats Apache Arrow (_parquet_ et _feather_).

#### Multitraitement

Pour le moment, Audit est la seule procédure offrant une option `multiprocessing`. Cette option est définie à `False` par défaut. Si ce paramètre est réglé sur `True`, G‑Confid tentera d’utiliser les processeurs logiques qui lui sont disponibles (`multiprocessing.cpu_count() - 1`) afin de paralléliser la résolution des problèmes de Programmation Linéaire nécessaires à son fonctionnement. L’utilisation de cette option sur les systèmes Linux (comme la Zone) ne devrait nécessiter aucune modification supplémentaire de vos scripts existants, mis à part définir le nouveau paramètre `multiprocess` à`True`. Pour utiliser cette fonctionnalité sur les machines **Windows**, en plus de l’activation du paramètre, votre script appelant G‑Confid doit être légèrement modifié en ajoutant ce que l’on appelle un guard à la fin du fichier, comme illustré ici :

<table>
<tr>
<th>Monoprocessus</th>
<th>Multiprocessus</th>
</tr>
<tr>
<td>

```python
import gconfid

f = gconfid.Auditing(
    incell= "suppresseddata.sas7bdat",
    inconstraint = "inconstraints.sas7bdat",
    ...
)

print(f.outaudit)
```

</td>
<td>

```python
import gconfid

def run_audit():
    f = gconfid.Auditing(
        incell= "suppresseddata.sas7bdat",
        inconstraint = "inconstraints.sas7bdat",
        multiprocess = True
        ...
    )

    print(f.outaudit)

if __name__=="__main__":
    run_audit()
```

</td>
</tr>
</table>