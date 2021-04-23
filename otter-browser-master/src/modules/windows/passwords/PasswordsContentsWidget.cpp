/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2016 - 2019 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "PasswordsContentsWidget.h"
#include "../../../core/Application.h"
#include "../../../core/HistoryManager.h"
#include "../../../core/PasswordsManager.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/Action.h"
#include "../../../ui/MainWindow.h"

#include "ui_PasswordsContentsWidget.h"

#include <QtCore/QTimer>
#include <QtGui/QClipboard>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>

namespace Otter
{

PasswordFieldDelegate::PasswordFieldDelegate(QObject *parent) : ItemDelegate (parent),
	m_arePasswordsVisible(false)
{
}

void PasswordFieldDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
	ItemDelegate::initStyleOption(option, index);

	if (!m_arePasswordsVisible && index.sibling(index.row(), 0).data(PasswordsContentsWidget::FieldTypeRole).toInt() == PasswordsManager::PasswordField)
	{
		option->text = QString(QChar(8226)).repeated(5);
	}
}

void PasswordFieldDelegate::setPasswordsVisibility(bool areVisible)
{
	m_arePasswordsVisible = areVisible;
}

void PasswordFieldDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	Q_UNUSED(editor)
	Q_UNUSED(index)
}

void PasswordFieldDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	Q_UNUSED(editor)
	Q_UNUSED(model)
	Q_UNUSED(index)
}

QWidget* PasswordFieldDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	Q_UNUSED(option)

	LineEditWidget *widget(new LineEditWidget(index.data(Qt::DisplayRole).toString(), parent));
	widget->setReadOnly(true);

	if (!m_arePasswordsVisible && index.sibling(index.row(), 0).data(PasswordsContentsWidget::FieldTypeRole).toInt() == PasswordsManager::PasswordField)
	{
		widget->setEchoMode(QLineEdit::Password);
		widget->setText(QString(QChar(8226)).repeated(5));
	}

	return widget;
}

PasswordsContentsWidget::PasswordsContentsWidget(const QVariantMap &parameters, Window *window, QWidget *parent) : ContentsWidget(parameters, window, parent),
	m_model(new QStandardItemModel(this)),
	m_delegate(nullptr),
	m_isLoading(true),
	m_ui(new Ui::PasswordsContentsWidget)
{
	m_ui->setupUi(this);

	m_delegate = new PasswordFieldDelegate(m_ui->passwordsViewWidget);

	m_ui->filterLineEditWidget->setClearOnEscape(true);
	m_ui->passwordsViewWidget->installEventFilter(this);
	m_ui->passwordsViewWidget->setViewMode(ItemViewWidget::TreeView);
	m_ui->passwordsViewWidget->setModel(m_model);
	m_ui->passwordsViewWidget->setItemDelegateForColumn(1, m_delegate);

	m_model->setHeaderData(0, Qt::Horizontal, 500, HeaderViewWidget::WidthRole);

	QTimer::singleShot(100, this, &PasswordsContentsWidget::populatePasswords);

	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, this, &PasswordsContentsWidget::filterPasswords);
	connect(m_ui->passwordsViewWidget, &ItemViewWidget::customContextMenuRequested, this, &PasswordsContentsWidget::showContextMenu);
	connect(m_ui->passwordsViewWidget, &ItemViewWidget::needsActionsUpdate, this, &PasswordsContentsWidget::updateActions);
	connect(m_ui->showPasswordsButton, &QPushButton::toggled, this, &PasswordsContentsWidget::togglePasswordsVisibility);
	connect(m_ui->deleteButton, &QPushButton::clicked, this, &PasswordsContentsWidget::removePasswords);
}

PasswordsContentsWidget::~PasswordsContentsWidget()
{
	delete m_ui;
}

void PasswordsContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		m_model->setHorizontalHeaderLabels({tr("Name"), tr("Value")});
	}
}

void PasswordsContentsWidget::print(QPrinter *printer)
{
	m_ui->passwordsViewWidget->render(printer);
}

void PasswordsContentsWidget::triggerAction(int identifier, const QVariantMap &parameters, ActionsManager::TriggerType trigger)
{
	switch (identifier)
	{
		case ActionsManager::SelectAllAction:
			m_ui->passwordsViewWidget->selectAll();

			break;
		case ActionsManager::DeleteAction:
			removePasswords();

			break;
		case ActionsManager::FindAction:
		case ActionsManager::QuickFindAction:
			m_ui->filterLineEditWidget->setFocus();

			break;
		case ActionsManager::ActivateContentAction:
			m_ui->passwordsViewWidget->setFocus();

			break;
		default:
			ContentsWidget::triggerAction(identifier, parameters, trigger);

			break;
	}
}

void PasswordsContentsWidget::populatePasswords()
{
	m_model->clear();
	m_model->setHorizontalHeaderLabels({tr("Name"), tr("Value")});
	m_model->setHeaderData(0, Qt::Horizontal, 500, HeaderViewWidget::WidthRole);

	const QStringList hosts(PasswordsManager::getHosts());

	for (int i = 0; i < hosts.count(); ++i)
	{
		const QUrl url(QStringLiteral("http://%1/").arg(hosts.at(i)));
		const QVector<PasswordsManager::PasswordInformation> passwords(PasswordsManager::getPasswords(url));
		QStandardItem *hostItem(new QStandardItem(HistoryManager::getIcon(url), hosts.at(i)));
		hostItem->setData(hosts.at(i), HostRole);
		hostItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

		for (int j = 0; j < passwords.count(); ++j)
		{
			QStandardItem *setItem(new QStandardItem(tr("Set #%1").arg(j + 1)));
			setItem->setData(passwords.at(j).url, UrlRole);
			setItem->setData(((passwords.at(j).type == PasswordsManager::AuthPassword) ? QLatin1String("auth") : QLatin1String("form")), AuthTypeRole);
			setItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

			for (int k = 0; k < passwords.at(j).fields.count(); ++k)
			{
				QList<QStandardItem*> fieldItems({new QStandardItem(passwords.at(j).fields.at(k).name), new QStandardItem(passwords.at(j).fields.at(k).value)});
				fieldItems[0]->setData(passwords.at(j).fields.at(k).type, FieldTypeRole);
				fieldItems[0]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
				fieldItems[1]->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);

				setItem->appendRow(fieldItems);
			}

			hostItem->appendRow({setItem, new QStandardItem()});
		}

		hostItem->setText(QStringLiteral("%1 (%2)").arg(hosts.at(i)).arg(hostItem->rowCount()));

		m_model->appendRow(hostItem);

		for (int j = 0; j < hostItem->rowCount(); ++j)
		{
			const QStandardItem *setItem(hostItem->child(j));

			if (setItem)
			{
				m_ui->passwordsViewWidget->expand(setItem->index());
			}
		}
	}

	m_model->sort(0);

	if (m_isLoading)
	{
		m_isLoading = false;

		emit loadingStateChanged(WebWidget::FinishedLoadingState);

		connect(PasswordsManager::getInstance(), &PasswordsManager::passwordsModified, this, &PasswordsContentsWidget::populatePasswords);
		connect(m_ui->passwordsViewWidget->selectionModel(), &QItemSelectionModel::selectionChanged, [&]()
		{
			emit arbitraryActionsStateChanged({ActionsManager::DeleteAction});
		});
	}
}

void PasswordsContentsWidget::filterPasswords(const QString &filter)
{
	for (int i = 0; i < m_model->rowCount(); ++i)
	{
		const QModelIndex domainIndex(m_model->index(i, 0, m_model->invisibleRootItem()->index()));
		int foundSets(0);
		bool hasDomainMatch(filter.isEmpty() || domainIndex.data(Qt::DisplayRole).toString().contains(filter, Qt::CaseInsensitive));

		for (int j = 0; j < m_model->rowCount(domainIndex); ++j)
		{
			const QModelIndex setIndex(m_model->index(j, 0, domainIndex));
			bool hasFieldMatch(hasDomainMatch || setIndex.data(Qt::DisplayRole).toString().contains(filter, Qt::CaseInsensitive));

			if (!hasFieldMatch)
			{
				for (int k = 0; k < m_model->rowCount(setIndex); ++k)
				{
					const QModelIndex fieldIndex(m_model->index(k, 0, setIndex));

					if (fieldIndex.data(Qt::DisplayRole).toString().contains(filter, Qt::CaseInsensitive) || (fieldIndex.data(FieldTypeRole).toInt() != PasswordsManager::PasswordField && fieldIndex.sibling(fieldIndex.row(), 1).data(Qt::DisplayRole).toString().contains(filter, Qt::CaseInsensitive)))
					{
						hasFieldMatch = true;

						break;
					}
				}

				if (hasFieldMatch)
				{
					++foundSets;
				}
			}

			m_ui->passwordsViewWidget->setRowHidden(j, domainIndex, (!filter.isEmpty() && !hasFieldMatch));
		}

		m_ui->passwordsViewWidget->setRowHidden(i, m_model->invisibleRootItem()->index(), (foundSets == 0 && !hasDomainMatch));
	}
}

void PasswordsContentsWidget::copyFieldName()
{
	const QModelIndex index(m_ui->passwordsViewWidget->currentIndex().sibling(m_ui->passwordsViewWidget->currentIndex().row(), 0));

	if (index.isValid())
	{
		QApplication::clipboard()->setText(index.data(Qt::DisplayRole).toString());
	}
}

void PasswordsContentsWidget::copyFieldValue()
{
	const QModelIndex index(m_ui->passwordsViewWidget->currentIndex().sibling(m_ui->passwordsViewWidget->currentIndex().row(), 1));

	if (index.isValid())
	{
		QApplication::clipboard()->setText(index.data(Qt::DisplayRole).toString());
	}
}

void PasswordsContentsWidget::removePasswords()
{
	const QModelIndexList indexes(m_ui->passwordsViewWidget->selectionModel()->selectedIndexes());

	if (indexes.isEmpty())
	{
		return;
	}

	QVector<PasswordsManager::PasswordInformation> passwords;
	passwords.reserve(indexes.count());

	for (int i = 0; i < indexes.count(); ++i)
	{
		if (!indexes.at(i).isValid() || indexes.at(i).column() > 0)
		{
			continue;
		}

		if (indexes.at(i).parent() == m_model->invisibleRootItem()->index())
		{
			const QModelIndex hostIndex(indexes.at(i));

			if (!hostIndex.isValid())
			{
				continue;
			}

			for (int j = 0; j < m_model->rowCount(hostIndex); ++j)
			{
				passwords.append(getPassword(m_model->index(j, 0, hostIndex)));
			}
		}
		else
		{
			const QModelIndex setIndex((indexes.at(i).parent().parent() == m_model->invisibleRootItem()->index()) ? indexes.at(i) : indexes.at(i).parent());

			if (setIndex.isValid())
			{
				passwords.append(getPassword(setIndex));
			}
		}
	}

	if (passwords.isEmpty())
	{
		return;
	}

	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("You are about to delete %n password(s).", "", passwords.count()));
	messageBox.setInformativeText(tr("Do you want to continue?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Yes);

	if (messageBox.exec() == QMessageBox::Yes)
	{
		for (int i = 0; i < passwords.count(); ++i)
		{
			PasswordsManager::removePassword(passwords.at(i));
		}
	}
}

void PasswordsContentsWidget::removeHostPasswords()
{
	const QModelIndexList indexes(m_ui->passwordsViewWidget->selectionModel()->selectedIndexes());

	if (indexes.isEmpty())
	{
		return;
	}

	QStringList hosts;
	int amount(0);

	for (int i = 0; i < indexes.count(); ++i)
	{
		QModelIndex hostIndex(indexes.at(i));

		while (hostIndex.parent().isValid() && hostIndex.parent() != m_model->invisibleRootItem()->index())
		{
			hostIndex = hostIndex.parent();
		}

		if (hostIndex.isValid())
		{
			const QString host(hostIndex.data(HostRole).toString());

			if (!host.isEmpty() && !hosts.contains(host))
			{
				hosts.append(host);

				amount += m_model->rowCount(hostIndex);
			}
		}
	}

	if (hosts.isEmpty())
	{
		return;
	}

	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("You are about to delete %n password(s).", "", amount));
	messageBox.setInformativeText(tr("Do you want to continue?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Yes);

	if (messageBox.exec() == QMessageBox::Yes)
	{
		for (int i = 0; i < hosts.count(); ++i)
		{
			PasswordsManager::clearPasswords(hosts.at(i));
		}
	}
}

void PasswordsContentsWidget::removeAllPasswords()
{
	QMessageBox messageBox;
	messageBox.setWindowTitle(tr("Question"));
	messageBox.setText(tr("You are about to delete all passwords."));
	messageBox.setInformativeText(tr("Do you want to continue?"));
	messageBox.setIcon(QMessageBox::Question);
	messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Yes);

	if (messageBox.exec() == QMessageBox::Yes)
	{
		PasswordsManager::clearPasswords();
	}
}

void PasswordsContentsWidget::togglePasswordsVisibility(bool areVisible)
{
	if (areVisible)
	{
		QMessageBox messageBox;
		messageBox.setWindowTitle(tr("Question"));
		messageBox.setText(tr("You are about to show all passwords."));
		messageBox.setInformativeText(tr("Do you want to continue?"));
		messageBox.setIcon(QMessageBox::Question);
		messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
		messageBox.setDefaultButton(QMessageBox::Yes);

		if (messageBox.exec() != QMessageBox::Yes)
		{
			m_ui->showPasswordsButton->setChecked(false);

			return;
		}
	}

	m_delegate->setPasswordsVisibility(areVisible);

	m_ui->passwordsViewWidget->viewport()->update();
}

void PasswordsContentsWidget::showContextMenu(const QPoint &position)
{
	MainWindow *mainWindow(MainWindow::findMainWindow(this));
	const QModelIndex index(m_ui->passwordsViewWidget->indexAt(position));
	QMenu menu(this);

	if (index.isValid())
	{
		if (index.parent() != m_model->invisibleRootItem()->index())
		{
			if (index.parent().parent().isValid() && index.parent().parent().parent() == m_model->invisibleRootItem()->index())
			{
				menu.addAction(tr("Copy Field Name"), this, &PasswordsContentsWidget::copyFieldName);
				menu.addAction(tr("Copy Field Value"), this, &PasswordsContentsWidget::copyFieldValue);
				menu.addSeparator();
			}

			menu.addAction(tr("Remove Password"), this, &PasswordsContentsWidget::removePasswords);
		}

		menu.addAction(tr("Remove All Passwords from This Domain…"), this, &PasswordsContentsWidget::removeHostPasswords);
	}

	menu.addAction(tr("Remove All Passwords…"), this, &PasswordsContentsWidget::removeAllPasswords)->setEnabled(m_ui->passwordsViewWidget->model()->rowCount() > 0);
	menu.addSeparator();
	menu.addAction(new Action(ActionsManager::ClearHistoryAction, {}, ActionExecutor::Object(mainWindow, mainWindow), &menu));
	menu.exec(m_ui->passwordsViewWidget->mapToGlobal(position));
}

void PasswordsContentsWidget::updateActions()
{
	const QModelIndex index(m_ui->passwordsViewWidget->getCurrentIndex());
	QModelIndex hostIndex(index);

	while (hostIndex.parent().isValid() && hostIndex.parent() != m_model->invisibleRootItem()->index())
	{
		hostIndex = hostIndex.parent();
	}

	if (hostIndex.isValid())
	{
		m_ui->domainLabelWidget->setText(hostIndex.data(HostRole).toString());
	}

	m_ui->deleteButton->setEnabled(index.isValid() && index.parent() != m_model->invisibleRootItem()->index());
}

QString PasswordsContentsWidget::getTitle() const
{
	return tr("Passwords");
}

QLatin1String PasswordsContentsWidget::getType() const
{
	return QLatin1String("passwords");
}

QUrl PasswordsContentsWidget::getUrl() const
{
	return QUrl(QLatin1String("about:passwords"));
}

QIcon PasswordsContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("dialog-password"), false);
}

ActionsManager::ActionDefinition::State PasswordsContentsWidget::getActionState(int identifier, const QVariantMap &parameters) const
{
	ActionsManager::ActionDefinition::State state(ActionsManager::getActionDefinition(identifier).getDefaultState());

	switch (identifier)
	{
		case ActionsManager::SelectAllAction:
			state.isEnabled = true;

			return state;
		case ActionsManager::DeleteAction:
			state.isEnabled = (m_ui->passwordsViewWidget->selectionModel() && !m_ui->passwordsViewWidget->selectionModel()->selectedIndexes().isEmpty());

			return state;
		default:
			break;
		}

	return ContentsWidget::getActionState(identifier, parameters);
}

PasswordsManager::PasswordInformation PasswordsContentsWidget::getPassword(const QModelIndex &index) const
{
	PasswordsManager::PasswordInformation password;
	password.url = index.data(UrlRole).toString();
	password.type = ((index.data(AuthTypeRole).toString() == QLatin1String("auth")) ? PasswordsManager::AuthPassword : PasswordsManager::FormPassword);

	for (int i = 0; i < m_model->rowCount(index); ++i)
	{
		const QModelIndex nameIndex(m_model->index(i, 0, index));
		PasswordsManager::PasswordInformation::Field field;
		field.name = nameIndex.data(Qt::DisplayRole).toString();
		field.value = ((nameIndex.data(FieldTypeRole).toInt() == PasswordsManager::PasswordField) ? QString() : m_model->index(i, 1, index).data(Qt::DisplayRole).toString());
		field.type = static_cast<PasswordsManager::FieldType>(nameIndex.data(FieldTypeRole).toInt());

		password.fields.append(field);
	}

	return password;
}

WebWidget::LoadingState PasswordsContentsWidget::getLoadingState() const
{
	return (m_isLoading ? WebWidget::OngoingLoadingState : WebWidget::FinishedLoadingState);
}

bool PasswordsContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->passwordsViewWidget && event->type() == QEvent::KeyPress && static_cast<QKeyEvent*>(event)->key() == Qt::Key_Delete)
	{
		removePasswords();

		return true;
	}

	return ContentsWidget::eventFilter(object, event);
}

}
