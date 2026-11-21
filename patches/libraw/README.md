# LibRaw integration changes

`dngsdk-glue.patch` records the integration changes relative to the upstream
LibRaw base, `f74ddd995c5447458f132a5377ca0f4b394dff6e`. It is retained as a
review record. The build helper recognizes an already-patched submodule and
only applies the patch when it is absent.

The fork is available at [BR1py/LibRaw](https://github.com/BR1py/LibRaw), and
DT's `.gitmodules` points to it. The local LibRaw `origin` uses this fork;
`upstream` retains `LibRaw/LibRaw`.

DT pins published commit `1b58cd78f3b1ab3cc3cd930650fcc7d2943367c6` on branch
[`0.22.0-dt_dngio.1`](https://github.com/BR1py/LibRaw/tree/0.22.0-dt_dngio.1).
That commit contains the integration changes and the fork's README explanation.
The branch name records the LibRaw base version; DT uses the exact commit pin
independently of the fork's default branch.
