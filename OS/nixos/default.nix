{ lib, pkgs, config, options, ... }:
{
  options.services.argonone = let
    inherit (lib) mkOption;
    inherit (lib.types) ints;
    getOrd = n: (toString (n + 1)) + (builtins.elemAt [ "st" "nd" "rd" ] n);
    mkNumOpt = type: n: default: mkOption {
      description = "The ${getOrd n} setting of fan ${type}";
      inherit default;
      type = ints.u8;
    };
  in {
    enable = lib.mkEnableOption "Argon ONE Fan and Button Daemon Service";
    logLevel = mkOption {
      description = ''
        Verbosity of logging to `/var/lib/argononed.log`,
        valid values ranging from 0 of turning off logging completely,
        then FATAL, CRITICAL, ERROR, WARNING, INFO, and DEBUG, with values 1 to 6
      '';
      default = 5;
      type = ints.between 0 6;
    };

    settings = {
      fanTemp0 = mkNumOpt "temperature" 0 55;
      fanSpeed0 = mkNumOpt "speed" 0 10;

      fanTemp1 = mkNumOpt "temperature" 1 60;
      fanSpeed1 = mkNumOpt "speed" 1 55;

      fanTemp2 = mkNumOpt "temperature" 2 65;
      fanSpeed2 = mkNumOpt "speed" 2 100;

      hysteresis = mkOption {
        description = "Tolerance (in degrees Celsius) before changes of state";
        default = 3;
        type = ints.u8;
      };
    };
  };

  config = let
    self = config.services.argonone;
    arg1pk = pkgs.callPackage ./pkg.nix {
      inherit (self) logLevel;
    };
  in lib.mkIf self.enable {
    environment.systemPackages = [ arg1pk ];

    hardware.i2c.enable = true;

    systemd = {
      services.argononed = let
        target = "multi-user.target";
      in {
        enable = true;
        after = [ target ];
        wantedBy = [ target ];
        description = "Argon ONE Fan and Button Daemon Service";

        serviceConfig = {
          Type = "forking";
          ExecStart = "${arg1pk}/argononed";
          PIDFile = "/run/argononed.pid";
          Restart = "on-failure";
        };
      };

      shutdown = {
        argonone = "${arg1pk}/argonone-shutdown";
      };
    };

    hardware.deviceTree.overlays = [{
      name = "argonone";
      dtboFile = "${arg1pk}/argonone.dtbo";
    } {
      name = "argonone-enable-overlay";
      dtsText = let
        inherit (builtins) concatStringsSep;
        src = with self.settings;
          [fanSpeed0 fanSpeed1 fanSpeed2 fanTemp0 fanTemp1 fanTemp2 hysteresis];
        fanCfg = concatStringsSep " " (map toString src);
      in ''
        /dts-v1/;
        /plugin/;
        / {
          compatible = "brcm,bcm2711";
          fragment@0 {
            target = <&argonone>;
            __overlay__ {
              argonone-cfg = /bits/ 8 <${fanCfg}>;
            };
          };
        };
      '';
    }];

    services.logrotate.settings.argononed = {
      files = toString /var/log/argononed.log;
      rotate = 2;
      frequency = "daily";

      create = "660 root root";

      missingok = true;
      notifempty = true;

      compress = true;
      delaycompress = true;
    };
  };
}
