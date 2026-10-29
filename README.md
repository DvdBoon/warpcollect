# warpcollect

Link driver for the WarpOS/WarpUP target of the mos2wos cross-compiler. The
`warpup` specs file points GCC at it with `*linker: warpcollect`, so it runs
in place of `collect2` and is handed the whole link command line.

What it does, in order:

1. runs the linker on the arguments GCC passed it;
2. with `KEEPWARPELF=1`, keeps the unstripped ELF as `<output>.warpelf`;
3. strips `.gnu.attributes`;
4. converts the ELF into an Amiga hunk executable.

## Converters

The conversion in step 4 is done by [elf2exe3](https://github.com/DvdBoon/elf2exe3),
or by the older `elf2exe2` that ships with the SDK.

| Build | Converter |
| --- | --- |
| Host (`-D_LINUX_`) | `elf2exe3` only |
| MorphOS / WarpOS | `elf2exe3`, falling back to `elf2exe2` |

`elf2exe3` is portable C99 and builds for any host; prebuilt converters are
provided for Windows, Linux x86-64 and AmigaOS 68k. `Elf2Exe2` is a
binary-only AmigaOS tool, running on WarpOS and the platforms that emulate
it, MorphOS and AmigaOS 4; on Linux it fails with `Exec format error`.

That difference is the whole point. The SDK's own readme lists obtaining
Elf2Exe2's source as an open wish, noting that otherwise "any host with
target ppc-morphos could compile also for WarpOS" — which is exactly what
`elf2exe3` makes possible. A host build therefore has nothing to fall back
to, and the fallback in the MorphOS build is for installs that still have
`Elf2Exe2` but not `elf2exe3`.

On MorphOS the fallback applies both when `elf2exe3` is not found and when
running it fails. The second case matters because the probe only tests that
the file is readable, so a build of `elf2exe3` for a different platform
sitting in `GG:bin/` would be picked up and then fail to run; without the
fallback that would kill the build. Retrying is safe because `elf2exe3`
validates its input before writing anything, so a failed run leaves the
linked ELF byte-identical.

If no converter is found at all, the linked ELF is left in place and the
build succeeds. A converter that runs and fails is an error: the output is
removed and warpcollect exits 20.

### Finding the converter

The two builds look for the converter differently:

- the host build scans `$PATH` and requires the file to be executable;
- the MorphOS build has no `$PATH` to scan, so it probes the current
  directory, `GG:bin/` and `C:`, and only tests that the file is readable.

A copy of `elf2exe3` that is present but not executable therefore looks
*absent* to the host build, and the link silently produces an ELF rather
than an executable. That is the first thing to check when the output is not
a hunk file.

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

Arguments are now appended in place. The same construct was present in the
MorphOS build, where the older C library happened to tolerate it.

Also in this release:

- Failures returned `errno`, which says nothing about a child process's
  exit status and is usually 0, so a failed link was reported as success.
  They return 20 now, matching the convention `elf2exe3` uses for errors.
- The `KEEPWARPELF` `if` had no braces, so the error check below it ran
  unconditionally.
- `elf2exe3` support, with the fallback behaviour described above.
- `LD_PATH` is a compile-time macro, so the two MorphOS variants — which
  differed only in the linker they drove — build from one source.
- The host build spells out `/gg/bin/ppc-morphos-strip`, matching the
  already-absolute linker path, so a link no longer depends on `/gg/bin`
  being on the caller's `$PATH` at that point.

## Note on invocation

warpcollect calls the converter as `elf2exe3 <output> <output>`, converting
in place, which is what the original did with `Elf2Exe2`. Both tools
document distinct names — `elf2exe3 [options] input.elf output` and
`Elf2Exe program.elf program.exe [verbose]` — and neither says the two may
be the same file, so this relies on unspecified behaviour. Tested against
elf2exe3 0.1.

## Credits

warpcollect was written by **Jarmo Laakkonen**, who took it through 0.3 in
2000 — his readme calls it "a quick hack to integrate Elf2Exe2 with
GCC-WarpUP", and records that 0.2 fixed a crash on big command lines and
0.3 improved error handling. That readme is preserved upstream in
[Sakura-IT/WarpOS_Ports](https://github.com/Sakura-IT/WarpOS_Ports/tree/master/warpcollect),
alongside the Makefiles for the host, MorphOS and WarpOS builds.

**Dennis Boon** adapted it for MorphOS and for Unix hosts as part of the
gcc-mos2wos SDK, distributed on Aminet as `dev/gcc/gcc-mos2wos`, and
released 0.4 in 2021. That is the version this repository starts from.

0.5 is the release here.
