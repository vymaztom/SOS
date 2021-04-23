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

#ifndef OTTER_HANDLERSMANAGER_H
#define OTTER_HANDLERSMANAGER_H

#include <QtCore/QMimeType>
#include <QtCore/QObject>
#include <QtCore/QUrl>

namespace Otter
{

class HandlersManager final : public QObject
{
	Q_OBJECT

public:
	struct MimeTypeHandlerDefinition final
	{
		enum TransferMode
		{
			IgnoreTransfer = 0,
			AskTransfer,
			OpenTransfer,
			SaveTransfer,
			SaveAsTransfer
		};

		QString openCommand;
		QString downloadsPath;
		QMimeType mimeType;
		TransferMode transferMode = IgnoreTransfer;
		bool isExplicit = true;
	};

	struct ProtocolHandlerDefinition final
	{
		enum HandlerMode
		{
			NoHandler = 0,
			InternalHandler,
			ExternalHandler
		};

		QString protocol;
		QString openCommand;
		HandlerMode handlerMode = InternalHandler;
		bool isExplicit = true;
	};

	static void createInstance();
	static void setMimeTypeHandler(const QMimeType &mimeType, const MimeTypeHandlerDefinition &definition);
	static HandlersManager* getInstance();
	static MimeTypeHandlerDefinition getMimeTypeHandler(const QMimeType &mimeType);
	static QVector<MimeTypeHandlerDefinition> getMimeTypeHandlers();
	static bool handleUrl(const QUrl &url);
	static bool canHandleUrl(const QUrl &url);

protected:
	explicit HandlersManager(QObject *parent);

private:
	static HandlersManager *m_instance;
};

}

#endif
