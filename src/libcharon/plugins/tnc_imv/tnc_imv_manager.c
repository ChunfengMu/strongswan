/*
 * Copyright (C) 2006 Mike McCauley
 * Copyright (C) 2010 Andreas Steffen, HSR Hochschule fuer Technik Rapperswil
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include "tnc_imv_manager.h"

#include <tnc/imv/imv_manager.h>
#include <tnc/tncifimv.h>

#include <debug.h>
#include <library.h>
#include <utils/linked_list.h>

typedef struct private_tnc_imv_manager_t private_tnc_imv_manager_t;

struct private_tnc_imv_manager_t {

	/**
	 * Public members of imv_manager_t.
	 */
	imv_manager_t public;

	/**
	 * Linked list of IMVs
	 */
	linked_list_t *imvs;

	/**
	 * Next IMV ID to be assigned
	 */
	TNC_IMVID next_imv_id;
};

METHOD(imv_manager_t, add, bool,
	private_tnc_imv_manager_t *this, imv_t *imv)
{
	TNC_Version version;

	/* Initialize the IMV module */
	imv->set_id(imv, this->next_imv_id);
	if (imv->initialize(imv->get_id(imv), TNC_IFIMV_VERSION_1,
		TNC_IFIMV_VERSION_1, &version) != TNC_RESULT_SUCCESS)
	{
		DBG1(DBG_TNC, "could not initialize IMV '%s'",
					   imv->get_name(imv));
		return FALSE;
	}
    if (imv->provide_bind_function(imv->get_id(imv), TNC_TNCS_BindFunction)
			!= TNC_RESULT_SUCCESS)
	{
		DBG1(DBG_TNC, "could not provide bind function for IMV '%s'",
					   imv->get_name(imv));
		return FALSE;
	}
	this->imvs->insert_last(this->imvs, imv);
	this->next_imv_id++;
	return TRUE;
}

METHOD(imv_manager_t, notify_connection_change, void,
	private_tnc_imv_manager_t *this, TNC_ConnectionID id,
	TNC_ConnectionState state)
{
	enumerator_t *enumerator;
	imv_t *imv;

	enumerator = this->imvs->create_enumerator(this->imvs);
	while (enumerator->enumerate(enumerator, &imv))
	{
		if (imv->notify_connection_change)
		{
			imv->notify_connection_change(imv->get_id(imv), id, state);
		}
	}
	enumerator->destroy(enumerator);
}

METHOD(imv_manager_t, destroy, void,
	private_tnc_imv_manager_t *this)
{
	imv_t *imv;

	while (this->imvs->remove_last(this->imvs, (void**)&imv) == SUCCESS)
	{
		if (imv->terminate &&
			imv->terminate(imv->get_id(imv)) != TNC_RESULT_SUCCESS)
		{
			DBG1(DBG_TNC, "IMV '%s' not terminated successfully",
						   imv->get_name(imv));
		}
		imv->destroy(imv);
	}
	this->imvs->destroy(this->imvs);
	free(this);
}

/**
 * Described in header.
 */
imv_manager_t* tnc_imv_manager_create(void)
{
	private_tnc_imv_manager_t *this;

	INIT(this,
		.public = {
			.add = _add,
			.notify_connection_change = _notify_connection_change,
			.destroy = _destroy,
        },
		.imvs = linked_list_create(),
		.next_imv_id = 1,
	);


	return &this->public;
}


