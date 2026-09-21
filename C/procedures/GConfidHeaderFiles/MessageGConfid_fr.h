#ifndef MESSAGEGCONFID_FR_H
#define MESSAGEGCONFID_FR_H

#include "GConfidIdentifiers.h"

/********************************************************************************************/
/* Introduced during redesign conversion                                                    */
/********************************************************************************************/
/* return code descriptions */
#define RC_DESC_SUCCESS "exécution réussie"
#define RC_DESC_FAIL_UNHANDLED "erreur inattendue"
#define RC_DESC_FAIL_INIT_IN_DATASET "échec d'initialisation du jeu de données d'entrée"
#define RC_DESC_FAIL_READ_PARMS_LEGACY "échec d'obtention des paramètres"
#define RC_DESC_FAIL_MISSING_DATASET "jeu de données obligatoire manquant"
#define RC_DESC_FAIL_VARLIST_NOT_FOUND "variable dans la liste de variables non trouvée"
#define RC_DESC_FAIL_VARLIST_INVALID_COUNT "nombre de variables non valide dans la liste de variables"
#define RC_DESC_FAIL_VARLIST_SYSTEM_GENERATED "liste de variables générée par le système non valide"
#define RC_DESC_FAIL_SETUP_DATASET_IN "échec de configuration du jeu de données d'entrée"
#define RC_DESC_FAIL_SETUP_OTHER "erreur de configuration inattendue"
#define RC_DESC_FAIL_ALLOCATE_MEMORY "échec d'allocation de mémoire"
#define RC_DESC_FAIL_LPI_INIT "échec d'initialisation LPI"
#define RC_DESC_FAIL_EDITS_PARSE "échec d'analyse des modifications"
#define RC_DESC_FAIL_VALIDATION_LEGACY "échec de validation de l'héritage"
#define RC_DESC_FAIL_VALIDATION_NEW "échec de validation"
#define RC_DESC_FAIL_EDITS_OTHER "erreur inattendue liée aux modifications"
#define RC_DESC_FAIL_EDITS_CONSISTENCY "échec de la vérification de cohérence des modifications"
#define RC_DESC_FAIL_EDITS_REDUNDANCY "échec des modifications Vérification de redondance"
#define RC_DESC_FAIL_NAME_TOO_LONG "le nom de la variable dépasse la longueur maximale"
#define RC_DESC_FAIL_READ_GENERIC "Erreur inattendue lors de la lecture du jeu de données"
#define RC_DESC_FAIL_WRONG_SORT_ORDER "Ordre de tri non valide"
#define RC_DESC_FAIL_READ_SYNC "Échec de la synchronisation des jeux de données"
#define RC_DESC_FAIL_READ_DUPLICATE_DATA "Données en double détectées"
#define RC_DESC_FAIL_WRITE_GENERIC "échec de l'écriture dans l'ensemble de données de sortie"
#define RC_DESC_FAIL_PROCESSING_GENERIC "Erreur de traitement inattendue"
#define RC_DESC_EIE_TRANSFORM_FAIL "Échec de la transformation"
#define RC_DESC_EIE_KDTREE_FAIL "Échec KDTREE"
#define RC_DESC_EIE_MATCHFIELDS_FAIL "Erreur de correspondance des champs"

/* Generic */

#define MsgParmMandatory "%s est obligatoire."
#define MsgParmNotSpecified "%s non spécifié."
#define MsgParmEqualDouble "%s = %.*f"
#define MsgParmEqualInteger "%s = %d"
#define MsgParmEqualString "%s = %s"
#define MsgParmEqualDoubleDefault "%s = %.*f (défaut)"
#define MsgParmEqualIntegerDefault "%s = %d (défaut)"
#define MsgFooterAllByGroup "Le message cité plus haut était valable pour le total de tous les regroupements 'by'."
#define MsgHeaderForByGroupAbove_SAS_FREE "Le message ci-dessus concernait le groupe " GPN_BY "  suivant :" "\n" MSG_INDENT_NOTE
#define MsgDoubleNotInRange "%s doit être entre %f et %f inclusivement."
#define MsgIntegerNotInRange "%s doit être entre %d et %d inclusivement."
#define MsgNoObservationsInDataSet "Aucune observation dans la table %s."
#define MsgNoValidObservationsInDataSet "Aucune observation valide dans la table %s."
#define MsgParmWithDuplicateVariable "La variable est répétée dans l'énoncé %s."
#define MsgVarNameInTwoStatementsExclusive "La variable %s apparaît dans les énoncés %s et %s. Ces énoncés sont incompatibles."
#define MsgStatementsExclusive "L'énoncé '%s' et '%s' sont mutuellement exclusifs et ne peuvent être spécifiés en même temps."
#define MsgBothOptionAndItsOpposite "Les options %s et %s ont été spécifiées. %s sera utilisé par défaut."
#define Msg2OptionsButNoStatement "Les options %s (%s, %s) ont été spécifiées alors que l'énoncé %s est manquant. Les options reliées à %s seront ignorées."
#define Msg3OptionsButNoStatement "Les options %s (%s, %s, %s) ont été spécifiées alors que l'énoncé %s est manquant. Les options reliées à %s seront ignorées."
#define MsgProxypercAndProxyratioSpecified "Les options %s et %s ont été spécifiées. %s sera ignoré."

#define MsgNumberDroppedInDataSetMissingValueForVar "%d observations ont été rejetées de la table %s parce que la variable %s est manquante."
#define MsgNumberDroppedInDataSetNegativeValueForVar "%d observations ont été rejetées de la table %s parce que la variable %s est négative."
#define MsgNumberReadInDataSetNegativeValueForVar "%d observations de la table %s ont des valeurs négatives dans la variable %s."
#define MsgNumberReadInDataSetNonPositiveValueForVar "%d observations de la table %s ont des valeurs négatives ou des zéros dans la variable %s."

#define MsgNumberReadInDataSetMissingValueForVar "%d observations de la table %s ont des valeurs manquantes dans la variable %s."
#define MsgNumberReadInDataSetNegativeOrMissingValueForVar "%d observations de la table %s ont des valeurs manquantes ou négatives dans la variable %s."

#define MsgWeightSruleInvalid "Combination invalide de SRULE et WEIGHTPROTLEVEL specifiée. \"Duffet\" et \"arb\" sont compatibles avec WEIGHTPROTLEVEL=\"EXACT\" pour PTN."
#define MsgSruleNkPtnTooManyRules "Un maximum de 2 paires peuvent être spécifiées lorsque la règle nk est utilisée avec l'énoncé WEIGHT et une valeur de WEIGHTPROTLEVEL autre que EXACT."
#define MsgInvalidOptionValue "Valeur invalide \"%s\" pour l'option \"%s\"."
#define MsgInvalidParameterCombination "Combination invalide de paramètres: une variable de poids et WEIGHTPROTLEVEL autre que \"EXACT\" spécifiée avec l'énoncé waivers."
#define MsgParameterMustBeGreaterThan1 "Le paramètre \"%s\" = %f doit être > 1.0."
#define MsgParameterMustBeGreaterThanX "La valeur du paramètre %s doit être supérieure à %1.0f."
#define MsgParam1RequiresParam2 "Une valeur pour le paramètre %s a été spécifiée mais aucune variable %s n'a été spécifiée."
#define MsgParameterRequired "Le paramètre %s doit être présent quand l'énoncé %s est spécifié."
#define MsgOuttargetsInvalid "OUTTARGETS spécifié, mais OUTTARGETS peut uniquement être créé avec l'énoncé WEIGHT si WEIGHTPROTLEVEL est différent de 'EXACT' et SRULETYPE égal à NK."
#define MsgOutpairsInvalid "OUTPAIRS spécifié, mais OUTPAIRS peut uniquement être créé avec l'énoncé WEIGHT si WEIGHTPROTLEVEL est différent de 'EXACT' et SRULETYPE égal à PQ."

/* Specific */

/* Sensitivity */

#define MsgNumberDroppedMissingDimension "%d observations ont été rejetées de la table %s parce qu'une des variables %s est manquante."
#define MsgNumberDroppedIllegalCharactersDimension "%d observations ont été rejetées de la table %s parce qu'une des variables %s contient des caractères illégaux."
#define MsgNumberDroppedDimensionNotInHierarchy "%d observations ont été rejetées de la table %s parce qu'une des variables %s n'est pas dans la hiérarchie."

#define MsgObsNotUsedForSensitivityCalculation "Ces observations ne seront pas utilisées pour calculer la sensibilité."

#define MsgValueMissing "La valeur de la variable %s est manquante. L'observation est rejetée."
#define MsgValueNegative "La valeur de la variable %s est négative. L'observation est rejetée."
#define MsgCodeMissing "Un code est manquant pour la dimension %s. L'observation est rejetée."
#define MsgCodeHasIllegalCharacters "Un code a des caractères illégaux pour la dimension %s. L'observation est rejetée."

#define MsgNoMoreMessages "Limite rencontrée... aucun autre message d'avertissement ne sera imprimé." //also defined in MessageAPI_*.h

#define MsgSensitivityChanged "La sensibilité a été modifiée parce qu'elle est plus élevée que la valeur de la cellule."
#define MsgInternalCellsSensitivityChanged "  Des cellules internes ont été modifiées %d fois."
#define MsgMarginalCellsSensitivityChanged "  Des cellules marginales ont été modifiées %d fois."
#define MsgAggregatesSensitivityChanged "  Des agrégats sensibles ont été modifiés %d fois."
#define MsgUserGroupsSensitivityChanged "  Des regroupements sensibles ont été modifiés %d fois."

#define MsgToleranceTooBigForYourOwnGood "Il n'est pas recommandé d'utiliser une %s plus grande que %f. Certaines cellules confidentielles pourraient ne pas être identifiées si la tolérance dépasse cette valeur."
#define MsgNbHierarchiesNotEqualNbDimensions "Le nombre de dimensions (%d) dans l'énoncé %s ne correspond pas au nombre de variables (%d) de l'énoncé %s."
#define MsgMinRespGreaterZero "La valeur du paramètre MINRESP doit être un entier supérieur à zéro."
#define MsgMinRespOutsideCommonRange "La valeur du paramètre MINRESP est hors de l'intervalle usuel de 3 à 5."
#define MsgToleranceMinRespNotCompatible "L'option TOLERANCE doit être plus petite que 1.00 quand l'option MINRESP est spécifiée."
#define MsgMinRespTooBigChanged "MINRESP est trop gros. Sa valeur a été changée par %d."

#define MsgReadingMicrodata "Lecture des microdonnées"  //also defined in MessageAPI_*.h

#define MsgMicrodataStatisticsHeader "Statistiques sur les microdonnées"
#define MsgCellStatisticsHeader "Statistiques des cellules"
#define MsgNumberCalculated "Nombre calculé"
#define MsgNumberSensitive "Nombre sensible"
#define MsgPercentSensitive "Pourcentage sensible"
#define MsgCellsZeroValue "Cellules ayant une valeur zéro"
#define MsgCellsNonZeroValue "Cellules ayant une valeur > zéro"
#define MsgInternalCells "Cellules internes"
#define MsgMarginalCells "Cellules marginales"
#define MsgAllCells "Toutes les cellules"
#define MsgAggregates "Agrégats"
#define MsgUserGroups "Regroupements"
#define MsgTotal "Totaux"

#define MsgWaiversListKvPairAdded "added key value pair (\"%s\", %d) to list of waiver flags keys and list of waiver flags values"
#define MsgWaiversListKvPairChanged "changed key value pair (\"%s\", %d) in list of waiver flags keys and list of waiver flags values to (\"%s\", %d)"
#define MsgWaiversListNoChangeNeeded "input waiver flag key value pair (\"%s\", %d) consistent with key value pair already in list of waiver flags keys and list of waiver flags values; lists left unchanged"
#define MsgWaiversListDisplayStart "waiver flag key value pairs = ["
#define MsgWaiversKeyListFailed	           "Échec en ajoutant la clé à la liste de clé pour waiver flag"
#define MsgWaiversKeyValuePairFailed	   "Échec en ajoutant valeur de la clé (\"%s\", %d) à la liste de clés waiver flag et liste de valeurs waiver flag"
#define MsgWaiversKeyValueListFailed	   "Échec en ajoutant la valeur à la liste de valeurs pour waiver flag"
#define MsgWaiversWaiverFlagNotConsistent  "Contributeur '%s' a un waiverflag pour certaines mais pas toutes les contributions.  Veuillez vérifier la cohérence et exécuter la procédure de nouveau.\n"
#define MsgWaiversWaiverFlagUnabletoUpdate "Impossible de mettre à jour la liste de waiver flag\n"
#define MsgWaiversReadDataMemoryError      "Erreur de mémoire dans l'appel à \"ReadData()\" "
/*#define MsgWaiversNumberWaiverFlagNotPresentDataset "There were %d observations with the waiver flag variable that is not present in the input dataset. The value 0 will be used." */
#define MsgWaiversNumberWaiverFlagMissingDataset    "%d observations avec une ou des variables manquantes dans la variable waiverflag. La valeur 0 sera utilisée (aucune renonciation)."
#define MsgWaiversNumberWaiverFlagInvalidDataset    "%d observations avec une ou des variables invalides dans la variable waiverflag. La valeur 0 sera utilisée (aucune renonciation)."
#define MsgInvalidValueInFunctionCall "Invalid value for \"%s\" in call to \"%s\"."

#define MsgProxyRatio "%8hProxyRatio (calculé à partir de ProxyPercentile = %10.5f) = %10.5f"
#define MsgCellZeroContributions "Cellule avec toutes les contributions égales à zéro trouvée sous \"generate_weight_and_proxy_diagnostics()\""
#define MsgCellInvalidContributions "Cellule avec valeur non-valide %d pour variable \"TotalMixedSignStatus\" trouvée sous \"generate_weight_and_proxy_diagnostics()\""
#define MsgInvalidWhichNoise "Error: invalid value for \"Cell->WhichNoise\" in call to \"WriteCellObs()\""

#define MsgMemoryErrorCalculateProxyRatio "Erreur dans \"CalculateProxyRatio()\"—impossible d'allouer mémoire pour matrice de doubles \"Heap\""
#define MsgMemoryError "heap property doesn't hold at c = %d, i = %d, child_ndx = %d, && parent_ndx = %d (Heap[child_ndx] = %f, && Heap[parent_ndx] = %f)"


/* #define MsgTotalObs "Nombre total d'observations"                            */
/* #define Msg1     "1. Nombre d'observations valides"                          */
/* #define Msg1a    "   a. Nombre de répondants anonymes"                       */
/* #define Msg1b    "   b. Nombre de répondants non-anonymes"                   */
/* #define Msg1c    "   c. Données négatives pour les répondants"               */
/* #define Msg1d    "   d. Donnée négative ou manquante pour la variable proxy" */
/* #define Msg1e    "   e. Donnée zero ou négative pour la variable weight"     */
/* #define Msg2     "2. Nombre d'observations avec une valeur égale à zéro"     */
/* #define Msg3     "3. Nombre d'observations invalides"                        */
/* #define Msg3a    "   a. Données manquantes pour les répondants"              */
/* #define Msg3b    "   b. Données négatives pour les répondants"               */
/* #define Msg3c    "   c. Code manquant pour une dimension"                    */
/* #define Msg3d    "   d. Code invalide pour une dimension"                    */
/* #define Msg3e    "   e. Donnée manquante pour la variable shadow"            */
/* #define Msg3f    "   f. Donnée manquante pour la variable weight"            */

#define MsgTotalObs "Nombre total d'observations"
#define Msg1     "1. Nombre d'observations valides"
#define Msg1a    "   a. Nombre de répondants anonymes"
#define Msg1b    "   b. Nombre de répondants non-anonymes"
#define Msg1c    "   c. Données négatives pour les répondants"
#define Msg1d    "   d. Donnée négative ou manquante pour la variable proxy"
#define Msg1e    "   e. Donnée zero ou négative pour la variable weight"
#define Msg1f    "   f. Donnée zero ou négative pour la variable waiver"
#define Msg2     "2. Nombre d'observations avec une valeur égale à zéro"
#define Msg3     "3. Nombre d'observations invalides"
#define Msg3a    "   a. Données manquantes pour les répondants"
#define Msg3b    "   b. Données négatives pour les répondants"
#define Msg3c    "   c. Code manquant pour une dimension"
#define Msg3d    "   d. Code invalide pour une dimension"
#define Msg3e    "   e. Donnée manquante pour la variable shadow"
#define Msg3f    "   f. Donnée manquante pour la variable weight"
#define Msg4     "4. Nombre d'observations exclues (pas dans la hiérarchie)"

/* #define Msg3f    "   f. Donnée négative ou manquante pour la variable proxy" */
/* #define Msg3g    "   g. Donnée manquante pour la variable weight"            */
/* #define Msg3h    "   h. Donnée zero ou négative pour la variable weight"     */

/* #define Msg4     "4. Nombre d'observations exclues (pas dans la hiérarchie)" */

#define TableW1L1        "Table W1: Effet de l'utilisation de poids sur la sensibilité"                 
#define TableW1LineShort "                                        ----------- ----------- -----------" 
#define TableW1L2        "                                        Sensibilité Sensibilité Sensibilité" 
#define TableW1L3        "                                            réduite   inchangée   augmentée" 
#define TableW1LineLong  "    ----------------------------------- ----------- ----------- -----------" 
#define TableW1L4        "    Nombre de cellules internes        "     "%12d"      "%12d"      "%12d"
#define TableW1L5        "    Nombre de cellules marginales      "     "%12d"      "%12d"      "%12d"
#define TableW1L6        "    Nombre de cellules publiées        "     "%12d"      "%12d"      "%12d"

#define TableW2L1        "Table W2: Effet de l'utilisation de poids sur les fréquences de cellules"           
#define TableW2LineShort "                                    ------------------- -------------------" 
#define TableW2L2        "                                            Sensibilité         Sensibilité" 
#define TableW2L3        "                                           non-pondérée            pondérée" 
#define TableW2L4        "                                  Sécuritaire Sensible Sécuritaire Sensible"
#define TableW2LineLong  "    ------------------------------- --------- --------- --------- ---------" 
#define TableW2L5        "    Nombre de cellules internes    "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableW2L6        "    Nombre de cellules marginales  "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableW2L7        "    Nombre de cellules publiées    "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableW2L8        "    Nombre d'agrégats              "   "%10d"    "%10d"    "%10d"    "%10d"
	
#define TableP1L1        "Table P1: Effet sur la sensibilité suite à l'utilisation de variable substitutive (PROXY)"                         
#define TableP1LineShort "                                               ----------- ----------- -----------"
#define TableP1L2        "                                               Sensibilité Sensibilité Sensibilité"
#define TableP1L3        "                                                   réduite   inchangée   augmentée"
#define TableP1LineLong  "    ------------------------------------------ ----------- ----------- -----------"
#define TableP1L4        "    Nombre de cellules internes               "     "%12d"      "%12d"      "%12d"
#define TableP1L5        "       Contient données positives et négatives"     "%12d"      "%12d"      "%12d"
#define TableP1L6        "       Toutes contributions >=0 et certaines>0"     "%12d"      "%12d"      "%12d"
#define TableP1L7        "       Toutes contributions = 0               "     "%12d"      "%12d"      "%12d"
#define TableP1L8        "       Toutes contributions <=0 et certaines<0"     "%12d"      "%12d"      "%12d"
#define TableP1L9        "    Nombre de cellules marginales             "     "%12d"      "%12d"      "%12d"
#define TableP1L10       "       Contient données positives et négatives"     "%12d"      "%12d"      "%12d"
#define TableP1L11       "       Toutes contributions >=0 et certaines>0"     "%12d"      "%12d"      "%12d"
#define TableP1L12       "       Toutes contributions = 0               "     "%12d"      "%12d"      "%12d"
#define TableP1L13       "       Toutes contributions <=0 et certaines<0"     "%12d"      "%12d"      "%12d"
#define TableP1L14       "    Nombre de cellules publiées               "     "%12d"      "%12d"      "%12d"
#define TableP1L15       "       Contient données positives et négatives"     "%12d"      "%12d"      "%12d"
#define TableP1L16       "       Toutes contributions >=0 et certaines>0"     "%12d"      "%12d"      "%12d"
#define TableP1L17       "       Toutes contributions = 0               "     "%12d"      "%12d"      "%12d"
#define TableP1L18       "       Toutes contributions <=0 et certaines<0"     "%12d"      "%12d"      "%12d"

#define TableP2L1         "Table P2: Effet de l'utilisation de variable substitutive (PROXY) sur les fréquences de cellules"                  
#define TableP2LineShort  "                                                ------------------- -------------------"
#define TableP2L2         "                                                        Sensibilité         Sensibilité"
#define TableP2L3         "                                                   sans la variable    avec la variable"
#define TableP2L4         "                                                        substitutive       substitutive"
#define TableP2LineShort2 "                                                --------- --------- --------- ---------"
#define TableP2L5         "                                              Sécuritaire  Sensible Sécuritaire Sensible"
#define TableP2LineLong   "    ------------------------------------------- --------- --------- --------- ---------"
#define TableP2L6         "    Nombre de cellules internes                "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L7         "       Contient données positives et négatives "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L8         "       Toutes contributions >=0 et certaines >0"   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L9         "       Toutes contributions = 0                "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L10        "       Toutes contributions <=0 et certaines <0"   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L11        "    Nombre de cellules marginales              "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L12        "       Contient données positives et négatives "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L13        "       Toutes contributions >=0 et certaines >0"   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L14        "       Toutes contributions = 0                "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L15        "       Toutes contributions <=0 et certaines <0"   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L16        "    Nombre de cellules publiées                "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L17        "       Contient données positives et négatives "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L18        "       Toutes contributions >=0 et certaines >0"   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L19        "       Toutes contributions = 0                "   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L20        "       Toutes contributions <=0 et certaines <0"   "%10d"    "%10d"    "%10d"    "%10d"
#define TableP2L21        "    Nombre d'agrégats                          "   "%10d"    "%10d"    "%10d"    "%10d"

#endif
