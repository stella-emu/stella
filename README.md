This is my fork of the VCS emulator [Stella](http://stella.sourceforge.net/) for
integrating the [6502.ts](https://github.com/6502ts/6502.ts) TIA core into Stella.
The upstream repo is hosted on [Sourceforge](https://sourceforge.net/projects/stella/).

# Usage

TIA cores can be switched by changing the `tia.core` option (via command line
`-tia.core` or via config file) to either `default` (the old core) or `6502.ts`
(the new core). The defaut setting is `6502.ts`. Setting `loglevel` to 1 or higher
will show the selected core on startup.

# Reporting issues

Please check the list of known issues first (see below) before reporting new ones.
If you encounter any issues, please open a new issue on the project
[issue tracker](https://github.com/DirtyHairy/stella/issues) and / or in the corresponding
[thread](http://atariage.com/forums/topic/259633-testing-the-new-stella-tia-core/) on
AtariAge.

# Known issues

Please check out the [issue tracker](https://github.com/DirtyHairy/stella/issues) for
a list of all currently known issues.