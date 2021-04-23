/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2020 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Piotr Wójcik <chocimier@tlen.pl>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
**************************************************************************/

#ifndef OTTER_FEEDSCOMBOBOXWIDGET_H
#define OTTER_FEEDSCOMBOBOXWIDGET_H

#include "ComboBoxWidget.h"
#include "../core/FeedsModel.h"

namespace Otter
{

class FeedsComboBoxWidget final : public ComboBoxWidget
{
	Q_OBJECT

public:
	explicit FeedsComboBoxWidget(QWidget *parent = nullptr);

	void setCurrentFolder(FeedsModel::Entry *folder);
	FeedsModel::Entry* getCurrentFolder() const;

public slots:
	void createFolder();

protected slots:
	void updateBranch(const QModelIndex &parent = {});
};

}

#endif
