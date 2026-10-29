{
  description = "Test joysticks and game controllers through SDL 1.2, SDL2 and SDL3";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";

    sdl_gamecontrollerdb.url = "github:gabomdq/SDL_GameControllerDB";
    sdl_gamecontrollerdb.flake = false;
  };

  outputs = { self, nixpkgs, sdl_gamecontrollerdb }:
    let
      lib = nixpkgs.lib;
      systems = [ "x86_64-linux" "aarch64-linux" "aarch64-darwin" ];
      forAllSystems = lib.genAttrs systems;

      # gamepkgstd VER-5: VERSION is the only place the version is written.
      versionBase = lib.fileContents ./VERSION;
      gitRev = self.shortRev or self.dirtyShortRev or "dirty";
      version =
        if lib.hasInfix "-dev" versionBase
        then "${versionBase}.${toString (self.revCount or 0)}+g${gitRev}"
        else versionBase;

      programs = [ "sdl1-jstest" "sdl2-jstest" "sdl3-jstest" ];

      # nixpkgs' reuse 6.2.0 fails to produce its man output (sphinx); the
      # tool alone is all the check and the dev shell need.
      reuseTool = pkgs: pkgs.reuse.overridePythonAttrs (old: {
        outputs = [ "out" ];
        sphinxBuilders = [ ];
        build-system = lib.filter (p: !(lib.hasPrefix "sphinx-hook" (p.name or "")))
          old.build-system;
        doCheck = false;
      });

      nativePackages = pkgs:
        let
          callSdlJstest = args: pkgs.callPackage ./sdl-jstest.nix ({
            inherit version sdl_gamecontrollerdb;
            SDL = pkgs.SDL;
            SDL2 = pkgs.SDL2;
            SDL3 = pkgs.sdl3;
          } // args);
        in {
          # All three programs (FLAKE-1: default and sdl-jstest are the same).
          sdl-jstest = callSdlJstest { };
          sdl1-jstest = callSdlJstest { buildSdl2 = false; buildSdl3 = false; };
          sdl2-jstest = callSdlJstest { buildSdl = false; buildSdl3 = false; };
          sdl3-jstest = callSdlJstest { buildSdl = false; buildSdl2 = false; };
        };
    in
    {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          native = nativePackages pkgs;
        in native // {
          default = native.sdl-jstest;
        });

      # FLAKE-5: `nix run` starts sdl2-jstest; each program by name.
      apps = forAllSystems (system:
        let
          pkg = self.packages.${system}.sdl-jstest;
          app = prog: {
            type = "app";
            program = "${pkg}/bin/${prog}";
            meta.description = "Run ${prog}";
          };
        in lib.genAttrs programs app // {
          default = app "sdl2-jstest";
          sdl-jstest = app "sdl2-jstest";
        });

      # gamepkgstd FLAKE-9..12: `nix flake check` builds every package,
      # checks every app's program and runs the extra checks below. Nothing
      # needs a display or a joystick.
      checks = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          pkg = self.packages.${system}.sdl-jstest;
          appChecks = lib.mapAttrs' (name: app:
            lib.nameValuePair "app-${name}"
              (pkgs.runCommand "sdl-jstest-check-app-${name}" { } ''
                test -x ${app.program}
                touch $out
              '')) self.apps.${system};
          forPrograms = f: lib.concatMapStringsSep "\n" f programs;
        in self.packages.${system} // appChecks // {
          version = pkgs.runCommand "sdl-jstest-check-version" { } (forPrograms (prog: ''
            out_v=$(${pkg}/bin/${prog} --version)
            if [ "$out_v" != "${prog} ${version}" ]; then
              echo "--version printed '$out_v', expected '${prog} ${version}'" >&2
              exit 1
            fi
          '') + "\ntouch $out\n");

          help = pkgs.runCommand "sdl-jstest-check-help" { } (forPrograms (prog: ''
            ${pkg}/bin/${prog} --help > /dev/null
            # OPT-1: unknown options fail with status 2.
            if ${pkg}/bin/${prog} --no-such-option 2> /dev/null; then
              echo "${prog} accepted --no-such-option" >&2
              exit 1
            fi
          '') + "\ntouch $out\n");

          # The startup report and --list without display or joystick
          # (dummy video, LOG-6).
          report = pkgs.runCommand "sdl-jstest-check-report" { } (forPrograms (prog: ''
            SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
              ${pkg}/bin/${prog} --list > list.out 2> report.err
            grep -q "^${prog}: ${version}" report.err
            grep -q "^joysticks: " report.err
          '') + ''
            grep -q "^gamecontrollerdb: [0-9]* mappings" report.err
            touch $out
          '');

          reuse = pkgs.runCommand "sdl-jstest-check-reuse" {
            nativeBuildInputs = [ (reuseTool pkgs) ];
          } ''
            cd ${self}
            HOME=$TMPDIR reuse lint
            touch $out
          '';

          man = pkgs.runCommand "sdl-jstest-check-man" {
            nativeBuildInputs = [ pkgs.mandoc ];
          } (forPrograms (prog: ''
            # stdenv fixup gzips man pages
            mandoc -T lint -W warning ${pkg}/share/man/man1/${prog}.1.gz
          '') + "\ntouch $out\n");

          layout = let
            files = [ "share/sdl-jstest/gamecontrollerdb.txt" ] ++ lib.concatMap (prog: [
              "bin/${prog}"
              "share/man/man1/${prog}.1.gz"
              "share/applications/${prog}.desktop"
              "share/metainfo/${prog}.metainfo.xml"
              "share/icons/hicolor/scalable/apps/${prog}.svg"
            ] ++ map (size: "share/icons/hicolor/${size}x${size}/apps/${prog}.png")
                [ "16" "32" "48" "64" "128" "256" ]) programs;
          in pkgs.runCommand "sdl-jstest-check-layout" { } ''
            for f in ${lib.escapeShellArgs files}; do
              if [ ! -e ${pkg}/$f ]; then
                echo "missing: $f" >&2
                exit 1
              fi
            done
            touch $out
          '';

          integration = pkgs.runCommand "sdl-jstest-check-integration" {
            nativeBuildInputs = [ pkgs.desktop-file-utils pkgs.appstream ];
          } (forPrograms (prog: ''
            desktop-file-validate ${pkg}/share/applications/${prog}.desktop
            # The app ID is the bare executable name on purpose (INT-1).
            appstreamcli validate --no-net \
              --override=cid-desktopapp-is-not-rdns=info \
              ${pkg}/share/metainfo/${prog}.metainfo.xml
          '') + "\ntouch $out\n");
        });

      # gamepkgstd DEV-1..8: build inputs plus sdl-jstest-{configure,build,
      # run,run-gdb,test} as real executables, so `nix develop -c
      # sdl-jstest-run` works.
      devShells = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          pkg = self.packages.${system}.sdl-jstest;

          preamble = ''
            set -euo pipefail
            if [ -z "''${SDL_JSTEST_SOURCE:-}" ]; then
              echo "$0: SDL_JSTEST_SOURCE is not set (enter the shell with: nix develop)" >&2
              exit 1
            fi
            SDL_JSTEST_BUILD_DIR="''${SDL_JSTEST_BUILD_DIR:-/tmp/sdl-jstest-build}"

            canon_path() {
              if [ -d "$1" ]; then ( cd "$1" && pwd ); else printf '%s\n' "''${1%/}"; fi
            }

            # sdl-jstest-run [sdl1|sdl2|sdl3|sdlN-jstest] ARGS: which program,
            # sdl2-jstest by default.
            pick_program() {
              PROGRAM=sdl2-jstest
              case "''${1:-}" in
                sdl1|sdl2|sdl3) PROGRAM="$1-jstest"; return 0 ;;
                sdl1-jstest|sdl2-jstest|sdl3-jstest) PROGRAM="$1"; return 0 ;;
              esac
              return 1
            }
          '';

          configure = pkgs.writeShellScriptBin "sdl-jstest-configure" (preamble + ''
            mkdir -p "$SDL_JSTEST_BUILD_DIR"
            cmake -S "$SDL_JSTEST_SOURCE" -B "$SDL_JSTEST_BUILD_DIR" -G Ninja \
              -DCMAKE_BUILD_TYPE="''${CMAKE_BUILD_TYPE:-Debug}" \
              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
              -DWARNINGS=ON \
              "$@"
            # clangd finds compile_commands.json through build/ (DEV-8).
            if [ ! -e "$SDL_JSTEST_SOURCE/build" ] || [ -L "$SDL_JSTEST_SOURCE/build" ]; then
              ln -sfn "$SDL_JSTEST_BUILD_DIR" "$SDL_JSTEST_SOURCE/build"
            fi
          '');

          build = pkgs.writeShellScriptBin "sdl-jstest-build" (preamble + ''
            # Reconfigure when the cache is missing or the checkout moved (DEV-5).
            cache="$SDL_JSTEST_BUILD_DIR/CMakeCache.txt"
            cached=""
            if [ -f "$cache" ]; then
              cached=$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "$cache" | head -n1)
            fi
            if [ -z "$cached" ] || [ ! -d "$cached" ] ||
               [ "$(canon_path "$cached")" != "$(canon_path "$SDL_JSTEST_SOURCE")" ]; then
              sdl-jstest-configure
            fi
            cmake --build "$SDL_JSTEST_BUILD_DIR" "$@"
          '');

          run = pkgs.writeShellScriptBin "sdl-jstest-run" (preamble + ''
            if pick_program "''${1:-}"; then shift; fi
            sdl-jstest-build
            # The build tree has bin/ next to share/sdl-jstest/ (DIR-3), so
            # no --data-dir is needed.
            exec "$SDL_JSTEST_BUILD_DIR/bin/$PROGRAM" "$@"
          '');

          runGdb = pkgs.writeShellScriptBin "sdl-jstest-run-gdb" (preamble + ''
            if pick_program "''${1:-}"; then shift; fi
            sdl-jstest-build
            gdb -q \
              -ex "set pagination off" \
              -ex "set confirm off" \
              -ex "set debuginfod enabled off" \
              -ex run \
              -ex 'python
            try:
              if int(gdb.parse_and_eval("$_exitcode")) == 0:
                gdb.execute("quit")
            except Exception:
              pass
            ' \
              --args "$SDL_JSTEST_BUILD_DIR/bin/$PROGRAM" "$@"
          '');

          test = pkgs.writeShellScriptBin "sdl-jstest-test" (preamble + ''
            sdl-jstest-build
            ctest --test-dir "$SDL_JSTEST_BUILD_DIR" --output-on-failure "$@"
          '');
        in {
          default = pkgs.mkShell {
            inputsFrom = [ pkg ];
            packages = [
              pkgs.cmake
              pkgs.ninja
              pkgs.ccache
              pkgs.mandoc
              (reuseTool pkgs)
              configure
              build
              run
              runGdb
              test
            ] ++ lib.optional (lib.meta.availableOn pkgs.stdenv.hostPlatform pkgs.gdb) pkgs.gdb;
            shellHook = ''
              export SDL_JSTEST_SOURCE="$PWD"
              export SDL_JSTEST_BUILD_DIR="''${SDL_JSTEST_BUILD_DIR:-/tmp/sdl-jstest-build}"
              export CMAKE_C_COMPILER_LAUNCHER="''${CMAKE_C_COMPILER_LAUNCHER:-ccache}"
              echo "sdl-jstest: sdl-jstest-{configure,build,run,run-gdb,test};"
              echo "  sdl-jstest-run [sdl1|sdl2|sdl3] ARGS (default sdl2), build in $SDL_JSTEST_BUILD_DIR"
            '';
          };
        });
    };
}
