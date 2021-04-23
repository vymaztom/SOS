/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2021 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2015 Jan Bajer aka bajasoft <jbajer@gmail.com>
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

#ifndef OTTER_SEARCHPREFERENCESPAGE_H
#define OTTER_SEARCHPREFERENCESPAGE_H

#include "PreferencesPage.h"
#include "../../../ui/ItemDelegate.h"
#include "../../../core/SearchEnginesManager.h"

namespace Otter
{

namespace Ui
{
	class SearchPreferencesPage;
}

class Animation;
class SearchEngineFetchJob;

class SearchEngineTitleDelegate final : public ItemDelegate
{
public:
	explicit SearchEngineTitleDelegate(QObject *parent);

	void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

class SearchEngineKeywordDelegate final : public ItemDelegate
{
public:
	explicit SearchEngineKeywordDelegate(QObject *parent);

	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class SearchPreferencesPage final : public PreferencesPage
{
	Q_OBJECT

public:
	enum DataRole
	{
		IdentifierRole = Qt::UserRole,
		IsUpdatingRole
	};

	explicit SearchPreferencesPage(QWidget *parent);
	~SearchPreferencesPage();

	static Animation* getUpdateAnimation();
	static QStringList getKeywords(const QAbstractItemModel *model, int excludeRow = -1);

public slots:
	void save() override;

protected:
	void changeEvent(QEvent *event) override;
	void addSearchEngine(const QString &path, const QString &identifier, bool isReadding);
	void updateReaddSearchEngineMenu();
	QList<QStandardItem*> createRow(const SearchEnginesManager::SearchEngineDefinition &searchEngine, bool isDefault = false) const;

protected slots:
	void createSearchEngine();
	void importSearchEngine();
	void readdSearchEngine(QAction *action);
	void editSearchEngine();
	void updateSearchEngine();
	void removeSearchEngine();
	void updateSearchEngineActions();

private:
	QStringList m_filesToRemove;
	QHash<QString, SearchEngineFetchJob*> m_updateJobs;
	QHash<QString, QPair<bool, SearchEnginesManager::SearchEngineDefinition> > m_searchEngines;
	Ui::SearchPreferencesPage *m_ui;

	static Animation* m_updateAnimation;
};

}

#endif
