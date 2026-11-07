# Epsilon FET v0.2.0

Private development repository for Epsilon Sounds' FET compressor.

## Validation status

- The owner has not yet installed, auditioned or tested this version in FL Studio.
- Earlier testing by Claude is reported by the owner; supplied measurements have not been independently reproduced here.
- Mac compilation, processor tests and pluginval validation are pending. The [first workflow attempt](https://github.com/Epsilonmuz/EpsilonFET/actions/runs/42328849305) was stopped by GitHub before the job started.
- No DSP code was changed during repository setup.

## Preserved source

The complete project and prepared Git history are preserved in `EpsilonFET_v0.2.0_Repository.bundle`. This includes the JUCE modules, Projucer and exported projects, manual, and supplied tests. The source files are inside this bundle.

Download the bundle and restore it locally with:

```sh
git clone EpsilonFET_v0.2.0_Repository.bundle EpsilonFET
```

The restored checkout includes the full project README, `PROJECT_STATUS.md`, the manual, and the original baseline tag `v0.2.0-import`.

## Mac builds

The [Build macOS workflow](https://github.com/Epsilonmuz/EpsilonFET/actions) opens the preserved bundle in a temporary workspace, builds universal Intel/Apple Silicon VST3, AU and standalone targets, and runs the supplied processor tests and pluginval on Intel macOS. It uses read-only repository permissions.

Once GitHub permits workflow execution, choose **Actions → Build macOS → Run workflow**. Successful runs provide the compiled plugins and logs as private Actions artifacts.

Automated validation does not replace listening tests or FL Studio testing.
