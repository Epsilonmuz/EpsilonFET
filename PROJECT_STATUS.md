# Project status

- Imported project: Epsilon FET v0.2.0, supplied as EpsilonFET-v0.2.0.zip.
- Owner reports earlier testing by Claude.
- Owner has not yet installed, auditioned or tested this version in FL Studio.
- Supplied measurements remain attributed to the imported project until reproduced.
- No plugin build or audio test was run during this import.
- Mac CI results: pending; the included workflow will build VST3, AU and standalone, run EpsilonPluginTest and validate the VST3 with pluginval.
- The supplied CI validates on Apple Silicon while building a universal plugin; native Intel execution remains a separate check.

The original extracted source is preserved in the first commit and tagged v0.2.0-import. Repository preparation adds this status note and makes the Actions shell use pipefail, so a failing test cannot be hidden by the tee command. The DSP code is preserved.
