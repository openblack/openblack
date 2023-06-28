# Contributing to openblack

The openblack project benefits from contributions from community members like you.
Here, we provide a set of guidelines to help you create pull requests that align with our project's standards and pass our continuous integration tests.

## Reporting bugs

To report a bug, ensure you have a GitHub account. Search the issues page to see if the bug has already been reported.
If not, create a new issue and write the steps to reproduce.


## Contributing code

A typical pull request will be done with the following steps:

1. First, ensure you have a GitHub account and [fork](https://help.github.com/articles/fork-a-repo/) the openblack repository.
2. Create a new branch and commit your changes to that.
3. Make sure you follow the [Formatting Guidelines](contributing-style.md#formatting-guidelines) and [Architecture Guidelines](contributing-style.md#architecture-guidelines).
4. Submit a new [Draft Pull Request](https://help.github.com/articles/using-pull-requests/).
5. Wait until the [GitHub Checks](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/collaborating-on-repositories-with-code-quality-features/about-status-checks) have finished and correct any compilation, style or suggestions the bots give you.
6. Set your Pull Request as Ready for Review
7. Wait for other users to test and for the maintainers to review your changes.

The bot will catch incorrect style.. For details about style and architectural consideration, refer to the [style guide](contributing-style.md).

Your PR title and description should be describing at a high level what was changed and why.
This helps busy maintainers more quickly review changes.
If applicable, add screenshots/animations and link to relevant issues.

The commits in your PR have [clean, incremental and logical changes](https://about.gitlab.com/blog/2018/06/07/keeping-git-commit-history-clean).
Additionally, each commit should compile and pass tests to allow us to use [git bisection](https://git-scm.com/docs/git-bisect) to track down regressions.
We prefer your corrections as well as your conflict resolution to be done with [rebase](https://docs.github.com/en/get-started/using-git/about-git-rebase).

While maintaining clean commits with rebase is not mandatory, if your PR contains too many `Fixed CI`, `Update file.cpp`, `Changes from review`, `Merge with master` style messages, we may edit or squash your commits when integrating the PR.

## Credits
If you are contributing to openblack, please add your name to [contributors.md](../contributors.md) so that you can be credited for your work outside and inside the game.
