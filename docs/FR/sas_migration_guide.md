# Guide de migration du progiciel SAS G-Confid

## Dans ce guide

Le public cible de ce guide est constitué des utilisateurs de G-Confid basé sur SAS, version 1.07.003 ou antérieure, qui migrent vers G-Confid basé sur Python, version 2.0.0b4 ou ultérieure.
Il résume les changements apportés à la version 2, y compris les nouveaux noms des paramètres et des tableaux, ainsi que des exemples de la façon dont les programmes SAS faisant appel à G-Confid peuvent être convertis en programmes Python équivalents.
Le présent document ne s’adresse pas aux *nouveaux* utilisateurs de G-Confid, car il ne fournit pas d’aperçu complet de G-Confid 2.x. Veuillez consulter le [guide de l’utilisateur](user_guide.md#aperçu) pour obtenir tous les détails sur l’utilisation de chaque module, ses paramètres et ses tableaux.

## Table des matières

- [Modifications générales](#modifications-générales)
- [Sensibilité](#sensibilité)
- [Suppression](#suppression)
- [Audit](#auditing)
- [Arrondissement optimisé](#arrondissement-optimisé)

## Modifications générales

- Les modules audit (vérification), suppress (suppression) et opt_round (arrondissement optimisé) utilisent un résolveur de problèmes linéaire. Dans la version SAS de G-Confid, la procédure `Proc OptModel` était utilisée. 
Dans la version Python, un progiciel Python générique appelé `PuLP` est utilisé pour servir d’interface avec divers résolveurs. Par défaut, PuLP est configuré pour utiliser le solveur `HiGHS`; celui-ci n'est pas inclus avec PuLP, mais fait partie du paquet `highspy` qui devrait être installé avec G-Confid. PuLP peut également utiliser d'autres solveurs commerciaux ou à code ouvert. Pour plus d'informations sur PuLP, consultez la [configuration du résolveur](user_guide.md#configuration-du-résolveur) et la [documentation de PuLP](https://pypi.org/project/PuLP/).
-Journalisation: dans la version SAS, l’information était écrite dans le journal SAS, qui pouvait être redirigé vers un fichier. Dans Python, des enregistreurs (loggers) sont utilisés. Les modules G-Confid acceptent un enregistreur comme paramètre d’entrée facultatif. Si aucun enregistreur n’est fourni, un enregistreur est créé et écrit dans le terminal.
- Format de tableaux: en SAS, les utilisateurs transmettaient le nom d’un ensemble de données SAS à G-Confid; en Python, les utilisateurs peuvent transmettre une fiche de données Pandas DataFrame, un tableau Arrow ou un nom de fichier d’un type pris en charge (`.parq`, `.parquet`, `.feather`, `.csv` ou `.sasbdat`). Voir la section [Formats pris en charge](user_guide.md#formats-pris-en-charge) pour obtenir de plus amples renseignements.
- Colonnes de tableau: en Python, les noms de variables sont généralement sensibles à la casse. Dans une fiche de données Pandas, les variables `Revenus`, `revenus` et `REVENUS` peuvent toutes coexister dans la même fiche de données. De plus, les noms de variables peuvent contenir des espaces et dépasser 32 caractères. G-Confid prend désormais en charge des noms de variables comprenant jusqu’à 64 caractères; toutefois, les noms de variables comportant des espaces ne sont toujours pas autorisés. Même si nous avons prévu une certaine souplesse, la casse des noms des variables devrait être uniforme entre les paramètres et les ensembles de données. G-Confid tentera de normaliser les variables normalisées dans les fichiers d’entrée, et les fichiers de sortie utiliseront ces noms normalisés. Cette normalisation ne permet pas la coexistence de plusieurs colonnes portant le même nom avec une casse différente. Nous utilisons généralement la casse Pascal pour les noms de variables d’ensemble de données (p. ex. `CELLID` devient `CellId` et `outstatus` devient `OutStatus`.) Les variables non normalisées, comme les variables de coût ou les variables utilisées pour le traitement, ne sont pas modifiées. L’utilisateur doit respecter la casse des données d’entrée lors de la définition des paramètres.
- Gestion des exceptions: en Python, la gestion des erreurs repose sur les exceptions plutôt que sur un code de retour. Voir le [journal G-Confid](user_guide.md#journal-g-confid) pour de plus amples renseignements
- Les fonctions suivantes ne sont pas disponibles en Python, car elles ont été jugées inutiles. Si ce n’est pas le cas, veuillez en informer l’équipe de soutien de G-Confid
  - Aggregate
  - ReportCells
  - GConfidRound: seule la fonction GConfidOptRound a été portée en Python

## Sensibilité

La procédure de sensibilité a été convertie en prenant le code source de la procédure G-Confid 1.07.003 dépendante de SAS et en le modifiant pour produire une procédure en code source ouvert *"encapsulée"* dans un module Python. Les calculs mathématiques sous-jacents demeurent en grande partie inchangés, à une exception près:
  - `MINRESP` et `MINRESPW` sont désormais ignorés lors du calcul de la sensibilité pour une cellule si tous les enregistrements de cette cellule bénéficient d’une renonciation

En raison des différences entre SAS et Python, les utilisateurs doivent adapter la *façon dont* ils définissent les paramètres et les tableaux; les ensembles de paramètres et de tableaux demeurent en grande partie inchangés (même si la plupart des [*identifiants de paramètres*](#tableau-des-paramètres-et-types-de-sensibilité) et [*identifiants de tableau*](#tableau-des-identifiants-des-tableaux-de-sensibilité) ont été modifiés).

### Paramètres de sensibilité

De nombreux noms de paramètres ont été modifiés pour mieux respecter les conventions d’appellation courantes en Python.

Les identifiants utilisés dans les programmes SAS correspondent aux identifiants et types Python suivants:

### Tableau des paramètres et types de sensibilité

|Identifiant SAS|Identifiant Python|Type Python|Note|
|--|--|--|--|
|`ACCEPTNEGATIVE`|`accept_negative`|`bool`||
|~~`REJECTNEGATIVE`~~|||utiliser `accept_negative=False`|
|`ADDITIVENOISE`|`additive_noise`|`bool`||
|~~`NOADDITIVENOISE`~~|||utiliser `additive_noise=False`|
|`BY`|`by`|`str`||
|`DBGFILEPREFIX`|`debug_file_prefix`|`str`||
|`DBGWORKDIRPATH`|`debug_work_dir_path`|`str`||
|`DIMENSION`|`dimension`|`str`||
|`HIERARCHY`|`hierarchy`|`str`||
|`ID`|`unit_id`|`str`||
|`LIMITWARNINGS`|`limit_warnings`|`bool`||
|~~`NOLIMITWARNINGS`~~|||utiliser `limit_warnings=False`|
|`M`|`m`|`int` ou `float`||
|`MINRESP`|`min_resp`|`int` ou `float`||
|`MINRESPW`|`min_resp_w`|`int` ou `float`||
|`PRINTCODES`|`print_codes`|`bool`||
|~~`NOPRINTCODES`~~|||utiliser `print_codes=False`|
|`PROXYDIAG`|`proxy_diag`|`bool`||
|~~`NOPROXYDIAG`~~|||utiliser `proxy_diag=False`|
|`PROXYPERCENTILE`|`proxy_percentile`|`int` ou `float`||
|`PROXYRATIO`|`proxy_ratio`|`int` ou `float`||
|`PROXYSIZE`|`proxy_size`|`str`||
|`PWAIVER`|`p_waiver`|`str`||
|`RANGE`|`code_range`|`str`||
|`SHADOW`|`shadow`|`str`||
|`SRULE`|`s_rule`|`str`||
|`TIMER`|`timer`|`bool`||
|`TOLERANCE`|`tolerance`|`int` ou `float`||
|`VAR`|`var`|`str`||
|`VERBOSE`|`verbose`|`bool`||
|`WAIVER`|`waiver`|`str`||
|`WEIGHT`|`weight`|`str`||
|`WEIGHTDIAG`|`weight_diag`|`bool`||
|~~`NOWEIGHTDIAG`~~|||use `weight_diag=False`|
|`WEIGHTPROTLEVEL`|`weight_prot_level`|`str`||
|`X`|`x`|`int` ou `float`||
|`Y`|`y`|`int` ou `float`||
|`Z`|`z`|`int` ou `float`||

### Exemple de spécification de paramètres en Python

Le code suivant montre comment préciser différents *types Python* associés à certains paramètres courants :  

```python
gconfid.sensitiv(
    indata=indata,
    outlargest=True,
    hierarchy="Tot_Reg R1 R2; Tot_Cla C1 C2 C3;",
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

> *Exemple de spécification de paramètres en SAS*
>
> ```sas
> PROC SENSITIVITY
>     DATA=test3
>     OUTCELL=outcell3
>     OUTCONSTRAINT=outconstraint3
>     OUTLARGEST=outlargest3
>     hierarchy="""Tot_Reg R1 R2;
>                 Tot_Cla C1 C2 C3;""",
>     SRULE="pq 0.2"
>     proxyratio=0.2
>     proxydiag
>     AcceptNegative
>     ;
>     ID ident;
>     VAR val;
>     DIMENSION reg cla;
>     PROXYSIZE proxyvar;
>     PWAIVER WaiverFlag;
> RUN;
> ```

|Paramètre|Note|
|--|--|
|`unit_id`|Nom d’une seule variable|
|`dimension`|Liste de 0 ou plusieurs noms de variable séparés par des espaces|
|`proxy_ratio`|Accepte une valeur numérique; voir le guide de l’utilisateur pour des recommandations|
|`accept_negative`|bool: `True`-> activé, `False`-> comportement opposé (égal à `REJECTNEGATIVE` en SAS)|
|`hierarchy`|Encadre les chaînes multilignes avec des guillemets triples (`"""edit>=string"""`); il est possible d’utiliser des guillemets simples pour une seule ligne|

### Nouvelles options de sensibilité

Les nouvelles options comprennent les suivantes

- [`presort`](#option-presort)
- [`skip_validation`](#option-skip_validation)
- [`capture`](#option-capture)
- [`trace`](#option-trace)

#### Option `presort`

Cette option est disponible dans la procédure de sensibilité et activée par défaut. Pour de plus amples renseignements, consultez les paramètres de sensibilité dans le [guide de l'utilisateur](./user_guide.md#paramètres-de-sensibilité)

#### Option `skip_validation`

Cette option est disponible dans toutes les procédures et activée par défaut. Pour de plus amples renseignements, consultez les paramètres de sensibilité dans le [guide de l'utilisateur](./user_guide.md#paramètres-de-sensibilité)

#### Option `capture`

Cette option est disponible dans toutes les procédures et activée par défaut. Pour de plus amples renseignements, consultez les paramètres de sensibilité dans le [guide de l'utilisateur](./user_guide.md#paramètres-de-sensibilité)

#### Option `trace`

Cette option est disponible dans toutes les procédures et permet de contrôler le niveau de verbosité des journaux de la console. Pour de plus amples renseignements, consultez la section suivante du [guide de l’utilisateur](user_guide.md/#niveau-de-verbosité-du-journal-python-trace)

## Sensitivity Tables

De nombreux noms de paramètres de tableau ont été modifiés pour mieux respecter les conventions d’appellation courantes en Python.

Les identifiants utilisés dans les programmes SAS correspondent aux identifiants suivants en Python:

### Tableau des identifiants des tableaux de sensibilité

|Identifiant SAS|Identifiant Python|Note|
|--|--|--|
|`DATA`|`indata`||
|`OUTCELL`|`outcell`||
|`OUTCONSTRAINT`|`outconstraint`||
|`OUTLARGEST`|`outlargest`||
|`OUTPAIRS`|`outpairs`||
|`OUTTARGETS`|`outtargets`||

#### Modifications apportées aux tableaux

Un aperçu des modifications est présenté dans les sous-sections suivantes, avec des liens vers des renseignements détaillés. 

## Suppression

Dans G-Confid 1.07.003, Suppress était une macro SAS pour la suppression. Cette macro SAS a été réécrite en Python.

### Paramètres de suppression

### Tableau des paramètres et types de suppression

|Identifiant SAS|Identifiant Python|Type Python|Note|
|--|--|--|--|
|InCell|incell|`pd.DataFrame` ou `pa.Table` ou `str`||
|Constraint|inconstraint|`pd.DataFrame` ou `pa.Table` ou `str`||
|OutCell|outsuppress|`str` ou `None`||
|OutComplement|outcomplement|`str` ou `Path` ou `bool`|Fournit soit le chemin d’accès où les données de sortie seront enregistrées, soit une expression booléenne indiquant qu’une fiche de données DataFrame devrait être créée à la place. Facultatif, valeur par défaut: False|
|CFunction1|cost_function1|str|Nouvelles fonctions de coût disponibles|
|CFunction2|cost_function2|str|Nouvelles fonctions de coût disponibles|
|CVar1|cost_var1|str||
|CVar2|cost_var2|str||
|ScaleCost|scale_cost|str||
|--|size_roundingbase|int|Utilisé avec la nouvelle fonction de coût de ROUNDEDSIZE|
|--|sen_roundingbase|int|Paramètre facultatif utilisé pour arrondir la valeur de sensibilité à la hausse|
|--|total_roundingbase|int|Paramètre facultatif, utilisé pour arrondir la valeur de bruit total à la baisse|
|--|constraint_scale|int|Divisé par la valeur de sensibilité pour déterminer l’ambiguïté; valeur par défaut: 2|
|--|suppress_order|int|Détermine comment choisir la cellule suivante: 0 = ambiguïté modifiée la plus élevée, 1 = ambiguïté la plus élevée, 2 = sensibilité la plus élevée; valeurs par défaut: 0|
|AmbiguityTolerance|ambiguity_tolerance|float|Tolérance liée à l’ambiguïté; valeur par défaut : 0,00001|
|DebugInfo|trace|bool ou int|L’option trace permet de contrôler le niveau de journalisation. Voir le [guide de l’utilisateur](user_guide.md/#niveau-de-verbosité-du-journal-python-trace)|
|ByVars ou By|by|str||
|--|skip_validation|bool|Nouvelle option: Désactive la validation de prétraitement des champs attendus dans vos tables d'entrée. Valeur par défaut: `False`|
|--|custom_solver|pulp.LpSolver|Nouvelle option: Un solveur PuLP personnalisé facultatif à utiliser à la place du solveur par défaut de PuLP|
|--|logger|logging.Logger|Voir [Utilisation de votre propre enregistreur `(logger=)`](user_guide.md#utilisation-de-votre-propre-enregistreur-logger) dans le guide de l’utilisateur.|
|--|capture|bool|Voir [Suppression et dépannage des messages de journal `(capture=)`](user_guide.md#suppression-et-dépannage-des-messages-de-journal-capture)|
|SaveMps|--|--|Voir [Configuration du résolveur](user_guide.md#configuration-du-résolveur) dans le guide de l’utilisateur.|
|SolverOptions|--|--|Voir [Configuration du résolveur](user_guide.md#configuration-du-résolveur) dans le guide de l’utilisateur.|
|OptmodelOptions|--|--|Voir [Configuration du résolveur](user_guide.md#configuration-du-résolveur) dans le guide de l’utilisateur.|
|ScaleCostUpperBound||||
|RoundFactor||||
|RoundLowerBoundFactor||||
<!-- |--|multiprocess|bool|New option - enables the use of multiple processors to distribute the work of the solver. Defaults to False. See [Multiprocessing](user_guide.md#Multiprocessing) in the user guide.| -->

#### Exemple de spécification de paramètres de suppression en Python

Le code suivant montre comment préciser différents *types Python* associés à certains paramètres courants.

```python
import gconfid

result=gconfid.Suppression(
    incell="./outcell_sens.sas7bdat",
    inconstraint="./outconstraint.sas7bdat",
    outsuppress="./outcell_sprs.csv",
    cost_function1="digits",
    cost_function2="information",
    by="year",
)
```

> *Exemple de spécification de paramètres en SAS*
>
>```sas
>%SUPPRESS(
>   INCELL=outcell_sens,
>   CONSTRAINT=outconstraint,  
>   CFUNCTION1=digits,
>   CFUNCTION2=information,
>   BYVARS=year,
>   OUTCELL=outcell_sprs);
>```

## Auditing

Dans G-Confid 1.07.003, Audit était une macro SAS pour la vérification. Cette macro SAS a été réécrite en Python.

### Tableau des paramètres et types de vérification

|Identifiant SAS|Identifiant Python|Type Python|Note|
|--|--|--|--|
|InCell|incell|`pd.DataFrame` ou `pa.Table` ou `str`||
|Constraint|inconstraint|`pd.DataFrame` ou `pa.Table` ou `str`||
|OutCell|outaudit|`str` ou `None`||
|ByVars ou By|by|||
|DebugInfo|trace|bool ou int|Le paramètre `trace=gconfid.log_level.ERROR` équivaut essentiellement à DebugInfo=1 et `gconfid.log_level.INFO`, à DebugInfo=0. Voir le [guide de l’utilisateur](user_guide.md/#niveau-de-verbosité-du-journal-python-trace).|
|SolverOptions|--|--|Voir [Configuration du résolveur](user_guide.md#configuration-du-résolveur) dans le guide de l’utilisateur.|
|LBFactor|lb_factor|float||
|UBFactor|ub_factor|float||
|ReportLevel|report_level|int|report_level 2 n’est plus disponible|
|SasConnect|--|--|Ne s’applique plus; il s’agit d’un paramètre propre à SAS.|
|AuditSensitiveCellsOnly|--|--|N’a pas été repris; à examiner.|
|Tolerance|--|--|N’a pas été repris; à examiner.|
|ParallelMode|--|--|Ne s’applique plus; il s’agit d’un paramètre propre à SAS.|
|NumberOfNodes|--|--|Ne s’applique plus; il s’agit d’un paramètre propre à SAS.|
|SASApplicationServer|--|--|Ne s’applique plus; il s’agit d’un paramètre propre à SAS.|
|SasConnectThreshold |--|--|Ne s’applique plus; il s’agit d’un paramètre propre à SAS.|
|LpFeasTol|--|--|Déclaré obsolète dans G-Confid 1.07.003.|
|PrintProgress|--|--|Déclaré obsolète dans G-Confid 1.07.003.|
|UseShuttle|use_shuttle|bool|Active l'algorithme "Shuttle" pour calculer les limites inférieures et supérieures. Paramètre caché dans G-Confid 1.07.003. Valeur par défaut: `False`|
|RelTol|--|--|Paramètre caché dans G-Confid 1.07.003.|
|AbsTol|--|--|Paramètre caché dans G-Confid 1.07.003.|
|MaxIter|--|--|Paramètre caché dans G-Confid 1.07.003.|
|--|multiprocess|bool|Nouvelle option: Active l'utilisation de plusieurs processeurs pour répartir la charge de travail du solveur. Valeur par défaut: `False`. Voir la section sur le [multitraitement](user_guide.md#multitraitement) dans le Guide de l'utilisateur.|
|--|skip_validation|bool|Nouvelle option: Désactive la validation de prétraitement des champs attendus dans vos tables d'entrée. Valeur par défaut: `False`|
|--|custom_solver|pulp.LpSolver|Nouvelle option: Un solveur PuLP personnalisé facultatif à utiliser à la place du solveur par défaut de PuLP.|
|--|logger|logging.Logger|Voir [Utilisation de votre propre enregistreur `(logger=)`](user_guide.md#utilisation-de-votre-propre-enregistreur-logger) dans le guide de l’utilisateur.|
|--|capture|bool|Voir [Suppression et dépannage des messages de journal `(capture=)`](user_guide.md#suppression-et-dépannage-des-messages-de-journal-capture).|

## Arrondissement optimisé

Dans G-Confid 1.07.003, OptRound était une macro SAS pour l’arrondissement optimisé. Cette macro SAS a été réécrite en Python.

Veuillez noter que dans la version SAS de G-Confid, il existait deux macros d’arrondissement `GConfidOptRound` et `GConfidRound`. Seule `GConfidOptRound` a été convertie en Python, car cette méthode était jugée plus rigoureuse sur le plan méthodologique. `GConfidOptRound` vise à fournir une solution optimale, la plus proche possible du tableau d’origine tout en respectant les contraintes. Les contraintes peuvent être utilisées pour obtenir un tableau arrondi à la fois additif et contrôlé. Toutefois, dans certains cas, le problème peut être irréalisable. `GConfidRound` produisait une solution contrôlée, mais non additive, accompagnée d’un rapport d’additivité. `GConfidOptRound` échoue si aucune solution ne peut être trouvée; les contraintes doivent être assouplies pour obtenir une solution.

### Tableau des paramètres et types d’arrondissement optimisé

|Identifiant SAS|Identifiant Python|Type Python|Note|
|--|--|--|--|
|twoLevelCatalogName|--|--|Ne s’applique plus; il s’agit d’un paramètre propre à SAS.|
|Language|--|--|La langue peut être définie pour le progiciel g-confid avant l’appel d’un module avec `gconfid.set_language = gconfid.SupportedLanguage.fr`|
|InCells|incell|`pd.DataFrame` ou `pa.Table` ou `str`| |
|InConstraints|additive_bound|`pd.DataFrame` ou `pa.Table` ou `str`| |
|InCellConstraints|additive_con|`pd.DataFrame` ou `pa.Table` ou `str`| |
|OutCells|outround|`pd.DataFrame` ou `pa.Table` ou `str`|Les noms des colonnes ont été allongés pour améliorer la lisibilité.|
|OutConstraints|--|--|N’a pas été repris; à examiner.|
|OutSummary|--|--|N’a pas été repris; à examiner.|
|Base|base|int||
|DebugInfo|trace|bool ou int|Le paramètre `trace=gconfid.log_level.ERROR` équivaut essentiellement à DebugInfo=1 et `gconfid.log_level.INFO`, à DebugInfo=0. Voir le [guide de l’utilisateur](user_guide.md/#niveau-de-verbosité-du-journal-python-trace).|
|Objective|--|--|N’a pas été repris; à examiner.|
|Solver|--|--|Voir [Configuration du résolveur](user_guide.md#configuration-du-résolveur) dans le guide de l’utilisateur.|
|OptModelLog|--|--|Voir [Configuration du résolveur](user_guide.md#configuration-du-résolveur) dans le guide de l’utilisateur.|
|OptModelStatus|--|--|N’a pas été repris; à examiner.|
|--|skip_validation|bool|Nouvelle option: Désactive la validation de prétraitement des champs attendus dans vos tables d'entrée. Valeur par défaut: `False`|
|--|custom_solver|pulp.LpSolver|Nouvelle option: Un solveur PuLP personnalisé facultatif à utiliser à la place du solveur par défaut de PuLP.|
|--|logger|logging.Logger|Voir [Utilisation de votre propre enregistreur `(logger=)`](user_guide.md#utilisation-de-votre-propre-enregistreur-logger) dans le guide de l’utilisateur.|
|--|capture|bool|Voir [Suppression et dépannage des messages de journal `(capture=)`](user_guide.md#suppression-et-dépannage-des-messages-de-journal-capture).|

## Autres options d’exécution Python

### Prise en charge des langues

G-Confid produit un [journal](user_guide.md#journal-g-confid) qui peut afficher des messages en anglais ou en français. Voir [*configuration de la langue du journal*](user_guide.md#configuration-de-la-langue-du-journal) dans le guide de l’utilisateur pour plus de détails.

### Option `capture`

Lors de l’exécution dans des notebooks Jupyter, certains messages du journal peuvent être absents. La spécification de `capture=True` dans l’appel d’une procédure pour corriger le problème. Voir la section sur la [suppression et dépannage des messages du journal](user_guide.md#suppression-et-dépannage-des-messages-de-journal-capture) dans le guide de l’utilisateur pour de plus amples détails.

## Considérations relatives au rendement

Certaines options et certains formats de tableau permettent d’obtenir un rendement optimal. Voir la section [Considérations relatives au rendement](user_guide.md#considérations-relatives-au-rendement) pour plus de détails.

## Erreurs et exceptions

Les erreurs sont traitées différemment en SAS et en Python, où elles sont appelées *exceptions*. Pour de plus amples renseignements, consultez la section [Erreurs et exceptions](user_guide.md#erreurs-et-exceptions) du guide de l’utilisateur.

## Fonctions utilitaires

### Working with SAS Files in Python

Le progiciel G-Confid fournit quelques fonctions utiles pour lire des fichiers SAS en Python. Pour en savoir plus, consultez la section [Utilisation de fichiers SAS en Python](user_guide.md#utilisation-des-fichiers-sas-en-python) dans le guide de l’utilisateur.
