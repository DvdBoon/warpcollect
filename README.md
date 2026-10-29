# warpcollect

Link driver for the WarpOS/WarpUP target of the
[mos2wos](https://github.com/DvdBoon) cross-compiler. The `warpup` specs
file points GCC at it with `*linker: warpcollect`, so it runs in place of
`collect2` and is handed the whole link command line.

What it does, in order:

1. runs the linker on the arguments GCC passed it;
2. with `KEEPWARPELF=1`, keeps the unstripped ELF as `<output>.warpelf`;
3. strips `.gnu.attributes`;
4. converts the ELF into an Amiga hunk executable.

## Converters

| Host | Converter |
| --- | --- |
| Linux | `elf2exe3` only |
| MorphOS / WarpOS | `elf2exe3`, falling back to `elf2exe2` |

`elf2exe3` is currently distributed for Linux x86-64 only. `elf2exe2` runs
on MorphOS and WarpOS but not on Linux, where it is an AmigaOS
`loadseg()`-able binary and the kernel refuses it with `Exec format error`.

On MorphOS the fallback applies both when `elf2exe3` is absent and when it
fails. The second case matters because a Linux `elf2exe3` sitting in
`GG:bin/` is readable, so the probe finds it, and running it on MorphOS
fails; without the fallback that would kill the build. Retrying is safe
because `elf2exe3` validates its input before writing anything, so a failed
run leaves the linked ELF byte-identical.

If no converter is installed at all, the linked ELF is left in place and
the build succeeds. A converter that runs and fails is an error: the output
is removed and warpcollect exits 20.

## Building

Host build:

    make

MorphOS-hosted builds, which need the cross-compiler on `$PATH`:

    make mos

The linker path is baked in at compile time and can be overridden:

    ppc-morphos-gcc -O2 -DLD_PATH='"/gg/ppc-warpos/bin/ld"' -o warpcollect warpcollect.c

## Install

| Path | Build |
| --- | --- |
| `/gg/bin/warpcollect` | host build, drives `/gg/bin/ppc-morphos-ld` |
| `/gg/ppc-warpos/bin/warpcollect` | MorphOS build, drives `/gg/ppc-warpos/bin/ld` |

`/gg/bin` must be on `$PATH`: GCC's own exec-prefix search does not cover
it, so without it the driver cannot find warpcollect at all.

## Changes in 0.5

**The link command line was being thrown away.** Arguments were appended
with

    sprintf(exec, "%s %s", exec, argv[i]);

whose destination buffer is also a source argument. That is undefined
behaviour. Under glibc only the final append survives, so the command
collapsed to the last item on the link line — the `*endfile` spec — and
`system()` tried to execute `end.o` as a shell command:

    sh: 1: /gg/ppc-warpos/lib/end.o: Exec format error

Arguments are now appended in place. The same bug was present in the
MorphOS build, where the older C library happened to tolerate it.

Also in this release:

- Failures returned `errno`, which says nothing about a child process's
  exit status and is usually 0, so a failed link was reported as success.
  They return 20 now.
- The `KEEPWARPELF` `if` had no braces, so the error check below it ran
  unconditionally.
- `elf2exe3` support, with the fallback behaviour described above.
- `LD_PATH` is a compile-time macro, so the two MorphOS variants — which
  differed only in the linker they drove — build from one source.
- The host build spells out `/gg/bin/ppc-morphos-strip`, matching the
  already-absolute linker path, so a link no longer depends on `/gg/bin`
  being on the caller's `$PATH` at that point.

## Origin

warpcollect is part of the mos2wos SDK. The binaries in `gcc-mos2wos.lha`
carry `$VER: Warpcollect 0.3 (19.07.2000)`; this repository starts from the
0.4 source. The source carries no copyright or licence notice.
