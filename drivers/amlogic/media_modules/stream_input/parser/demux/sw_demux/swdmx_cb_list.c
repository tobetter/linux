/***************************************************************************
 * Copyright (C) 2018 Amlogic, Inc. All rights reserved.                   *
 ***************************************************************************/

#include "swdemux_internal.h"

void
swdmx_cb_list_add (SWDMX_List *l, SWDMX_Ptr cb, SWDMX_Ptr data)
{
	SWDMX_CbEntry *ent;

	SWDMX_ASSERT(l && cb);

	ent = swdmx_malloc(sizeof(SWDMX_CbEntry));
	SWDMX_ASSERT(ent);

	ent->cb   = cb;
	ent->data = data;

	swdmx_list_append(l, &ent->ln);
}

void
swdmx_cb_list_remove (SWDMX_List *l, SWDMX_Ptr cb, SWDMX_Ptr data)
{
	SWDMX_CbEntry *ent, *nent;

	SWDMX_LIST_FOR_EACH_SAFE(ent, nent, l, ln) {
		if ((ent->cb == cb) && (ent->data == data)) {
			swdmx_list_remove(&ent->ln);
			swdmx_free(ent);
			break;
		}
	}
}

void
swdmx_cb_list_clear (SWDMX_List *l)
{
	SWDMX_CbEntry *ent, *nent;

	SWDMX_LIST_FOR_EACH_SAFE(ent, nent, l, ln) {
		swdmx_list_remove(&ent->ln);
		swdmx_free(ent);
	}

	swdmx_list_init(l);
}

