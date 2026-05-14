<h1 align="center">Welcome to the aurorachat repository!</h1>
This is the Switch client for Aurorachat.<br>
For more clients and stuff, see the <a href="https://github.com/Unitendo/aurorachat">main repo</a>.
The license, code of conduct, and security/contributing guidelines in the main repo also apply here.

<br>This repository is <b>open</b> for contributions! If you'd like to, you may open a PR or an issue, contributing helps us as we develop aurorachat!

<h1 align="center">How to build aurorachat</h1>

Install devkitpro with the Switch development libraries and make, then execute the following commands based on your OS:

Windows:
```sh
pacman -S switch-harfbuzz switch-freetype switch-bzip2 switch-libpng switch-zlib switch-curl switch-mbedtls switch-sdl2_mixer libnx
git clone https://github.com/Unitendo/aurorachat-switch
cd aurorachat-switch
make
```

Arch Linux or other distros with pacman:
```sh
yay -S devkitpro-pacman
sudo dkp-pacman -S switch-harfbuzz switch-freetype switch-bzip2 switch-libpng switch-zlib switch-curl switch-mbedtls switch-sdl2_mixer libnx
git clone https://github.com/Unitendo/aurorachat-switch
cd aurorachat-switch
make
```

Other Linux distros without pacman:
```sh
sudo dkp-pacman -S switch-harfbuzz switch-freetype switch-bzip2 switch-libpng switch-zlib switch-curl switch-mbedtls switch-sdl2_mixer libnx
git clone https://github.com/Unitendo/aurorachat-switch
cd aurorachat-switch
make
```

(At least that's what I think you gotta do)