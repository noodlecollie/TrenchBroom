As per [here](https://github.com/TrenchBroom/TrenchBroom/issues/4072), these changes probably won't be useful to TrenchBroom in general, so they should stay on this fork. Annoyingly, this will require more maintenance, so the plan is:

* The `development` branch will be the main branch of the fork, which holds completed/relatively stable code.
* Individual features will be developed by branching off `development` and then merging back in with a squash commit.
* When upstream creates a new release that we want to use, we can update local `master` to mirror `upstream/master` and then rebase `development` on the release tag.

## V0 (minimum required)

* Use an asset scheme where content is sourced from VPKs, plus directories on disk.
	* This is done, at least in a first pass.
* Add support for reading VPKs and VMT/VTF files for textures.
	* Again, done in a limited way for the first pass.
* Add support for reading TF2 FGD. If there's fancy stuff in there (eg. entity I/O), just ignore it for now.
	* This is done, and entities are recognised correctly.
* Add support for exporting MAP to VMF. Unclear whether this would be better implemented as an external utility or an internal routine.
	* This is done.

### VPK Support

The file system config within the game's config file describes where assets should be loaded from (`GameConfigParser::parsePackageFormatConfig()`). This is specified via a search path and the package format. The package format has a format string and a list of supported extensions.

`GameFileSystem::addFileSystemPackages()` reads the package format string and creates the relevant file system adapter. Additional search paths can also be specified via enabled "mods" for the game - we may not need to rely on this, as `tf` may be the only search path required if the purpose of the path is to encapsulate _all_ assets.

Potentially useful libraries for reading assets:

* https://github.com/panzi/unvpk for VPKs
* https://github.com/panzi/VTFLib for VTFs

### Questions to Resolve

* Can the TF2 FGD be loaded as-is, or does TB fall over? If the syntax is supported as-is, that makes our life easier.
* Should MAP2VMF be written as an external utility, or written into TB?
	* I'm leaning towards writing it into the editor, in the same way that "export to .obj" is done.

### End-to-End Test

* Create a basic map that functions in TF2 without entity I/O: two spawn rooms with resupply lockers, fighting arena, lights, skybox.
* Compile this map using TrenchBroom compiler configuration on Windows.
* Test map in game.

## V1

* Add support for reading MDL files for models.
* Add support for entity I/O.
* Add support for compiling maps on non-Windows platforms.

### MDLs

There is no obvious library on GitHub for reading MDLs. [Assimp](https://github.com/assimp/assimp/blob/master/code/AssetLib/MDL/MDLLoader.h#L142) has an apparently incomplete implementation, so we could perhaps use this to write our own.

### Entity I/O

This implementation will be non-trivial. It will require at least a new UI to be built to configure I/O properties. It will also potentially need to visualise entity links as TB does for other things, allow picking entities by clicking on them, and tie into problem reporting. It could even have a script-like text syntax, to make setting up bulk I/O easier?

```
Output        Target    Input       Variant                Delay
OnStartTouch->other_ent:ShowMessage("This is an argument")[0.5]
```

One option, until we implement a proper UI, could be to create I/O links by adding TB-specific properties to entities using this syntax, and then converting the syntax to a proper I/O string when saving to VMF. For example:

```
_tb_source_io_0 "OnStartTouch->other_ent:ShowMessage(\"This is an argument\")[0.5]"
_tb_source_io_1 "..."
```

This would at least allow us to specify I/O without needing a specific UI.

### Compiling Maps

Compiling maps is a bit of a difficult one, since the official map compilers only work on Windows. As far as I see we have a few options:

1. Run the TF2 map compilers through Wine (or similar) on Linux.
	This repo, which puts the TF2 map compilers into a Docker container, may help: https://github.com/re1ard/linux_map_compiler

2. Re-write the map compilers to run on Linux.
	Although a noble task, this would be a huge undertaking, and would probably require many tweaks to support many different Source games.

3. Recompile the existing map compilers (eg. from the leaked source code) with Linux support.
	Unclear exactly what would be required for Linux support. However, if the leaked source code already has Linux support for games, then porting the compile tools to linux _might_ not bee too hard?
	Code: https://github.com/OthmanAba/TeamFortress2/tree/1b81dded673d49adebf4d0958e52236ecc28a956/tf2_src/utils/vbsp

## V2

* Add support for displacement editing (geometry and blending).

### Implementation

This implementation will be _extrememly_ non-trivial. The easiest way for us to put together a usable feature set would be:

* Allow subdivision powers of 2, 3, or 4.
* Allow moving vertices, or groups of vertices, in the 2D views.
* Allow alpha painting in the 3D view.
* Allow sewing adjacent displacements.

Fancier brush types, and moving along face normals and in 3D space, could be added in future. The encoding of displacements in VMF files would need to be researched, to see whether there's anything special in there regarding saving neighbours which have been sewn together.

## V2+ (QoL)

* Rendering of particle systems.
* Easier texture/model/sound browsing.
* Viewing collision models.
