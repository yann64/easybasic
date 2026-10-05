#include "sema.hpp"

#include <algorithm>

namespace easybasic {

ValueKind familyOf(TypeSuffix suffix) {
    switch (suffix) {
        case TypeSuffix::Float:
        case TypeSuffix::Double:
            return ValueKind::FloatFamily;
        case TypeSuffix::String:
            return ValueKind::StringFamily;
        default:
            return ValueKind::IntegerFamily;
    }
}

Sema::Sema(DiagnosticEngine& diagnostics) : diagnostics_(diagnostics) {
    registerStringLibBuiltins();
    registerMathLibBuiltins();
    registerMemoryLibBuiltins();
    registerFileLibBuiltins();
    registerDateLibBuiltins();
    registerThreadLibBuiltins();
    registerGuiLibBuiltins();
    registerBuiltinConstants();
}

void Sema::registerStringLibBuiltins() {
    struct Signature {
        const char* name;
        TypeSuffix returnSuffix;
        std::vector<TypeSuffix> paramSuffixes;
        std::size_t requiredParamCount;
    };
    // `Mid`'s optional `count` and `StrF`'s optional `decimals` reuse the
    // exact same "fewer args than paramSuffixes.size() means the trailing
    // ones were defaulted" mechanism a real Procedure's own default-valued
    // parameters already use - Codegen relies on the runtime function
    // itself supplying the C++-level default (see stringlib.hpp).
    static const std::vector<Signature> signatures = {
        {"len", TypeSuffix::Integer, {TypeSuffix::String}, 1},
        {"left", TypeSuffix::String, {TypeSuffix::String, TypeSuffix::Integer}, 2},
        {"right", TypeSuffix::String, {TypeSuffix::String, TypeSuffix::Integer}, 2},
        {"mid", TypeSuffix::String, {TypeSuffix::String, TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"ucase", TypeSuffix::String, {TypeSuffix::String}, 1},
        {"lcase", TypeSuffix::String, {TypeSuffix::String}, 1},
        {"trim", TypeSuffix::String, {TypeSuffix::String}, 1},
        {"ltrim", TypeSuffix::String, {TypeSuffix::String}, 1},
        {"rtrim", TypeSuffix::String, {TypeSuffix::String}, 1},
        {"str", TypeSuffix::String, {TypeSuffix::Integer}, 1},
        {"val", TypeSuffix::Integer, {TypeSuffix::String}, 1},
        {"strf", TypeSuffix::String, {TypeSuffix::Float, TypeSuffix::Integer}, 1},
        {"valf", TypeSuffix::Float, {TypeSuffix::String}, 1},
        {"chr", TypeSuffix::String, {TypeSuffix::Integer}, 1},
        {"asc", TypeSuffix::Integer, {TypeSuffix::String}, 1},
    };
    for (const auto& sig : signatures) {
        ProcedureInfo info;
        info.returnSuffix = sig.returnSuffix;
        info.paramSuffixes = sig.paramSuffixes;
        info.requiredParamCount = sig.requiredParamCount;
        procedures_[sig.name] = info;
    }
}

bool Sema::isStringLibBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "len", "left", "right", "mid", "ucase", "lcase", "trim", "ltrim",
        "rtrim", "str", "val", "strf", "valf", "chr", "asc",
    };
    return names.contains(lowerName);
}

void Sema::registerMathLibBuiltins() {
    struct Signature {
        const char* name;
        TypeSuffix returnSuffix;
        std::vector<TypeSuffix> paramSuffixes;
        std::size_t requiredParamCount;
    };
    // `Round`'s `mode` and `Random`'s optional `min` both reuse the same
    // "fewer call-site args than paramSuffixes.size()" mechanism as a real
    // Procedure's own defaulted trailing parameters (see
    // registerStringLibBuiltins's identical note on `Mid`/`StrF`) - except
    // `Round` has no real default of its own (its mode argument is always
    // required); only `Random`'s `min` is genuinely optional.
    static const std::vector<Signature> signatures = {
        {"abs", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"sqr", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"pow", TypeSuffix::Double, {TypeSuffix::Double, TypeSuffix::Double}, 2},
        {"sin", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"cos", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"tan", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"asin", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"acos", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"atan", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"atan2", TypeSuffix::Double, {TypeSuffix::Double, TypeSuffix::Double}, 2},
        {"exp", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"log", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"log10", TypeSuffix::Double, {TypeSuffix::Double}, 1},
        {"round", TypeSuffix::Double, {TypeSuffix::Double, TypeSuffix::Integer}, 2},
        {"int", TypeSuffix::Integer, {TypeSuffix::Double}, 1},
        {"random", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 1},
        {"randomseed", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        // A prerequisite for the Requester family (M7b) - see mathlib.hpp's
        // own pbRGB doc comment.
        {"rgb", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer}, 3},
        {"rgba", TypeSuffix::Integer,
         {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer}, 4},
        {"red", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"green", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"blue", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"alpha", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
    };
    for (const auto& sig : signatures) {
        ProcedureInfo info;
        info.returnSuffix = sig.returnSuffix;
        info.paramSuffixes = sig.paramSuffixes;
        info.requiredParamCount = sig.requiredParamCount;
        procedures_[sig.name] = info;
    }
}

bool Sema::isMathLibBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "abs", "sqr", "pow", "sin", "cos", "tan", "asin", "acos", "atan",
        "atan2", "exp", "log", "log10", "round", "int", "random", "randomseed",
        "rgb", "rgba", "red", "green", "blue", "alpha",
    };
    return names.contains(lowerName);
}

void Sema::registerMemoryLibBuiltins() {
    struct Signature {
        const char* name;
        TypeSuffix returnSuffix;
        std::vector<TypeSuffix> paramSuffixes;
        std::size_t requiredParamCount;
    };
    // Every Peek* takes one Integer address and returns the matching
    // primitive type; every Poke* takes that same address plus the value to
    // write, returning an Integer (see registerMemoryLibBuiltins's own
    // runtime-side doc comment on why that return value is a placeholder,
    // not an oracle-verified one). `PeekS`'s optional length argument reuses
    // the same "fewer call-site args than paramSuffixes.size()" default-
    // parameter mechanism the String/Math libraries already use.
    static const std::vector<Signature> signatures = {
        {"peekb", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peeka", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peekc", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peekw", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peeku", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peekl", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peekq", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"peekf", TypeSuffix::Float, {TypeSuffix::Integer}, 1},
        {"peekd", TypeSuffix::Double, {TypeSuffix::Integer}, 1},
        {"peeks", TypeSuffix::String, {TypeSuffix::Integer, TypeSuffix::Integer}, 1},
        {"pokeb", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokea", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokec", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokew", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokeu", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokel", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokeq", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"pokef", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Float}, 2},
        {"poked", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Double}, 2},
        {"pokes", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::String}, 2},
    };
    for (const auto& sig : signatures) {
        ProcedureInfo info;
        info.returnSuffix = sig.returnSuffix;
        info.paramSuffixes = sig.paramSuffixes;
        info.requiredParamCount = sig.requiredParamCount;
        procedures_[sig.name] = info;
    }
}

bool Sema::isMemoryLibBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "peekb", "peeka", "peekc", "peekw", "peeku", "peekl", "peekq", "peekf", "peekd", "peeks",
        "pokeb", "pokea", "pokec", "pokew", "pokeu", "pokel", "pokeq", "pokef", "poked", "pokes",
    };
    return names.contains(lowerName);
}

void Sema::registerFileLibBuiltins() {
    struct Signature {
        const char* name;
        TypeSuffix returnSuffix;
        std::vector<TypeSuffix> paramSuffixes;
        std::size_t requiredParamCount;
    };
    // A file "number" is a plain Integer the PB program itself picks
    // (oracle-verified), not a handle PB hands back - so every one of
    // these is just an ordinary Integer/String parameter, exactly like the
    // Memory library's addresses.
    static const std::vector<Signature> signatures = {
        {"createfile", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::String}, 2},
        {"openfile", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::String}, 2},
        {"readfile", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::String}, 2},
        {"closefile", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"writestring", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::String}, 2},
        {"writestringn", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::String}, 2},
        {"readstring", TypeSuffix::String, {TypeSuffix::Integer}, 1},
        {"eof", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"filesize", TypeSuffix::Integer, {TypeSuffix::String}, 1},
        {"deletefile", TypeSuffix::Integer, {TypeSuffix::String}, 1},
        {"renamefile", TypeSuffix::Integer, {TypeSuffix::String, TypeSuffix::String}, 2},
        {"fileseek", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"loc", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"lof", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
    };
    for (const auto& sig : signatures) {
        ProcedureInfo info;
        info.returnSuffix = sig.returnSuffix;
        info.paramSuffixes = sig.paramSuffixes;
        info.requiredParamCount = sig.requiredParamCount;
        procedures_[sig.name] = info;
    }
}

bool Sema::isFileLibBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "createfile", "openfile", "readfile", "closefile", "writestring", "writestringn",
        "readstring", "eof", "filesize", "deletefile", "renamefile", "fileseek", "loc", "lof",
    };
    return names.contains(lowerName);
}

void Sema::registerDateLibBuiltins() {
    struct Signature {
        const char* name;
        TypeSuffix returnSuffix;
        std::vector<TypeSuffix> paramSuffixes;
        std::size_t requiredParamCount;
    };
    static const std::vector<Signature> signatures = {
        // `Date` - see isDateLibBuiltinName's own doc comment on why [0, 6]
        // is a deliberate, documented approximation of its real "exactly 0
        // or 6" arity.
        {"date", TypeSuffix::Integer,
         {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer,
          TypeSuffix::Integer},
         0},
        {"year", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"month", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"day", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"hour", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"minute", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"second", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"dayofweek", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"formatdate", TypeSuffix::String, {TypeSuffix::String, TypeSuffix::Integer}, 2},
        {"adddate", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer}, 3},
    };
    for (const auto& sig : signatures) {
        ProcedureInfo info;
        info.returnSuffix = sig.returnSuffix;
        info.paramSuffixes = sig.paramSuffixes;
        info.requiredParamCount = sig.requiredParamCount;
        procedures_[sig.name] = info;
    }
}

bool Sema::isDateLibBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "date", "year", "month", "day", "hour", "minute", "second", "dayofweek", "formatdate", "adddate",
    };
    return names.contains(lowerName);
}

void Sema::registerThreadLibBuiltins() {
    struct Signature {
        const char* name;
        TypeSuffix returnSuffix;
        std::vector<TypeSuffix> paramSuffixes;
        std::size_t requiredParamCount;
    };
    static const std::vector<Signature> signatures = {
        // CreateThread's first argument is `@Procedure()` (an AddressOfExpr,
        // already Integer-typed - see its own dedicated Sema/Codegen
        // handling), so it needs no special treatment here beyond being
        // declared Integer like any other parameter.
        {"createthread", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"isthread", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"waitthread", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        // M7a's own deferred KillThread/PauseThread/ResumeThread/ThreadID -
        // see threadlib.hpp's own doc comments on pbKillThread/
        // pbPauseThread/pbThreadID for the oracle findings and
        // implementation tradeoffs behind each.
        {"killthread", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"pausethread", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"resumethread", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"threadid", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"createmutex", TypeSuffix::Integer, {}, 0},
        {"lockmutex", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"unlockmutex", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"trylockmutex", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"freemutex", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        // CreateSemaphore's initial-count argument is optional (oracle-
        // verified: `CreateSemaphore()` defaults to 0) - the runtime
        // function itself carries the matching C++ default value, so
        // Codegen's own "only emit the args actually given" call-site
        // logic (see genExpr's Call case) needs no special handling here.
        {"createsemaphore", TypeSuffix::Integer, {TypeSuffix::Integer}, 0},
        {"signalsemaphore", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"waitsemaphore", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"trysemaphore", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"freesemaphore", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        // Not thread-specific commands in real PB, but bundled here - see
        // threadlib.hpp's own doc comment on `pbDelay`/`pbElapsedMilliseconds`.
        {"delay", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"elapsedmilliseconds", TypeSuffix::Integer, {}, 0},
    };
    for (const auto& sig : signatures) {
        ProcedureInfo info;
        info.returnSuffix = sig.returnSuffix;
        info.paramSuffixes = sig.paramSuffixes;
        info.requiredParamCount = sig.requiredParamCount;
        procedures_[sig.name] = info;
    }
}

bool Sema::isThreadLibBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "createthread",  "isthread",        "waitthread",      "createmutex",    "lockmutex",
        "unlockmutex",   "trylockmutex",    "freemutex",       "createsemaphore", "signalsemaphore",
        "waitsemaphore", "trysemaphore",    "freesemaphore",   "delay",          "elapsedmilliseconds",
        "killthread",    "pausethread",     "resumethread",    "threadid",
    };
    return names.contains(lowerName);
}

void Sema::registerGuiLibBuiltins() {
    struct Signature {
        const char* name;
        TypeSuffix returnSuffix;
        std::vector<TypeSuffix> paramSuffixes;
        std::size_t requiredParamCount;
    };
    static const std::vector<Signature> signatures = {
        // OpenWindow's Title is the one String-typed parameter here; Flags
        // is optional (oracle-verified: `OpenWindow(id,x,y,w,h,title$)`
        // with no flags at all is legal).
        {"openwindow", TypeSuffix::Integer,
         {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer,
          TypeSuffix::String, TypeSuffix::Integer},
         6},
        {"closewindow", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"iswindow", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"resizewindow", TypeSuffix::Integer,
         {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer},
         5},
        {"hidewindow", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"windowevent", TypeSuffix::Integer, {}, 0},
        // WaitWindowEvent's Timeout argument is optional (oracle-verified:
        // `WaitWindowEvent()` with no argument blocks indefinitely).
        {"waitwindowevent", TypeSuffix::Integer, {TypeSuffix::Integer}, 0},
        {"eventwindow", TypeSuffix::Integer, {}, 0},
        {"eventgadget", TypeSuffix::Integer, {}, 0},
        {"eventtype", TypeSuffix::Integer, {}, 0},
        // M7b's second GUI slice: basic gadgets. Oracle-verified: gadget-
        // creation functions take *no* window parameter at all (a new
        // gadget always lands in whichever window was most recently opened
        // - see guilib.hpp's own `activeWindowId()` doc comment); Text$ is
        // required, Flags is optional (oracle-verified: all five accept a
        // trailing numeric Flags, and all five also compile fine without
        // it).
        {"buttongadget", TypeSuffix::Integer,
         {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer,
          TypeSuffix::String, TypeSuffix::Integer},
         6},
        {"textgadget", TypeSuffix::Integer,
         {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer,
          TypeSuffix::String, TypeSuffix::Integer},
         6},
        {"stringgadget", TypeSuffix::Integer,
         {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer,
          TypeSuffix::String, TypeSuffix::Integer},
         6},
        {"checkboxgadget", TypeSuffix::Integer,
         {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer,
          TypeSuffix::String, TypeSuffix::Integer},
         6},
        {"framegadget", TypeSuffix::Integer,
         {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer,
          TypeSuffix::String, TypeSuffix::Integer},
         6},
        {"isgadget", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"freegadget", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"resizegadget", TypeSuffix::Integer,
         {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer},
         5},
        {"hidegadget", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"disablegadget", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"setgadgettext", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::String}, 2},
        {"getgadgettext", TypeSuffix::String, {TypeSuffix::Integer}, 1},
        {"setgadgetstate", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"getgadgetstate", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        // M7b's third GUI slice: MessageRequester. Flags is optional
        // (oracle-verified: a plain two-argument call defaults to an
        // Ok-only, iconless dialog).
        {"messagerequester", TypeSuffix::Integer, {TypeSuffix::String, TypeSuffix::String, TypeSuffix::Integer}, 2},
        // M7b: the rest of the Requester family (MessageRequester, the
        // third slice, was the first). RGB()/RGBA()/Red()/Green()/Blue()/
        // Alpha() (MathLib) are a prerequisite ColorRequester/FontRequester
        // both need - see mathlib.hpp's own pbRGB doc comment.
        {"colorrequester", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 0},
        {"fontrequester", TypeSuffix::Integer,
         {TypeSuffix::String, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer,
          TypeSuffix::Integer},
         3},
        {"selectedfontname", TypeSuffix::String, {}, 0},
        {"selectedfontsize", TypeSuffix::Integer, {}, 0},
        {"selectedfontstyle", TypeSuffix::Integer, {}, 0},
        {"selectedfontcolor", TypeSuffix::Integer, {}, 0},
        {"inputrequester", TypeSuffix::String,
         {TypeSuffix::String, TypeSuffix::String, TypeSuffix::String, TypeSuffix::Integer, TypeSuffix::Integer}, 3},
        {"openfilerequester", TypeSuffix::String,
         {TypeSuffix::String, TypeSuffix::String, TypeSuffix::String, TypeSuffix::Integer, TypeSuffix::Integer,
          TypeSuffix::Integer},
         4},
        {"savefilerequester", TypeSuffix::String,
         {TypeSuffix::String, TypeSuffix::String, TypeSuffix::String, TypeSuffix::Integer, TypeSuffix::Integer}, 4},
        {"nextselectedfilename", TypeSuffix::String, {}, 0},
        {"selectedfilepattern", TypeSuffix::Integer, {}, 0},
        {"pathrequester", TypeSuffix::String, {TypeSuffix::String, TypeSuffix::String, TypeSuffix::Integer}, 2},
        // M7b's fourth GUI slice: Menu/StatusBar. WindowID is a real gap-
        // filler this slice needed too - oracle-verified CreateMenu/
        // CreateStatusBar's own second argument must be real PB's own
        // WindowID(#Window), not the plain PB-level window ID every other
        // GUI builtin takes directly (see guilib.hpp's pbWindowID doc
        // comment).
        {"windowid", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"createmenu", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        // MenuTitle/MenuItem/MenuBar/OpenSubMenu/CloseSubMenu all operate on
        // whichever #Menu was most recently CreateMenu'd - no #Menu
        // argument at all (oracle-verified, see guilib.hpp's own
        // menuBuildStack doc comment).
        {"menutitle", TypeSuffix::Integer, {TypeSuffix::String}, 1},
        // ImageID is optional and accepted-but-ignored - see guilib.hpp's
        // pbMenuItem doc comment (Image library out of scope).
        {"menuitem", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::String, TypeSuffix::Integer}, 2},
        {"menubar", TypeSuffix::Integer, {}, 0},
        {"opensubmenu", TypeSuffix::Integer, {TypeSuffix::String, TypeSuffix::Integer}, 1},
        {"closesubmenu", TypeSuffix::Integer, {}, 0},
        {"ismenu", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"freemenu", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"hidemenu", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"disablemenuitem", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer}, 3},
        {"getmenuitemstate", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"setmenuitemstate", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer}, 3},
        {"getmenuitemtext", TypeSuffix::String, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"setmenuitemtext", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::String}, 3},
        {"getmenutitletext", TypeSuffix::String, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"setmenutitletext", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::String}, 3},
        {"menuheight", TypeSuffix::Integer, {}, 0},
        {"menuid", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"eventmenu", TypeSuffix::Integer, {}, 0},
        {"createstatusbar", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"addstatusbarfield", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"statusbartext", TypeSuffix::Integer,
         {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::String, TypeSuffix::Integer}, 3},
        {"isstatusbar", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"freestatusbar", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"statusbarheight", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"statusbarid", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        // M7b's fifth GUI slice: the Image library, scoped to just enough
        // to unblock CreateImageMenu/MenuItem/OpenSubMenu's own ImageID
        // argument and (later) ToolBar - see guilib.hpp's pbCreateImage
        // doc comment. Depth/BackgroundColor (CreateImage's own optional
        // 4th/5th args) and LoadImage's Options arg are deliberately not
        // supported yet - RGB()/RGBA() don't exist in this project at all,
        // so there's no way to construct a meaningful color argument for
        // them yet either.
        {"createimage", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer}, 3},
        {"loadimage", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::String}, 2},
        {"isimage", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"freeimage", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"imageid", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"imagewidth", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"imageheight", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        // CreateImageMenu's own Options arg (`#PB_Menu_NativeImageSize`) is
        // Windows-only - accepted but not acted on, same simplification
        // already used elsewhere for an OS-specific niceity.
        {"createimagemenu", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        // M7b's sixth GUI slice: ToolBar, unblocked by the Image library
        // (fifth slice) - see guilib.hpp's pbCreateToolBar doc comment for
        // why this was deferred until then.
        {"createtoolbar", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"toolbarimagebutton", TypeSuffix::Integer,
         {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::String}, 2},
        {"toolbarseparator", TypeSuffix::Integer, {}, 0},
        {"istoolbar", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"freetoolbar", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"disabletoolbarbutton", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer}, 3},
        {"gettoolbarbuttonstate", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"settoolbarbuttonstate", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer}, 3},
        {"toolbarbuttontext", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::String}, 3},
        {"toolbartooltip", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::String}, 3},
        {"toolbarheight", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"toolbarid", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        // M7b's seventh GUI slice: SysTrayIcon, plus its own hard
        // prerequisite - oracle-verified SysTrayIconMenu requires a popup
        // menu (CreatePopupMenu/CreatePopupImageMenu), not a plain
        // CreateMenu one. CreatePopupImageMenu's own Options arg
        // (#PB_Menu_NativeImageSize) doesn't even exist as a constant on
        // this oracle's own Linux build - accepted but not acted on here
        // either, the same treatment CreateImageMenu's own already gets.
        {"createpopupmenu", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"createpopupimagemenu", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 1},
        {"addsystrayicon", TypeSuffix::Integer,
         {TypeSuffix::Integer, TypeSuffix::Integer, TypeSuffix::Integer}, 3},
        {"changesystrayicon", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"issystrayicon", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"removesystrayicon", TypeSuffix::Integer, {TypeSuffix::Integer}, 1},
        {"systrayiconmenu", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::Integer}, 2},
        {"systrayicontooltip", TypeSuffix::Integer, {TypeSuffix::Integer, TypeSuffix::String}, 2},
    };
    for (const auto& sig : signatures) {
        ProcedureInfo info;
        info.returnSuffix = sig.returnSuffix;
        info.paramSuffixes = sig.paramSuffixes;
        info.requiredParamCount = sig.requiredParamCount;
        procedures_[sig.name] = info;
    }
}

bool Sema::isGuiLibBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "openwindow",    "closewindow", "iswindow",         "resizewindow",   "hidewindow",
        "windowevent",   "waitwindowevent", "eventwindow",  "eventgadget",    "eventtype",
        "buttongadget",  "textgadget",  "stringgadget",     "checkboxgadget", "framegadget",
        "isgadget",      "freegadget",  "resizegadget",     "hidegadget",     "disablegadget",
        "setgadgettext", "getgadgettext", "setgadgetstate", "getgadgetstate", "messagerequester",
        "windowid", "createmenu", "menutitle", "menuitem", "menubar", "opensubmenu", "closesubmenu", "ismenu",
        "freemenu", "hidemenu", "disablemenuitem", "getmenuitemstate", "setmenuitemstate", "getmenuitemtext",
        "setmenuitemtext", "getmenutitletext", "setmenutitletext", "menuheight", "menuid", "eventmenu",
        "createstatusbar", "addstatusbarfield", "statusbartext", "isstatusbar", "freestatusbar", "statusbarheight",
        "statusbarid", "createimage", "loadimage", "isimage", "freeimage", "imageid", "imagewidth", "imageheight",
        "createimagemenu", "createtoolbar", "toolbarimagebutton", "toolbarseparator", "istoolbar", "freetoolbar",
        "disabletoolbarbutton", "gettoolbarbuttonstate", "settoolbarbuttonstate", "toolbarbuttontext",
        "toolbartooltip", "toolbarheight", "toolbarid", "createpopupmenu", "createpopupimagemenu",
        "addsystrayicon", "changesystrayicon", "issystrayicon", "removesystrayicon", "systrayiconmenu",
        "systrayicontooltip", "colorrequester", "fontrequester", "selectedfontname", "selectedfontsize",
        "selectedfontstyle", "selectedfontcolor", "inputrequester", "openfilerequester", "savefilerequester",
        "nextselectedfilename", "selectedfilepattern", "pathrequester",
    };
    return names.contains(lowerName);
}

bool Sema::isSysTrayLibBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "createpopupmenu", "createpopupimagemenu", "addsystrayicon", "changesystrayicon",
        "issystrayicon",   "removesystrayicon",     "systrayiconmenu", "systrayicontooltip",
    };
    return names.contains(lowerName);
}

namespace {

// `#PB_Compiler_OS`/`#PB_Compiler_Processor` reflect whatever platform
// `pbcxx` *itself* was built for, not a cross-compilation target - this
// project doesn't support cross-compiling (it shells out to a native g++/
// clang++ for the same machine it runs on), so "the platform pbcxx runs
// on" and "the platform the generated binary will run on" are always the
// same, exactly like real `pbcompilerc`'s own single-target model.
#if defined(_WIN32)
constexpr std::int64_t kCompilerOs = 1; // #PB_OS_Windows (oracle-verified)
#elif defined(__HAIKU__)
// Haiku is not an officially PB-supported platform at all, so there is no
// real `#PB_OS_Haiku` to match - this is a pbcxx-specific extension value
// (continuing the power-of-two pattern the three confirmed values follow),
// not something oracle-verified against real PB.
constexpr std::int64_t kCompilerOs = 8;
#elif defined(__APPLE__)
constexpr std::int64_t kCompilerOs = 4; // #PB_OS_MacOS (oracle-verified)
#else
constexpr std::int64_t kCompilerOs = 2; // #PB_OS_Linux (oracle-verified) - the default/fallback.
#endif

#if defined(__x86_64__) || defined(_M_X64)
constexpr std::int64_t kCompilerProcessor = 4; // #PB_Processor_x64 (oracle-verified)
#elif defined(__aarch64__) || defined(_M_ARM64)
// Not independently oracle-verified (no ARM machine to check against) -
// chosen only to be internally consistent (distinct from the two verified
// x86/x64 values).
constexpr std::int64_t kCompilerProcessor = 8;
#elif defined(__arm__) || defined(_M_ARM)
constexpr std::int64_t kCompilerProcessor = 16; // Likewise unverified.
#else
constexpr std::int64_t kCompilerProcessor = 2; // #PB_Processor_x86 (oracle-verified) - the default/fallback.
#endif

const std::unordered_map<std::string, std::int64_t>& builtinConstantTable() {
    // Oracle-verified values (`pbcompilerc`): `#PB_Round_Down` = 0,
    // `#PB_Round_Up` = 1, `#PB_Round_Nearest` = 2. A fourth, plausible-
    // sounding `#PB_Round_Truncate` does NOT exist in real PB ("Constant
    // not found"). `#PB_Date_*` (for `AddDate`'s own unit argument) is
    // oracle-verified too: Year=0, Month=1, Week=2, Day=3, Hour=4,
    // Minute=5, Second=6. `#PB_OS_Windows`=1/`#PB_OS_Linux`=2/
    // `#PB_OS_MacOS`=4 and `#PB_Processor_x86`=2/`#PB_Processor_x64`=4 are
    // oracle-verified on this Linux/x64 machine (`#PB_Compiler_OS` read `2`,
    // `#PB_Compiler_Processor` read `4`); `#PB_OS_Haiku` and the ARM
    // processor constants are pbcxx-specific extensions/unverified guesses
    // (see `kCompilerOs`/`kCompilerProcessor`'s own comments). `#PB_Event_*`
    // and `#PB_Window_*` (M7b) are oracle-verified via a direct
    // `Debug #PB_Event_Xxx` probe on this Linux/x64 machine.
    static const std::unordered_map<std::string, std::int64_t> table = {
        {"pb_round_down", 0},
        {"pb_round_up", 1},
        {"pb_round_nearest", 2},
        {"pb_date_year", 0},
        {"pb_date_month", 1},
        {"pb_date_week", 2},
        {"pb_date_day", 3},
        {"pb_date_hour", 4},
        {"pb_date_minute", 5},
        {"pb_date_second", 6},
        {"pb_os_windows", 1},
        {"pb_os_linux", 2},
        {"pb_os_macos", 4},
        {"pb_os_haiku", 8},
        {"pb_processor_x86", 2},
        {"pb_processor_x64", 4},
        {"pb_processor_arm64", 8},
        {"pb_processor_arm", 16},
        {"pb_compiler_os", kCompilerOs},
        {"pb_compiler_processor", kCompilerProcessor},
        {"pb_event_menu", 1},
        {"pb_event_closewindow", 2},
        {"pb_event_gadget", 3},
        {"pb_event_repaint", 4},
        {"pb_event_movewindow", 5},
        {"pb_event_sizewindow", 6},
        {"pb_event_activatewindow", 7},
        // M7b's seventh GUI slice: SysTrayIcon - oracle-verified via a
        // direct `Debug #PB_Event_SysTray` probe. `#PB_Menu_SysTrayLook`
        // (SysTrayIconMenu's own docs name it as the required
        // CreatePopupImageMenu Options bit) doesn't exist as a constant on
        // this oracle's own Linux build at all ("Constant not found") - not
        // registered here either, consistent with not inventing a value
        // real PB itself doesn't expose on this platform.
        {"pb_event_systray", 9},
        {"pb_event_timer", 15},
        {"pb_event_firstcustomvalue", 65536},
        {"pb_window_invisible", 1},
        {"pb_window_sizegadget", 2},
        {"pb_window_systemmenu", 4},
        {"pb_window_titlebar", 8},
        {"pb_window_maximizegadget", 16},
        {"pb_window_minimizegadget", 32},
        {"pb_window_screencentered", 64},
        // Oracle-verified via a direct `Debug #PB_EventType_Xxx` probe
        // (M7b's second GUI slice - gadget events' own EventType()
        // sub-codes). Note the "obvious"-looking guess for a checkbox
        // toggle would be Change, but it's actually LeftClick (see
        // guilib.hpp's own onGadgetClicked doc comment).
        {"pb_eventtype_leftclick", 0},
        {"pb_eventtype_rightclick", 1},
        {"pb_eventtype_leftdoubleclick", 2},
        {"pb_eventtype_focus", 256},
        {"pb_eventtype_lostfocus", 512},
        {"pb_eventtype_change", 768},
        // Oracle-verified (M7b's third GUI slice, MessageRequester): the
        // button-set flags (Ok/YesNo/YesNoCancel) are a 0/1/2 sequence, not
        // independent bits - see guilib.hpp's pbMessageRequester for the
        // `Flags & 3` masking this implies. The button-pressed return
        // values (Yes/No/Cancel) are a separate group that happens to
        // reuse some of the same numbers (Cancel=2 coincides with the
        // YesNoCancel flag, Yes=6 does not collide with anything).
        {"pb_messagerequester_ok", 0},
        {"pb_messagerequester_yesno", 1},
        {"pb_messagerequester_yesnocancel", 2},
        {"pb_messagerequester_info", 4},
        {"pb_messagerequester_error", 8},
        {"pb_messagerequester_warning", 16},
        {"pb_messagerequester_yes", 6},
        {"pb_messagerequester_no", 7},
        {"pb_messagerequester_cancel", 2},
        // M7b's fourth GUI slice: Menu/StatusBar. `#PB_StatusBar_*` is
        // oracle-verified via a direct `Debug #PB_StatusBar_Xxx` probe -
        // independent bits, unlike MessageRequester's own grouped flags.
        // `#PB_Ignore`/`#PB_All` are general-purpose PB sentinels (not GUI-
        // specific), first needed by this slice (`AddStatusBarField`'s
        // auto-size width, `FreeMenu`/`FreeStatusBar`'s "free everything")
        // but oracle-verified standalone, not assumed from context -
        // surprisingly, both share the same value (`-1`) in real PB.
        {"pb_statusbar_raised", 1},
        {"pb_statusbar_borderless", 2},
        {"pb_statusbar_center", 4},
        {"pb_statusbar_right", 8},
        {"pb_ignore", -65535},
        {"pb_all", -1},
        {"pb_any", -1},
        // ToolBar (M7b's sixth GUI slice) - oracle-verified via a direct
        // `Debug #PB_ToolBar_Xxx` probe, same independent-bits shape as
        // `#PB_StatusBar_*`'s own.
        {"pb_toolbar_small", 1},
        {"pb_toolbar_large", 2},
        {"pb_toolbar_text", 4},
        {"pb_toolbar_inlinetext", 8},
        {"pb_toolbar_normal", 0},
        {"pb_toolbar_toggle", 1},
        // The Requester family's remaining pieces (M7b) - oracle-verified
        // via direct `Debug` probes. `#PB_Font_StrikeOut`/`#PB_Font_Underline`
        // are both genuinely `0` on this oracle's own Linux build (not a
        // probe mistake - confirmed twice, with explicit labels) - neither
        // bit is ever detectable via `SelectedFontStyle()` on this platform
        // at all, consistent with GTK's own font chooser having no
        // strikeout/underline toggle of its own either.
        // `#PB_InputRequester_Cancel` is a *String* constant (`Chr(10) +
        // Chr(9)`, confirmed byte-by-byte) - this table is Integer-only,
        // so it isn't registered here; see guilib.hpp's own pbInputRequester
        // doc comment for the resulting, deliberately narrow gap.
        {"pb_inputrequester_password", 1},
        {"pb_inputrequester_handlecancel", 2},
        {"pb_fontrequester_effects", 1},
        {"pb_font_bold", 1},
        {"pb_font_italic", 2},
        {"pb_font_strikeout", 0},
        {"pb_font_underline", 0},
        {"pb_requester_multiselection", 1},
    };
    return table;
}
} // namespace

void Sema::registerBuiltinConstants() {
    for (const auto& [name, value] : builtinConstantTable()) {
        constants_[name] = TypeSuffix::Integer;
        constantIntValues_[name] = value;
    }
}

std::optional<std::int64_t> Sema::builtinConstantValue(const std::string& lowerName) {
    const auto& table = builtinConstantTable();
    auto it = table.find(lowerName);
    if (it == table.end()) {
        return std::nullopt;
    }
    return it->second;
}

void Sema::declare(const std::string& lowerName, const std::string& spelling, TypeSuffix suffix,
                    SourceLoc loc) {
    auto it = symbols_.find(lowerName);
    if (it == symbols_.end()) {
        symbols_.emplace(lowerName, suffix);
        order_.emplace_back(lowerName, suffix);
        return;
    }
    // Re-Define'ing (or implicitly re-touching) the same name with a
    // different suffix is a real PB error ("Redefinition of ..."); M0 keeps
    // this simple and just flags a mismatch rather than modeling PB's exact
    // diagnostic wording yet.
    if (it->second != suffix && suffix != TypeSuffix::None) {
        diagnostics_.error(loc, "'" + spelling + "' redeclared with a different type suffix");
    }
}

void Sema::declareImplicit(const std::string& lowerName, const std::string& spelling, TypeSuffix suffix,
                            SourceLoc loc) {
    // Oracle-verified error text (pbcompilerc, `EnableExplicit` + an
    // undeclared variable): "With 'EnableExplicit', variables have to be
    // declared: x." `declare()` still runs regardless so Codegen never sees
    // a symbol with no recorded type - the driver already stops before
    // Codegen once any error is recorded (see main.cpp), so this is a
    // defensive fallback, not a way to let the violation silently through.
    if (explicitEnabled_ && !symbols_.contains(lowerName)) {
        diagnostics_.error(loc, "With 'EnableExplicit', variables have to be declared: " + spelling + ".");
    }
    declare(lowerName, spelling, suffix, loc);
}

void Sema::declareConst(const std::string& lowerName, const std::string& spelling, TypeSuffix suffix,
                         SourceLoc loc) {
    if (constants_.contains(lowerName)) {
        diagnostics_.error(loc, "'#" + spelling + "' is already declared as a constant");
        return;
    }
    constants_.emplace(lowerName, suffix);
    constOrder_.emplace_back(lowerName, suffix);
}

bool Sema::analyze(ast::Module& module) {
    // Two whole-Module pre-passes, each needing to see the *entire* tree
    // before any "real" per-statement semantic analysis begins:
    //  1. preResolveCompilerDirectives splices away every CompilerIf/
    //     CompilerSelect node (see its own doc comment) - must run first,
    //     since collectDataSections below needs to see the tree in its
    //     final, post-splicing shape (a DataSection originally nested
    //     inside a CompilerIf branch only "exists" once that branch has
    //     actually been selected and spliced in).
    //  2. collectDataSections assigns each Data label a flat index into
    //     the eventual runtime data pool, needed because `Restore` can
    //     forward-reference a label defined later in the file (oracle-
    //     verified) - impossible to resolve correctly with only a single
    //     top-to-bottom walk.
    preResolveCompilerDirectives(module.statements);
    collectDataSections(module.statements);
    visitBlock(module.statements);
    // Every `Declare` must eventually be fulfilled by a matching
    // `Procedure` (oracle-verified: "The procedure 'name()' has been
    // declared but not defined." otherwise) - checked only after the whole
    // module has been walked, since the fulfilling `Procedure` may appear
    // anywhere later in the file.
    for (const auto& [lowerName, spellingAndLoc] : declaredNotDefined_) {
        diagnostics_.error(spellingAndLoc.second,
                            "The procedure '" + spellingAndLoc.first + "()' has been declared but not defined.");
    }
    return !diagnostics_.hasErrors();
}

void Sema::visitBlock(ast::Block& block) {
    // No CompilerIf/CompilerSelect special-casing needed here any more -
    // preResolveCompilerDirectives() (run once, recursively, over the whole
    // Module before visitBlock is ever called - see Sema::analyze()) has
    // already spliced every one of those nodes out of every block in the
    // tree by this point.
    for (auto& stmt : block) {
        visitStmt(*stmt);
    }
}

void Sema::visitNestedBlock(ast::Block& block) {
    ++controlFlowDepth_;
    visitBlock(block);
    --controlFlowDepth_;
}

void Sema::preResolveCompilerDirectives(ast::Block& block) {
    std::size_t i = 0;
    while (i < block.size()) {
        ast::Stmt& stmt = *block[i];
        switch (stmt.kind) {
            case ast::StmtKind::ConstDecl: {
                // A lightweight preview of what the real ConstDecl case
                // (in visitStmt) will later do properly (declareConst,
                // full type classification) - just enough so a CompilerIf
                // reached *during this same pass* can already fold a
                // reference to a #Constant declared earlier in the file.
                // Harmless to recompute the same value again later.
                auto& constDecl = static_cast<ast::ConstDeclStmt&>(stmt);
                if (auto value = evalConstExpr(*constDecl.value)) {
                    constantIntValues_[constDecl.name] = *value;
                }
                ++i;
                continue;
            }
            case ast::StmtKind::Enumeration: {
                auto& enumStmt = static_cast<ast::EnumerationStmt&>(stmt);
                std::int64_t nextValue = 0;
                for (auto& member : enumStmt.members) {
                    if (member.explicitValue) {
                        if (auto value = evalConstExpr(*member.explicitValue)) {
                            nextValue = *value;
                        }
                    }
                    constantIntValues_[member.name] = nextValue;
                    ++nextValue;
                }
                ++i;
                continue;
            }
            case ast::StmtKind::CompilerIf:
                resolveCompilerIf(block, i);
                continue; // Re-examine position `i` - see visitBlock's own
                          // former version of this comment (now moved here).
            case ast::StmtKind::CompilerSelect:
                resolveCompilerSelect(block, i);
                continue;
            case ast::StmtKind::If: {
                auto& ifStmt = static_cast<ast::IfStmt&>(stmt);
                for (auto& branch : ifStmt.branches) {
                    preResolveCompilerDirectives(branch.body);
                }
                break;
            }
            case ast::StmtKind::Select: {
                auto& sel = static_cast<ast::SelectStmt&>(stmt);
                for (auto& branch : sel.cases) {
                    preResolveCompilerDirectives(branch.body);
                }
                break;
            }
            case ast::StmtKind::For:
                preResolveCompilerDirectives(static_cast<ast::ForStmt&>(stmt).body);
                break;
            case ast::StmtKind::While:
                preResolveCompilerDirectives(static_cast<ast::WhileStmt&>(stmt).body);
                break;
            case ast::StmtKind::Repeat:
                preResolveCompilerDirectives(static_cast<ast::RepeatStmt&>(stmt).body);
                break;
            case ast::StmtKind::ForEach:
                preResolveCompilerDirectives(static_cast<ast::ForEachStmt&>(stmt).body);
                break;
            case ast::StmtKind::ProcedureDecl:
                preResolveCompilerDirectives(static_cast<ast::ProcedureDeclStmt&>(stmt).body);
                break;
            default:
                break;
        }
        ++i;
    }
}

void Sema::collectDataSections(ast::Block& block) {
    for (auto& stmt : block) {
        switch (stmt->kind) {
            case ast::StmtKind::DataSection: {
                // By this point preResolveCompilerDirectives() has already
                // run over the whole Module, so every DataSection here -
                // including one originally nested inside a CompilerIf
                // branch - is one that genuinely survives, in the same
                // flat order Codegen's own, identically-shaped recursive
                // scan will later see (see genDataPool's own notes).
                auto& dataSection = static_cast<ast::DataSectionStmt&>(*stmt);
                // `currentLabel` tracks whichever label this run of Data
                // items follows (M7c's own dataLabelItemSuffixes_, used only
                // to validate a later `?Label` use - see its own doc
                // comment); empty before the DataSection's first label, in
                // which case a leading Data item (oracle-verified legal)
                // contributes to dataCount_ as usual but isn't attributed to
                // any label.
                std::string currentLabel;
                for (auto& child : dataSection.body) {
                    if (child->kind == ast::StmtKind::DataLabel) {
                        auto& label = static_cast<ast::DataLabelStmt&>(*child);
                        // M7d's second slice: a label declared inside a
                        // Module's own body belongs to that module's own
                        // namespace - mangled here, once, this pre-pass's
                        // only chance to do so before dataLabels_ itself is
                        // populated (the main visitStmt walk's own
                        // DataSection case never revisits a DataLabelStmt).
                        if (!currentModule_.empty()) {
                            label.name = currentModule_ + "::" + label.name;
                        }
                        dataLabels_[label.name] = dataCount_;
                        currentLabel = label.name;
                    } else {
                        const auto& data = static_cast<const ast::DataStmt&>(*child);
                        dataCount_ += data.values.size();
                        if (!currentLabel.empty()) {
                            dataLabelItemSuffixes_[currentLabel].push_back(data.suffix);
                        }
                    }
                }
                break;
            }
            case ast::StmtKind::If: {
                auto& ifStmt = static_cast<ast::IfStmt&>(*stmt);
                for (auto& branch : ifStmt.branches) {
                    collectDataSections(branch.body);
                }
                break;
            }
            case ast::StmtKind::Select: {
                auto& sel = static_cast<ast::SelectStmt&>(*stmt);
                for (auto& branch : sel.cases) {
                    collectDataSections(branch.body);
                }
                break;
            }
            case ast::StmtKind::For:
                collectDataSections(static_cast<ast::ForStmt&>(*stmt).body);
                break;
            case ast::StmtKind::While:
                collectDataSections(static_cast<ast::WhileStmt&>(*stmt).body);
                break;
            case ast::StmtKind::Repeat:
                collectDataSections(static_cast<ast::RepeatStmt&>(*stmt).body);
                break;
            case ast::StmtKind::ForEach:
                collectDataSections(static_cast<ast::ForEachStmt&>(*stmt).body);
                break;
            case ast::StmtKind::ProcedureDecl:
                // Oracle-verified legal: a DataSection inside a Procedure
                // contributes to the same global, shared pool/cursor as a
                // top-level one.
                collectDataSections(static_cast<ast::ProcedureDeclStmt&>(*stmt).body);
                break;
            case ast::StmtKind::DeclareModule: {
                // M7d's second slice: a module's own public DataSection
                // labels live in its DeclareModule section - `currentModule_`
                // is reused directly (this pre-pass runs to completion, and
                // clears it again on the way out, before the main
                // visitStmt walk - which owns the same field - ever starts).
                auto& decl = static_cast<ast::DeclareModuleStmt&>(*stmt);
                currentModule_ = decl.name;
                collectDataSections(decl.body);
                currentModule_.clear();
                break;
            }
            case ast::StmtKind::Module: {
                auto& mod = static_cast<ast::ModuleStmt&>(*stmt);
                currentModule_ = mod.name;
                collectDataSections(mod.body);
                currentModule_.clear();
                break;
            }
            default:
                break;
        }
    }
}

void Sema::resolveCompilerIf(ast::Block& block, std::size_t index) {
    auto& ci = static_cast<ast::CompilerIfStmt&>(*block[index]);
    ast::Block selected; // empty if nothing matched and there was no CompilerElse.
    for (auto& branch : ci.branches) {
        if (!branch.condition) {
            selected = std::move(branch.body); // CompilerElse
            break;
        }
        auto value = evalConstExpr(*branch.condition);
        if (!value) {
            diagnostics_.error(branch.condition->loc, "'CompilerIf' condition must be a constant expression");
            break;
        }
        if (*value != 0) {
            selected = std::move(branch.body);
            break;
        }
    }
    // `selected` already owns (moved out of `ci`) everything it needs, so
    // erasing the CompilerIfStmt node itself first - destroying `ci` and
    // whatever's left of its branches - is safe.
    auto pos = block.begin() + static_cast<std::ptrdiff_t>(index);
    block.erase(pos);
    block.insert(block.begin() + static_cast<std::ptrdiff_t>(index), std::make_move_iterator(selected.begin()),
                 std::make_move_iterator(selected.end()));
}

void Sema::resolveCompilerSelect(ast::Block& block, std::size_t index) {
    auto& cs = static_cast<ast::CompilerSelectStmt&>(*block[index]);
    auto selectorValue = evalConstExpr(*cs.selector);
    if (!selectorValue) {
        diagnostics_.error(cs.selector->loc, "'CompilerSelect' selector must be a constant expression");
    }
    ast::Block selected;
    for (auto& caseBranch : cs.cases) {
        if (caseBranch.values.empty()) {
            selected = std::move(caseBranch.body); // CompilerDefault
            break;
        }
        if (!selectorValue) {
            continue; // Already reported above; nothing left to meaningfully match against.
        }
        bool matches = false;
        for (const auto& valueExpr : caseBranch.values) {
            auto caseValue = evalConstExpr(*valueExpr);
            if (!caseValue) {
                diagnostics_.error(valueExpr->loc, "'CompilerCase' value must be a constant expression");
                continue;
            }
            if (*caseValue == *selectorValue) {
                matches = true;
                break;
            }
        }
        if (matches) {
            selected = std::move(caseBranch.body);
            break;
        }
    }
    auto pos = block.begin() + static_cast<std::ptrdiff_t>(index);
    block.erase(pos);
    block.insert(block.begin() + static_cast<std::ptrdiff_t>(index), std::make_move_iterator(selected.begin()),
                 std::make_move_iterator(selected.end()));
}

std::optional<std::int64_t> Sema::evalConstExpr(const ast::Expr& expr) const {
    switch (expr.kind) {
        case ast::ExprKind::IntLiteral:
            return static_cast<const ast::IntLiteralExpr&>(expr).value;
        case ast::ExprKind::ConstRef: {
            const auto& ref = static_cast<const ast::ConstRefExpr&>(expr);
            auto it = constantIntValues_.find(ref.name);
            return it == constantIntValues_.end() ? std::optional<std::int64_t>{} : it->second;
        }
        case ast::ExprKind::Unary: {
            const auto& un = static_cast<const ast::UnaryExpr&>(expr);
            auto operand = evalConstExpr(*un.operand);
            if (!operand) {
                return std::nullopt;
            }
            switch (un.op) {
                case ast::UnaryOp::Negate: return -*operand;
                case ast::UnaryOp::BitNot: return ~*operand;
                case ast::UnaryOp::LogicalNot: return *operand == 0 ? 1 : 0;
            }
            return std::nullopt;
        }
        case ast::ExprKind::Binary: {
            const auto& bin = static_cast<const ast::BinaryExpr&>(expr);
            // Oracle-verified-untrusted elsewhere (see BinaryOp::LogicalXOr's
            // own doc comment) - not propagated into compile-time folding
            // either.
            if (bin.op == ast::BinaryOp::LogicalXOr) {
                return std::nullopt;
            }
            auto lhs = evalConstExpr(*bin.lhs);
            auto rhs = evalConstExpr(*bin.rhs);
            if (!lhs || !rhs) {
                return std::nullopt;
            }
            switch (bin.op) {
                case ast::BinaryOp::Add: return *lhs + *rhs;
                case ast::BinaryOp::Sub: return *lhs - *rhs;
                case ast::BinaryOp::Mul: return *lhs * *rhs;
                case ast::BinaryOp::Div: return *rhs != 0 ? std::optional<std::int64_t>(*lhs / *rhs) : std::nullopt;
                case ast::BinaryOp::Mod: return *rhs != 0 ? std::optional<std::int64_t>(*lhs % *rhs) : std::nullopt;
                case ast::BinaryOp::BitAnd: return *lhs & *rhs;
                case ast::BinaryOp::BitOr: return *lhs | *rhs;
                case ast::BinaryOp::BitXor: return *lhs ^ *rhs;
                case ast::BinaryOp::ShiftLeft: return *lhs << *rhs;
                case ast::BinaryOp::ShiftRight: return *lhs >> *rhs;
                case ast::BinaryOp::Eq: return *lhs == *rhs ? 1 : 0;
                case ast::BinaryOp::Ne: return *lhs != *rhs ? 1 : 0;
                case ast::BinaryOp::Lt: return *lhs < *rhs ? 1 : 0;
                case ast::BinaryOp::Gt: return *lhs > *rhs ? 1 : 0;
                case ast::BinaryOp::Le: return *lhs <= *rhs ? 1 : 0;
                case ast::BinaryOp::Ge: return *lhs >= *rhs ? 1 : 0;
                case ast::BinaryOp::LogicalAnd: return (*lhs != 0 && *rhs != 0) ? 1 : 0;
                case ast::BinaryOp::LogicalOr: return (*lhs != 0 || *rhs != 0) ? 1 : 0;
                case ast::BinaryOp::LogicalXOr: return std::nullopt; // unreachable - handled above
            }
            return std::nullopt;
        }
        default:
            // VarRef, Call, FieldAccess, String/Float literals, AddressOf -
            // none of these are constant-foldable here.
            return std::nullopt;
    }
}

void Sema::visitStmt(ast::Stmt& stmt) {
    switch (stmt.kind) {
        case ast::StmtKind::Define: {
            auto& def = static_cast<ast::DefineStmt&>(stmt);
            for (auto& decl : def.declarators) {
                // M7d: a Global declared inside a Module's own body belongs
                // to that module's own namespace (mangled here, once, before
                // `declare`/`globalNames_` ever see it) - see
                // ModuleStmt's own doc comment. A plain/Protected Define
                // (isGlobal false) is unaffected even inside a module's own
                // Procedure body (ordinary procedure-local scoping already
                // handles that correctly, untouched by this).
                if (def.isGlobal && !currentModule_.empty()) {
                    decl.name = currentModule_ + "::" + decl.name;
                }
                if (decl.isPointer) {
                    // A pointer variable's own storage is always a plain
                    // Integer address (see ast::DefineStmt::Declarator's own
                    // doc comment); `decl.suffix`/`structTypeName` describe
                    // what it *points at*, recorded separately below.
                    declare(decl.name, decl.spelling, TypeSuffix::Integer, def.loc);
                    resolveModuleQualifiedTypeName(decl.structTypeName, def.loc);
                    pointerPointeeType_[decl.name] = resolvePointeeType(decl.suffix, decl.structTypeName,
                                                                         decl.structTypeSpelling, def.loc);
                    if (decl.init) {
                        visitExpr(*decl.init);
                        checkAssignable(decl.spelling, TypeSuffix::Integer, *decl.init, def.loc);
                    }
                    if (def.isGlobal) {
                        globalNames_.insert(decl.name);
                    }
                    continue;
                }
                TypeSuffix suffix = decl.suffix == TypeSuffix::None ? TypeSuffix::Integer : decl.suffix;
                declare(decl.name, decl.spelling, suffix, def.loc);
                if (suffix == TypeSuffix::Struct) {
                    resolveModuleQualifiedTypeName(decl.structTypeName, def.loc);
                    if (!structures_.contains(decl.structTypeName)) {
                        diagnostics_.error(def.loc, "'" + decl.structTypeSpelling + "' is not a declared Structure");
                    }
                    varStructType_[decl.name] = decl.structTypeName;
                    if (decl.init) {
                        diagnostics_.error(def.loc, "Structure-typed variables cannot have an initializer");
                    }
                } else if (decl.init) {
                    visitExpr(*decl.init);
                    checkAssignable(decl.spelling, suffix, *decl.init, def.loc);
                }
                if (def.isGlobal) {
                    globalNames_.insert(decl.name);
                }
            }
            break;
        }
        case ast::StmtKind::Shared: {
            auto& shared = static_cast<ast::SharedStmt&>(stmt);
            for (auto& name : shared.names) {
                if (outerScopeForShared_ != nullptr) {
                    bringIntoScope(*outerScopeForShared_, name.name, name.spelling, shared.loc, "Shared");
                } else {
                    diagnostics_.error(shared.loc, "'Shared' is only valid inside a procedure");
                }
            }
            break;
        }
        case ast::StmtKind::Dim: {
            auto& dim = static_cast<ast::DimStmt&>(stmt);
            for (auto& size : dim.dimensionSizes) {
                visitExpr(*size);
            }
            // M7d's second slice: an array declared inside a Module's own
            // body belongs to that module's own namespace, same convention
            // as every other declaration kind there.
            if (!currentModule_.empty()) {
                dim.name = currentModule_ + "::" + dim.name;
            }
            if (arrays_.contains(dim.name)) {
                diagnostics_.error(dim.loc, "'" + dim.spelling + "' is already declared as an array");
                break;
            }
            ArrayInfo info;
            info.elementSuffix = dim.suffix == TypeSuffix::None ? TypeSuffix::Integer : dim.suffix;
            info.dimensionCount = static_cast<int>(dim.dimensionSizes.size());
            if (info.elementSuffix == TypeSuffix::Struct) {
                resolveModuleQualifiedTypeName(dim.structTypeName, dim.loc);
                if (!structures_.contains(dim.structTypeName)) {
                    diagnostics_.error(dim.loc, "'" + dim.structTypeSpelling + "' is not a declared Structure");
                }
                info.elementStructName = dim.structTypeName;
            }
            arrays_.emplace(dim.name, info);
            arrayOrder_.emplace_back(dim.name, info);
            break;
        }
        case ast::StmtKind::NewList: {
            auto& newList = static_cast<ast::NewListStmt&>(stmt);
            if (!currentModule_.empty()) {
                newList.name = currentModule_ + "::" + newList.name;
            }
            if (lists_.contains(newList.name)) {
                diagnostics_.error(newList.loc, "'" + newList.spelling + "' is already declared as a List");
                break;
            }
            ListInfo info;
            info.elementSuffix = newList.suffix == TypeSuffix::None ? TypeSuffix::Integer : newList.suffix;
            if (info.elementSuffix == TypeSuffix::Struct) {
                resolveModuleQualifiedTypeName(newList.structTypeName, newList.loc);
                if (!structures_.contains(newList.structTypeName)) {
                    diagnostics_.error(newList.loc, "'" + newList.structTypeSpelling + "' is not a declared Structure");
                }
                info.elementStructName = newList.structTypeName;
            }
            lists_.emplace(newList.name, info);
            listOrder_.emplace_back(newList.name, info);
            break;
        }
        case ast::StmtKind::NewMap: {
            auto& newMap = static_cast<ast::NewMapStmt&>(stmt);
            if (!currentModule_.empty()) {
                newMap.name = currentModule_ + "::" + newMap.name;
            }
            if (maps_.contains(newMap.name)) {
                diagnostics_.error(newMap.loc, "'" + newMap.spelling + "' is already declared as a Map");
                break;
            }
            MapInfo info;
            info.elementSuffix = newMap.suffix == TypeSuffix::None ? TypeSuffix::Integer : newMap.suffix;
            if (info.elementSuffix == TypeSuffix::Struct) {
                resolveModuleQualifiedTypeName(newMap.structTypeName, newMap.loc);
                if (!structures_.contains(newMap.structTypeName)) {
                    diagnostics_.error(newMap.loc, "'" + newMap.structTypeSpelling + "' is not a declared Structure");
                }
                info.elementStructName = newMap.structTypeName;
            }
            maps_.emplace(newMap.name, info);
            mapOrder_.emplace_back(newMap.name, info);
            break;
        }
        case ast::StmtKind::ForEach: {
            auto& forEach = static_cast<ast::ForEachStmt&>(stmt);
            resolveModuleQualifiedName(
                forEach.name, [this](const std::string& n) { return lists_.contains(n) || maps_.contains(n); },
                [](const std::string&) { return false; });
            checkModuleAccess(forEach.name, forEach.loc);
            if (listInfo(forEach.name) == nullptr && mapInfo(forEach.name) == nullptr) {
                diagnostics_.error(forEach.loc, "'" + forEach.spelling + "' is not a declared List or Map");
            }
            visitNestedBlock(forEach.body);
            break;
        }
        case ast::StmtKind::IndexAssign: {
            auto& indexAssign = static_cast<ast::IndexAssignStmt&>(stmt);
            resolveModuleQualifiedName(
                indexAssign.name,
                [this](const std::string& n) {
                    return lists_.contains(n) || maps_.contains(n) || arrays_.contains(n);
                },
                [](const std::string&) { return false; }); // no builtin array/List/Map name exists
            checkModuleAccess(indexAssign.name, indexAssign.loc);
            for (auto& idx : indexAssign.indices) {
                visitExpr(*idx);
            }
            visitExpr(*indexAssign.value);
            if (indexAssign.indices.empty()) {
                if (const ListInfo* listInfoPtr = listInfo(indexAssign.name)) {
                    if (listInfoPtr->elementSuffix == TypeSuffix::Struct) {
                        diagnostics_.error(indexAssign.loc,
                                            "cannot assign directly to '" + indexAssign.spelling +
                                                "()', a Structure element - assign to one of its own fields instead");
                    } else {
                        checkAssignable(indexAssign.spelling, listInfoPtr->elementSuffix, *indexAssign.value,
                                         indexAssign.loc);
                    }
                    break;
                }
                // `name() = expr` is also how a Map's *current* element is
                // set (M3f) - identical shape to a List's own zero-arg form.
                if (const MapInfo* mapInfoPtr = mapInfo(indexAssign.name)) {
                    if (mapInfoPtr->elementSuffix == TypeSuffix::Struct) {
                        diagnostics_.error(indexAssign.loc,
                                            "cannot assign directly to '" + indexAssign.spelling +
                                                "()', a Structure element - assign to one of its own fields instead");
                    } else {
                        checkAssignable(indexAssign.spelling, mapInfoPtr->elementSuffix, *indexAssign.value,
                                         indexAssign.loc);
                    }
                    break;
                }
            }
            if (indexAssign.indices.size() == 1) {
                if (const MapInfo* mapInfoPtr = mapInfo(indexAssign.name)) {
                    checkMapKey(*indexAssign.indices.front(), indexAssign.loc);
                    if (mapInfoPtr->elementSuffix == TypeSuffix::Struct) {
                        diagnostics_.error(indexAssign.loc,
                                            "cannot assign directly to '" + indexAssign.spelling +
                                                "(...)', a Structure element - assign to one of its own fields instead");
                    } else {
                        checkAssignable(indexAssign.spelling, mapInfoPtr->elementSuffix, *indexAssign.value,
                                         indexAssign.loc);
                    }
                    break;
                }
            }
            const ArrayInfo* info = arrayInfo(indexAssign.name);
            if (info == nullptr) {
                diagnostics_.error(indexAssign.loc, "'" + indexAssign.spelling + "' is not a declared array");
                break;
            }
            if (std::cmp_not_equal(indexAssign.indices.size(), info->dimensionCount)) {
                diagnostics_.error(indexAssign.loc, "'" + indexAssign.spelling +
                                                         "' indexed with the wrong number of dimensions");
            }
            if (info->elementSuffix == TypeSuffix::Struct) {
                diagnostics_.error(indexAssign.loc, "cannot assign directly to '" + indexAssign.spelling +
                                                         "(...)', a Structure element - assign to one of its "
                                                         "own fields instead");
            } else {
                checkAssignable(indexAssign.spelling, info->elementSuffix, *indexAssign.value, indexAssign.loc);
            }
            break;
        }
        case ast::StmtKind::Assign: {
            auto& assign = static_cast<ast::AssignStmt&>(stmt);
            resolveModuleQualifiedName(
                assign.name, [this](const std::string& n) { return symbols_.contains(n); },
                [](const std::string&) { return false; }); // no such thing as a builtin global variable
            checkModuleAccess(assign.name, assign.loc);
            TypeSuffix suffix = assign.suffix == TypeSuffix::None ? typeOf(assign.name) : assign.suffix;
            declareImplicit(assign.name, assign.spelling, suffix, assign.loc);
            visitExpr(*assign.value);
            checkAssignable(assign.spelling, symbols_[assign.name], *assign.value, assign.loc);
            break;
        }
        case ast::StmtKind::Debug: {
            auto& dbg = static_cast<ast::DebugStmt&>(stmt);
            visitExpr(*dbg.value);
            break;
        }
        case ast::StmtKind::If: {
            auto& ifStmt = static_cast<ast::IfStmt&>(stmt);
            for (auto& branch : ifStmt.branches) {
                if (branch.condition) {
                    visitCondition(*branch.condition);
                }
                visitNestedBlock(branch.body);
            }
            break;
        }
        case ast::StmtKind::Select: {
            auto& sel = static_cast<ast::SelectStmt&>(stmt);
            visitExpr(*sel.selector);
            for (auto& branch : sel.cases) {
                for (auto& value : branch.values) {
                    visitExpr(*value);
                }
                visitNestedBlock(branch.body);
            }
            break;
        }
        case ast::StmtKind::CompilerIf:
        case ast::StmtKind::CompilerSelect:
            // Unreachable: preResolveCompilerDirectives() (run once, before
            // visitBlock's main walk even starts - see Sema::analyze())
            // already resolves and splices away every one of these nodes
            // directly.
        case ast::StmtKind::DataLabel:
        case ast::StmtKind::Data:
            // Unreachable: these only ever exist inside a DataSectionStmt's
            // own `body`, which the DataSection case below walks directly -
            // they're never a direct child of any Block that visitBlock
            // itself iterates. Grouped with the CompilerIf/CompilerSelect
            // case just above (rather than its own identical `break;`)
            // since clang-tidy's bugprone-branch-clone flags two adjacent
            // switch cases with the same body - both exist purely so
            // -Wswitch stays an exhaustiveness net for this enum.
            break;
        case ast::StmtKind::DataSection: {
            auto& dataSection = static_cast<ast::DataSectionStmt&>(stmt);
            for (auto& child : dataSection.body) {
                if (child->kind == ast::StmtKind::Data) {
                    for (auto& value : static_cast<ast::DataStmt&>(*child).values) {
                        visitExpr(*value);
                    }
                }
                // DataLabelStmt: nothing left to do - collectDataSections()
                // already recorded its index in an earlier whole-Module
                // pre-pass (see Sema::analyze()).
            }
            break;
        }
        case ast::StmtKind::Read: {
            auto& read = static_cast<ast::ReadStmt&>(stmt);
            // Oracle-verified: `Read[.suffix] varname` auto-declares an
            // undeclared `varname` as Integer regardless of `Read`'s own
            // suffix - exactly the same declareImplicit(name, spelling,
            // typeOf(name), loc) pattern the `Assign` case above uses,
            // which is already idempotent for an already-declared name
            // (typeOf returns its existing suffix, so declare()'s own
            // mismatch check sees no change at all).
            declareImplicit(read.varName, read.varSpelling, typeOf(read.varName), read.loc);
            break;
        }
        case ast::StmtKind::Restore: {
            auto& restore = static_cast<ast::RestoreStmt&>(stmt);
            resolveModuleQualifiedName(
                restore.labelName, [this](const std::string& n) { return dataLabels_.contains(n); },
                [](const std::string&) { return false; }); // no builtin DataSection label exists
            checkModuleAccess(restore.labelName, restore.loc);
            if (!dataLabelIndex(restore.labelName)) {
                diagnostics_.error(restore.loc, "'" + restore.labelSpelling + "' is not a declared Data label");
            }
            break;
        }
        case ast::StmtKind::For: {
            auto& forStmt = static_cast<ast::ForStmt&>(stmt);
            TypeSuffix suffix = forStmt.suffix == TypeSuffix::None ? typeOf(forStmt.varName) : forStmt.suffix;
            visitExpr(*forStmt.from);
            visitExpr(*forStmt.to);
            if (forStmt.step) {
                visitExpr(*forStmt.step);
            }
            declareImplicit(forStmt.varName, forStmt.varSpelling, suffix, forStmt.loc);
            visitNestedBlock(forStmt.body);
            break;
        }
        case ast::StmtKind::While: {
            auto& whileStmt = static_cast<ast::WhileStmt&>(stmt);
            visitCondition(*whileStmt.condition);
            visitNestedBlock(whileStmt.body);
            break;
        }
        case ast::StmtKind::Repeat: {
            auto& repeatStmt = static_cast<ast::RepeatStmt&>(stmt);
            visitNestedBlock(repeatStmt.body);
            if (repeatStmt.untilCondition) {
                visitCondition(*repeatStmt.untilCondition);
            }
            break;
        }
        case ast::StmtKind::Break:
        case ast::StmtKind::Continue:
            break; // Nothing to resolve; "outside any loop" checking is a follow-up, not M1-blocking.
        case ast::StmtKind::EnableExplicit:
            explicitEnabled_ = true;
            break;
        case ast::StmtKind::ConstDecl: {
            auto& constDecl = static_cast<ast::ConstDeclStmt&>(stmt);
            // M7d's second slice: a #Constant declared inside a Module's
            // own body belongs to that module's own namespace.
            if (!currentModule_.empty()) {
                constDecl.name = currentModule_ + "::" + constDecl.name;
            }
            visitExpr(*constDecl.value);
            ValueKind family = familyOfExpr(*constDecl.value);
            TypeSuffix suffix = TypeSuffix::Integer;
            if (family == ValueKind::FloatFamily) {
                suffix = TypeSuffix::Double;
            } else if (family == ValueKind::StringFamily) {
                suffix = TypeSuffix::String;
            } else if (auto value = evalConstExpr(*constDecl.value)) {
                // Needed so a later CompilerIf/CompilerSelect can reference
                // this constant (evalConstExpr only knows Integer-family
                // values - see constantIntValues_'s own doc comment); a
                // genuinely non-constant-foldable Integer-family init
                // (there isn't one in this project's grammar today, but
                // nothing rules it out structurally) just leaves this
                // constant unusable in a CompilerIf condition, not an error.
                constantIntValues_[constDecl.name] = *value;
            }
            declareConst(constDecl.name, constDecl.spelling, suffix, constDecl.loc);
            break;
        }
        case ast::StmtKind::Enumeration: {
            auto& enumStmt = static_cast<ast::EnumerationStmt&>(stmt);
            std::int64_t nextValue = 0;
            for (auto& member : enumStmt.members) {
                // M7d's second slice: same module-namespacing as ConstDecl's
                // own, applied per member.
                if (!currentModule_.empty()) {
                    member.name = currentModule_ + "::" + member.name;
                }
                if (member.explicitValue) {
                    visitExpr(*member.explicitValue);
                    if (auto value = evalConstExpr(*member.explicitValue)) {
                        nextValue = *value;
                    }
                }
                constantIntValues_[member.name] = nextValue;
                ++nextValue;
                declareConst(member.name, member.spelling, TypeSuffix::Integer, enumStmt.loc);
            }
            break;
        }
        case ast::StmtKind::StructureDecl: {
            auto& structDecl = static_cast<ast::StructureDeclStmt&>(stmt);
            // M7d's second slice: a Structure declared inside a Module's
            // own body belongs to that module's own namespace.
            if (!currentModule_.empty()) {
                structDecl.name = currentModule_ + "::" + structDecl.name;
            }
            if (structures_.contains(structDecl.name) || interfaces_.contains(structDecl.name)) {
                diagnostics_.error(structDecl.loc,
                                    "'" + structDecl.spelling + "' is already declared as a Structure or Interface");
                break;
            }
            StructureInfo info;
            for (auto& field : structDecl.fields) {
                FieldInfo fieldInfo;
                fieldInfo.name = field.name;
                fieldInfo.spelling = field.spelling;
                fieldInfo.suffix = field.suffix == TypeSuffix::None ? TypeSuffix::Integer : field.suffix;
                if (fieldInfo.suffix == TypeSuffix::Struct) {
                    // Oracle-verified: a field can name another, previously
                    // declared Structure (nesting) - `structures_` only
                    // contains Structures already fully processed by this
                    // point in source order, so this naturally enforces
                    // PB's own declare-before-use rule for free.
                    resolveModuleQualifiedTypeName(field.structTypeName, structDecl.loc);
                    if (!structures_.contains(field.structTypeName)) {
                        diagnostics_.error(structDecl.loc,
                                            "'" + field.structTypeSpelling + "' is not a declared Structure");
                    }
                    fieldInfo.structTypeName = field.structTypeName;
                }
                info.fields.push_back(std::move(fieldInfo));
            }
            structures_.emplace(structDecl.name, info);
            structureOrder_.emplace_back(structDecl.name, info);
            break;
        }
        case ast::StmtKind::InterfaceDecl: {
            auto& ifaceDecl = static_cast<ast::InterfaceDeclStmt&>(stmt);
            // M7d's third slice: an Interface declared inside a Module's own
            // body belongs to that module's own namespace - the exact same
            // rule StructureDecl's own case already follows (see its doc
            // comment); resolveModuleQualifiedTypeName (the reference side)
            // already checked both tables together, this is just the
            // declaration side catching up to match.
            if (!currentModule_.empty()) {
                ifaceDecl.name = currentModule_ + "::" + ifaceDecl.name;
            }
            if (interfaces_.contains(ifaceDecl.name) || structures_.contains(ifaceDecl.name)) {
                diagnostics_.error(ifaceDecl.loc,
                                    "'" + ifaceDecl.spelling + "' is already declared as a Structure or Interface");
                break;
            }
            InterfaceInfo info;
            for (auto& method : ifaceDecl.methods) {
                InterfaceMethodInfo methodInfo;
                methodInfo.name = method.name;
                methodInfo.spelling = method.spelling;
                // No-suffix defaults to Integer, the same rule a Procedure's
                // own return type (and, below, a Declare's own parameter
                // list) already follows.
                methodInfo.returnSuffix = method.returnSuffix == TypeSuffix::None ? TypeSuffix::Integer
                                                                                   : method.returnSuffix;
                for (auto& param : method.params) {
                    // Deliberately primitive-suffix-only (no pointer/
                    // Structure params) - see InterfaceDeclStmt's own doc
                    // comment.
                    if (param.suffix == TypeSuffix::Struct) {
                        diagnostics_.error(ifaceDecl.loc, "'" + param.spelling + "' - Interface method parameters "
                                                           "can't be Structure-typed");
                    }
                    methodInfo.paramSuffixes.push_back(param.suffix == TypeSuffix::None ? TypeSuffix::Integer
                                                                                         : param.suffix);
                }
                info.methods.push_back(std::move(methodInfo));
            }
            interfaces_.emplace(ifaceDecl.name, std::move(info));
            break;
        }
        case ast::StmtKind::FieldAssign: {
            auto& fieldAssign = static_cast<ast::FieldAssignStmt&>(stmt);
            visitExpr(*fieldAssign.target);
            visitExpr(*fieldAssign.value);
            ResolvedType targetType = resolveType(*fieldAssign.target);
            const std::string& targetSpelling = static_cast<ast::FieldAccessExpr&>(*fieldAssign.target).fieldSpelling;
            if (targetType.suffix == TypeSuffix::Struct) {
                // Whole-Structure assignment (`r\topLeft = ...` where
                // topLeft is itself a nested Structure, not a leaf
                // primitive field) isn't supported - only leaf primitive
                // fields can be assigned to (`r\topLeft\x = ...` instead).
                diagnostics_.error(fieldAssign.loc,
                                    "cannot assign directly to '" + targetSpelling +
                                        "', which is a Structure - assign to one of its own fields instead");
            } else {
                checkAssignable(targetSpelling, targetType.suffix, *fieldAssign.value, fieldAssign.loc);
            }
            break;
        }
        case ast::StmtKind::Declare: {
            auto& decl = static_cast<ast::DeclareStmt&>(stmt);
            // M7d: a Declare inside a DeclareModule's own body promises a
            // procedure within that module's own namespace - mangled once,
            // here, before any of the matching logic below runs (the real
            // Procedure fulfilling it, inside the matching Module's body,
            // gets the identical mangled key via the exact same rule below).
            if (!currentModule_.empty()) {
                decl.name = currentModule_ + "::" + decl.name;
            }
            if (procedures_.contains(decl.name)) {
                // Either a genuine duplicate `Declare`, or one appearing
                // after the real `Procedure` already fully defined it -
                // neither is the intended "forward-declare, define later"
                // usage this exists for, so it's simplest to just flag it
                // the same way any other redeclaration is flagged rather
                // than modeling PB's own (unverified) behavior for this
                // unusual ordering.
                diagnostics_.error(decl.loc, "'" + decl.spelling + "' is already declared as a procedure");
                break;
            }
            ProcedureInfo info;
            info.returnSuffix = decl.returnSuffix == TypeSuffix::None ? TypeSuffix::Integer : decl.returnSuffix;
            for (auto& param : decl.params) {
                info.paramSuffixes.push_back(param.suffix == TypeSuffix::None ? TypeSuffix::Integer : param.suffix);
                if (!param.hasDefault) {
                    ++info.requiredParamCount;
                }
            }
            procedures_[decl.name] = info;
            declaredNotDefined_[decl.name] = {decl.spelling, decl.loc};
            break;
        }
        case ast::StmtKind::ProcedureDecl: {
            auto& proc = static_cast<ast::ProcedureDeclStmt&>(stmt);
            // Oracle-verified: PB rejects a Procedure declared in either of
            // these positions outright (distinct wording for each), rather
            // than accepting it and doing something PB-specific with it - so
            // this reports the error and stops, matching that (unlike a
            // recoverable issue like a redeclaration, just below, there is
            // no sensible "continue processing anyway" for a Procedure whose
            // very presence here is illegal).
            if (insideProcedure_) {
                diagnostics_.error(proc.loc, "Can't define a procedure inside another procedure.");
                break;
            }
            if (controlFlowDepth_ > 0) {
                diagnostics_.error(proc.loc,
                                    "A procedure can't be declared inside an If, Repeat, While or For.");
                break;
            }
            // M7d: a Procedure declared inside a Module's own body belongs
            // to that module's own namespace - mangled once, here, so
            // everything below (duplicate-detection, Declare-fulfillment
            // matching, registration) operates on the final key already.
            if (!currentModule_.empty()) {
                proc.name = currentModule_ + "::" + proc.name;
            }
            TypeSuffix returnSuffix = proc.returnSuffix == TypeSuffix::None ? TypeSuffix::Integer : proc.returnSuffix;

            // Fulfilling an earlier `Declare` looks identical to a plain
            // redeclaration (`procedures_` already contains the name
            // either way) - `declaredNotDefined_` is what tells the two
            // apart, so only a genuine second full definition is flagged
            // here.
            bool fulfillsDeclare = declaredNotDefined_.contains(proc.name);
            if (procedures_.contains(proc.name) && !fulfillsDeclare) {
                diagnostics_.error(proc.loc, "'" + proc.spelling + "' is already declared as a procedure");
            }

            ProcedureInfo info;
            info.returnSuffix = returnSuffix;
            bool sawDefault = false;
            for (auto& param : proc.params) {
                // A pointer parameter's own runtime representation is always
                // a plain Integer address, exactly like a pointer Define
                // (see the Define case above) - `param.suffix` here
                // describes the pointee, not the parameter's own storage.
                TypeSuffix paramSuffix = TypeSuffix::Integer;
                if (!param.isPointer && param.suffix != TypeSuffix::None) {
                    paramSuffix = param.suffix;
                }
                info.paramSuffixes.push_back(paramSuffix);
                if (param.defaultValue) {
                    sawDefault = true;
                } else if (sawDefault) {
                    diagnostics_.error(proc.loc,
                                        "'" + param.spelling + "': a required parameter cannot follow one with a default value");
                } else {
                    ++info.requiredParamCount;
                }
            }
            if (fulfillsDeclare) {
                // Oracle-verified: the real Procedure must match the
                // Declare's promised signature exactly (return type AND
                // every parameter type, not just arity) - "Declare doesn't
                // match with real Procedure." otherwise.
                const ProcedureInfo& declared = procedures_[proc.name];
                if (declared.returnSuffix != info.returnSuffix || declared.paramSuffixes != info.paramSuffixes) {
                    diagnostics_.error(proc.loc, "Declare doesn't match with real Procedure.");
                }
                declaredNotDefined_.erase(proc.name);
            }
            // Registered BEFORE the body is visited - oracle-verified PB
            // requires this for self-recursion to resolve at all (see the
            // struct's own doc comment), so this mirrors real PB exactly.
            procedures_[proc.name] = info;

            // Default values are evaluated in the OUTER scope, not against
            // the procedure's own (not-yet-entered) locals.
            for (auto& param : proc.params) {
                if (param.defaultValue) {
                    visitExpr(*param.defaultValue);
                }
            }

            // Swap to a fresh, ISOLATED local scope - oracle-verified: a
            // procedure body cannot see outer variables at all by default
            // (see ast::ProcedureDeclStmt's own doc comment). PB procedures
            // never nest, so a simple save/restore is enough, not a stack.
            auto savedSymbols = std::move(symbols_);
            auto savedOrder = std::move(order_);
            symbols_ = {};
            order_ = {};

            for (auto& param : proc.params) {
                if (param.isPointer) {
                    declare(param.name, param.spelling, TypeSuffix::Integer, proc.loc);
                    resolveModuleQualifiedTypeName(param.structTypeName, proc.loc);
                    pointerPointeeType_[param.name] = resolvePointeeType(param.suffix, param.structTypeName,
                                                                         param.structTypeSpelling, proc.loc);
                    continue;
                }
                TypeSuffix paramSuffix = param.suffix == TypeSuffix::None ? TypeSuffix::Integer : param.suffix;
                declare(param.name, param.spelling, paramSuffix, proc.loc);
            }

            // `Global` variables are auto-visible in every procedure with no
            // `Shared` needed (oracle-verified) - pre-populate for type
            // resolution, but never overwrite a same-named parameter, and
            // deliberately without adding to `order_` (see
            // Sema::bringIntoScope's own doc comment for why).
            for (const auto& globalName : globalNames_) {
                if (symbols_.contains(globalName)) {
                    continue;
                }
                auto globalIt = savedSymbols.find(globalName);
                if (globalIt != savedSymbols.end()) {
                    symbols_[globalName] = globalIt->second;
                }
            }

            bool savedInside = insideProcedure_;
            TypeSuffix savedReturnSuffix = currentProcedureReturnSuffix_;
            const std::unordered_map<std::string, TypeSuffix>* savedOuterForShared = outerScopeForShared_;
            insideProcedure_ = true;
            currentProcedureReturnSuffix_ = returnSuffix;
            outerScopeForShared_ = &savedSymbols;

            visitBlock(proc.body);

            insideProcedure_ = savedInside;
            currentProcedureReturnSuffix_ = savedReturnSuffix;
            outerScopeForShared_ = savedOuterForShared;

            procedures_[proc.name].locals = order_; // params first, then any body-internal locals
            // Captures this procedure's own pointer locals' pointee types
            // before the cross-procedure leak described in ProcedureInfo::
            // pointerPointeeTypes's own doc comment can happen - a later
            // sibling procedure reusing the same pointer parameter name
            // (e.g. every Interface-implementing procedure idiomatically
            // naming its own "this" parameter the same way) would otherwise
            // silently overwrite pointerPointeeType_'s shared entry before
            // Codegen ever reads it back.
            for (const auto& local : order_) {
                auto ptrIt = pointerPointeeType_.find(local.first);
                if (ptrIt != pointerPointeeType_.end()) {
                    procedures_[proc.name].pointerPointeeTypes[local.first] = ptrIt->second;
                }
            }

            symbols_ = std::move(savedSymbols);
            order_ = std::move(savedOrder);
            break;
        }
        case ast::StmtKind::DeclareModule: {
            auto& decl = static_cast<ast::DeclareModuleStmt&>(stmt);
            if (insideProcedure_ || controlFlowDepth_ > 0 || !currentModule_.empty()) {
                diagnostics_.error(decl.loc, "A Module can't be declared inside a Procedure, a control-flow "
                                              "block, or another Module.");
                break;
            }
            currentModule_ = decl.name;
            // `Declare` (a public procedure signature), `Global` (a public
            // variable), `Structure`, `Interface`, `Enumeration`, a
            // `#Constant`, `Dim`, `NewList`, `NewMap`, and `DataSection` are
            // all supported inside a DeclareModule section. `Macro` needs no
            // case here at all - the preprocessor's own separate pass
            // already expands it away before this construct even has AST
            // shape (see DeclareModuleStmt's own doc comment). Each allowed
            // statement is visited directly (not
            // via visitBlock) so this restriction is enforced here, once,
            // rather than threaded through the general statement
            // dispatcher; the public set itself is always keyed by the
            // *plain*, unmangled member name, captured *before* visiting
            // (visitStmt's own case for each kind mangles `.name` in place).
            auto& publicSet = modulePublicMembers_[decl.name];
            for (auto& bodyStmt : decl.body) {
                switch (bodyStmt->kind) {
                    case ast::StmtKind::Declare:
                        publicSet.insert(static_cast<ast::DeclareStmt&>(*bodyStmt).name);
                        visitStmt(*bodyStmt);
                        break;
                    case ast::StmtKind::Define:
                        if (static_cast<ast::DefineStmt&>(*bodyStmt).isGlobal) {
                            for (auto& decl2 : static_cast<ast::DefineStmt&>(*bodyStmt).declarators) {
                                publicSet.insert(decl2.name);
                            }
                            visitStmt(*bodyStmt);
                        } else {
                            diagnostics_.error(bodyStmt->loc, "Only 'Global' (not a plain 'Define'/'Protected') "
                                                               "is supported inside a DeclareModule section.");
                        }
                        break;
                    case ast::StmtKind::StructureDecl:
                        publicSet.insert(static_cast<ast::StructureDeclStmt&>(*bodyStmt).name);
                        visitStmt(*bodyStmt);
                        break;
                    case ast::StmtKind::InterfaceDecl:
                        publicSet.insert(static_cast<ast::InterfaceDeclStmt&>(*bodyStmt).name);
                        visitStmt(*bodyStmt);
                        break;
                    case ast::StmtKind::Enumeration:
                        for (auto& member : static_cast<ast::EnumerationStmt&>(*bodyStmt).members) {
                            publicSet.insert(member.name);
                        }
                        visitStmt(*bodyStmt);
                        break;
                    case ast::StmtKind::ConstDecl:
                        publicSet.insert(static_cast<ast::ConstDeclStmt&>(*bodyStmt).name);
                        visitStmt(*bodyStmt);
                        break;
                    case ast::StmtKind::Dim:
                        publicSet.insert(static_cast<ast::DimStmt&>(*bodyStmt).name);
                        visitStmt(*bodyStmt);
                        break;
                    case ast::StmtKind::NewList:
                        publicSet.insert(static_cast<ast::NewListStmt&>(*bodyStmt).name);
                        visitStmt(*bodyStmt);
                        break;
                    case ast::StmtKind::NewMap:
                        publicSet.insert(static_cast<ast::NewMapStmt&>(*bodyStmt).name);
                        visitStmt(*bodyStmt);
                        break;
                    case ast::StmtKind::DataSection:
                        for (auto& child : static_cast<ast::DataSectionStmt&>(*bodyStmt).body) {
                            if (child->kind == ast::StmtKind::DataLabel) {
                                // Unlike every other kind here, collectDataSections()
                                // (a whole-Module pre-pass that runs *before* this
                                // walk - see Sema::analyze()) has already mangled
                                // this label's own `.name` to its qualified form,
                                // so the plain member name for the public set has
                                // to be extracted back out of it instead of read
                                // directly.
                                const std::string& mangled = static_cast<ast::DataLabelStmt&>(*child).name;
                                auto sep = mangled.rfind("::");
                                publicSet.insert(sep == std::string::npos ? mangled : mangled.substr(sep + 2));
                            }
                        }
                        visitStmt(*bodyStmt);
                        break;
                    default:
                        diagnostics_.error(bodyStmt->loc,
                                            "This statement kind is not currently supported inside a "
                                            "DeclareModule section.");
                        break;
                }
            }
            currentModule_.clear();
            break;
        }
        case ast::StmtKind::Module: {
            auto& mod = static_cast<ast::ModuleStmt&>(stmt);
            if (insideProcedure_ || controlFlowDepth_ > 0 || !currentModule_.empty()) {
                diagnostics_.error(mod.loc, "A Module can't be declared inside a Procedure, a control-flow "
                                             "block, or another Module.");
                break;
            }
            currentModule_ = mod.name;
            // `UseModule`/`UnuseModule` used inside this Module's own body
            // (oracle-verified legal - a "common" module's Globals shared
            // by several other modules each UseModule it internally) are
            // scoped to it, not leaked past this body's own EndModule.
            auto savedImports = activeImports_;
            // Oracle-verified: a Module's own top-level body isn't just
            // declarations - ordinary executable code (a plain assignment,
            // in the "common module" example) runs right there, at its own
            // textual position, exactly like top-level code outside any
            // module (see ModuleStmt's own doc comment). So this is a
            // *blocklist*, not an allowlist like DeclareModule's own body
            // has: everything not explicitly deferred here (nested modules -
            // not module-scoped, so letting one through would silently leak
            // into the flat top-level tables instead of this module's own
            // namespace; `Macro` needs no case at all here, since the
            // preprocessor's own separate pass already expands it away
            // before this construct even has AST shape - see
            // ModuleStmt's own doc comment) is visited through the ordinary
            // dispatcher, which is already module-aware via `currentModule_`
            // for everything this slice supports (Procedure/Declare/Global/
            // Structure/Interface/Enumeration/constant/Dim/NewList/NewMap/
            // DataSection/plain statements referencing them).
            for (auto& bodyStmt : mod.body) {
                switch (bodyStmt->kind) {
                    case ast::StmtKind::DeclareModule:
                    case ast::StmtKind::Module:
                        diagnostics_.error(bodyStmt->loc,
                                            "This statement kind is not yet supported inside a Module section.");
                        break;
                    case ast::StmtKind::Define:
                        if (static_cast<ast::DefineStmt&>(*bodyStmt).isGlobal) {
                            visitStmt(*bodyStmt);
                        } else {
                            diagnostics_.error(bodyStmt->loc, "Only 'Global' (not a plain 'Define'/'Protected') "
                                                               "is currently supported directly inside a Module "
                                                               "section's own top level (a Procedure's own local "
                                                               "Define is unaffected).");
                        }
                        break;
                    default:
                        visitStmt(*bodyStmt);
                        break;
                }
            }
            activeImports_ = std::move(savedImports);
            currentModule_.clear();
            break;
        }
        case ast::StmtKind::UseModule: {
            auto& use = static_cast<ast::UseModuleStmt&>(stmt);
            if (std::find(activeImports_.begin(), activeImports_.end(), use.name) == activeImports_.end()) {
                activeImports_.push_back(use.name);
            }
            break;
        }
        case ast::StmtKind::UnuseModule: {
            auto& unuse = static_cast<ast::UnuseModuleStmt&>(stmt);
            activeImports_.erase(std::remove(activeImports_.begin(), activeImports_.end(), unuse.name),
                                  activeImports_.end());
            break;
        }
        case ast::StmtKind::ProcedureReturn: {
            auto& ret = static_cast<ast::ProcedureReturnStmt&>(stmt);
            if (ret.value) {
                visitExpr(*ret.value);
                checkAssignable("return value", currentProcedureReturnSuffix_, *ret.value, ret.loc);
            }
            break;
        }
        case ast::StmtKind::ExprStmt: {
            auto& exprStmt = static_cast<ast::ExprStmt&>(stmt);
            visitExpr(*exprStmt.expr);
            break;
        }
    }
}

void Sema::visitExpr(ast::Expr& expr) {
    switch (expr.kind) {
        case ast::ExprKind::VarRef: {
            auto& ref = static_cast<ast::VarRefExpr&>(expr);
            resolveModuleQualifiedName(
                ref.name, [this](const std::string& n) { return symbols_.contains(n); },
                [](const std::string&) { return false; }); // no such thing as a builtin global variable
            checkModuleAccess(ref.name, ref.loc);
            if (!symbols_.contains(ref.name)) {
                // Implicit read of a never-assigned name: real PB gives it
                // type Integer and value 0 (or errors under EnableExplicit).
                declareImplicit(ref.name, ref.spelling, TypeSuffix::Integer, ref.loc);
            }
            break;
        }
        case ast::ExprKind::ConstRef: {
            auto& ref = static_cast<ast::ConstRefExpr&>(expr);
            resolveModuleQualifiedName(
                ref.name, [this](const std::string& n) { return constants_.contains(n); },
                [](const std::string& n) { return builtinConstantValue(n).has_value(); });
            checkModuleAccess(ref.name, ref.loc);
            if (!constants_.contains(ref.name)) {
                diagnostics_.error(ref.loc, "'#" + ref.spelling + "' is not declared");
                declareConst(ref.name, ref.spelling, TypeSuffix::Integer, ref.loc); // recovery fallback
            }
            break;
        }
        case ast::ExprKind::Binary: {
            auto& bin = static_cast<ast::BinaryExpr&>(expr);
            visitExpr(*bin.lhs);
            visitExpr(*bin.rhs);
            break;
        }
        case ast::ExprKind::Unary: {
            auto& un = static_cast<ast::UnaryExpr&>(expr);
            visitExpr(*un.operand);
            break;
        }
        case ast::ExprKind::Call: {
            auto& call = static_cast<ast::CallExpr&>(expr);
            // M7d's second slice: resolved once, here, ahead of every kind
            // of `Name(args)` this case can mean (procedure/array/List/Map/
            // built-in) - everything below (including visitCall's own,
            // separate call site) sees the final key already.
            resolveModuleQualifiedName(
                call.name,
                [this](const std::string& n) {
                    return procedures_.contains(n) || lists_.contains(n) || maps_.contains(n) ||
                           arrays_.contains(n);
                },
                [this](const std::string& n) {
                    return isStringLibBuiltinName(n) || isMathLibBuiltinName(n) || isMemoryLibBuiltinName(n) ||
                           isFileLibBuiltinName(n) || isDateLibBuiltinName(n) || isThreadLibBuiltinName(n) ||
                           isGuiLibBuiltinName(n) || isPointerBuiltinName(n) || isListBuiltinName(n) ||
                           isMapBuiltinName(n);
                });
            checkModuleAccess(call.name, call.loc);
            // The handful of pointer/memory built-ins are recognized by name
            // before anything else - `AllocateStructure`'s sole argument in
            // particular must NOT fall through to the ordinary call/array
            // handling below (see visitPointerBuiltinCall's own doc comment).
            if (visitPointerBuiltinCall(call)) {
                break;
            }
            if (visitListBuiltinCall(call)) {
                break;
            }
            if (visitMapBuiltinCall(call)) {
                break;
            }
            if (listInfo(call.name) != nullptr) {
                // `name()` reads the List's current element (M3e) - no args
                // to visit (see Sema::ListInfo's own doc comment).
                if (!call.args.empty()) {
                    diagnostics_.error(call.loc, "'" + call.spelling + "' is a List and takes no arguments here");
                }
                break;
            }
            if (mapInfo(call.name) != nullptr) {
                // `name()` reads the Map's current element; `name(key)`
                // reads (and auto-creates) by key (M3f - see Sema::MapInfo's
                // own doc comment).
                if (call.args.size() == 1) {
                    visitExpr(*call.args.front());
                    checkMapKey(*call.args.front(), call.loc);
                } else if (!call.args.empty()) {
                    diagnostics_.error(call.loc, "'" + call.spelling + "' takes at most one key argument");
                }
                break;
            }
            // `Name(args)` is ambiguous with a call at parse time - a
            // `Dim`'d name always means an array read here (see ArrayInfo's
            // own doc comment).
            if (const ArrayInfo* info = arrayInfo(call.name)) {
                if (std::cmp_not_equal(call.args.size(), info->dimensionCount)) {
                    diagnostics_.error(call.loc,
                                        "'" + call.spelling + "' indexed with the wrong number of dimensions");
                }
                for (auto& idx : call.args) {
                    visitExpr(*idx);
                }
            } else {
                visitCall(call);
            }
            break;
        }
        case ast::ExprKind::FieldAccess: {
            auto& access = static_cast<ast::FieldAccessExpr&>(expr);
            visitExpr(*access.base);
            resolveType(expr); // reported errors only; handles the pointer-dereference case too.
            break;
        }
        case ast::ExprKind::AddressOf: {
            auto& addr = static_cast<ast::AddressOfExpr&>(expr);
            // `@ProcedureName()` - the procedure's own address (oracle-
            // verified, e.g. `CreateThread(@Worker(), param)`) - is NOT an
            // ordinary call to visit normally: it's always written with
            // empty parens regardless of the named procedure's own
            // parameter list (oracle-verified: `@Worker()` is legal even
            // though `Worker` itself takes one parameter), so running it
            // through the normal call-arity machinery would wrongly flag
            // it as a missing-argument error. Recognized here, before the
            // operand is visited at all, exactly like the List/Map bare
            // `name()` special-casing elsewhere in this function.
            //
            // M7d's third slice: `call.name` is resolved through
            // `resolveModuleQualifiedName` *first* - a real, pre-existing
            // gap this slice closes (flagged but left unfixed by the first
            // Module slice, since no oracle example exercised it then): an
            // Interface's vtable DataSection (`Data.i @ProcName()`) is the
            // one realistic case that actually needs a module-scoped
            // `@ProcedureName()` to work at all, so it can no longer stay
            // deferred once Interface itself is module-scoped. Safe to call
            // here even though a plain (non-`@`) call to the same procedure
            // resolves it again later - the function is a no-op once a name
            // already contains "::" (see its own doc comment).
            if (addr.operand->kind == ast::ExprKind::Call) {
                auto& call = static_cast<ast::CallExpr&>(*addr.operand);
                resolveModuleQualifiedName(
                    call.name, [this](const std::string& n) { return procedures_.contains(n); },
                    [](const std::string&) { return false; }); // no builtin procedure can be @-addressed
                if (procedureInfo(call.name) != nullptr) {
                    checkModuleAccess(call.name, addr.loc);
                    if (!call.args.empty()) {
                        diagnostics_.error(addr.loc,
                                            "'@" + call.spelling + "()' takes no arguments - it names the "
                                            "procedure itself, not a call to it");
                    }
                    break;
                }
            }
            visitExpr(*addr.operand);
            break;
        }
        case ast::ExprKind::DataLabelAddress: {
            auto& addr = static_cast<ast::DataLabelAddressExpr&>(expr);
            resolveModuleQualifiedName(
                addr.labelName, [this](const std::string& n) { return dataLabels_.contains(n); },
                [](const std::string&) { return false; }); // no builtin DataSection label exists
            checkModuleAccess(addr.labelName, addr.loc);
            if (!dataLabels_.contains(addr.labelName)) {
                diagnostics_.error(addr.loc, "'" + addr.labelSpelling + "' is not a declared DataSection label");
            } else if (!dataLabelAddressable(addr.labelName)) {
                diagnostics_.error(addr.loc, "'?" + addr.labelSpelling + "' is only supported for a DataSection "
                                              "label whose own Data items are all '.i'-typed");
            }
            break;
        }
        case ast::ExprKind::MethodCall: {
            auto& call = static_cast<ast::MethodCallExpr&>(expr);
            visitExpr(*call.base);
            for (auto& arg : call.args) {
                visitExpr(*arg);
            }
            const InterfaceMethodInfo* methodInfo = resolveInterfaceMethod(call);
            if (methodInfo != nullptr && call.args.size() != methodInfo->paramSuffixes.size()) {
                diagnostics_.error(call.loc,
                                    "'" + call.methodSpelling + "' called with the wrong number of arguments");
            }
            if (methodInfo != nullptr) {
                for (std::size_t i = 0; i < call.args.size() && i < methodInfo->paramSuffixes.size(); ++i) {
                    // Same String<->numeric mismatch check an ordinary
                    // call's own arguments already get (see visitCall).
                    bool paramIsString = familyOf(methodInfo->paramSuffixes[i]) == ValueKind::StringFamily;
                    bool argIsString = classify(*call.args[i], false) == ValueKind::StringFamily;
                    if (paramIsString != argIsString) {
                        diagnostics_.error(call.args[i]->loc,
                                            paramIsString ? "Bad parameter type: a string is expected."
                                                           : "Bad parameter type, number expected instead of string.");
                    }
                }
            }
            break;
        }
        case ast::ExprKind::IntLiteral:
        case ast::ExprKind::FloatLiteral:
        case ast::ExprKind::StringLiteral:
            break;
    }
}

void Sema::bringIntoScope(const std::unordered_map<std::string, TypeSuffix>& outerScope,
                           const std::string& lowerName, const std::string& spelling, SourceLoc loc,
                           const char* directiveNameForError) {
    auto it = outerScope.find(lowerName);
    TypeSuffix suffix = TypeSuffix::Integer;
    if (it == outerScope.end()) {
        diagnostics_.error(loc, "'" + spelling + "' is not declared, cannot be used with '" +
                                     directiveNameForError + "'");
    } else {
        suffix = it->second;
    }
    symbols_[lowerName] = suffix; // Deliberately NOT added to order_ - see this method's own doc comment.
}

void Sema::visitCall(ast::CallExpr& call) {
    // Already resolved + public/private-checked by visitExpr's own Call
    // case, the only caller - see its own doc comment.
    if (isGuiLibBuiltinName(call.name)) {
        usesGui_ = true;
    }
    if (isSysTrayLibBuiltinName(call.name)) {
        usesSysTray_ = true;
    }
    auto it = procedures_.find(call.name);
    if (it == procedures_.end()) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' is not a declared procedure");
        for (auto& arg : call.args) {
            visitExpr(*arg);
        }
        return;
    }
    const ProcedureInfo& info = it->second;
    if (call.args.size() < info.requiredParamCount || call.args.size() > info.paramSuffixes.size()) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' called with the wrong number of arguments");
    }
    for (std::size_t i = 0; i < call.args.size(); ++i) {
        visitExpr(*call.args[i]);
        if (i >= info.paramSuffixes.size()) {
            continue; // Already flagged above as a wrong-argument-count error.
        }
        // Oracle-verified: real PB rejects a String<->numeric mismatch here
        // too (with its own distinct wording from `checkAssignable`'s, and -
        // unlike an arity mismatch - only caught by a full compile, not by
        // `-k`'s syntax-only check, a general lesson recorded in this
        // project's oracle-testing methodology doc): "Bad parameter type,
        // number expected instead of string." when a String argument is
        // passed for a numeric parameter, or "Bad parameter type: a string
        // is expected." for the reverse.
        bool paramIsString = familyOf(info.paramSuffixes[i]) == ValueKind::StringFamily;
        bool argIsString = familyOfExpr(*call.args[i]) == ValueKind::StringFamily;
        if (paramIsString != argIsString) {
            diagnostics_.error(call.loc, paramIsString ? "Bad parameter type: a string is expected."
                                                        : "Bad parameter type, number expected instead of string.");
        }
    }
}

void Sema::visitCondition(ast::Expr& expr) {
    if (expr.kind == ast::ExprKind::Binary) {
        auto& bin = static_cast<ast::BinaryExpr&>(expr);
        switch (bin.op) {
            case ast::BinaryOp::LogicalAnd:
            case ast::BinaryOp::LogicalOr:
                visitCondition(*bin.lhs);
                visitCondition(*bin.rhs);
                return;
            case ast::BinaryOp::LogicalXOr:
                // Real PB's logical XOr showed a runtime truth table that
                // didn't match ANY consistent interpretation under oracle
                // testing (see docs/architecture/roadmap.md's M1 notes) -
                // rather than guess, this is flagged as unsupported.
                diagnostics_.error(bin.loc,
                                    "logical 'XOr' is not yet supported (see "
                                    "docs/architecture/roadmap.md's M1 notes)");
                visitCondition(*bin.lhs);
                visitCondition(*bin.rhs);
                return;
            case ast::BinaryOp::Eq:
            case ast::BinaryOp::Ne:
            case ast::BinaryOp::Lt:
            case ast::BinaryOp::Gt:
            case ast::BinaryOp::Le:
            case ast::BinaryOp::Ge:
                visitExpr(*bin.lhs);
                visitExpr(*bin.rhs);
                return;
            default:
                break; // A bare arithmetic expression used as a condition - fall through.
        }
    } else if (expr.kind == ast::ExprKind::Unary) {
        auto& un = static_cast<ast::UnaryExpr&>(expr);
        if (un.op == ast::UnaryOp::LogicalNot) {
            visitCondition(*un.operand);
            return;
        }
    }
    visitExpr(expr);
}

void Sema::checkAssignable(const std::string& targetSpelling, TypeSuffix targetSuffix,
                            const ast::Expr& value, SourceLoc loc) {
    ValueKind targetFamily = familyOf(targetSuffix);
    ValueKind valueFamily = familyOfExpr(value);
    bool targetIsString = targetFamily == ValueKind::StringFamily;
    bool valueIsString = valueFamily == ValueKind::StringFamily;
    if (targetIsString != valueIsString) {
        diagnostics_.error(loc, "cannot assign " +
                                     std::string(valueIsString ? "a String" : "a numeric value") +
                                     " to '" + targetSpelling + "', which is " +
                                     (targetIsString ? "a String" : "numeric") +
                                     " (use Str()/Val() to convert explicitly)");
    }
}

void Sema::checkMapKey(const ast::Expr& keyExpr, SourceLoc loc) const {
    if (familyOfExpr(keyExpr) != ValueKind::StringFamily) {
        diagnostics_.error(loc, "A string expression is expected");
    }
}

TypeSuffix Sema::typeOf(const std::string& lowerName) const {
    auto it = symbols_.find(lowerName);
    return it == symbols_.end() ? TypeSuffix::Integer : it->second;
}

const std::string& Sema::structTypeOfVar(const std::string& lowerName) const {
    static const std::string empty;
    auto it = varStructType_.find(lowerName);
    return it == varStructType_.end() ? empty : it->second;
}

ValueKind Sema::familyOfExpr(const ast::Expr& expr) const { return classify(expr, false); }

ValueKind Sema::classify(const ast::Expr& expr, bool floatContext) const {
    switch (expr.kind) {
        case ast::ExprKind::IntLiteral:
            return floatContext ? ValueKind::FloatFamily : ValueKind::IntegerFamily;
        case ast::ExprKind::FloatLiteral:
            return ValueKind::FloatFamily;
        case ast::ExprKind::StringLiteral:
            return ValueKind::StringFamily; // Context never turns a String into a number.
        case ast::ExprKind::VarRef: {
            const auto& ref = static_cast<const ast::VarRefExpr&>(expr);
            ValueKind natural = familyOf(typeOf(ref.name));
            if (natural == ValueKind::StringFamily) {
                return ValueKind::StringFamily;
            }
            return floatContext ? ValueKind::FloatFamily : natural;
        }
        case ast::ExprKind::ConstRef: {
            const auto& ref = static_cast<const ast::ConstRefExpr&>(expr);
            ValueKind natural = familyOf(constTypeOf(ref.name));
            if (natural == ValueKind::StringFamily) {
                return ValueKind::StringFamily;
            }
            return floatContext ? ValueKind::FloatFamily : natural;
        }
        case ast::ExprKind::Call: {
            const auto& call = static_cast<const ast::CallExpr&>(expr);
            if (call.name == "mapkey") {
                // The one List/Map/pointer built-in that returns something
                // other than a plain Integer status code - MapKey(map())
                // yields the current element's String key (oracle-verified:
                // it's used directly as a String value, e.g. `Debug
                // MapKey(m())`) - so unlike every other builtin here, it
                // can't rely on the generic "unrecognized Call defaults to
                // Integer" fallback below.
                return ValueKind::StringFamily;
            }
            const ListInfo* list = listInfo(call.name);
            const MapInfo* map = list == nullptr ? mapInfo(call.name) : nullptr;
            const ArrayInfo* array = (list == nullptr && map == nullptr) ? arrayInfo(call.name) : nullptr;
            const ProcedureInfo* proc =
                (list == nullptr && map == nullptr && array == nullptr) ? procedureInfo(call.name) : nullptr;
            ValueKind natural = ValueKind::IntegerFamily;
            if (list != nullptr) {
                natural = familyOf(list->elementSuffix);
            } else if (map != nullptr) {
                natural = familyOf(map->elementSuffix);
            } else if (array != nullptr) {
                natural = familyOf(array->elementSuffix);
            } else if (proc != nullptr) {
                natural = familyOf(proc->returnSuffix);
            }
            if (natural == ValueKind::StringFamily) {
                return ValueKind::StringFamily;
            }
            return floatContext ? ValueKind::FloatFamily : natural;
        }
        case ast::ExprKind::FieldAccess: {
            ValueKind natural = familyOf(resolveType(expr).suffix);
            if (natural == ValueKind::StringFamily) {
                return ValueKind::StringFamily;
            }
            return floatContext ? ValueKind::FloatFamily : natural;
        }
        case ast::ExprKind::Unary: {
            const auto& un = static_cast<const ast::UnaryExpr&>(expr);
            return classify(*un.operand, floatContext);
        }
        case ast::ExprKind::AddressOf:
            // `@operand` always yields a plain Integer address, regardless
            // of any enclosing Float destination.
            return ValueKind::IntegerFamily;
        case ast::ExprKind::DataLabelAddress:
            // `?Label` likewise always yields a plain Integer address.
            return ValueKind::IntegerFamily;
        case ast::ExprKind::MethodCall: {
            const auto& call = static_cast<const ast::MethodCallExpr&>(expr);
            ValueKind natural = ValueKind::IntegerFamily;
            if (const InterfaceMethodInfo* methodInfo = resolveInterfaceMethod(call)) {
                natural = familyOf(methodInfo->returnSuffix);
            }
            if (natural == ValueKind::StringFamily) {
                return ValueKind::StringFamily;
            }
            return floatContext ? ValueKind::FloatFamily : natural;
        }
        case ast::ExprKind::Binary: {
            const auto& bin = static_cast<const ast::BinaryExpr&>(expr);
            switch (bin.op) {
                case ast::BinaryOp::Div:
                case ast::BinaryOp::Mod: {
                    // Target-typed: real (Div) / real-modulo (Mod) exactly
                    // when floatContext is set OR either operand is
                    // *naturally* (context-free) float-family - oracle-
                    // verified for Div; Mod follows by consistent
                    // extrapolation only (see classify()'s own doc comment).
                    bool lhsFloat = classify(*bin.lhs, false) == ValueKind::FloatFamily;
                    bool rhsFloat = classify(*bin.rhs, false) == ValueKind::FloatFamily;
                    return (floatContext || lhsFloat || rhsFloat) ? ValueKind::FloatFamily
                                                                   : ValueKind::IntegerFamily;
                }
                case ast::BinaryOp::BitAnd:
                case ast::BinaryOp::BitOr:
                case ast::BinaryOp::BitXor:
                case ast::BinaryOp::ShiftLeft:
                case ast::BinaryOp::ShiftRight:
                    // Bitwise ops are integer-only regardless of any
                    // enclosing Float destination - unlike Div/Mod, a
                    // "real-valued shift/and/or/xor" has no natural meaning,
                    // so (unlike Div/Mod) floatContext is deliberately NOT
                    // propagated here. Not independently oracle-verified
                    // (every real PB program uses these on integers anyway);
                    // flagged as an assumption in docs/architecture/roadmap.md.
                    return ValueKind::IntegerFamily;
                case ast::BinaryOp::Add:
                case ast::BinaryOp::Sub:
                case ast::BinaryOp::Mul: {
                    ValueKind lhs = classify(*bin.lhs, floatContext);
                    ValueKind rhs = classify(*bin.rhs, floatContext);
                    if (lhs == ValueKind::StringFamily || rhs == ValueKind::StringFamily) {
                        return ValueKind::StringFamily; // `+` as string concatenation.
                    }
                    if (floatContext || lhs == ValueKind::FloatFamily || rhs == ValueKind::FloatFamily) {
                        return ValueKind::FloatFamily;
                    }
                    return ValueKind::IntegerFamily;
                }
                default:
                    // Eq/Ne/Lt/Gt/Le/Ge/LogicalAnd/LogicalOr/LogicalXOr only
                    // ever appear inside a condition tree, which Codegen
                    // lowers via genCondition (not genExpr/classify) - this
                    // is an unreachable-in-practice, safe fallback only.
                    return ValueKind::IntegerFamily;
            }
        }
    }
    return ValueKind::IntegerFamily;
}

TypeSuffix Sema::constTypeOf(const std::string& lowerName) const {
    auto it = constants_.find(lowerName);
    return it == constants_.end() ? TypeSuffix::Integer : it->second;
}

const Sema::ProcedureInfo* Sema::procedureInfo(const std::string& lowerName) const {
    auto it = procedures_.find(lowerName);
    return it == procedures_.end() ? nullptr : &it->second;
}

std::optional<std::size_t> Sema::dataLabelIndex(const std::string& lowerName) const {
    auto it = dataLabels_.find(lowerName);
    return it == dataLabels_.end() ? std::optional<std::size_t>{} : it->second;
}

const Sema::ArrayInfo* Sema::arrayInfo(const std::string& lowerName) const {
    auto it = arrays_.find(lowerName);
    return it == arrays_.end() ? nullptr : &it->second;
}

const Sema::ListInfo* Sema::listInfo(const std::string& lowerName) const {
    auto it = lists_.find(lowerName);
    return it == lists_.end() ? nullptr : &it->second;
}

const Sema::ListInfo* Sema::requireList(const std::string& lowerName, const std::string& spelling,
                                        SourceLoc loc) const {
    const ListInfo* info = listInfo(lowerName);
    if (info == nullptr) {
        diagnostics_.error(loc, "'" + spelling + "' is not a declared List");
    }
    return info;
}

bool Sema::isListBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "addelement",  "insertelement", "deleteelement", "clearlist",     "firstelement",
        "lastelement", "nextelement",   "previouselement", "listsize",   "selectelement", "listindex",
    };
    return names.contains(lowerName);
}

bool Sema::visitListBuiltinCall(ast::CallExpr& call) {
    if (!isListBuiltinName(call.name)) {
        return false;
    }
    if (call.args.empty()) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects a List argument");
        return true;
    }
    // Every one of these takes a bare `name()` as its first argument -
    // naming the List itself, not reading its current element - so it's
    // validated directly rather than visited as an ordinary expression (see
    // this method's own doc comment). Never independently reaches
    // visitExpr's own Call case (the chokepoint every *other* call name
    // resolves through), so it needs its own resolveModuleQualifiedName
    // call here (M7d's second slice).
    ast::Expr& listArg = *call.args.front();
    if (listArg.kind == ast::ExprKind::Call) {
        auto& listCall = static_cast<ast::CallExpr&>(listArg);
        if (listCall.args.empty()) {
            resolveModuleQualifiedName(
                listCall.name, [this](const std::string& n) { return lists_.contains(n); },
                [](const std::string&) { return false; }); // no builtin List name exists
            checkModuleAccess(listCall.name, call.loc);
            requireList(listCall.name, listCall.spelling, call.loc);
        } else {
            diagnostics_.error(call.loc, "'" + call.spelling + "' expects a bare 'name()' List argument");
        }
    } else {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects a bare 'name()' List argument");
    }
    if (call.name == "selectelement") {
        if (call.args.size() != 2) {
            diagnostics_.error(call.loc, "'SelectElement' expects a List and an index argument");
        } else {
            visitExpr(*call.args[1]);
        }
    } else if (call.args.size() != 1) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects exactly one List argument");
    }
    return true;
}

const Sema::MapInfo* Sema::mapInfo(const std::string& lowerName) const {
    auto it = maps_.find(lowerName);
    return it == maps_.end() ? nullptr : &it->second;
}

bool Sema::isMapBuiltinName(const std::string& lowerName) {
    static const std::unordered_set<std::string> names = {
        "addmapelement", "deletemapelement", "clearmap", "mapsize",
        "mapkey",        "resetmap",         "nextmapelement", "findmapelement",
    };
    return names.contains(lowerName);
}

bool Sema::visitMapBuiltinCall(ast::CallExpr& call) {
    if (!isMapBuiltinName(call.name)) {
        return false;
    }
    if (call.args.empty()) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects a Map argument");
        return true;
    }
    // As with visitListBuiltinCall, the first argument names the Map itself
    // (a bare `name()`) rather than being visited as an ordinary expression -
    // and likewise needs its own resolveModuleQualifiedName call (M7d's
    // second slice), never reaching visitExpr's own Call case.
    ast::Expr& mapArg = *call.args.front();
    if (mapArg.kind == ast::ExprKind::Call) {
        auto& mapCall = static_cast<ast::CallExpr&>(mapArg);
        if (mapCall.args.empty()) {
            resolveModuleQualifiedName(
                mapCall.name, [this](const std::string& n) { return maps_.contains(n); },
                [](const std::string&) { return false; }); // no builtin Map name exists
            checkModuleAccess(mapCall.name, call.loc);
            if (mapInfo(mapCall.name) == nullptr) {
                diagnostics_.error(call.loc, "'" + mapCall.spelling + "' is not a declared Map");
            }
        } else {
            diagnostics_.error(call.loc, "'" + call.spelling + "' expects a bare 'name()' Map argument");
        }
    } else {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects a bare 'name()' Map argument");
    }
    if (call.name == "addmapelement" || call.name == "findmapelement") {
        if (call.args.size() != 2) {
            diagnostics_.error(call.loc, "'" + call.spelling + "' expects a Map and a key argument");
        } else {
            visitExpr(*call.args[1]);
            checkMapKey(*call.args[1], call.loc);
        }
    } else if (call.name == "deletemapelement") {
        // Oracle-verified: DeleteMapElement accepts either 1 arg (deletes
        // the current cursor element) or 2 (deletes by key).
        if (call.args.size() == 2) {
            visitExpr(*call.args[1]);
            checkMapKey(*call.args[1], call.loc);
        } else if (call.args.size() != 1) {
            diagnostics_.error(call.loc, "'DeleteMapElement' expects a Map, and optionally a key, argument");
        }
    } else if (call.args.size() != 1) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects exactly one Map argument");
    }
    return true;
}

const Sema::StructureInfo* Sema::structureInfo(const std::string& lowerName) const {
    auto it = structures_.find(lowerName);
    return it == structures_.end() ? nullptr : &it->second;
}

const Sema::InterfaceInfo* Sema::interfaceInfo(const std::string& lowerName) const {
    auto it = interfaces_.find(lowerName);
    return it == interfaces_.end() ? nullptr : &it->second;
}

std::optional<std::size_t> Sema::interfaceMethodIndex(const InterfaceInfo& info, const std::string& methodLowerName) {
    for (std::size_t i = 0; i < info.methods.size(); ++i) {
        if (info.methods[i].name == methodLowerName) {
            return i;
        }
    }
    return std::nullopt;
}

/// `?Label` (M7c) is only supported for a DataSection label whose own run
/// of Data items (up to the next label or EndDataSection) is non-empty and
/// entirely `.i`-suffix - see DataLabelAddressExpr's own doc comment for
/// why (the oracle-verified real use case, an Interface's vtable, is always
/// shaped exactly this way; a label with no items, or any non-`.i` item, is
/// rejected with a real diagnostic rather than silently miscompiled).
bool Sema::dataLabelAddressable(const std::string& labelLowerName) const {
    auto it = dataLabelItemSuffixes_.find(labelLowerName);
    if (it == dataLabelItemSuffixes_.end() || it->second.empty()) {
        return false;
    }
    for (TypeSuffix suffix : it->second) {
        if (suffix != TypeSuffix::Integer) {
            return false;
        }
    }
    return true;
}

const Sema::InterfaceMethodInfo* Sema::resolveInterfaceMethod(const ast::MethodCallExpr& call) const {
    const char* notAnInterfacePtr = "' used on a value that isn't an Interface-typed pointer";
    if (call.base->kind != ast::ExprKind::VarRef) {
        diagnostics_.error(call.loc, "'\\" + call.methodSpelling + "(...)" + notAnInterfacePtr);
        return nullptr;
    }
    const auto& baseRef = static_cast<const ast::VarRefExpr&>(*call.base);
    if (baseRef.name.empty() || baseRef.name.front() != '*') {
        diagnostics_.error(call.loc, "'\\" + call.methodSpelling + "(...)" + notAnInterfacePtr);
        return nullptr;
    }
    ResolvedType pointee = pointeeTypeOf(baseRef.name);
    if (pointee.suffix != TypeSuffix::Interface) {
        diagnostics_.error(call.loc, "'\\" + call.methodSpelling + "' used on '" + baseRef.spelling +
                                          "', which is not an Interface-typed pointer");
        return nullptr;
    }
    const InterfaceInfo* info = interfaceInfo(pointee.structName);
    if (info == nullptr) {
        diagnostics_.error(call.loc, "'" + baseRef.spelling + "' points at an unknown Interface type");
        return nullptr;
    }
    auto idx = interfaceMethodIndex(*info, call.method);
    if (!idx) {
        diagnostics_.error(call.loc, "'" + call.methodSpelling + "' is not a method of this Interface");
        return nullptr;
    }
    return &info->methods[*idx];
}

/// M7d: a Module is a genuinely separate, "sealed box" namespace - oracle-
/// verified: main-code names (even `Global`s) are never visible inside a
/// module, and vice versa, so a bare reference resolves purely from the
/// current module's own namespace, an active `UseModule` import, or a real
/// PB builtin (always available - checked last here since a builtin is
/// registered under its own plain, unmangled key in the very same table
/// `exists` queries, e.g. `procedures_["len"]`). An already-`Module::Member`-
/// qualified `name` (containing "::", built that way directly by the Parser)
/// is left untouched - it's already the final key; `checkModuleAccess`
/// handles its own public/private validation separately.
void Sema::resolveModuleQualifiedName(std::string& name, const std::function<bool(const std::string&)>& exists,
                                       const std::function<bool(const std::string&)>& isBuiltin) const {
    if (name.find("::") != std::string::npos) {
        return;
    }
    // A name already declared in the *currently active* scope - top level,
    // or, if inside a Procedure, that procedure's own params/locals - is
    // unambiguously already resolved, module or no module: `order_` is
    // exactly that scope's own declaration list, and (critically) a
    // pre-populated Global is deliberately never added to it (see
    // bringIntoScope's own doc comment) - only a *genuine* local is. Skips
    // every module step below entirely; without this, a plain procedure-
    // local inside a module's own Procedure (e.g. `Define p.Point` inside
    // `Module Ferrari`'s `Procedure MakePoint()`) would wrongly be treated
    // as an unresolved bare name and silently re-declared as a brand new
    // "ferrari::p" instead of reusing the real local already in scope - a
    // real bug this exact check was added to fix, not a hypothetical.
    for (const auto& local : order_) {
        if (local.first == name) {
            return;
        }
    }
    if (!currentModule_.empty()) {
        std::string qualified = currentModule_ + "::" + name;
        if (exists(qualified)) {
            name = qualified;
            return;
        }
    }
    for (const auto& imported : activeImports_) {
        std::string qualified = imported + "::" + name;
        if (exists(qualified)) {
            name = qualified;
            return;
        }
    }
    if (currentModule_.empty()) {
        return; // Top level: whatever `name` already resolves to - unchanged, existing behavior.
    }
    // Oracle-verified "sealed box" model: main-code names, even `Global`s,
    // are never visible inside a module - only a real PB command is
    // (checked via `isBuiltin`, deliberately *not* `exists(name)`, which
    // would also match an invisible same-named top-level user declaration
    // sharing the very same flat `symbols_`/`procedures_` table).
    if (isBuiltin(name)) {
        return;
    }
    // Genuinely new/unknown while inside a module's own body - implicitly
    // declared within *that* module's own namespace, not the top level's.
    name = currentModule_ + "::" + name;
}

void Sema::checkModuleAccess(const std::string& qualifiedName, SourceLoc loc) const {
    auto sep = qualifiedName.find("::");
    if (sep == std::string::npos) {
        return;
    }
    std::string modName = qualifiedName.substr(0, sep);
    std::string memberName = qualifiedName.substr(sep + 2);
    if (modName == currentModule_) {
        return; // A module's own code can always see its own members, public or private.
    }
    auto it = modulePublicMembers_.find(modName);
    if (it == modulePublicMembers_.end() || !it->second.contains(memberName)) {
        diagnostics_.error(loc, "Module item '" + memberName + "' is not declared as public.");
    }
}

void Sema::resolveModuleQualifiedTypeName(std::string& typeName, SourceLoc loc) const {
    resolveModuleQualifiedName(
        typeName, [this](const std::string& n) { return structures_.contains(n) || interfaces_.contains(n); },
        [](const std::string&) { return false; }); // no such thing as a builtin Structure
    checkModuleAccess(typeName, loc);
}

Sema::ResolvedType Sema::resolveField(const ResolvedType& baseType, const std::string& fieldLowerName,
                                      const std::string& fieldSpelling, SourceLoc loc) const {
    if (baseType.suffix != TypeSuffix::Struct) {
        diagnostics_.error(loc, "'\\" + fieldSpelling + "' used on a value that isn't a Structure");
        return ResolvedType{};
    }
    const StructureInfo* info = structureInfo(baseType.structName);
    if (info == nullptr) {
        diagnostics_.error(loc, "'\\" + fieldSpelling + "' used on an unknown Structure type");
        return ResolvedType{};
    }
    for (const auto& field : info->fields) {
        if (field.name == fieldLowerName) {
            ResolvedType result;
            result.suffix = field.suffix;
            if (field.suffix == TypeSuffix::Struct) {
                result.structName = field.structTypeName;
            }
            return result;
        }
    }
    diagnostics_.error(loc, "'" + fieldSpelling + "' is not a field of this Structure");
    return ResolvedType{};
}

Sema::ResolvedType Sema::resolveType(const ast::Expr& expr) const {
    switch (expr.kind) {
        case ast::ExprKind::VarRef: {
            const auto& ref = static_cast<const ast::VarRefExpr&>(expr);
            TypeSuffix suffix = typeOf(ref.name);
            ResolvedType result;
            result.suffix = suffix;
            if (suffix == TypeSuffix::Struct) {
                auto it = varStructType_.find(ref.name);
                result.structName = it != varStructType_.end() ? it->second : "";
            }
            return result;
        }
        case ast::ExprKind::Call: {
            const auto& call = static_cast<const ast::CallExpr&>(expr);
            if (const ListInfo* lst = listInfo(call.name)) {
                ResolvedType result;
                result.suffix = lst->elementSuffix;
                if (lst->elementSuffix == TypeSuffix::Struct) {
                    result.structName = lst->elementStructName;
                }
                return result;
            }
            if (const MapInfo* map = mapInfo(call.name)) {
                ResolvedType result;
                result.suffix = map->elementSuffix;
                if (map->elementSuffix == TypeSuffix::Struct) {
                    result.structName = map->elementStructName;
                }
                return result;
            }
            if (const ArrayInfo* arr = arrayInfo(call.name)) {
                ResolvedType result;
                result.suffix = arr->elementSuffix;
                if (arr->elementSuffix == TypeSuffix::Struct) {
                    result.structName = arr->elementStructName;
                }
                return result;
            }
            if (const ProcedureInfo* proc = procedureInfo(call.name)) {
                return ResolvedType{proc->returnSuffix, ""};
            }
            return ResolvedType{};
        }
        case ast::ExprKind::FieldAccess: {
            const auto& access = static_cast<const ast::FieldAccessExpr&>(expr);
            // `*ptr\field` - the base is a bare pointer-value VarRef (its
            // name always starts with '*', see DefineStmt::Declarator's own
            // doc comment), whose *own* symbols_ entry is forced to plain
            // Integer (the address itself) - so resolving the dereference's
            // type has to go through pointerPointeeType_ instead of
            // recursing into resolveType(*access.base) normally, which
            // would just see "Integer" and reject the field access outright.
            if (access.base->kind == ast::ExprKind::VarRef) {
                const auto& baseRef = static_cast<const ast::VarRefExpr&>(*access.base);
                if (!baseRef.name.empty() && baseRef.name.front() == '*') {
                    ResolvedType pointee = pointeeTypeOf(baseRef.name);
                    if (pointee.suffix == TypeSuffix::Struct) {
                        return resolveField(pointee, access.field, access.fieldSpelling, access.loc);
                    }
                    // An untyped pointer (`Define *pa`, no Structure type)
                    // has no fields to dereference - real dereferencing for
                    // one of these goes through the Memory library's
                    // Peek*/Poke* functions instead (not yet implemented -
                    // M4), so `\field` on one is rejected outright here.
                    diagnostics_.error(access.loc, "'\\" + access.fieldSpelling + "' used on '" + baseRef.spelling +
                                                        "', which is not a Structure-typed pointer");
                    return ResolvedType{};
                }
            }
            ResolvedType baseType = resolveType(*access.base);
            return resolveField(baseType, access.field, access.fieldSpelling, access.loc);
        }
        default:
            return ResolvedType{};
    }
}

Sema::ResolvedType Sema::pointeeTypeOf(const std::string& pointerKey) const {
    auto it = pointerPointeeType_.find(pointerKey);
    return it == pointerPointeeType_.end() ? ResolvedType{} : it->second;
}

bool Sema::isPointerBuiltinName(const std::string& lowerName) {
    return lowerName == "allocatememory" || lowerName == "freememory" ||
           lowerName == "allocatestructure" || lowerName == "freestructure";
}

bool Sema::visitPointerBuiltinCall(ast::CallExpr& call) {
    if (!isPointerBuiltinName(call.name)) {
        return false;
    }
    if (call.name == "allocatestructure") {
        // Oracle-verified syntax: `AllocateStructure(Point)` - the sole
        // argument is a bare Structure type name, not a variable read, so
        // it's deliberately not visited as an expression (doing so would
        // implicitly declare a bogus variable named after the type).
        if (call.args.size() != 1 || call.args.front()->kind != ast::ExprKind::VarRef) {
            diagnostics_.error(call.loc, "'AllocateStructure' expects a single Structure type name");
            return true;
        }
        auto& typeRef = static_cast<ast::VarRefExpr&>(*call.args.front());
        resolveModuleQualifiedTypeName(typeRef.name, call.loc);
        if (!structures_.contains(typeRef.name)) {
            diagnostics_.error(call.loc, "'" + typeRef.spelling + "' is not a declared Structure");
        }
        return true;
    }
    // AllocateMemory(size)/FreeMemory(ptr)/FreeStructure(ptr) all take one
    // ordinary expression argument (a size or a pointer value) - visited
    // normally like any other call argument.
    if (call.args.size() != 1) {
        diagnostics_.error(call.loc, "'" + call.spelling + "' expects a single argument");
    }
    for (auto& arg : call.args) {
        visitExpr(*arg);
    }
    return true;
}

Sema::ResolvedType Sema::resolvePointeeType(TypeSuffix suffix, const std::string& structTypeName,
                                             const std::string& structTypeSpelling, SourceLoc loc) {
    ResolvedType pointee;
    if (suffix == TypeSuffix::Struct) {
        // A `.Name` suffix is lexically identical for a Structure or an
        // Interface (see TypeSuffix::Interface's own doc comment) - checked
        // against `structures_` first (the more common case), falling back
        // to `interfaces_` only when that fails, rather than erroring
        // immediately.
        if (structures_.contains(structTypeName)) {
            pointee.suffix = TypeSuffix::Struct;
            pointee.structName = structTypeName;
        } else if (interfaces_.contains(structTypeName)) {
            pointee.suffix = TypeSuffix::Interface;
            pointee.structName = structTypeName;
        } else {
            diagnostics_.error(loc, "'" + structTypeSpelling + "' is not a declared Structure or Interface");
            pointee.suffix = TypeSuffix::Struct;
            pointee.structName = structTypeName;
        }
    } else if (suffix != TypeSuffix::None) {
        // Oracle-verified (`pbcompilerc`, `Define *pa.i`): a pointer can
        // only be untyped or Structure-typed, never a primitive.
        diagnostics_.error(loc, "Native types can't be used with pointers.");
        pointee.suffix = TypeSuffix::None;
    } else {
        pointee.suffix = TypeSuffix::None; // Untyped pointer - see resolveType()'s own FieldAccess notes.
    }
    return pointee;
}

} // namespace easybasic
