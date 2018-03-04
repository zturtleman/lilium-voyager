<img src="https://raw.githubusercontent.com/zturtleman/lilium-voyager/master/misc/lilium.png" width="64">

**Lilium Voyager** is a fork of [ioquake3](https://github.com/ioquake/ioq3) for running _Star Trek Voyager: Elite Force_ multiplayer. It is based on Thilo Schulz' [ioEF engine](http://thilo.kickchat.com/efport-progress/) (also known as iostvoyHM). The focus for Lilium Voyager is to maintain Elite Force multiplayer support on newer ioquake3 versions.

Differences from ioEF 1.38-rc1 (2011):

  * Better compatibility with newer operating systems (from ioq3).
  * VoIP uses Opus codec instead of Speex (from ioq3).
  * Player origin rounding is compatible with the original QVMs.
  * Network compatible with ioEF 1.37.
  * Dedicated servers are listed on official Raven master server.
  * Support for ioquake3's OpenGL2 renderer.
  * Client and server use separate config files like ioq3.

Lilium Voyager code commits: [compare/upstream...master](https://github.com/zturtleman/lilium-voyager/compare/upstream...master)

The source code for the Elite Force game, cgame, and ui code is not included as it remains under a non-free license.


## Compiling

Lilium Voyager is compiled using `make`. For details see [building ioquake3](http://wiki.ioquake3.org/Building_ioquake3).

The Visual Studio project files are not supported.

Additional make variables not in ioquake3:
* `USE_CODEC_MP3=1` - Enable MP3 support using libmad (defaults to 1).
* `USE_INTERNAL_MP3=1` - Use libmad in the local source tree (defaults to 1).


## Discussion

[Magical Clover Forum](https://forum.clover.moe). It supports creating an account or logging in using a GitHub account.


## License

Lilium Voyager is licensed under [the GNU GPLv2](COPYING.txt) (or at your option, any later version).


## Credits

* Quake 3 - id Software
* ioquake3 - ioquake3 contributors
* ioEF - Thilo Schulz & contributers
* Lilium Voyager - Zack Middleton


## Contributing

Please submit all patches at the [Magical Clover Forum](https://forum.clover.moe)
or as a GitHub pull request.

