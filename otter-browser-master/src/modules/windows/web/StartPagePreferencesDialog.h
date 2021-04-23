/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2020 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2016 Piotr Wójcik <chocimier@tlen.pl>
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

#ifndef OTTER_STARTPAGEPREFERENCESDIALOG_H
#define OTTER_STARTPAGEPREFERENCESDIALOG_H

#include "../../../ui/Dialog.h"

namespace Otter
{

namespace Ui
{
	class StartPagePreferencesDialog;
}

class StartPagePreferencesDialog final : public Dialog
{
	Q_OBJECT

public:
	explicit StartPagePreferencesDialog(QWidget *parent = nullptr);
	~StartPagePreferencesDialog();

protected:
	void changeEvent(QEvent *event) override;

protected slots:
	void save();

private:
	Ui::StartPagePreferencesDialog *m_ui;
};

}

#endif
