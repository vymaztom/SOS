/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2018 - 2020 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "TabHistoryContentsWidget.h"
#include "../../../core/ThemesManager.h"
#include "../../../ui/Action.h"
#include "../../../ui/MainWindow.h"
#include "../../../ui/Window.h"

#include "ui_TabHistoryContentsWidget.h"

#include <QtWidgets/QDesktopWidget>
#include <QtWidgets/QToolTip>

namespace Otter
{

TabHistoryContentsWidget::TabHistoryContentsWidget(const QVariantMap &parameters, QWidget *parent) : ActiveWindowObserverContentsWidget(parameters, nullptr, parent),
	m_ui(new Ui::TabHistoryContentsWidget)
{
	m_ui->setupUi(this);
	m_ui->filterLineEditWidget->setClearOnEscape(true);
	m_ui->historyViewWidget->setModel(new QStandardItemModel(this));
	m_ui->historyViewWidget->viewport()->installEventFilter(this);
	m_ui->historyViewWidget->viewport()->setMouseTracking(true);

	updateHistory();

	connect(this, &TabHistoryContentsWidget::activeWindowChanged, this, [=](Window *window, Window *previousWindow)
	{
		if (previousWindow)
		{
			disconnect(previousWindow, &Window::loadingStateChanged, this, &TabHistoryContentsWidget::updateHistory);
		}

		if (window)
		{
			connect(window, &Window::loadingStateChanged, this, &TabHistoryContentsWidget::updateHistory);
		}

		updateHistory();
	});
	connect(m_ui->filterLineEditWidget, &LineEditWidget::textChanged, m_ui->historyViewWidget, &ItemViewWidget::setFilterString);
	connect(m_ui->historyViewWidget, &ItemViewWidget::customContextMenuRequested, this, &TabHistoryContentsWidget::showContextMenu);
	connect(m_ui->historyViewWidget, &ItemViewWidget::clicked, [&](const QModelIndex &index)
	{
		Window *window(getActiveWindow());

		if (window && window->getWebWidget())
		{
			window->triggerAction(ActionsManager::GoToHistoryIndexAction, {{QLatin1String("index"), index.row()}});
		}
	});
}

TabHistoryContentsWidget::~TabHistoryContentsWidget()
{
	delete m_ui;
}

void TabHistoryContentsWidget::changeEvent(QEvent *event)
{
	ContentsWidget::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);
	}
}

void TabHistoryContentsWidget::updateHistory()
{
	Window *window(getActiveWindow());

	if (window)
	{
		const Session::Window::History history(window->getHistory());

		m_ui->historyViewWidget->getSourceModel()->clear();

		for (int i = 0; i < history.entries.count(); ++i)
		{
			QStandardItem *item(new QStandardItem(history.entries.at(i).getTitle()));
			item->setData((history.entries.at(i).icon.isNull() ? ThemesManager::createIcon(QLatin1String("text-html")) : history.entries.at(i).icon), Qt::DecorationRole);
			item->setData(history.entries.at(i).url, UrlRole);
			item->setData(history.entries.at(i).time, TimeVisitedRole);
			item->setFlags(item->flags() | Qt::ItemNeverHasChildren);

			if (i == history.index)
			{
				QFont font(item->font());
				font.setBold(true);

				item->setFont(font);
			}

			m_ui->historyViewWidget->getSourceModel()->appendRow(item);
		}
	}
	else
	{
		m_ui->historyViewWidget->getSourceModel()->clear();
	}
}

void TabHistoryContentsWidget::showContextMenu(const QPoint &position)
{
	Window *window(getActiveWindow());
	const QModelIndex index(m_ui->historyViewWidget->indexAt(position));
	ActionExecutor::Object executor(window, window);
	QMenu menu(this);

	if (index.isValid())
	{
		menu.addAction(new Action(ActionsManager::GoToHistoryIndexAction, {{QLatin1String("index"), index.row()}}, {{QLatin1String("icon"), {}}, {QLatin1String("text"), QCoreApplication::translate("actions", "Go to History Entry")}}, executor, &menu));
		menu.addSeparator();
		menu.addAction(new Action(ActionsManager::RemoveHistoryIndexAction, {{QLatin1String("index"), index.row()}}, executor, &menu));
		menu.addAction(new Action(ActionsManager::RemoveHistoryIndexAction, {{QLatin1String("index"), index.row()}, {QLatin1String("clearGlobalHistory"), true}}, executor, &menu));
		menu.addSeparator();
	}

	menu.addAction(new Action(ActionsManager::ClearTabHistoryAction, {}, executor, &menu));
	menu.addAction(new Action(ActionsManager::ClearTabHistoryAction, {{QLatin1String("clearGlobalHistory"), true}}, {{QLatin1String("text"), QT_TRANSLATE_NOOP("actions", "Purge Tab History")}}, executor, &menu));
	menu.exec(m_ui->historyViewWidget->mapToGlobal(position));
}

QString TabHistoryContentsWidget::getTitle() const
{
	return tr("Tab History");
}

QLatin1String TabHistoryContentsWidget::getType() const
{
	return QLatin1String("tabHistory");
}

QUrl TabHistoryContentsWidget::getUrl() const
{
	return {};
}

QIcon TabHistoryContentsWidget::getIcon() const
{
	return ThemesManager::createIcon(QLatin1String("tab-history"), false);
}

bool TabHistoryContentsWidget::eventFilter(QObject *object, QEvent *event)
{
	if (object == m_ui->historyViewWidget->viewport() && event->type() == QEvent::ToolTip)
	{
		const QHelpEvent *helpEvent(static_cast<QHelpEvent*>(event));
		const QModelIndex index(m_ui->historyViewWidget->indexAt(helpEvent->pos()));
		QString toolTip;

		if (index.isValid())
		{
			toolTip = tr("Title: %1").arg(index.data(TitleRole).toString()) + QLatin1Char('\n') + tr("Address: %1").arg(index.data(UrlRole).toUrl().toDisplayString());

			if (!index.data(TimeVisitedRole).isNull())
			{
				toolTip.append(QLatin1Char('\n') + tr("Date: %1").arg(Utils::formatDateTime(index.data(TimeVisitedRole).toDateTime())));
			}
		}

		QToolTip::showText(helpEvent->globalPos(), QFontMetrics(QToolTip::font()).elidedText(toolTip, Qt::ElideRight, (QApplication::desktop()->screenGeometry(m_ui->historyViewWidget).width() / 2)), m_ui->historyViewWidget, m_ui->historyViewWidget->visualRect(index));

		return true;
	}

	return ContentsWidget::eventFilter(object, event);
}

}
