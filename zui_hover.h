#ifndef ZUI_HOVER_H
#define ZUI_HOVER_H

#include "nuklear.h"

typedef struct
{
    struct nk_rect volume;
    struct nk_rect bright;
    struct nk_rect contrast;
    struct nk_rect gamma;
    struct nk_rect ping;
    struct nk_rect reboot;
    struct nk_rect exit;
    struct nk_rect power;
} ZuiHoverRects;

extern ZuiHoverRects g_hover;

#endif // ZUI_HOVER_H