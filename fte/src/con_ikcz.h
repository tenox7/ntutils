#ifndef  CON_IKCS_H
#define  CON_IKCS_H

#include "con_i18n.h"
#include <X11/keysym.h>

struct remapKey {
    KeySym key_english;
    KeySym key_remap;
};

struct keyboardRec {
    const struct remapKey *tab;
    KeySym deadkey;
    short next;
};

static const struct remapKey keyboardStd[] =
{
    {0,}
};

static const struct remapKey keyboardHalfCz[] =
{
    {XK_2, 0xff & 'ì'},
    {XK_3, 0xff & '¹'},
    {XK_4, 0xff & 'è'},
    {XK_5, 0xff & 'ø'},
    {XK_6, 0xff & '¾'},
    {XK_7, 0xff & 'ý'},
    {XK_8, 0xff & 'á'},
    {XK_9, 0xff & 'í'},
    {XK_0, 0xff & 'é'},
    {0,}
};

static const struct remapKey keyboardFullCz[] =
{
    {XK_1, 0xff & '+'},
    {XK_exclam, XK_1},
    {XK_2, 0xff & 'ì'},
    {XK_at, XK_2},
    {XK_3, 0xff & '¹'},
    {XK_numbersign, XK_3},
    {XK_4, 0xff & 'è'},
    {XK_dollar, XK_4},
    {XK_5, 0xff & 'ø'},
    {XK_percent, XK_5},
    {XK_6, 0xff & '¾'},
    {XK_asciicircum, XK_6},
    {XK_7, 0xff & 'ý'},
    {XK_ampersand, XK_7},
    {XK_8, 0xff & 'á'},
    {XK_asterisk, XK_8},
    {XK_9, 0xff & 'í'},
    {XK_parenleft, XK_9},
    {XK_0, 0xff & 'é'},
    {XK_parenright, XK_0},

    {XK_minus, XK_equal},
    {XK_underscore, XK_percent},

    {XK_bracketleft, 0xff & 'ú'},
    {XK_braceleft, 0xff & '/'},
    {XK_bracketright, 0xff & ')'},
    {XK_braceright, 0xff & '('},

    {XK_semicolon, 0xff & 'ù'},
    {XK_apostrophe, 0xff & '§'},
    {XK_colon, 0xff & '"'},
    {XK_quotedbl, 0xff & '!'},

    {XK_comma, 0xff & ','},
    {XK_less, 0xff & '?'},
    {XK_period, 0xff & '.'},
    {XK_greater, 0xff & ':'},
    {XK_question, 0xff & '_'},
    {XK_slash, 0xff & '-'},

    {0,}
};

static const struct remapKey keyboardAcute[] =
{
    {XK_w, 0xff & 'ì'},
    {XK_W, 0xff & 'Ì'},
    {XK_e, 0xff & 'é'},
    {XK_E, 0xff & 'É'},
    {XK_r, 0xff & 'à'},
    {XK_R, 0xff & 'À'},
    {XK_t, 0xff & '»'},
    {XK_T, 0xff & '«'},
    {XK_y, 0xff & 'ý'},
    {XK_Y, 0xff & 'Ý'},
    {XK_u, 0xff & 'ú'},
    {XK_U, 0xff & 'Ú'},
    {XK_i, 0xff & 'í'},
    {XK_I, 0xff & 'Í'},
    {XK_o, 0xff & 'ó'},
    {XK_O, 0xff & 'Ó'},
    {XK_p, 0xff & '§'},
    {XK_P, 0xff & '§'},

    {XK_a, 0xff & 'á'},
    {XK_A, 0xff & 'Á'},
    {XK_s, 0xff & '¶'},
    {XK_S, 0xff & '¦'},
    {XK_d, 0xff & 'ï'},
    {XK_D, 0xff & 'Ï'},
    {XK_h, 0xff & 'þ'},
    {XK_H, 0xff & 'Þ'},
    {XK_l, 0xff & 'å'},
    {XK_L, 0xff & 'Å'},

    {XK_z, 0xff & '¼'},
    {XK_Z, 0xff & '¬'},
    {XK_x, 0xff & '×'},
    {XK_X, 0xff & '×'},
    {XK_c, 0xff & 'æ'},
    {XK_C, 0xff & 'Æ'},
    {XK_b, 0xff & 'ß'},
    {XK_B, 0xff & 'ß'},
    {XK_n, 0xff & 'ñ'},
    {XK_N, 0xff & 'Ñ'},

    {XK_equal, 0xff & '´'},

    {0,}
};

static struct remapKey keyboardCaron[] =
{
    {XK_e, 0xff & 'ì'},
    {XK_E, 0xff & 'Ì'},
    {XK_r, 0xff & 'ø'},
    {XK_R, 0xff & 'Ø'},
    {XK_t, 0xff & '»'},
    {XK_T, 0xff & '«'},
    {XK_u, 0xff & 'ù'},
    {XK_U, 0xff & 'Ù'},
    {XK_i, 0xff & 'î'},
    {XK_I, 0xff & 'Î'},
    {XK_o, 0xff & 'ö'},
    {XK_O, 0xff & 'Ö'},

    {XK_a, 0xff & 'ä'},
    {XK_A, 0xff & 'Ä'},
    {XK_s, 0xff & '¹'},
    {XK_S, 0xff & '©'},
    {XK_d, 0xff & 'ï'},
    {XK_D, 0xff & 'Ï'},
    {XK_l, 0xff & 'µ'},
    {XK_L, 0xff & '¥'},

    {XK_z, 0xff & '¾'},
    {XK_Z, 0xff & '®'},
    {XK_x, 0xff & '¤'},
    {XK_X, 0xff & '¤'},
    {XK_c, 0xff & 'è'},
    {XK_C, 0xff & 'È'},
    {XK_n, 0xff & 'ò'},
    {XK_N, 0xff & 'Ò'},

    {XK_plus, 0xff & '·'},
    {0,}
};

static struct remapKey keyboardFirst[] =
{
    {XK_w, 0xff & 'ì'},
    {XK_W, 0xff & 'Ì'},
    {XK_e, 0xff & 'é'},
    {XK_E, 0xff & 'É'},
    {XK_r, 0xff & 'ø'},
    {XK_R, 0xff & 'Ø'},
    {XK_t, 0xff & '»'},
    {XK_T, 0xff & '«'},
    {XK_y, 0xff & 'ý'},
    {XK_Y, 0xff & 'Ý'},
    {XK_u, 0xff & 'ú'},
    {XK_U, 0xff & 'Ú'},
    {XK_i, 0xff & 'í'},
    {XK_I, 0xff & 'Í'},
    {XK_o, 0xff & 'ó'},
    {XK_O, 0xff & 'Ó'},
    {XK_p, 0xff & '§'},
    {XK_P, 0xff & '§'},

    {XK_a, 0xff & 'á'},
    {XK_A, 0xff & 'Á'},
    {XK_s, 0xff & '¹'},
    {XK_S, 0xff & '©'},
    {XK_d, 0xff & 'ï'},
    {XK_D, 0xff & 'Ï'},
    {XK_h, 0xff & 'þ'},
    {XK_H, 0xff & 'Þ'},
    {XK_l, 0xff & 'å'},
    {XK_L, 0xff & 'Å'},

    {XK_z, 0xff & '¾'},
    {XK_Z, 0xff & '®'},
    {XK_x, 0xff & '×'},
    {XK_X, 0xff & '×'},
    {XK_c, 0xff & 'è'},
    {XK_C, 0xff & 'È'},
    {XK_b, 0xff & 'ß'},
    {XK_B, 0xff & 'ß'},
    {XK_n, 0xff & 'ò'},
    {XK_N, 0xff & 'Ò'},

    {0,}
};

static struct remapKey keyboardSecond[] =
{
    {XK_equal, 0xff & '´'},
    {XK_plus, 0xff & '·'},

    {XK_e, 0xff & 'ì'},
    {XK_E, 0xff & 'Ì'},
    {XK_r, 0xff & 'à'},
    {XK_R, 0xff & 'À'},
    {XK_t, 0xff & 'þ'},
    {XK_T, 0xff & 'Þ'},
    {XK_u, 0xff & 'ù'},
    {XK_U, 0xff & 'Ù'},
    {XK_i, 0xff & 'î'},
    {XK_I, 0xff & 'Î'},
    {XK_o, 0xff & 'ö'},
    {XK_O, 0xff & 'Ö'},

    {XK_a, 0xff & 'ä'},
    {XK_A, 0xff & 'Ä'},
    {XK_s, 0xff & '¶'},
    {XK_S, 0xff & '¦'},
    {XK_d, 0xff & 'ð'},
    {XK_D, 0xff & 'Ð'},
    {XK_l, 0xff & 'µ'},
    {XK_L, 0xff & '¥'},

    {XK_z, 0xff & '¼'},
    {XK_Z, 0xff & '¬'},
    {XK_x, 0xff & '¤'},
    {XK_X, 0xff & '¤'},
    {XK_c, 0xff & 'æ'},
    {XK_C, 0xff & 'Æ'},
    {XK_n, 0xff & 'ñ'},
    {XK_N, 0xff & 'Ñ'},

    {0,}
};



static struct remapKey keyboardThird[] =
{
    {XK_a, 0xff & 'â'},
    {XK_A, 0xff & 'Â'},
    {XK_e, 0xff & 'ë'},
    {XK_E, 0xff & 'Ë'},
    {XK_u, 0xff & 'ü'},
    {XK_U, 0xff & 'Ü'},
    {XK_o, 0xff & 'ô'},
    {XK_O, 0xff & 'Ô'},

    {XK_s, 0xff & 'ß'},
    {XK_S, 0xff & 'ß'},
    {XK_l, 0xff & '³'},
    {XK_L, 0xff & '£'},

    {XK_z, 0xff & '¿'},
    {XK_Z, 0xff & '¯'},
    {XK_c, 0xff & 'ç'},
    {XK_C, 0xff & 'Ç'},

    {0,}
};

static struct remapKey keyboardFourth[] =
{
    {XK_a, 0xff & 'ã'},
    {XK_A, 0xff & 'Ã'},
    {XK_e, 0xff & 'ê'},
    {XK_E, 0xff & 'Ê'},
    {XK_u, 0xff & 'û'},
    {XK_U, 0xff & 'Ü'},
    {XK_o, 0xff & 'õ'},
    {XK_O, 0xff & 'Õ'},

    {XK_l, 0xff & '£'},
    {XK_L, 0xff & '£'},

    {0,}
};

static struct remapKey keyboardFifth[] =
{
    {XK_a, 0xff & '±'},
    {XK_A, 0xff & '¡'},
    {XK_o, 0xff & '§'},
    {XK_O, 0xff & '§'},

    {XK_l, 0xff & '|'},
    {XK_L, 0xff & '|'},

    {0,}
};

#define KEYMAPS_MACRO \
    {keyboardAcute, 0, 0},      /*  1 */ \
    {keyboardCaron, 0, 0},      /*  2 */ \
    {keyboardFirst, 0, 0},      /*  3 */ \
    {keyboardSecond, 0, 0},     /*  4 */ \
    {keyboardThird, 0, 0},      /*  5 */ \
    {keyboardFourth, 0, 0},     /*  6 */ \
    {keyboardFifth, 0, 0},      /*  7 */

#ifdef XK_dead_acute
#define XKB_DEAD_KEYS \
    {keyboardStd, XK_dead_acute, 1}, \
    {keyboardStd, XK_dead_caron, 2}, \
    {keyboardStd, XK_dead_iota, 3}, \
    {keyboardStd, XK_dead_iota, 4}, \
    {keyboardStd, XK_dead_iota, 5}, \
    {keyboardStd, XK_dead_iota, 6}, \
    {keyboardStd, XK_dead_iota, 7}, \
    {keyboardStd, XK_dead_iota, 0},

#else
#define XKB_DEAD_KEYS
#endif

#ifdef XK_F22
#define F22_DEAD_KEYS \
    {keyboardStd, XK_F22, 3}, \
    {keyboardStd, XK_F22, 4}, \
    {keyboardStd, XK_F22, 5}, \
    {keyboardStd, XK_F22, 6}, \
    {keyboardStd, XK_F22, 7}, \
    {keyboardStd, XK_F22, 0},
#else
#define F22_DEAD_KEYS
#endif

#define KBD_MACRO \
    {keyboardStd, XK_Print, 3}, \
    {keyboardStd, XK_Print, 4}, \
    {keyboardStd, XK_Print, 5}, \
    {keyboardStd, XK_Print, 6}, \
    {keyboardStd, XK_Print, 7}, \
    {keyboardStd, XK_Print, 0}, \
    XKB_DEAD_KEYS \
    F22_DEAD_KEYS


static const struct keyboardRec kbdStdRec[] =
{
    { keyboardStd, 0, 0 },        /*  0 */

    KEYMAPS_MACRO

    KBD_MACRO

    { NULL }
};

static const struct keyboardRec kbdHalfCzRec[] =
{
    { keyboardHalfCz, 0, 0 },     /*  0 */

    KEYMAPS_MACRO

    KBD_MACRO

    { NULL }
};

static const struct keyboardRec kbdFullCzRec[] =
{
    { keyboardFullCz, 0, 0 },     /*  0 */

    KEYMAPS_MACRO

    { keyboardStd, XK_equal, 1 },
    { keyboardStd, XK_plus, 2 },

    KBD_MACRO

    { NULL }
};

/*
 * one standart keyboard and two national keyboards
 * (for programmers and for writers)
 */
static const struct keyboardRec* nationalKey[] =
{
    kbdStdRec,
    kbdHalfCzRec,
    kbdFullCzRec,
    NULL
};

#endif // CON_IKCS_H
