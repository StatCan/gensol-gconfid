#ifndef MESSAGEGCONFIDAPI_FR_H
#define MESSAGEGCONFIDAPI_FR_H

/**************************************************************/
/* M10000 series - Information messages                       */
/* - Severity is EIE_INFORMATION with EI_AddMessage()         */
/* - Other cases: word 'NOTE:' added at the beginning         */
/**************************************************************/
#define M10034 "Intervalle pour la dimension %s"
#define M10035 "Code %-10s Début  %10d Fin %10d"
#define M10036 "Code %-10s Données %-10s"
#define M10037 "Intervalle vide"
#define M10039 "SRule Alpha[%d][%d] = %f"
#define M10040 "Groupe %d"
//#define M10041 "Ce groupe fait référence à une cellule sans donnée. Le groupe ne sera pas traité."
//#define M10042 "Seulement une cellule du groupe contient des données. Le groupe ne sera pas traité."
//#define M10043 "Ce groupe n'est pas sensible."
//#define M10044 "Ce groupe est sensible. Le ConstraintId est %d. Le CellId est %d."
//#define M10045 "Statistiques sur les groupes"
//#define M10046 "Seulement une cellule du groupe a une valeur supérieur à zéro. Le groupe ne sera pas traité."
/**************************************************************/
/* M20000 series - Warning messages                           */
/* - Severity is EIE_WARNING with EI_AddMessage()             */
/* - Other cases: word 'AVERTISSEMENT:' added at the beginning*/
/**************************************************************/
//#define M20028 "Le fichier de microdonnées contient un code qui n'est pas dans la hiérarchie ou dans l'intervalle pour la dimension %s, code '%s'."
#define M20028 "Le fichier de microdonnées contient un code qui est soit un parent dans la hiérarchie ou un code qui n'est pas dans la hiérarchie/intervalle pour la dimension %s, code '%s'. Seuls les codes enfants seront traités."
#define M20032 "Paramètre k %g de la règle NK est inférieur à %1f."
#define M20033 "Paramètre PQ (%f). La valeur devrait être entre %.2f et %.2f inclusivement."
#define M20034 "L'intervalle pour la dimension %s spécifie un code '%s' qui n'est pas dans la hiérarchie pour la même dimension."
#define M20035 "L'intervalle pour la dimension %s spécifie un code '%s' qui n'est pas un code final dans la hiérarchie pour la même dimension."
#define M20036 "Le groupe %d fait référence à moins de 2 cellules ayant des données valides. Il ne sera pas traité."
#define M20037 "La règle %d a la même valeur de k que la règle %d. 2 règles ne devraient pas avoir la même valeur pour k."
/**************************************************************/
/* M30000 series - Error messages                             */       
/* - Severity is EIE_ERROR with EI_AddMessage()               */
/* - Other cases: word 'ERREUR:' added at the beginning       */
/**************************************************************/
#define M30003 "Il y a trop d'erreurs. La compilation doit s'arrêter."
#define M30007 "Cherchons '%s' mais avons trouvé '%s' à la place."
#define M30106 "La hiérarchie (%d dimensions) et l'intervalle (%d dimensions) n'ont pas le même nombre de dimensions."
#define M30107 "Un code est répété ou une intervalle n'est pas disjoint lors de l'ajout du code '%d'."
#define M30108 "Les décompositions %d et %d du code '%s' ne sont pas équivalentes."
#define M30109 "Décomposition %d:"
#define M30111 "Les codes '%s' et '%s' de la décomposition %d du code '%s' contiennent des codes ou des intervalles qui se chevauchent."
#define M30112 "Les codes."
#define M30113 "Code de l'intervalle répété. Le code '%s' a été trouvé deux fois."
#define M30115 "Le code '%s' a 2 décompositions identiques."
#define M30116 "Le code '%s' se retrouve dans deux branches."
#define M30117 "Première branche."
#define M30118 "Deuxième branche."
#define M30119 "La dimension %d n'est pas valide."
#define M30120 "Le code '%s' a deux codes enfants ayant la même valeur '%s'."
#define M30121 "La règle C2 ne requiert aucun paramètre."
#define M30122 "Le code de début %d est plus grand que le code de fin %d dans l'intervalle: Code '%s' Début=%d Fin=%d"
#define M30123 "La règle C2 n'est pas supportée."
#define M30126 "La règle de sensibilité a besoin d'un argument."
#define M30127 "Type de règle de sensibilité invalide (%s)."
#define M30128 "La règle Duffett n'est pas supportée."
#define M30129 "La règle Duffett n'est pas recommandée. Vous devriez utiliser une autre règle de sensibilité."
#define M30130 "Règle de sensibilité invalide (%s)."
#define M30131 "La règle ARB a besoin de %d à %d paramètres."
#define M30132 "La règle NK a besoin d'au moins 2 paramètres."
#define M30133 "La règle NK a besoin d'un maximum de 3 paires de paramètres."
#define M30134 "La règle NK a besoin d'un nombre pair de paramètres."
#define M30135 "La règle PQ a besoin de 1 paramètre."
#define M30136 "La règle Duffett ne requiert aucun paramètre."
#define M30137 "%s n'est pas une méthode connue"
#define M30138 "Paramètre ARB invalide (%g). Doit être entre -1 et 1 inclusivement."
#define M30139 "Paramètres ARB invalides (%g %g). Les paramètres doivent être en ordre décroissant."

#define M30141 "Paramètre NK invalide (%g). N doit être un entier entre %d et %d inclusivement."
#define M30142 "Paramètre NK invalide (%G). k doit être un nombre strictement entre 0 et 100."
#define M30143 "Paramètre PQ invalide (%f). La valeur doit être strictement entre 0 et 1."
#define M30144 "SRule Alpha %d %d %f est < -1"
#define M30145 "SRule Alpha %d %d %f est < SRule Alpha %d %d %f"
#define M30146 "La règle %d a la même valeur de n que la règle %d. 2 règles ne peuvent avoir la même valeur pour n."

#define M30148 "L'incrément n'est pas précédé d'un code numérique."
#define M30149 "Un incrément n'est pas permis quand le code de fin n'est pas numérique."
#define M30150 "Un incrément n'est pas permis quand le code de début n'est pas numérique."
#define M30151 "Le code est trop long."
#define M30152 "L'incrément est trop long."
#define M30153 "Le code de fin '%s' doit être numérique."
#define M30154 "Le code de début '%s' doit être numérique."
#define M30155 "L'incrément '%s' doit être numérique."
#define M30156 "L'incrément ne peut être zéro!"
#define M30157 "Le code de fin '%d' doit être plus grand que le code de début '%d'."
#define M30158 "Le code enfant '%s' est le même que le code parent '%s'."
#define M30159 "Le code enfant '%s' est un ancêtre du parent '%s'."
#define M30160 "Il y a trop de codes dans la coordonnée. %d codes devraient être fournis."
#define M30161 "Le code '%s' n'est pas dans le hiérarchie %s."
#define M30162 "Il y a trop peu de code dans la coordonnée. %d codes devraient être fournis."
#define M30163 "Un groupe doit avoir au moins 2 cellules."
#define M30164 "Caractère non reconnu dans groupe."
#define M30165 "Deux coordonnées du groupe %d sont des descendant l'un de l'autre (les coordinées %d et %d sont apparentées)."
#define M30166 "La hiérarchie contient plus d'une racine alors qu’une seule est permise.\n"
#define M30167 "Racines multiples trouvées: %s"

#define M30201 "Clé '%s' non trouvée dans waiver_flag_slist\n" /* NO LONGER USED as of changeset 455679*/
#define M30202 "ERROR: Weight variable specified and weight protection level is not exact (so PTN sensitivity should be used) but sensitivity rule type is not compatible with PTN."
#define M30203 "error; value of \"number_of_respondents\" is zero in \"STC_SRuleSensitivity()\""
#define M30204 "error; invalid value '%c' for variable \"ptn_type1\" in \"STC_SRuleSensitivity()\""
#define M30205 "error: value of Cell->WhichNoise is neither 'n' nor 'N' in call to STC_WriteTargets()"
#define M30206 "heap property doesn't hold at i = %d, child_ndx = %d, and parent_ndx = %d ((WhichF=='T')?(LargestFX[child_ndx]->FT):(WhichF=='W')?(LargestFX[child_ndx]->FW):(LargestFX[child_ndx]->FS) = %f, and "
#define M30207 "(WhichF=='T')?(LargestFX[parent_ndx]->FT):(WhichF=='W')?(LargestFX[parent_ndx]->FW):(LargestFX[parent_ndx]->FS) = %f)"

/**************************************************************/
/* M40000 series - Statistics (reports)                        */
/**************************************************************/
/**************************************************************/
/* M00000 series - Miscellaneous                              */
/**************************************************************/
#define M00002 "Erreur près ou avant"
#define M00004 "Deux points"
#define M00005 "Fin"
#define M00006 "Erreur"
#define M00011 "Point virgule"
#define M00014 "Caractère non reconnu"
#define M00042 "Terme non reconnu"
#define M00043 "Lecture des microdonnées" //also defined in MessageGConfid_*.h
#define M00044 "Parseur de règle de sensibilité"
#define M00045 "Parseur de hiérarchie"
#define M00046 "Parseur d'intervalle"
#define M00047 "Parseur de groupe"
#define M00048 "Code"
#define M00049 "Incrément"
#define M00050 "Limite rencontrée... aucun autre message d'avertissement ne seront imprimées." //also defined in MessageGConfid_*.h
#define M00051 "Validation de hiérarchie"
#define M00052 "Validation de groupe"

#endif
