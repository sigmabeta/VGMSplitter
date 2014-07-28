VGMSplitter
======

A tool that splits music files ripped from video game soundtracks into multiple WAV files, each containing a single voice.

Has been tested with & supports the following formats:

 * SPC (Super NES)
 * NSF (NES)
 * VGM (Sega Genesis)
 * GBS (Game Boy / Color)

VGMSplitter uses Blarg's [game-music-emu] (https://code.google.com/p/game-music-emu/) library, so it's likely to work with a few other consoles' sound formats as well, but has not been tested with them.


Usage
-----

Place any files you want to split into a subidrectory "in". Run the executable and the resulting files will be placed in the "out" subdirectory, which will be created if missing.

In the case of sound formats which contain the soundtrack for the entire game in a single file (NSF, GBS, etc.) all tracks will be split. Be careful with this, as it can easily fill your hard drive with GiB's of large WAV files.

Only supports Linux (and maybe OS X?) for now. Sorry Windows!

TODO
-----
 * Support Windows
 * A GUI maybe? 
 * The ability to do just one track at a time
