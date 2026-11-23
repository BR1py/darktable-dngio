# LibRaw integration changes

`dngsdk-glue.patch` records the integration changes relative to the upstream
LibRaw base, `f74ddd995c5447458f132a5377ca0f4b394dff6e`. It is retained as a
review record. The build helper recognizes an already-patched submodule and
only applies the patch when it is absent.

The fork is available at [BR1py/LibRaw](https://github.com/BR1py/LibRaw), and
DT's `.gitmodules` points to it. The local LibRaw `origin` uses this fork;
`upstream` retains `LibRaw/LibRaw`.

DT's `5.6.2-dngio.3` release pins LibRaw commit
`9f992f377548359a22067782f9029d6948cb9631`. It includes the integration changes and
explicit JPEG XL DNG linearization-table handling. The commit is published on
branch [`5.6.2-dngio.3`](https://github.com/BR1py/LibRaw/tree/5.6.2-dngio.3).

The previous published pin was `1b58cd78f3b1ab3cc3cd930650fcc7d2943367c6` on
branch [`0.22.0-dt_dngio.1`](https://github.com/BR1py/LibRaw/tree/0.22.0-dt_dngio.1).
DT uses the exact commit pin independently of the fork's default branch.
