/*
 * The Sciter Engine of Terra Informatica Software, Inc.
 * http://sciter.com
 *
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 *
 * (C) Terra Informatica Software, Inc.
 */

 /*
 *  Message definitions to be passed to SciterProcX function
 */


#ifndef __sciter_x_msg_h__
#define __sciter_x_msg_h__

#include "sciter-x-behavior.h"
#include "sciter-x-types.h"
#include "sciter-x-def.h"

/** #SCITER_X_MSG_CODE message/function identifier */
typedef enum SCITER_X_MSG_CODE {
  SXM_CREATE  = 0,
  SXM_DESTROY = 1,
  SXM_SIZE    = 2,
  SXM_PAINT   = 3,
  SXM_RESOLUTION = 4,
  SXM_HEARTBIT = 5,
  SXM_MOUSE = 6,
  SXM_KEY = 7,
  SXM_FOCUS = 8,
} SCITER_X_MSG_CODE;

/** #SCITER_X_MSG common header of message structures passed to SciterProcX */
typedef struct SCITER_X_MSG
{ 
  UINT msg;     /**< [in]  one of the codes of #SCITER_X_MSG_CODE.*/
#ifdef __cplusplus
  SCITER_X_MSG(UINT m) : msg(m) {}
#endif
} SCITER_X_MSG;

typedef enum SL_TARGET {
  SL_TARGET_BITMAP,
  SL_TARGET_DX9_TEXTURE,
  SL_TARGET_DX11_TEXTURE,
} SL_TARGET;

typedef struct SCITER_X_MSG_CREATE
{
  SCITER_X_MSG    header;
  SL_TARGET       backend;
  LPVOID device;   // e.g. ID3D11Device*, IDirect3DDevice9*, etc.
 
#ifdef __cplusplus
   SCITER_X_MSG_CREATE(SL_TARGET backendType, LPVOID pdevice = NULL )
     : header(SXM_CREATE), backend(backendType), device(pdevice) {
   }
#endif
} SCITER_X_MSG_CREATE;

typedef struct SCITER_X_MSG_DESTROY {
  SCITER_X_MSG header;
#ifdef __cplusplus
  SCITER_X_MSG_DESTROY() : header(SXM_DESTROY) {}
#endif
} SCITER_X_MSG_DESTROY;

typedef struct SL_SURFACE {
  union {
    LPVOID texture;
    struct {
      LPVOID pixels; // RGBA or BGRA
      UINT   stride; // row length in bytes
    } bitmap;
  };
} SL_SURFACE;

typedef struct SCITER_X_MSG_SIZE {
  SCITER_X_MSG        header;
          UINT        width;
          UINT        height;
          SL_SURFACE  surface;
#ifdef __cplusplus
  SCITER_X_MSG_SIZE(UINT w, UINT h, SL_SURFACE surf) : header(SXM_SIZE), width(w), height(h), surface(surf) {}
#endif
} SCITER_X_MSG_SIZE;

typedef struct SCITER_X_MSG_RESOLUTION {
  SCITER_X_MSG header;
  UINT pixelsPerInch;
#ifdef __cplusplus
  SCITER_X_MSG_RESOLUTION(UINT ppi) : header(SXM_RESOLUTION), pixelsPerInch(ppi) {}
#endif
} SCITER_X_MSG_RESOLUTION;

typedef struct SCITER_X_MSG_MOUSE {
  SCITER_X_MSG    header;
  MOUSE_EVENTS    event;
  MOUSE_BUTTONS   button;
  KEYBOARD_STATES modifiers;
  POINT           pos;
#ifdef __cplusplus
  SCITER_X_MSG_MOUSE(MOUSE_EVENTS e, MOUSE_BUTTONS b, KEYBOARD_STATES mods, POINT p) : header(SXM_MOUSE), event(e), button(b), modifiers(mods), pos(p) {}
#endif
} SCITER_X_MSG_MOUSE;

typedef struct SCITER_X_MSG_KEY {
  SCITER_X_MSG    header;
  KEY_EVENTS      event;
  UINT            code;
  KEYBOARD_STATES modifiers;
#ifdef __cplusplus
  SCITER_X_MSG_KEY(KEY_EVENTS e, UINT c, KEYBOARD_STATES mods) : header(SXM_KEY), event(e), code(c), modifiers(mods) {}
#endif
} SCITER_X_MSG_KEY;

typedef struct SCITER_X_MSG_FOCUS {
  SCITER_X_MSG    header;
  SBOOL            got; // true - got, false - lost
#ifdef __cplusplus
  SCITER_X_MSG_FOCUS(SBOOL g) : header(SXM_FOCUS), got(g) {}
#endif
} SCITER_X_MSG_FOCUS;

typedef struct SCITER_X_MSG_HEARTBIT {
  SCITER_X_MSG header;
  UINT time;
#ifdef __cplusplus
  SCITER_X_MSG_HEARTBIT(UINT t) : header(SXM_HEARTBIT), time(t) {}
#endif
} SCITER_X_MSG_HEARTBIT;

typedef struct SCITER_X_MSG_PAINT {
  SCITER_X_MSG header;
  HELEMENT element;    /**< [in] layer #HELEMENT, can be NULL if whole tree (document) needs to be rendered.*/
  SBOOL    isFore;     /**< [in] if element is not null tells if that element is fore-layer.*/    
  RECT     rcPaint;
#ifdef __cplusplus
  SCITER_X_MSG_PAINT(HELEMENT layerElement = NULL, SBOOL foreLayer = TRUE) : header(SXM_PAINT), element(layerElement), isFore(foreLayer) {
    rcPaint = { 0,0,0,0 };
  }
  SCITER_X_MSG_PAINT(const RECT& rc, HELEMENT layerElement = NULL, SBOOL foreLayer = TRUE) : header(SXM_PAINT), element(layerElement), isFore(foreLayer) {
    rcPaint = rc;
  }

#endif
} SCITER_X_MSG_PAINT;

#endif
