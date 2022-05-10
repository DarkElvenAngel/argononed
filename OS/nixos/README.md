# Argon ONE Daemon on NixOS

## Support Status
**Supported Devices**:
- Argon ONE case for Raspberry Pi 4B (v2, M.2)
- Argon Artik Fan HAT (on RPi 4B)

**Untested Devices** (it probably works but I couldn't test it)(Someone please test it and maybe gives some feedback):
- Argon ONE case for Raspberry Pi 3B
- Argon Artik Fan HAT (on other Pi's)

Something more probably needs to appear here I guess, but I forgor what it should be💀

## Installation

### Non-Flake configurations
Find youselves a way to merge this snippet into your own `configuration.nix` (a new module also suffice):
```nix
{ lib, pkgs, config, ...}:
{
  imports = let
    argononed = fetchGit {
      url = "https://gitlab.com/DarkElvenAngel/argononed.git";
      ref = "master"; # Or any other branches deemed suitable
    };
  in
    [ "${argononed}/OS/nixos" ];

  services.argonone = {
    enable = true;
    logLevel = 4;
    settings = {
      fanTemp0 = 36; fanSpeed0 = 10;
      fanTemp1 = 41; fanSpeed1 = 50;
      fanTemp2 = 46; fanSpeed2 = 80;
      hysteresis = 4;
    };
  };
}
```

And done!

### Flake configurations
Sorry but this repo won't be available as a flake, but this repo doesn't have any external dependencies so it doesn't hurt your config's purity. Alternatively try this:

In `flake.nix`, add:
```nix
{
  inputs.argononed = {
    url = "gitlab:ykis-0-0/argononed/feat/nixos";
    flake = false;
  };
}
```

In a new module (or `configuraion.nix`):
```nix
{ argononed, ...}:
{
  imports = [ "${argononed}/OS/nixos" ];

  services.argonone = {
    enable = true;
    logLevel = 4;
    settings = {
      fanTemp0 = 36; fanSpeed0 = 10;
      fanTemp1 = 41; fanSpeed1 = 50;
      fanTemp2 = 46; fanSpeed2 = 80;
      hysteresis = 4;
    };
  };
}
```

That's all!
Bye!

No I'm just kidding.
## To-Do List (If you want to help)
* `services.logrotate.extraConfig` have been deprecated and going to be removed in NixOS 22.11 ☹ We need to find a way to adapt this.

## Down the rabbit hole(?)
[Welcome.](./SPOILER.md)
