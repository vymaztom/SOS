/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2013 - 2021 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
* Copyright (C) 2014 Piotr Wójcik <chocimier@tlen.pl>
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

#include "BookmarkPropertiesDialog.h"
#include "BookmarksComboBoxWidget.h"
#include "../core/BookmarksManager.h"

#include "ui_BookmarkPropertiesDialog.h"

#include <QtCore/QDateTime>
#include <QtWidgets/QMessageBox>

namespace Otter
{

BookmarkPropertiesDialog::BookmarkPropertiesDialog(BookmarksModel::Bookmark *bookmark, QWidget *parent) : Dialog(parent),
	m_bookmark(bookmark),
	m_index(-1),
	m_ui(new Ui::BookmarkPropertiesDialog)
{
	const BookmarksModel::BookmarkType type(bookmark->getType());
	const bool isFeed(type == BookmarksModel::FeedBookmark);
	const bool isUrl(isFeed || type == BookmarksModel::UrlBookmark);

	m_ui->setupUi(this);
	m_ui->folderComboBox->setCurrentFolder(bookmark->getParent());
	m_ui->titleLineEditWidget->setText(m_bookmark->getTitle());
	m_ui->addressLineEditWidget->setText(m_bookmark->getUrl().toDisplayString());
	m_ui->addressLineEditWidget->setVisible(isUrl);
	m_ui->addressLabel->setVisible(isUrl);
	m_ui->descriptionTextEditWidget->setPlainText(m_bookmark->getDescription());
	m_ui->keywordLineEditWidget->setText(m_bookmark->getKeyword());
	m_ui->addedLabelWidget->setDateTime(m_bookmark->getTimeAdded());
	m_ui->modifiedLabelWidget->setDateTime(m_bookmark->getTimeModified());

	if (isFeed)
	{
		m_ui->lastVisitLabel->hide();
		m_ui->lastVisitLabelWidget->hide();
		m_ui->visitsLabel->hide();
		m_ui->visitsLabelWidget->hide();
	}
	else if (isUrl)
	{
		m_ui->lastVisitLabelWidget->setDateTime(m_bookmark->getTimeVisited());
		m_ui->visitsLabelWidget->setText(QString::number(m_bookmark->getVisits()));
	}
	else
	{
		m_ui->visitsLabel->hide();
		m_ui->visitsLabelWidget->hide();
		m_ui->lastVisitLabel->hide();
		m_ui->lastVisitLabelWidget->hide();
	}

	if (bookmark->data(BookmarksModel::IsTrashedRole).toBool())
	{
		setWindowTitle(tr("View Bookmark"));

		m_ui->folderLabel->hide();
		m_ui->folderComboBox->hide();
		m_ui->newFolderButton->hide();
		m_ui->titleLineEditWidget->setReadOnly(true);
		m_ui->addressLineEditWidget->setReadOnly(true);
		m_ui->descriptionTextEditWidget->setReadOnly(true);
		m_ui->keywordLineEditWidget->setReadOnly(true);
	}
	else
	{
		setWindowTitle(tr("Edit Bookmark"));
	}

	connect(m_ui->newFolderButton, &QPushButton::clicked, m_ui->folderComboBox, &BookmarksComboBoxWidget::createFolder);
	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &BookmarkPropertiesDialog::saveBookmark);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &BookmarkPropertiesDialog::close);
}

BookmarkPropertiesDialog::BookmarkPropertiesDialog(const QUrl &url, const QString &title, const QString &description, BookmarksModel::Bookmark *folder, int index, bool isUrl, QWidget *parent) : Dialog(parent),
	m_bookmark(nullptr),
	m_index(index),
	m_ui(new Ui::BookmarkPropertiesDialog)
{
	m_ui->setupUi(this);
	m_ui->folderComboBox->setCurrentFolder(folder ? folder : BookmarksManager::getLastUsedFolder());
	m_ui->titleLineEditWidget->setText(title);
	m_ui->addressLineEditWidget->setText(url.toString());
	m_ui->addressLineEditWidget->setVisible(isUrl);
	m_ui->addressLabel->setVisible(isUrl);
	m_ui->descriptionTextEditWidget->setPlainText(description);
	m_ui->visitsLabel->hide();
	m_ui->visitsLabelWidget->hide();
	m_ui->lastVisitLabel->hide();
	m_ui->lastVisitLabelWidget->hide();
	m_ui->addedLabel->hide();
	m_ui->addedLabelWidget->hide();
	m_ui->modifiedLabel->hide();
	m_ui->modifiedLabelWidget->hide();

	setWindowTitle(tr("Add Bookmark"));

	connect(m_ui->newFolderButton, &QPushButton::clicked, m_ui->folderComboBox, &BookmarksComboBoxWidget::createFolder);
	connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &BookmarkPropertiesDialog::saveBookmark);
	connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &BookmarkPropertiesDialog::close);
}

BookmarkPropertiesDialog::~BookmarkPropertiesDialog()
{
	delete m_ui;
}

void BookmarkPropertiesDialog::changeEvent(QEvent *event)
{
	QDialog::changeEvent(event);

	if (event->type() == QEvent::LanguageChange)
	{
		m_ui->retranslateUi(this);

		if (m_bookmark)
		{
			setWindowTitle(m_bookmark->data(BookmarksModel::IsTrashedRole).toBool() ? tr("View Bookmark") : tr("Edit Bookmark"));
		}
		else
		{
			setWindowTitle(tr("Add Bookmark"));
		}
	}
}

void BookmarkPropertiesDialog::saveBookmark()
{
	if (m_ui->folderComboBox->isEnabled())
	{
		const QString keyword(m_ui->keywordLineEditWidget->text());

		if (!keyword.isEmpty() && (!m_bookmark || m_bookmark->getKeyword() != keyword) && BookmarksManager::getModel()->getBookmarkByKeyword(keyword))
		{
			QMessageBox::critical(this, tr("Error"), tr("Bookmark with this keyword already exists."), QMessageBox::Close);

			return;
		}

		BookmarksModel::Bookmark *folderBookmark(m_ui->folderComboBox->getCurrentFolder());

		if (!m_bookmark)
		{
			QMap<int, QVariant> metaData({{BookmarksModel::TitleRole, m_ui->titleLineEditWidget->text()}});
			const bool isUrl(m_ui->addressLineEditWidget->isVisible());

			if (isUrl)
			{
				metaData[BookmarksModel::UrlRole] = QUrl(m_ui->addressLineEditWidget->text());
			}

			m_bookmark = BookmarksManager::addBookmark((isUrl ? BookmarksModel::UrlBookmark : BookmarksModel::FolderBookmark), metaData, folderBookmark, m_index);
		}

		m_bookmark->setData(m_ui->addressLineEditWidget->text(), BookmarksModel::UrlRole);
		m_bookmark->setData(m_ui->titleLineEditWidget->text(), BookmarksModel::TitleRole);
		m_bookmark->setData(m_ui->descriptionTextEditWidget->toPlainText(), BookmarksModel::DescriptionRole);
		m_bookmark->setData(keyword, BookmarksModel::KeywordRole);

		if (m_bookmark->parent() != folderBookmark)
		{
			BookmarksManager::getModel()->moveBookmark(m_bookmark, folderBookmark);
		}

		BookmarksManager::setLastUsedFolder(folderBookmark);
	}

	accept();
}

}
