#pragma once

namespace clipengine {

/**
 * @brief Platform-independent key codes
 *
 * These key codes provide a common set of keys that work across different
 * window systems (GLFW, Win32, SDL, Qt, etc.). When using platform-specific
 * adapters, they convert their native key codes to these common codes.
 *
 * Note: You can also use platform-specific key codes directly if needed.
 */
namespace KeyCode {

    // Printable keys
    constexpr int Space         = 32;
    constexpr int Apostrophe    = 39;  // '
    constexpr int Comma         = 44;  // ,
    constexpr int Minus         = 45;  // -
    constexpr int Period        = 46;  // .
    constexpr int Slash         = 47;  // /

    constexpr int Key0          = 48;
    constexpr int Key1          = 49;
    constexpr int Key2          = 50;
    constexpr int Key3          = 51;
    constexpr int Key4          = 52;
    constexpr int Key5          = 53;
    constexpr int Key6          = 54;
    constexpr int Key7          = 55;
    constexpr int Key8          = 56;
    constexpr int Key9          = 57;

    constexpr int Semicolon     = 59;  // ;
    constexpr int Equal         = 61;  // =

    constexpr int A             = 65;
    constexpr int B             = 66;
    constexpr int C             = 67;
    constexpr int D             = 68;
    constexpr int E             = 69;
    constexpr int F             = 70;
    constexpr int G             = 71;
    constexpr int H             = 72;
    constexpr int I             = 73;
    constexpr int J             = 74;
    constexpr int K             = 75;
    constexpr int L             = 76;
    constexpr int M             = 77;
    constexpr int N             = 78;
    constexpr int O             = 79;
    constexpr int P             = 80;
    constexpr int Q             = 81;
    constexpr int R             = 82;
    constexpr int S             = 83;
    constexpr int T             = 84;
    constexpr int U             = 85;
    constexpr int V             = 86;
    constexpr int W             = 87;
    constexpr int X             = 88;
    constexpr int Y             = 89;
    constexpr int Z             = 90;

    constexpr int LeftBracket   = 91;  // [
    constexpr int Backslash     = 92;  // \
    constexpr int RightBracket  = 93;  // ]
    constexpr int GraveAccent   = 96;  // `

    // Function keys
    constexpr int Escape        = 256;
    constexpr int Enter         = 257;
    constexpr int Tab           = 258;
    constexpr int Backspace     = 259;
    constexpr int Insert        = 260;
    constexpr int Delete        = 261;
    constexpr int Right         = 262;
    constexpr int Left          = 263;
    constexpr int Down          = 264;
    constexpr int Up            = 265;
    constexpr int PageUp        = 266;
    constexpr int PageDown      = 267;
    constexpr int Home          = 268;
    constexpr int End           = 269;
    constexpr int CapsLock      = 280;
    constexpr int ScrollLock    = 281;
    constexpr int NumLock       = 282;
    constexpr int PrintScreen   = 283;
    constexpr int Pause         = 284;

    constexpr int F1            = 290;
    constexpr int F2            = 291;
    constexpr int F3            = 292;
    constexpr int F4            = 293;
    constexpr int F5            = 294;
    constexpr int F6            = 295;
    constexpr int F7            = 296;
    constexpr int F8            = 297;
    constexpr int F9            = 298;
    constexpr int F10           = 299;
    constexpr int F11           = 300;
    constexpr int F12           = 301;
    constexpr int F13           = 302;
    constexpr int F14           = 303;
    constexpr int F15           = 304;
    constexpr int F16           = 305;
    constexpr int F17           = 306;
    constexpr int F18           = 307;
    constexpr int F19           = 308;
    constexpr int F20           = 309;
    constexpr int F21           = 310;
    constexpr int F22           = 311;
    constexpr int F23           = 312;
    constexpr int F24           = 313;
    constexpr int F25           = 314;

    // Keypad
    constexpr int KP0           = 320;
    constexpr int KP1           = 321;
    constexpr int KP2           = 322;
    constexpr int KP3           = 323;
    constexpr int KP4           = 324;
    constexpr int KP5           = 325;
    constexpr int KP6           = 326;
    constexpr int KP7           = 327;
    constexpr int KP8           = 328;
    constexpr int KP9           = 329;
    constexpr int KPDecimal     = 330;
    constexpr int KPDivide      = 331;
    constexpr int KPMultiply    = 332;
    constexpr int KPSubtract    = 333;
    constexpr int KPAdd         = 334;
    constexpr int KPEnter       = 335;
    constexpr int KPEqual       = 336;

    // Modifiers
    constexpr int LeftShift     = 340;
    constexpr int LeftControl   = 341;
    constexpr int LeftAlt       = 342;
    constexpr int LeftSuper     = 343;  // Windows/Command key
    constexpr int RightShift    = 344;
    constexpr int RightControl  = 345;
    constexpr int RightAlt      = 346;
    constexpr int RightSuper    = 347;
    constexpr int Menu          = 348;

    /**
     * @brief Helper function to check if a key code is printable
     */
    inline bool isPrintable(int keyCode) {
        return (keyCode >= 32 && keyCode <= 96);
    }

    /**
     * @brief Helper function to check if a key code is a letter
     */
    inline bool isLetter(int keyCode) {
        return (keyCode >= A && keyCode <= Z);
    }

    /**
     * @brief Helper function to check if a key code is a digit
     */
    inline bool isDigit(int keyCode) {
        return (keyCode >= Key0 && keyCode <= Key9);
    }

    /**
     * @brief Helper function to check if a key code is a function key
     */
    inline bool isFunctionKey(int keyCode) {
        return (keyCode >= F1 && keyCode <= F25);
    }

} // namespace KeyCode

} // namespace clipengine
