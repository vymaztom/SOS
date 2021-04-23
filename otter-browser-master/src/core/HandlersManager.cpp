/**************************************************************************
* Otter Browser: Web browser controlled by the user, not vice-versa.
* Copyright (C) 2015 - 2020 Michal Dutkiewicz aka Emdek <michal@emdek.pl>
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

#include "HandlersManager.h"
#include "AdblockContentFiltersProfile.h"
#include "IniSettings.h"
#include "SessionsManager.h"
#include "SettingsManager.h"
#include "Utils.h"
#include "../ui/ContentBlockingProfileDialog.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtCore/QUrlQuery>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

namespace Otter
{

HandlersManager* HandlersManager::m_instance(nullptr);

HandlersManager::HandlersManager(QObject *parent) : QObject(parent)
{
}

void HandlersManager::createInstance()
{
	if (!m_instance)
	{
		m_instance = new HandlersManager(QCoreApplication::instance());
	}
}

void HandlersManager::setMimeTypeHandler(const QMimeType &mimeType, const MimeTypeHandlerDefinition &definition)
{
	if (SessionsManager::isReadOnly())
	{
		return;
	}

	const QString path(SessionsManager::getWritableDataPath(QLatin1String("handlers.ini")));
	IniSettings settings(QFile::exists(path) ? path : SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")));
	QString transferMode;

	switch (definition.transferMode)
	{
		case MimeTypeHandlerDefinition::IgnoreTransfer:
			transferMode = QLatin1String("ignore");

			break;
		case MimeTypeHandlerDefinition::OpenTransfer:
			transferMode = QLatin1String("open");

			break;
		case MimeTypeHandlerDefinition::SaveTransfer:
			transferMode = QLatin1String("save");

			break;
		case MimeTypeHandlerDefinition::SaveAsTransfer:
			transferMode = QLatin1String("saveAs");

			break;
		default:
			transferMode = QLatin1String("ask");

			break;
	}

	settings.beginGroup(mimeType.isValid() ? mimeType.name() : QLatin1String("*"));
	settings.setValue(QLatin1String("openCommand"), definition.openCommand);
	settings.setValue(QLatin1String("downloadsPath"), definition.downloadsPath);
	settings.setValue(QLatin1String("transferMode"), transferMode);
	settings.save(path);
}

HandlersManager* HandlersManager::getInstance()
{
	return m_instance;
}

HandlersManager::MimeTypeHandlerDefinition HandlersManager::getMimeTypeHandler(const QMimeType &mimeType)
{
	IniSettings settings(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini")));
	const QString name(mimeType.isValid() ? mimeType.name() : QLatin1String("*"));
	MimeTypeHandlerDefinition definition;
	definition.mimeType = mimeType;
	definition.isExplicit = settings.getGroups().contains(name);

	if (definition.isExplicit)
	{
		settings.beginGroup(name);
	}
	else
	{
		settings.beginGroup(QLatin1String("*"));
	}

	const QString downloadsPath(settings.getValue(QLatin1String("downloadsPath"), {}).toString());
	const QString transferMode(settings.getValue(QLatin1String("transferMode"), {}).toString());

	definition.openCommand = settings.getValue(QLatin1String("openCommand"), {}).toString();
	definition.downloadsPath = (downloadsPath.isEmpty() ? SettingsManager::getOption(SettingsManager::Paths_DownloadsOption).toString() : downloadsPath);

	if (transferMode == QLatin1String("ignore"))
	{
		definition.transferMode = MimeTypeHandlerDefinition::IgnoreTransfer;
	}
	else if (transferMode == QLatin1String("open"))
	{
		definition.transferMode = MimeTypeHandlerDefinition::OpenTransfer;
	}
	else if (transferMode == QLatin1String("save"))
	{
		definition.transferMode = MimeTypeHandlerDefinition::SaveTransfer;
	}
	else if (transferMode == QLatin1String("saveAs"))
	{
		definition.transferMode = MimeTypeHandlerDefinition::SaveAsTransfer;
	}
	else
	{
		definition.transferMode = MimeTypeHandlerDefinition::AskTransfer;
	}

	return definition;
}

QVector<HandlersManager::MimeTypeHandlerDefinition> HandlersManager::getMimeTypeHandlers()
{
	const QMimeDatabase mimeDatabase;
	const QStringList mimeTypes(IniSettings(SessionsManager::getReadableDataPath(QLatin1String("handlers.ini"))).getGroups());
	QVector<HandlersManager::MimeTypeHandlerDefinition> handlers;
	handlers.reserve(mimeTypes.count());

	for (int i = 0; i < mimeTypes.count(); ++i)
	{
		handlers.append(getMimeTypeHandler(mimeDatabase.mimeTypeForName(mimeTypes.at(i))));
	}

	return handlers;
}

bool HandlersManager::handleUrl(const QUrl &url)
{
	if (url.scheme() == QLatin1String("abp"))
	{
		const QUrlQuery query(url.query());
		const QUrl location(QUrl::fromPercentEncoding(query.queryItemValue(QLatin1String("location")).toUtf8()));

		if (location.isValid())
		{
			if (ContentFiltersManager::getProfile(location))
			{
				QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Profile with this address already exists."), QMessageBox::Close);
			}
			else if (QMessageBox::question(QApplication::activeWindow(), tr("Question"), tr("Do you want to add this content blocking profile?"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
			{
				ContentFiltersProfile::ProfileSummary profileSummary;
				profileSummary.updateUrl = location;

				ContentBlockingProfileDialog dialog(profileSummary, {}, QApplication::activeWindow());

				if (dialog.exec() == QDialog::Accepted)
				{
					profileSummary = dialog.getProfile();

					QFile file(dialog.getRulesPath());

					if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
					{
						QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Failed to create profile file."), QMessageBox::Close);

						return false;
					}

					profileSummary.name = Utils::createIdentifier(QFileInfo(location.path()).baseName(), ContentFiltersManager::getProfileNames());

					if (!AdblockContentFiltersProfile::create(profileSummary, &file))
					{
						QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Failed to create profile file."), QMessageBox::Close);
					}

					file.close();
					file.remove();
				}
			}
		}

		return true;
	}

	if (url.scheme() == QLatin1String("mailto"))
	{
		QDesktopServices::openUrl(url);

		return true;
	}

	return false;
}

bool HandlersManager::canHandleUrl(const QUrl &url)
{
	return (url.scheme() == QLatin1String("abp") || url.scheme() == QLatin1String("mailto"));
}

}
