# MultiStart Agent Guide

## Scope

These instructions apply to the entire repository.

## Product contract

MultiStart is a Windows Qt launcher that reads `apps.json` beside the executable and presents a dynamic icon grid. A configured item can launch an executable, execute a BAT/CMD file, or open a directory.

## Compatibility requirements

- The source must compile with Qt 5.15.2 and Qt 6.x.
- C++17 is the language baseline.
- Use the version-neutral CMake targets `Qt${QT_VERSION_MAJOR}::*`.
- Do not introduce Qt 6-only APIs unless they are guarded by an explicit Qt version check and a Qt 5.15.2 implementation is provided.
- Windows is the primary platform because BAT/CMD execution and ICO resources are product requirements.

## Standard build workflow

Use `dux build` as the primary build command. The checked-in `dux.toml` defaults to x86 and Qt 5.15.2.

```powershell
dux build
```

Useful verification variants:

```powershell
dux build --clean
dux build --qt 5.15.2 --arch x86
dux build --install
```

Qt 6 remains supported by the version-neutral CMake project, but only use a Qt 6 `dux` alias after `dux env .` confirms that its compiler environment is available.

After a build, run CTest in the build directory selected by `dux env .`. Both `ConfigLoaderTests` and `MainWindowSmokeTests` must pass.

## Architecture

- `ConfigLoader`: JSON parsing, validation, defaults, and path resolution.
- `AppItem`: version-neutral configuration data model.
- `Launcher`: EXE, BAT/CMD, and folder launch behavior.
- `MainWindow`: dynamic list UI, icons, startup configuration loading, and user-visible errors.
- `resources`: Qt resources and the Windows executable resource script.

Keep launch logic out of the UI and keep configuration parsing independent of widgets.

## Configuration contract

- Relative paths are resolved from the directory containing `apps.json`.
- The default configuration is `apps.json` beside `MultiStart.exe` and is loaded once at startup.
- Supported item types are `exe`, `bat`, and `folder`.
- `arguments` must remain a JSON array of strings.
- Missing custom icons must fall back to the executable/native file icon or a suitable Qt standard icon.
- Invalid items should be skipped with actionable warnings; a malformed root configuration should fail clearly without crashing.

## Change checklist

- Preserve Qt 5.15.2 source compatibility.
- Update tests when configuration or launch behavior changes.
- Keep `assets/app.png` and `assets/app.ico` embedded through `resources.qrc` and `resources/app.rc`.
- Do not edit generated `build*`, `install`, or `dist*` contents as source files.
- Do not commit machine-specific CMake cache or generated preset files.
