[(Le message en français suit)](#aperçu)

# G-Confid

Welcome to the project repository for the G-Confid. Please consider watching this repo to be notified of updates and giving us a star.

## Overview

G-Confid is a Statistics Canada generalized system that offers a methodology designed to prevent the release of confidential data. It is comprised of four modules, The `Sensitivity` module is used to determine the sensitivity of table cells (and combinations of cells). The `Suppression` module is used to determine a suppression pattern, or set of complementary cells, necessary to protect the sensitive cells (or combinations of cells). The third component, the `Auditing` module, is used to verify the validity of a suppression pattern. In addition to these three modules, the `Optimized Rounding` module provides rounding which is both controlled and additive.

## Project Overview

The G‑Confid system is undergoing a transition from SAS to Python. Beta releases were introduced for testing in March 2025, and the production-ready version was released on April 30, 2026. Further information regarding the Generalized Systems move to open source is available [here](https://confluenceb.statcan.ca/spaces/GENSYS/pages/1054175817/Transition+to+open-source)

## Installation

The G-Confid package is installed using the `pip` package installer for Python. For general information about `pip`, please see the [pip documentation](https://pip.pypa.io/en/stable/cli/pip_install/).

G-Confid deployed to Statistics Canada's Artifactory server in the `pypi-local` registry found here: [https://artifactory.cloud.statcan.ca/ui/repos/tree/General/pypi-local](https://artifactory.cloud.statcan.ca/ui/repos/tree/General/pypi-local).

It is recommended to use a different Python virtual environment for testing development packages than you use for production or other versions. To create a new venv:

```shell
py -m venv my-venv-name
```

Then to activate it:
```shell
.\my-venv-name\Scripts\Activate
```


G-Confid can be installed with following command

```shell
pip install gconfid
```

Note that the additional options can be specified and we recommended targeting a specific version for production.

```shell
pip install gconfid==2.0.0
```

To update an existing installation to the latest version

```shell
pip install --upgrade gconfid
```

## Documentation

Documentation can be found in [this folder](./docs) and are detailed below:

| Document(s)            | Description                                                                                   |
| ---------------------- | --------------------------------------------------------------------------------------------- |
| [User guide](./docs/EN/user_guide.md) | An overview of the G-Confid package, detailing concepts and parameters for each module. |
| [SAS migration guide](./docs/EN/sas_migration_guide.md) | A guide for existing users switching from the SAS-based version of G-Confid. |
| [SAS migration tutorial](./docs/EN/sas_migration_tutorial.md) | A step-by-step tutorial demonstrating how to replicate a typical SAS-based call to G-Confid using the Python version. |
| [Release notes](./docs/EN/release-notes.md) | Detailed list of changes in each new version of G-Confid, starting with version 2.0. |

## Examples

Sample programs for each module are available in [this folder](./Python/sample_programs).

## Handling of Sensitive Statistical Information

Please respect your organization's policies regarding the handling for Sensitive Statistical Information. Output files may require encryption and need to be stored in a secure location. The consumer of the package is responsible for ensuring sensitive data is processed in a secure environment.

## Contributing

As contributors and maintainers to this project, you are expected to abide by our code of conduct. More information can be found at: [Contributor Code of Conduct](./CODE_OF_CONDUCT.md) and [CONTRIBUTING.md](./CONTRIBUTING.md).

## License

[GNU GENERAL PUBLIC LICENSE](./LICENSE)

## Acknowledgements

Over the years many people have contributed to G-Confid dating back to the 1980s. The system was originally called Confid and developed by C, In was later integrated with SAS and renamed Confid2 and then finally renamed to G-Confid. In 2024-25, G-Confid was made available in Python.

## Support

G-Confid is fully supported by a team comprised of methodologists and IT. Please do not hesitate to reach out to us for any support requests, including the following:

- Help with installation
- Errors when running components
- Guidance on how to implement disclosure avoidance methods
- Setting up G-Confid for production

There are two ways to initiate a support request:

- [GitLab Issues](https://gitlab.k8s.cloud.statcan.ca/gensys/g-confid/-/issues) are perfect for reporting errors or for questions regarding individual procedure calls. Issues are public by default, so you may find solutions to your problem by looking through existing issues. Remember not to include *Sensitive Statistical Information* (SSI) in your request.
- For general questions, please send an email to our [functional mailbox](mailto:statcan.gconfid-gconfid.statcan@statcan.gc.ca). This is also a great way to set up a general consultation with our methodology team.

Support requests will typically receive a reply within one or two business days.

---

# G-Confid

Bienvenue dans le répertoire de projet de G-Confid. Veuillez envisager de suivre ce répertoire pour être informé des mises à jour et de nous accorder une étoile.

## Aperçu

G-Confid est un système généralisé de Statistique Canada qui offre une méthodologie conçue pour empêcher la divulgation de données confidentielles. Il se compose de quatre modules : Le module `Sensitivity` (Sensibilité) est utilisé pour déterminer la sensibilité des cellules d'un tableau (et des combinaisons de cellules). Le module `Suppression` est utilisé pour déterminer un schéma de suppression, ou un ensemble de cellules complémentaires, nécessaire pour protéger les cellules sensibles (ou les combinaisons de cellules). La troisième composante, le module `Auditing` (Vérification), est utilisée pour vérifier la validité d'un schéma de suppression. En plus de ces trois modules, le module `Optimized Rounding` (Arrondissement optimisé) permet d'effectuer un arrondissement à la fois contrôlé et additif.

## Aperçu du projet

Le système G‑Confid fait l'objet d'une transition de SAS vers Python. Des versions bêta ont été introduites à des fins d'essai en mars 2025, et la version prête pour la production a été publiée le 30 avril 2026. De plus amples informations concernant le passage des systèmes généralisés au code source ouvert sont disponibles [ici](https://confluenceb.statcan.ca/spaces/GENSYS/pages/1054175817/Transition+to+open-source) (en anglais uniquement).

## Installation

L'ensemble G-Confid est installé à l'aide de l'installateur de paquets `pip` pour Python. Pour des informations générales sur `pip`, veuillez consulter la [documentation de pip](https://pip.pypa.io/en/stable/cli/pip_install/).

G-Confid est déployé sur le serveur Artifactory de Statistique Canada dans le registre `pypi-local` situé ici : [https://artifactory.cloud.statcan.ca/ui/repos/tree/General/pypi-local](https://artifactory.cloud.statcan.ca/ui/repos/tree/General/pypi-local).

Il est recommandé d'utiliser un environnement virtuel Python différent pour tester les paquets de développement de celui que vous utilisez pour la production ou d'autres versions. Pour créer un nouvel environnement virtuel (venv) :

```shell
py -m venv nom-de-mon-venv
```

Ensuite, pour l'activer :
```shell
.\nom-de-mon-venv\Scripts\Activate
```


G-Confid peut être installé avec la commande suivante :

```shell
pip install gconfid
```

Notez que des options supplémentaires peuvent être spécifiées et nous recommandons de cibler une version spécifique pour la production.

```shell
pip install gconfid==2.0.0
```

Pour mettre à jour une installation existante vers la version la plus récente :

```shell
pip install --upgrade gconfid
```

## Documentation

La documentation se trouve dans [ce dossier](./docs) et est détaillée ci-dessous :

| Document(s) | Description |
| ---------------------- | --------------------------------------------------------------------------------------------- |
| [Guide de l'utilisateur](./docs/FR/user_guide.md) | Un aperçu de l'ensemble G-Confid, détaillant les concepts et les paramètres pour chaque module. |
| [Guide de migration SAS](./docs/FR/sas_migration_guide.md) | Un guide pour les utilisateurs actuels passant de la version de G-Confid basée sur SAS. |
| [Tutoriel de migration SAS](./docs/FR/sas_migration_tutorial.md) | Un tutoriel étape par étape démontrant comment reproduire un appel type basé sur SAS vers G-Confid en utilisant la version Python. |
| [Notes de version](./docs/FR/release-notes.md) | Liste détaillée des changements dans chaque nouvelle version de G-Confid, à partir de la version 2.0. |

## Exemples

Des exemples de programmes pour chaque module sont disponibles dans [ce dossier](./Python/sample_programs).

## Manipulation de l'information statistique sensible

Veuillez respecter les politiques de votre organisation concernant la manipulation de l'information statistique sensible (ISS). Les fichiers de sortie peuvent nécessiter un cryptage et doivent être stockés dans un endroit sûr. L'utilisateur de l'ensemble est responsable de s'assurer que les données sensibles sont traitées dans un environnement sécurisé.

## Contribution

En tant que contributeurs et mainteneurs de ce projet, vous êtes tenus de respecter notre code de conduite. De plus amples informations peuvent être trouvées à : [Code de conduite du contributeur](./CODE_OF_CONDUCT.md) et [CONTRIBUTING.md](./CONTRIBUTING.md).

## Licence

[LICENCE PUBLIQUE GÉNÉRALE GNU](./LICENSE)

## Remerciements

Au fil des ans, de nombreuses personnes ont contribué à G-Confid, dont les origines remontent aux années 1980. Le système s'appelait initialement Confid et a été développé en C. Il a ensuite été intégré à SAS et renommé Confid2, puis finalement G-Confid. En 2024-2025, G-Confid a été rendu disponible en Python.

## Soutien

G-Confid est entièrement soutenu par une équipe composée de méthodologistes et de spécialistes en TI. N'hésitez pas à nous contacter pour toute demande de soutien, y compris les suivantes :

- Aide à l'installation
- Erreurs lors de l'exécution des composantes
- Conseils sur la mise en œuvre des méthodes de protection contre la divulgation
- Configuration de G-Confid pour la production

Il existe deux façons de lancer une demande de soutien :

- Les [demandes de service GitLab (Issues)](https://gitlab.k8s.cloud.statcan.ca/gensys/g-confid/-/issues) sont parfaites pour signaler des erreurs ou pour des questions concernant des appels de procédure individuels. Les demandes sont publiques par défaut, vous pourriez donc trouver des solutions à votre problème en consultant les demandes existantes. N'oubliez pas de ne pas inclure d'*Information statistique sensible* (ISS) dans votre demande.
- Pour des questions générales, veuillez envoyer un courriel à notre [boîte de réception fonctionnelle](mailto:statcan.gconfid-gconfid.statcan@statcan.gc.ca). C'est également un excellent moyen de planifier une consultation générale avec notre équipe de méthodologie.

Les demandes de soutien recevront généralement une réponse dans un délai d'un ou deux jours ouvrables.
