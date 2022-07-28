## V0 (minimum required)

* Use an asset scheme where content is sourced from VPKs, plus directories on disk.
* Add support for reading VPKs and VMT/VTF files for textures.
* Add support for reading TF2 FGD. If there's fancy stuff in there (eg. entity I/O), just ignore it for now.
* Add support for exporting MAP to VMF. Unclear whether this would be better implemented as an external utility or an internal routine.

### VPK Support

The file system config within the game's config file describes where assets should be loaded from (`GameConfigParser::parsePackageFormatConfig()`). This is specified via a search path and the package format. The package format has a format string and a list of supported extensions.

`GameFileSystem::addFileSystemPackages()` reads the package format string and creates the relevant file system adapter. Additional search paths can also be specified via enabled "mods" for the game - we may not need to rely on this, as `tf` may be the only search path required if the purpose of the path is to encapsulate _all_ assets.

Potentially useful libraries for reading assets:

* https://github.com/panzi/unvpk for VPKs
* https://github.com/panzi/VTFLib for VTFs

### Questions to Resolve

* Can the TF2 FGD be loaded as-is, or does TB fall over? If the syntax is supported as-is, that makes our life easier.
* Should MAP2VMF be written as an external utility, or written into TB?

## V1

* Add support for reading MDL files for models.
* Add support for entity I/O.

### MDLs

There is no obvious library on GitHub for reading MDLs. [Assimp](https://github.com/assimp/assimp/blob/master/code/AssetLib/MDL/MDLLoader.h#L142) has an apparently incomplete implementation, so we could perhaps use this to write our own.

### Entity I/O

This implementation will be non-trivial. It will require at least a new UI to be built to configure I/O properties. It will also potentially need to visualise entity links as TB does for other things, allow picking entities by clicking on them, and tie into problem reporting.

## V2

* Add support for displacement editing (geometry and blending).

## V2+ (QoL)

* Rendering of particle systems.
* Easier texture/model browsing.
* Viewing collision models.
