/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2018 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "SelectPasswordDialog.h"

#include "ui_SelectPasswordDialog.h"

#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QMessageBox>

namespace Otter
{

SelectPasswordDialog::SelectPasswordDialog(const QVector<PasswordsManager::PasswordInformation> &passwords, QWidget *parent) : Dialog(parent),
	m_passwords(passwords),
	m_ui(new Ui::SelectPasswordDialog)
{
	m_ui->setupUi(this);

	QStandardItemModel *model(new QStandardItemModel(this));
	model->setHorizontalHeaderLabels({tr("Name"), tr("Value")});

	for (int i = 0; i < passwords.count(); ++i)
	{
		QStandardItem *setItem(new QStandardItem(tr("Set #%1").arg(i + 1)));
		setItem->setData(i, Qt::UserRole);

		for (int j = 0; j < passwords.at(i).fields.count(); ++j)
		{
			QList<QStandardItem*> fieldItems({new QStandardItem(passwords.at(i).fields.at(j).name), new QStandardItem((passwords.at(i).fields.at(j).type == PasswordsManager::PasswordField) ? QLatin1String("*****") : passwords.at(i).fields.at(j).value)});
			fieldItems[0]->setFlags(fieldItems[0]->flags() | Qt::ItemNeverHasChildren);
			fieldItems[1]->setFlags(fieldItems[1]->flags() | Qt::ItemNeverHasChildren);

			setItem->appendRow(fieldItems);
		}

		model->appendRow(setItem);
	}

	m_ui->passwordsViewWidget->setModel(model);
	m_ui->passwordsViewWidget->setViewMode(ItemViewWidget::TreeView);
	m_ui->passwordsViewWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_ui->passwordsViewWidget->selectionModel()->select(m_ui->passwordsViewWidget->getIndex(0, 0), (QItemSelectionModel::Select | QItemSelectionModel::Rows));
	m_ui->passwordsViewWidget->expandAll();

	connect(m_ui->removeButton, &QPushButton::clicked, this, &SelectPasswordDialog::removePassword);
	connect(m_ui->passwordsViewWidget, &ItemViewWidget::needsActionsUpdate, this, &SelectPasswordDialog::updateActions);
}

SelectPasswordDialog::~SelectPasswordDialog()
{
	delete m_ui;
}

void SelectPasswordDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
		m_ui->passwordsViewWidget->getSourceModel()->setHorizontalHeaderLabels({tr("Name"), tr("Value")});
	}
}

void SelectPasswordDialog::removePassword()
{
	const int currentSet(getCurrentSet());

	if (currentSet < 0 || currentSet >= m_passwords.count() || QMessageBox::question(this, tr("Question"), tr("Do you really want to remove this credentials set?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
	{
		return;
	}

	PasswordsManager::removePassword(m_passwords.value(currentSet));

	m_passwords.replace(currentSet, {});

	for (int i = 0; i < m_ui->passwordsViewWidget->getRowCount(); ++i)
	{
		const QModelIndex index(m_ui->passwordsViewWidget->getIndex(i, 0));

		if (index.data(Qt::UserRole).isValid() && index.data(Qt::UserRole).toInt() == currentSet)
		{
			m_ui->passwordsViewWidget->getSourceModel()->removeRow(i);

			break;
		}
	}

	if (m_ui->passwordsViewWidget->getRowCount() == 0)
	{
		m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	}
}

void SelectPasswordDialog::updateActions()
{
	const int currentSet(getCurrentSet());

	m_ui->removeButton->setEnabled(currentSet >= 0 && currentSet < m_ui->passwordsViewWidget->getRowCount());
}

PasswordsManager::PasswordInformation SelectPasswordDialog::getPassword() const
{
	return m_passwords.value(getCurrentSet());
}

int SelectPasswordDialog::getCurrentSet() const
{
	const QModelIndex index(m_ui->passwordsViewWidget->currentIndex());

	if (index.isValid() && m_ui->passwordsViewWidget->selectionModel()->hasSelection())
	{
		const QModelIndex parentIndex(index.parent());

		if (parentIndex.isValid() && parentIndex.data(Qt::UserRole).isValid())
		{
			return parentIndex.data(Qt::UserRole).toInt();
		}

		if (index.data(Qt::UserRole).isValid())
		{
			return index.data(Qt::UserRole).toInt();
		}
	}

	return -1;
}

}
