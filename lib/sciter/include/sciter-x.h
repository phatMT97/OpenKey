/*
 * The Sciter Engine of Terra Informatica Software, Inc.
 * http://sciter.com
 *
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 *
 *
 * (C) 2003-2015, Terra Informatica Software, Inc.
 */

/*
 * Sciter main include file
 */


#ifndef __SCITER_X__
#define __SCITER_X__


#include "sciter-x-types.h"
#include "sciter-x-def.h"
#include "sciter-x-dom.h"
#include "sciter-x-value.h"
#include "sciter-x-api.h"
#include "sciter-x-msg.h"
#include "sciter-om.h"

#if defined(__cplusplus) && !defined( PLAIN_API_ONLY )
  // C++, namespace sciter things
  #include "sciter-x-dom.hpp"
  #include "sciter-x-host-callback.h"
  #include "sciter-x-debug.h"
#endif

/** Signatire of Sciter extension library entry point
  * \param[in] psapi #ISciterAPI - Sciter API to be used inside the DLL.
  * \param[out] plibobject #SCITER_VALUE* - value to initialize, can be native functor, sciter::om::asset, array, map, etc.
  * \return TRUE, if it populates plibobject;
  *
  * The DLL should have function exported with the name "SciterLibraryInit" and wit this signature.
  * In script such sciter extension library can be imported as:
  *
  *  include library "name-without-extension";
  *
 **/

typedef SBOOL SCAPI SciterLibraryInitFunc(ISciterAPI* psapi, SCITER_VALUE* plibobject);

/** Signatire of Sciter behavior factory entry point
  * \param[in] psapi #ISciterAPI - Sciter API to be used inside the DLL.
  * \param[out] pfactory #SciterBehaviorFactory** - reference to behavior factory implementation.
  * \return TRUE, if it populates plibobject;
  *
  * The DLL should have function exported with the name "SciterBehaviorFactoryInit" and with this signature.
  * In CSS the behavior from this library can be attached as:
  *
  *  widget.my { behavior: behavior-name library(dll-name-without-extension); }
  *
 **/

typedef SBOOL SCAPI SciterBehaviorFactoryInitFunc(ISciterAPI* psapi, SciterBehaviorFactory** pfactory);




#endif



