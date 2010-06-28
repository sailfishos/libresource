/******************************************************************************/
/*  This file is part of libresource                                          */
/*                                                                            */
/*  Copyright (C) 2010 Nokia Corporation.                                     */
/*                                                                            */
/*  This library is free software; you can redistribute                       */
/*  it and/or modify it under the terms of the GNU Lesser General Public      */
/*  License as published by the Free Software Foundation                      */
/*  version 2.1 of the License.                                               */
/*                                                                            */
/*  This library is distributed in the hope that it will be useful,           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU          */
/*  Lesser General Public License for more details.                           */
/*                                                                            */
/*  You should have received a copy of the GNU Lesser General Public          */
/*  License along with this library; if not, write to the Free Software       */
/*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  */
/*  USA.                                                                      */
/******************************************************************************/

#ifndef __RES_SET_H__
#define __RES_SET_H__

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif



union resconn_u;

typedef enum {
    RESPROTO_RSET_STATE_CREATED = 0,
    RESPROTO_RSET_STATE_CONNECTED,
    RESPROTO_RSET_STATE_KILLED,
} resset_state_t;

typedef struct resset_s {
    struct resset_s  *next;
    int32_t           refcnt;
    union resconn_u  *resconn;
    char             *peer;
    uint32_t          id;
    resset_state_t    state;
    char             *klass;
    uint32_t          mode;
    struct {
        uint32_t all;
        uint32_t opt;
        uint32_t share;
        uint32_t mask;
    }                 flags;
    void             *userdata;
} resset_t;


#ifdef	__cplusplus
};
#endif

#endif /* __RES_SET_H__ */

/* 
 * Local Variables:
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 * vim:set expandtab shiftwidth=4:
 */
