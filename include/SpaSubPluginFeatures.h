/*
 * SpaSubPluginFeatures.h - derivation from
 *                          Plugin::Descriptor::SubPluginFeatures for
 *                          hosting SPA plugins
 *
 * Copyright (c) 2018-2019 Johannes Lorenz <j.git$$$lorenz-ho.me, $$$=@>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef SPA_SUBPLUGIN_FEATURES_H
#define SPA_SUBPLUGIN_FEATURES_H

#include "lmmsconfig.h"

#ifdef LMMS_HAVE_SPA

#include <spa/spa_fwd.h>

#include "Plugin.h"

class SpaSubPluginFeatures : public Plugin::Descriptor::SubPluginFeatures
{
private:
	static spa::descriptor *spaDescriptor(const Key &k);

public:
	SpaSubPluginFeatures(Plugin::PluginTypes _type);

	virtual void fillDescriptionWidget(
		QWidget *_parent, const Key *k) const override;

	QString additionalFileExtensions(const Key &k) const override;
	QString displayName(const Key &k) const override;
	QString description(const Key &k) const override;
	const PixmapLoader *logo(const Key &k) const override;

	void listSubPluginKeys(
		const Plugin::Descriptor *_desc, KeyList &_kl) const override;
};

#endif // LMMS_HAVE_SPA

#endif
