# Epsilon FET v0.2.0

Development repository for Epsilon Sounds' FET compressor.

## User manual

[Read the illustrated Epsilon FET v0.2.0 manual](EpsilonFET-v0.2.0-User-Manual.pdf) for macOS and FL Studio setup, every control and meter mode, all nine factory presets, and examples for vocals, 808s, drums, and mix buses.

## Validation status

- The owner has not yet installed, auditioned or tested this version in FL Studio.
- Earlier testing by Claude is reported by the owner; supplied measurements have not been independently reproduced here.
- The [Build macOS runs](https://github.com/Epsilonmuz/EpsilonFET/actions/workflows/build.yml) show current compilation, processor test and pluginval results.
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

Choose **Actions → Build macOS → Run workflow** to start a build from `main`. Successful runs include a ZIP with the VST3, AU and standalone builds plus validation logs in the Actions artifact.

Automated validation does not replace listening tests or FL Studio testing.
