# C++ based Rhythm Game

## Information

This is a basic rhythm game project I am doing for fun using C++. I do not have a lot of experience, nor do I have plans to continue this for a long time, but we will see.
It currently has:
- Basic gameplay
- Basic song select & result screen
- Support for .sm and .osu files
- A dynamic audio system through streams, using OpenAL

## Todo

Since this is early on in development, there is a lot of things missing. I have a lot of plans, since this is an early "concept".
- Refactor this to use OpenGL, getting rid of SDL.
- Design most of the states, add settings configuration, add skin/HUD editor.
- Rework audio management, maybe move it to BASS from OpenAL for better audio playback.

I am open to any contributions, feel free to just hit me up on Discord to get started (@fireable).
I will open some sort of a Discord server later on for the project, so people can be kept up to date.

## Get started
###### Detailed instructions will come later, good luck to anyone trying this out.

To get the repository loaded, you need to clone it with --recurse-submodules.
There are .bat files to get it loaded up on Windows, depending on CMake. 


## Resources
This project uses `SDL3`, `SDL3_ttf`, `SDL3_image` and `OpenAL` as its dependencies. It also uses [stb_vorbis.c](https://github.com/nothings/stb/blob/master/stb_vorbis.c), [dr_mp3.h](https://github.com/mackron/dr_libs/blob/master/dr_mp3.h) and [dr_wav](https://github.com/mackron/dr_libs/blob/master/dr_wav.h).