[![It works, why?](https://i.redd.it/b1g2tyl2y2i01.png)](https://www.reddit.com/r/ProgrammerHumor/comments/7ztypx/it_works_why/)

Let me try to explain this.

First thing first. We only focus on `default.nix`, `pkg.nix`, `override.in` and `patches/` here, [Go to the previous checkpoint](../README.md) if you don't know what's `OS.conf`.

# `pkg.nix`
### TL;DR / What it emits:
It builds this codebase in Nix, which `default.nix` will make use of.

### What's inside the output dir
* `argononed` (at `out/argononed`)
* `argonone-cli` (at `out/bin/argonone-cli`)
* `argonone-shutdown` (at `out/argonone-shutdown`)
* `argonone.dtbo` (at `out/aronone.dtbo`)
* `argonone.logrotate` (at `out/argonone.logrotate`)
* Bash Completions at somewhere inside `out/`. Maybe I'll find and fill it back here, but mostly I'wont

### What it consumes
The whole `pkg.nix` is indeed a function, it's parameters:
```nix
{
  lib, stdenv, # Nixpkgs things
  nix-gitignore, # Explained later
  dtc, installShellFiles, # Dependencies
  logLevel ? 5, # Log verbosity of the daemon, the only thing we are interested on
  ... # Some other things, but who cares?
}: # blah blah blah
```

Where's did we tell Nix about the source code? In line #10:
```nix
  src = cleanSrc;
```
ok how clean it is? Lines #9-#31:
```nix
  cleanSrc = let
    repo = ../..;
    ignores = /* too long didn't copy */;
  in builtins.path {
    path = repo;
    name = "argononed-src";
    filter = nix-gitignore.gitignoreFilterPure (_: _: true) ignores repo;
  };
```
*Translation*: We take the whole `repo` (`../..`) and a `.gitignore`-esque list (`ignores`, go see the code), pumping it through a file filter (`nix-gitignore.gitignoreFilterPure`) and grab those passed the ruthless filter.

Why? so that OS-specific changes (other than those occurred here) doesn't trigger a rebuild for NixOS (normally would I say).

### How this get the whole thing built?
Nix took care most of it for us, so I'm just listing what's changed here
1. pre-`configurePhase` (i.e. before Nix run `<repo>/configure` for us):
    * `TARGET_DISTRO` is set to and exported as `nixos`
2. `patchPhase` (things in `./patches` are patched into the codebase)
3. `buildPhase`:
    * <a name="log-level"></a>`make` is called with `LOGLEVEL=${logLevel}`, specified in the arguments
4. `installPhase` (things get copied into Nix store)
5. `postInstall`:
    * `OS/_common/argonone-cli-complete.bash` is installed to Nix store as shell completions

# `default.nix`
A [NixOS Module](https://nixos.wiki/wiki/Module), when enabled will do these:
1. Build `pkg.nix` with [`logLevel`](#log-level) specified in `services.argonone.logLevel`
2. Enable I2C by doing `hardware.i2c.enable = true`;
3. Add a `argononed.service` File to the system
    > Contents are hand-crafted to replicate whats in `OS/_common/argononed.service`, so human attention is required to follow the updates
4. And the shutdown handler `argonone-shutdown`
5. Add the these device-tree overlays to the flattened DT:
    - `argonone.dtbo`
    - An ad-hoc `.dtbo` for the fan settings, generated on each change of the relevant settings (i.e. anything within `services.argonone.settings`)
6. Finally also a logrotate config (an include of `OS/_common/argononed.logrotate`)
    > `services.logrotate.extraConfig` have been deprecated and going to be remove in NixOS 22.11 ☹ We need to find a way to adapt this.

# `override.in`
Basically what it does here is overriding all of the (un)install targets, and replace them with a version of copying to the Nix store, since *that*'s the way of installing things on NixOS.

Oh. And the uninstall target is left undefined, since... We're talking about NixOS here so you know\
![We don't do that here](https://i.kym-cdn.com/entries/icons/original/000/026/366/pather.jpg)\
yep.

# `patches/`
Some NixOS-specfic patches or workarounds (emm I mean hacks but "workarounds" sounds more legit)
### `nixos.patch`
Why?
> * NixOS will outright ignore if compatibility target is set to `bcm2708`
> * Name of I2C node seems to be different

## `shutdown.patch`
Why?
> * The assumption of paths by `argononed` doesn't hold in NixOS

That's all, byebye!
