# C++ based Rhythm Game

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![GitHub stars](https://img.shields.io/github/stars/ItzFireable/untitled-rhythm-game.svg?style=social&label=Star)](https://github.com/ItzFireable/untitled-rhythm-game)

> [!NOTE]
> Want to help? Have a question? Feel free then to join our [Discord server](https://discord.gg/XMrbDvZYSm) for any discussion.

## Information

Since this is early on in development, there is a lot of things missing. I have a lot of plans, since this is an early "concept".
- Refactor this to use OpenGL, getting rid of SDL.
- Design most of the states, add settings configuration, add skin/HUD editor.
- Rework audio management, maybe move it to BASS from OpenAL for better audio playback.

## Get started

> [!NOTE]
> Detailed instructions will come later, good luck to anyone trying this out.

To get the repository, you need to clone it with --recurse-submodules.
There are .bat files in the /tools folder to get it loaded up on Windows, but you do need `CMake` for it. I recommend using [w64devkit](https://github.com/skeeto/w64devkit)!

## Dependencies

This project uses `SDL3`, `SDL3_ttf`, `SDL3_image` and `OpenAL` as its dependencies. It also uses [stb_vorbis.c](https://github.com/nothings/stb/blob/master/stb_vorbis.c), [dr_mp3.h](https://github.com/mackron/dr_libs/blob/master/dr_mp3.h) and [dr_wav](https://github.com/mackron/dr_libs/blob/master/dr_wav.h).

# Contributing

If you want to contribute to the project, feel free to fork the repository and submit a pull request. We are open to any suggestions and improvements.
