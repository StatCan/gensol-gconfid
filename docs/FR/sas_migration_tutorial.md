# Tutoriel

Ce tutoriel présente l’[équivalent en Python](https://gitlab.k8s.cloud.statcan.ca/gensys/g-confid/-/blob/main/Python/sample_programs/conversion_examples/sensitiv03.py) du [programme exemple 3 de la procédure Proc Sensitivity du langage SAS](https://gitlab.k8s.cloud.statcan.ca/gensys/g-confid/-/blob/main/Python/sample_programs/conversion_examples/sensitiv03.sas).

De nombreux autres équivalents entre SAS et Python sont également disponibles dans le dossier [`conversion_examples`](https://gitlab.k8s.cloud.statcan.ca/gensys/g-confid/-/tree/main/Python/sample_programs/conversion_examples).

L’exemple de programme montre comment :

-   créer un tableau synthétique;
-   trier le tableau;
-   préciser :
    -   les paramètres;
    -   le tableau d’entrée;
    -   les options des tableaux de sortie;
-   accéder aux résultats.

Il aborde également certaines différences pertinentes entre SAS et Python.

## Exemple en langage SAS

```sas
%let num=4;
data test&num;
    INPUT ident$ reg$ cla$ WaiverFlag val proxyvar; 
    CARDS;
    SA064 R1 C1 0 90 0.5
    SA021 R1 C1 1 160 0.5
    SA021 R1 C2 1 210 0.5
    SA059 R1 C2 0 20 0.5
    SA033 R1 C2 1 120 0.5
    SA033 R1 C3 0 110 0.5
    SA076 R1 C3 0 30 0.5
    SA082 R1 C3 1 200 0.5
    SA114 R2 C1 1 120 0.5
    SA128 R2 C2 0 90 0.5
    SA177 R2 C2 0 50 0.5
    SA105 R2 C2 0 80 0.4
    SA105 R2 C3 0 40 0.4
    SA161 R2 C3 0 30 0.4
    SA143 R2 C3 0 10 0.4
RUN;

PROC SENSITIVITY 
    DATA=test&num 
    OUTCELL=outcell&num 
    OUTCONSTRAINT=outconstraint&num 
    OUTLARGEST=outlargest&num
    HIERARCHY="Tot_Reg R1 R2; Tot_Cla C1 C2 C3;" 
    SRULE="pq 0.2"
    proxyratio=0.2
    proxydiag
    AcceptNegative
    ;
    ID ident; 
    VAR val;
    DIMENSION reg cla; 
    PROXYSIZE proxyvar; 
    PWAIVER WaiverFlag;
RUN;
```

## Python language equivalent

```python
import pyarrow as pa

import gconfid

# create a schema for the indata dataset
indata_schema = pa.schema([
            ("ident", pa.string()),
            ("reg", pa.string()),
            ("cla", pa.string()),
            ("WaiverFlag", pa.int64()),
            ("val", pa.int64()),
            ("proxyvar", pa.float64()),
    ])

# create table using schema and lists of values for each column
indata = pa.table(
    schema=indata_schema,
    data=[
        ["SA064", "SA021", "SA021", "SA059", "SA033", "SA033", "SA076", "SA082", "SA114", "SA128", "SA177", "SA105", "SA105", "SA161", "SA143"],
        ["R1", "R1", "R1", "R1", "R1", "R1", "R1", "R1", "R2", "R2", "R2", "R2", "R2", "R2", "R2"],
        ["C1", "C1", "C2", "C2", "C2", "C3", "C3", "C3", "C1", "C2", "C2", "C2", "C3", "C3", "C3"],
        [0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0],
        [90, 160, 210, 20, 120, 110, 30, 200, 120, 90, 50, 80, 40, 30, 10],
        [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.4, 0.4, 0.4, 0.4],
    ],
)

sensitivity_call = gconfid.sensitiv(
    indata=indata,
    outlargest=True,
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

## Explication ligne par ligne

### Importer des bibliothèques

Dans Python, les *bibliothèques* doivent être *importées* dans une session avant de pouvoir être utilisées.

```python
import pyarrow as pa  
  
import gconfid
```

La bibliothèque [`pyarrow`](https://pypi.org/project/pyarrow/) est utilisée pour créer et manipuler des tableaux. Veuillez noter que l’*alias* `pa` est utilisé pour la bibliothèque `pyarrow`.

### Création de données synthétiques

Nous créons le même tableau synthétique que celui créé en SAS.

Un objet _Pyarrow Schema_ est créé et attribué à la variable `indata_schema`.

```python
# créer un schéma pour l’ensemble de données indata
indata_schema = pa.schema([
            ("ident", pa.string()),
            ("reg", pa.string()),
            ("cla", pa.string()),
            ("WaiverFlag", pa.int64()),
            ("val", pa.int64()),
            ("proxyvar", pa.float64()),
    ])
```

-   Ceci sert à définir le nom et le type de données de chaque colonne d’un tableau.
-   Pyarrow offre un [vaste ensemble de types de données](https://arrow.apache.org/docs/python/api/datatypes.html#factory-functions).
-   Pour de plus amples détails, consultez la [documentation sur `pyarrow.schema`](https://arrow.apache.org/docs/python/generated/pyarrow.schema.html).

Un objet *Pyarrow Table* est ensuite créé et stocké dans la variable `indata`.

```python
# create table using schema and lists of values for each column
indata = pa.table(
    schema=indata_schema,
    data=[
        ["SA064", "SA021", "SA021", "SA059", "SA033", "SA033", "SA076", "SA082", "SA114", "SA128", "SA177", "SA105", "SA105", "SA161", "SA143"],
        ["R1", "R1", "R1", "R1", "R1", "R1", "R1", "R1", "R2", "R2", "R2", "R2", "R2", "R2", "R2"],
        ["C1", "C1", "C2", "C2", "C2", "C3", "C3", "C3", "C1", "C2", "C2", "C2", "C3", "C3", "C3"],
        [0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0],
        [90, 160, 210, 20, 120, 110, 30, 200, 120, 90, 50, 80, 40, 30, 10],
        [0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.4, 0.4, 0.4, 0.4],
    ],
)
```

-   L’ordre des colonnes dans le schéma correspond à l’ordre des listes de données.
-   Pour obtenir de plus amples renseignements, voir la [documentation sur `pyarrow.table`](https://arrow.apache.org/docs/python/generated/pyarrow.table.html).
-   Contrairement à SAS, où les tableaux sont généralement stockés sous forme de fichiers, les tableaux Pyarrow sont généralement stockés en mémoire en tant que "*objects*"; d’où la nécessité de les attribuer à une variable `indata`.

#### **Concepts Python**

> - En Python, les variables sont souvent des _objets_, contenant non seulement des données, mais également des _méthodes_.
> - Les méthodes sont souvent utiles pour obtenir des renseignements ou effectuer des opérations sur les données.
> - La variable `indata` est un objet *Pyarrow Table* (`pyarrow.Table`) et possède donc sa propre [méthode `sort_by()`](https://arrow.apache.org/docs/python/generated/pyarrow.Table.html#pyarrow.Table.sort_by)
> - La documentation sur les autres méthodes disponibles pour les tableaux Pyarrow est accessible [**ici**](https://arrow.apache.org/docs/python/generated/pyarrow.Table.html#pyarrow.Table).

### Exécution de la procédure de Sensibilité

```python
sensitivity_call = gconfid.sensitiv(
    indata=indata,
    outlargest=True,
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

L’appel à `gconfid.sensitiv()` entraîne l’exécution de la procédure de sensibilité et l’attribution d’un objet à la variable `sensitivity_call`. Cet objet peut être utilisé pour accéder aux tableaux de sortie.

-   Il convient de noter que l’ensemble des paramètres et des tableaux est précisé sous forme de paires clé-valeur séparées par des virgules.
-   Ils peuvent apparaître dans n’importe quel ordre.
-   `indata=indata` indique que le tableau `indata` (récemment triée) devrait être fourni comme tableau "indata".

### Exécution de la procédure

Au cours de l’exécution de la procédure, le texte affiché dans la console devrait être presque identique au journal SAS de l’exemple équivalent.

### Accès aux tableaux de sortie

Une fois l’exécution terminée, les options des tableaux de sortie sont traitées. L’option par défaut ayant été précisée ci-dessus, les tableaux de sortie sont disponibles sous forme de tableaux pyarrow.

Ils sont stockés dans l’objet `sensitivity_call`. On peut y accéder à l’aide de `sensitivity_call.outcell` et de `sensitivity_call.outlargest`. À partir de là, ils peuvent être utilisés comme n’importe quel autre tableau Pyarrow, par exemple :

-   inscrits dans un fichier;
-   manipulés (tri, fusion, etc.);
-   utilisés comme entrée pour une autre procédure (`incell=sensitivity_call.outcell`).

### Autres options d’entrée et de sortie

Pour fournir des moyens cohérents de lecture/écriture de fichiers et de conversion entre formats de tableaux, tout en maintenant la plus grande précision possible en virgule flottante, la prise en charge de divers formats de tableaux en entrée et en sortie a été mise en œuvre.

Veuillez consulter le guide de l’utilisateur pour obtenir des renseignements sur les [**formats pris en charge**](user_guide.md#formats-pris-en-charge) :

Le code suivant illustre l’utilisation de fichiers pour les tableaux d’entrée et de sortie en modifiant l’exemple ci-dessus.

```python
gconfid.sensitiv(
    indata=r"C:\temp\in_data.feather",
    outcell=r"C:\temp\outcell.parquet",
    outlargest=r"C:\temp\outlargest.feather",
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

-   Notez qu’il n’est pas nécessaire d’attribuer l’objet retourné par `gconfid.sensitiv()` à une variable, puisque les données de sortie sont écrites directement sur le disque.