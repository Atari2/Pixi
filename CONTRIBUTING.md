# Contributing

## Should I implement feature "X"?
Always ask before jumping into development, if the main maintaner decides that your idea isn't worth it you just wasted hours of your time into something that won't get merged. Please always ask before coding stuff up.

## Pixi does "X" but I want "Y"!
If Pixi does X and you want Y, explain your idea to the maintainer, if it sounds reasonable, they may implement it! Be prepared to receive a "No" thought, especially if the scope of your idea is big. This is all volunteer work! If you <i>really wish</i> for something to be implemented... well, good news, this repository accepts pull requests.

## I found a bug and I want to fix it.
That is really nice of you, just please remember to always, <i>always</i> check if the bug is in fact in pixi or it's coming from some other code. In case it's really coming from the tool, open an issue on the GitHub repo so I can easily find it.

## Areas where you can contribute
- C++ (the main program, everything that is in src/)
- Asm (main.asm, extended.asm, all the routines, etc...)

### C++

#### <b>Requirements</b>
To contribute to the C++ side of Pixi, you should know basic C++ at the very least, semantics, syntax and classes, the codebase does heavy use of C++17 and that's the standard it follows. If you have questions I'll always be available, as long as you know the basics.

#### <b>Code style</b>
Use clang-format on Visual Studio formatting style settings. Always.

### ASM

#### <b>Requirements</b>
Decent knowledge of 65816 asm and willingness to optimize code for speed.

#### <b>Code style</b>
No particular code style. 

If you're making a shared routine, please try to keep a normal formatting and not more than 2 instruction per line (e.g. SEC : SCB #$10 is fine). Add a small comment at the top of the routine with your name, a small description of what the routine does, what input it takes and what output it returns, as well as eventual ram usage.
Everything has to be SA-1 hybrid.

## Why should you contribute?
Well, of course you should contribute because this is an open source tools made by volunteers for a community. So any love and help is appreciated!
